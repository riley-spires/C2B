#ifndef QBS_H
#define QBS_H

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace qbs {

    enum Compiler { clang, gpp };
    // Major cxx versions taken from https://en.cppreference.com/w/cpp/language/history
    enum CxxVersion { cpp11, cpp14, cpp17, cpp20, cpp23 };
    enum BuildType { exe, lib };
    enum FileType { cpp, c, h, hpp };

    class Utils {
        public:
            static std::vector<std::string> splitString(std::string str, char delim) {
                std::vector<std::string> ret;
                std::string sb;
                
                for (const auto &c : str) {
                    if (c == delim) {
                        ret.push_back(sb);
                        sb = "";
                    }

                    sb += c;
                }

                if (str[str.length() - 1] != delim && !sb.empty()) ret.push_back(sb);

                return ret;
            }
    };

    class Cmd {
        private:
            std::vector<std::string> args;
            size_t length;
    
            // to handle variadic append_many recursion end
            void append_many() {};
        public:
            template<typename... Args>
            Cmd(Args... args) : args{std::forward<Args>(args)...} {
                this->length = this->args.size();
            }

            void append(std::string arg) {
                this->args.push_back(arg);
                this->length += 1;
            }

            template<typename... Args>
            void append_many(std::string first, Args... args) {
                append(first);
                append_many(args...);
            }
    
            void set_length(size_t length) {
                this->length = length;
                this->args.resize(length);
            }

            size_t get_length() const {
                return this->length;
            }

            std::string string() const {
                std::string sb;
                for (const auto &arg : args) {
                    sb += arg;
                    sb += " ";
                }

                return sb;
            }
            
            int run() {
                std::string cmd;

                for (const auto &arg : args) {
                    cmd += arg;
                    cmd += " ";
                }

                std::cout << "[INFO] " << this->string() << std::endl;
                
                return std::system(cmd.c_str());
            }
    };

    class Build {
        private:
            std::vector<std::string> sourceFiles;
            std::vector<std::string> includeDirs;
            std::vector<std::string> linkDirs;
            std::vector<std::string> linkFiles;
            std::vector<std::string> flags;
            Compiler compiler;
            CxxVersion version;
            BuildType buildType;
            std::string projectName;
            std::string outputDir;

            void prep_compile_cmd(Cmd *cmd, FileType fileType) {
                switch (this->compiler) {
                    case Compiler::clang :
                        cmd->append("clang");
                        break;
                    case Compiler::gpp:
                        cmd->append("g++");
                        break;
                    default:
                        throw std::invalid_argument("Unknown compiler option");
                }

                if (fileType == FileType::cpp) {
                    switch (this->version) {
                        case CxxVersion::cpp11:
                            cmd->append("--std=c++11");
                            break;
                        case CxxVersion::cpp14:
                            cmd->append("--std=c++14");
                            break;
                        case CxxVersion::cpp17:
                            cmd->append("--std=c++17");
                            break;
                        case CxxVersion::cpp20:
                            cmd->append("--std=c++20");
                            break;
                        case CxxVersion::cpp23:
                            cmd->append("--std=c++23");
                            break;
                        default:
                            throw std::invalid_argument("Unknown cpp version");
                    }
                }


                for (const auto &include : includeDirs) {
                    std::string includeArg;
                    includeArg += "-I";
                    includeArg += include;
                    cmd->append(includeArg);
                }

                for (const auto &linkDir : linkDirs) {
                    std::string linkDirArg;
                    linkDirArg += "-L";
                    linkDirArg += linkDir;
                    cmd->append(linkDirArg);
                }

                for (const auto &linkFile : linkFiles) {
                    std::string linkFileArg;
                    linkFileArg += "-l";
                    linkFileArg += linkFile;
                    cmd->append(linkFileArg);
                }

                for (const auto &flag : flags) {
                    cmd->append(flag);
                }
            }

            // To handle variadic recursion
            void add_flags() {}
            void add_source_files() {}
            void add_include_dirs() {}
            void link_files() {}
            void add_link_dirs() {}
        public:
            static FileType get_file_type_from_ext(std::string path) {
                std::string ext = fs::path(path).extension();

                if (ext == ".cpp") return FileType::cpp;
                else if (ext == ".c") return FileType::c;
                else if (ext == ".h") return FileType::h;
                else if (ext == ".hpp") return FileType::hpp;
                else throw std::invalid_argument(ext + " is an unknown extension");
            }

            Build(std::string projectName,
                  std::string outputDir = "./",
                  BuildType buildType = BuildType::exe,
                  CxxVersion version = CxxVersion::cpp20,
                  Compiler compiler = Compiler::gpp) {
                this->projectName = projectName;
                this->version = version;
                this->compiler = compiler;
                this->buildType = buildType;

                if (outputDir[outputDir.length() - 1] != '/') {
                    outputDir += '/';
                }
                this->outputDir = outputDir;
            }

            void set_cxx_version(CxxVersion version) {
                this->version = version;
            }

            void set_compiler(Compiler compiler) {
                this->compiler = compiler;
            }

            void set_build_type(BuildType buildType) {
                this->buildType = buildType;
            }

            void add_source_file(std::string path) {
                if (fs::is_directory(path)) {
                    throw std::invalid_argument(path + " is not a file!");
                }

                std::string fileExt = fs::path(path).extension();

                if (fileExt != ".cpp" && fileExt != ".c")
                    return;
                
                this->sourceFiles.push_back(path);
            }

            template<typename... Args>
            void add_source_files(std::string first, Args... args) {
                this->add_source_file(first);
                this->add_source_files(args...);
            }

            void add_include_dir(std::string path, bool recursive = true) {
                if (!fs::is_directory(path)) {
                    throw std::invalid_argument(path + " is not a directory!");
                }

                if (recursive) {
                    for (const auto &entry : fs::directory_iterator(path)) {
                        if (!entry.is_directory()) continue;

                        this->add_include_dir(entry.path().string());
                    }
                }

                this->includeDirs.push_back(path);
            }

            template<typename... Args>
            void add_include_dirs(std::string first, Args... args) {
                this->add_include_dir(first);
                this->add_include_dirs(args...);
            }

            void add_flag(std::string flag) {
                this->flags.push_back(flag);
            }

            template<typename... Args>
            void add_flags(std::string first, Args... args) {
                this->add_flag(first);
                this->add_flags(args...);
            }

            void add_source_dir(std::string path, bool recursive = true) {
                if (!fs::is_directory(path)) {
                    throw std::invalid_argument(path + " is not a directory!");
                }

                for (const auto &entry : fs::directory_iterator(path)) {
                    if (entry.is_directory()) {
                        if (!recursive) continue;

                        this->add_source_dir(entry.path().string());
                    } else {
                        this->add_source_file(entry.path().string());
                    }
                }
            }

            void add_link_dir(std::string path) {
                if (!fs::is_directory(path)) {
                    throw std::invalid_argument(path + " is not a directory!");
                }

                this->linkDirs.push_back(path);
            }

            template<typename... Args>
            void add_link_dirs(std::string first, Args... args) {
                this->add_link_dir(first);
                this->add_link_dirs(args...);
            }

            void link_file(std::string path) {
                this->linkFiles.push_back(path);
            }

            template<typename... Args>
            void link_files(std::string first, Args... args) {
                this->link_file(first);
                this->link_files(args...);
            }

            void set_output_dir(std::string path) {
                if (!fs::exists(path)) {
                    fs::create_directories(path);
                } else if (!fs::is_directory(path)) {
                    throw std::invalid_argument(path + " is not a directory!");
                }

                if (path[path.length()-1] != '/') {
                    path += '/';
                }

                this->outputDir = path;
            }

            int build() {
                Cmd cmd;
                int ret;
                std::vector<std::string> oFiles;
                FileType ft;

                switch (this->buildType) {
                    case BuildType::exe:
                        for (const auto &file : sourceFiles) {
                            cmd.append(file);
                            ft = Build::get_file_type_from_ext(file);
                        }   
                        this->prep_compile_cmd(&cmd, ft);
                        cmd.append_many("-o", outputDir + projectName);
                        ret = cmd.run();

                        return ret;
                    case BuildType::lib:
                        ret = 0;

                        for (const auto &file : sourceFiles) {
                            ft = Build::get_file_type_from_ext(file);
                            cmd.set_length(0);
                            this->prep_compile_cmd(&cmd, ft);

                            std::string fileName = fs::path(file).stem();
                            
                            cmd.append_many("-c", file, "-o", outputDir + fileName + ".o");
                            oFiles.push_back(outputDir + fileName + ".o");

                            ret += cmd.run();
                        }

                        cmd.set_length(0);

                        cmd.append_many("ar", "rcs", "lib" + projectName + ".a");

                        for (const auto &oFile : oFiles) {
                            cmd.append(oFile);
                        }

                        ret += cmd.run();

                        return ret;
                    default:
                        throw std::invalid_argument("Unknown build type");
                }




            }


            int build_and_run() {
                int buildCode = this->build();
            
                if (buildCode != 0) {
                    return buildCode;
                }
                    
                std::string exe = "./" + projectName;

                Cmd cmd(exe);

                return cmd.run();
            }
    };



}



#endif //QBS_H
