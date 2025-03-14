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
    enum FetchType { git, http };


    /**
     * @class Cmd
     * @brief A class that runs shell commands
     *
     */
    class Cmd {
        private:
            std::vector<std::string> args;
            size_t length;
    
            // to handle variadic append_many recursion end
            void append() {};
        public:
            template<typename... Args>
            Cmd(Args... args) : args{std::forward<Args>(args)...} {
                this->length = this->args.size();
            }

            /**
             * @brief add a variadic amount of arguments to this Cmd
             *
             * @param arg The first argument to append
             * @param args The rest of the arguments to append
             */
            template<typename... Args>
            void append(std::string arg, Args... args) {
                this->args.push_back(arg);
                this->length += 1;

                append(args...);
            }
    
            
            /**
             * @brief Sets the length of the cmd
             *        Main use is to reuse a Cmd object by set_length(0)
             *
             * @param length The new length
             */
            void set_length(size_t length) {
                this->length = length;
                this->args.resize(length);
            }

            /**
             * @brief Get the current length of the Cmd
             *
             * @return The current length
             */
            size_t get_length() const {
                return this->length;
            }

            /**
             * @brief Get a string representation of the Cmd
             *
             * @return String representation
             */
            std::string string() const {
                std::string sb;
                for (const auto &arg : args) {
                    sb += arg;
                    sb += " ";
                }

                return sb;
            }
            
            /**
             * @brief Runs the cmd in current user shell
             *
             * @return status code from the cmd
             */
            int run() {
                std::string cmd = this->string();

                std::cout << "[INFO] " << cmd << std::endl;
                
                return std::system(cmd.c_str());
            }
    };

    namespace Utils {
            /**
             * @brief Splits the string based on the delimeter provided
             *
             * @param str The string to be split
             * @param delim The character to split the string with
             * @return A vector of strings containing each substring from the split
             */
            std::vector<std::string> splitString(std::string str, char delim) {
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

            /**
             * @brief Fetches a file from a url
             *        DEPENDS ON WGET OR CURL
             *
             * @param url The url to receive the file
             * @param fetchType The target FetchType (http, git)
             * @return Status code from attempting to fetch file
             */
            int fetch(std::string url, FetchType fetchType = FetchType::http) {
                Cmd cmd;
                std::string mode;

                switch (fetchType) {
                    case http:
                            if (std::system("wget > /dev/null 2>&1") == 127) {
                                cmd.append("curl");
                                mode = "curl";
                            } else {
                                cmd.append("wget");
                                mode = "wget";
                            }

                            if (mode == "wget") {
                                cmd.append(url);
                            } else if (mode == "curl"){
                                cmd.append("-O", url);
                            } else {
                                throw std::logic_error("UNREACHABLE: How did you get here?");
                            }

                        break;
                    case git:
                        cmd.append("git", "clone", url);
                        
                        break;
                    
                    default:
                        throw std::invalid_argument("Unknown fetch type");
                }


                return cmd.run();
            }

            /**
             * @brief Decompress the provided path
             *        Currently supports .zip, .tar, .gz, and .rar
             *        Relies upon the following shell commands:
             *          tar
             *          gzip
             *          unrar
             *          unzip
             * @param path The file to be decompressed
             * @return  The sum of ran command's exit codes
             */
            int decompress(std::string path) {
                auto file = fs::path(path);

                if (!file.has_extension()) {
                    throw std::invalid_argument("Unable to compress file that doesn't have an extension");
                }


                std::string ext = file.extension();
        
                if (ext != ".gz" && ext != ".tar" && ext != ".zip" && ext != ".rar") {
                    throw std::invalid_argument(ext + " is not a recognized compressed file type");
                }

                Cmd cmd;
                int ret = 0;
                while (ext == ".gz" || ext == ".tar" || ext == ".zip" || ext == ".rar") {
                    std::string fileName = file.relative_path().string() + file.filename().string();
                    if (ext == ".gz") {
                        std::cout << file << std::endl;
                        cmd.append("gzip", "-d", file);
                    } else if (".tar") {
                        cmd.append("tar", "-xf", file);
                    } else if (".zip") {
                        cmd.append("unzip", file);
                    } else if (".rar") {
                        cmd.append("unrar", "x", file);
                    } else {
                        throw std::logic_error("UNREACHABLE: How did you get here?");
                    }

                    ret += cmd.run();
                    cmd.set_length(0);
                    
                    int count = file.string().length() - ext.length();
                    file = fs::path(file.string().substr(0, count));
                    ext = file.extension();
                }

                return ret;
            }

            /**
             * @brief Makes a directory at the provided path if that directory does not exist
             *
             * @param path Path to the directory to be made
             * @return Return code of `mkdir` Command
             */
            int make_dir_if_not_exists(std::string path) {
                auto dir = fs::directory_entry(path);

                if (dir.exists()) return 0;

                Cmd cmd;
                
                cmd.append("mkdir", "-p", path);

                return cmd.run();
            }
    };

    /**
     * @class Build
     * @brief Build a whole project with this api
     *
     */
    class Build {
        private:
            std::vector<std::string> sourceFiles;
            std::vector<std::string> includeDirs;
            std::vector<std::string> linkDirs;
            std::vector<std::string> linkFiles;
            std::vector<std::string> flags;
            std::vector<std::string> runArgs;
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

            static FileType get_file_type_from_ext(std::string path) {
                std::string ext = fs::path(path).extension();

                if (ext == ".cpp") return FileType::cpp;
                else if (ext == ".c") return FileType::c;
                else if (ext == ".h") return FileType::h;
                else if (ext == ".hpp") return FileType::hpp;
                else throw std::invalid_argument(ext + " is an unknown extension");
            }


            // To handle variadic recursion
            void append_flag() {}
            void append_include_dir() {}
            void append_link_dir() {}
            void append_source_file() {}
            void append_link_file() {}

            int build_and_run() {
                if (this->buildType == BuildType::lib) {
                    throw std::logic_error("Cannot run a library");
                }

                int buildCode = this->build();
            
                if (buildCode != 0) {
                    return buildCode;
                }
                    
                std::string exe = "./" + projectName;

                for (const auto &arg : this->runArgs) {
                    exe += " ";
                    exe += arg;
                }

                Cmd cmd(exe);

                return cmd.run();
            }
        public:

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

            /**
             * @brief Set the cxx version
             *
             * @param version The target CxxVersion
             */
            void set_cxx_version(CxxVersion version) {
                this->version = version;
            }

            /**
             * @brief Set the compiler type (Currently clang, g++ supported)
             *
             * @param compiler The target Compiler 
             */
            void set_compiler(Compiler compiler) {
                this->compiler = compiler;
            }

            /**
             * @brief Sets the build type (Executable, or library)
             *
             * @param buildType The target BuildType
             */
            void set_build_type(BuildType buildType) {
                this->buildType = buildType;
            }

            /**
             * @brief Sets the output directory
             *
             * @param path The target output path
             */
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

            /**
             * @brief Append "Wall" and "Wextra" flags
             */
            void enable_warnings() {
                this->append_flag("Wall", "Wextra");
            }


            /**
             * @brief Append many source files to build
             *
             * @param path The first source file
             * @param args The rest of the source files
             */
            template<typename... Args>
            void append_source_file(std::string path, Args... args) {
                if (fs::is_directory(path)) {
                    throw std::invalid_argument(path + " is not a file!");
                }

                std::string fileExt = fs::path(path).extension();

                if (fileExt != ".cpp" && fileExt != ".c")
                    return;
                
                this->sourceFiles.push_back(path);

                this->append_source_file(args...);
            }

            /**
             * @brief Append many directories to search for included files
             *
             * @param path The first include directory
             * @param args The rest of the include directories
             */
            template<typename... Args>
            void append_include_dir(std::string path, Args... args) {
                if (!fs::is_directory(path)) {
                    throw std::invalid_argument(path + " is not a directory!");
                }

                for (const auto &entry : fs::directory_iterator(path)) {
                    if (!entry.is_directory()) continue;

                    this->append_include_dir(entry.path().string());
                }

                this->includeDirs.push_back(path);

                this->append_include_dir(args...);
            }

            /**
             * @brief Append many flags to file build cmd (Do not include '-')
             *
             * @param first The first flag
             * @param args The rest of the flags
             */
            template<typename... Args>
            void append_flag(std::string first, Args... args) {
                this->flags.push_back("-" + first);

                this->append_flag(args...);
            }

            /**
             * @brief Append every source file within the provided directory
             *
             * @param path The source directory
             * @param recursive Whether or not to add source files from subdirectories
             */
            void append_source_dir(std::string path, bool recursive = true) {
                if (!fs::is_directory(path)) {
                    throw std::invalid_argument(path + " is not a directory!");
                }

                for (const auto &entry : fs::directory_iterator(path)) {
                    if (entry.is_directory()) {
                        if (!recursive) continue;

                        this->append_source_dir(entry.path().string());
                    } else {
                        this->append_source_file(entry.path().string());
                    }
                }
            }

            /**
             * @brief Append many directories to search for link files
             *
             * @param path The first link directory
             * @param args The rest of the link directories
             */
            template<typename... Args>
            void append_link_dir(std::string path, Args... args) {
                if (!fs::is_directory(path)) {
                    throw std::invalid_argument(path + " is not a directory!");
                }

                this->linkDirs.push_back(path);

                this->append_link_dir(args...);
            }

            /**
             * @brief Append many files to link
             *
             * @param first The first file to link
             * @param args  The rest of the files to link
             */
            template<typename... Args>
            void append_link_file(std::string first, Args... args) {
                this->linkFiles.push_back(first);

                this->append_link_file(args...);
            }

            /**
             * @brief Clears the build completely
             *
             * @param projectName the new name for the project
             */
            void clear(std::string projectName) {
            this->sourceFiles.clear();
            this->includeDirs.clear();
            this-> linkDirs.clear();
            this->linkFiles.clear();
            this->flags.clear();
            this->compiler = Compiler::gpp;
            this->version = CxxVersion::cpp23;
            this->buildType = BuildType::exe;
            this->projectName = projectName;
            this->outputDir = "./";
            }


            /**
             * @brief Build the project
             *
             * @return Status codes summed up from build commands
             */
            int build() {
                Cmd cmd;
                int ret;
                std::vector<std::string> oFiles;
                FileType ft;

                switch (this->buildType) {
                    case BuildType::exe:
                        ft = Build::get_file_type_from_ext(this->sourceFiles[0]);
                        this->prep_compile_cmd(&cmd, ft);
                        for (const auto &file : sourceFiles) {
                            cmd.append(file);
                        }   
                        cmd.append("-o", outputDir + projectName);
                        ret = cmd.run();

                        return ret;
                    case BuildType::lib:
                        ret = 0;

                        for (const auto &file : sourceFiles) {
                            ft = Build::get_file_type_from_ext(file);
                            cmd.set_length(0);
                            this->prep_compile_cmd(&cmd, ft);

                            std::string fileName = fs::path(file).stem();
                            
                            cmd.append("-c", file, "-o", outputDir + fileName + ".o");
                            oFiles.push_back(outputDir + fileName + ".o");

                            ret += cmd.run();
                        }

                        cmd.set_length(0);

                        cmd.append("ar", "rcs", outputDir + "lib" + projectName + ".a");

                        for (const auto &oFile : oFiles) {
                            cmd.append(oFile);
                        }

                        ret += cmd.run();

                        return ret;
                    default:
                        throw std::invalid_argument("Unknown build type");
                }




            }


            /**
             * @brief Bulid the project then run the final executable with the provided arguments as cli arguments
             *        DOES NOT WORK IF BuildType IS LIBRARY
             *
             * @param arg The first argument to add to the cli arguments when running the executable
             * @param args The rest of the arguments to add to the cli arguments when running the executable
             *
             * @return Status code from build or final executable if built properly
             */
            template<typename... Args>
            int build_and_run(std::string arg, Args... args) {
                this->runArgs.push_back(arg);
                return build_and_run(args...);
            }
    };



}



#endif //QBS_H
