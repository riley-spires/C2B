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

    enum Compiler { clangd, gpp };
    // Major cxx versions taken from https://en.cppreference.com/w/cpp/language/history
    enum CxxVersion { cpp11, cpp14, cpp17, cpp20, cpp23 };

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
            
            int run() {
                std::string cmd;

                for (const auto &arg : args) {
                    cmd += arg;
                    cmd += " ";
                }
                
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
            std::string projectName;
            std::string outputDir;

            // To handle variadic recursion
            void add_flags() {}
            void add_source_files() {}
            void add_include_dirs() {}
            void link_files() {}
            void add_link_dirs() {}
        public:
            Build(std::string projectName,
                  std::string outputDir = "./",
                  CxxVersion version = CxxVersion::cpp20,
                  Compiler compiler = Compiler::gpp) {
                this->projectName = projectName;
                this->version = version;
                this->compiler = compiler;

                if (outputDir[outputDir.length() - 1] != '/') {
                    outputDir += '/';
                }
                this->outputDir = outputDir;
            }

            void set_cxx_version(CxxVersion version) {
                this->version = version;
            }

            void add_source_file(std::string path) {
                if (fs::is_directory(path)) {
                    throw std::invalid_argument(path + " is not a file!");
                }
                this->sourceFiles.push_back(path);
            }

            template<typename... Args>
            void add_source_files(std::string first, Args... args) {
                this->add_source_file(first);
                this->add_source_files(args...);
            }

            void add_include_dir(std::string path) {
                if (!fs::is_directory(path)) {
                    throw std::invalid_argument(path + " is not a directory!");
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

                switch (this->compiler) {
                    case Compiler::clangd :
                        cmd.append("clangd");
                        break;
                    case Compiler::gpp:
                        cmd.append("g++");
                        break;
                    default:
                        throw std::invalid_argument("Unknown compiler option");
                }

                switch (this->version) {
                    case CxxVersion::cpp11:
                        cmd.append("--std=c++11");
                        break;
                    case CxxVersion::cpp14:
                        cmd.append("--std=c++14");
                        break;
                    case CxxVersion::cpp17:
                        cmd.append("--std=c++17");
                        break;
                    case CxxVersion::cpp20:
                        cmd.append("--std=c++20");
                        break;
                    case CxxVersion::cpp23:
                        cmd.append("--std=c++23");
                        break;
                    default:
                        throw std::invalid_argument("Unknown cpp version");
                }


                for (const auto &include : includeDirs) {
                    std::string includeArg;
                    includeArg += "-I";
                    includeArg += include;
                    cmd.append(includeArg);
                }

                for (const auto &linkDir : linkDirs) {
                    std::string linkDirArg;
                    linkDirArg += "-L";
                    linkDirArg += linkDir;
                    cmd.append(linkDirArg);
                }

                for (const auto &linkFile : linkFiles) {
                    std::string linkFileArg;
                    linkFileArg += "-l";
                    linkFileArg += linkFile;
                    cmd.append(linkFileArg);
                }

                for (const auto &flag : flags) {
                    cmd.append(flag);
                }

                for (const auto &file : sourceFiles) {
                    cmd.append(file);
                }


                cmd.append_many("-o", outputDir + projectName);


                return cmd.run();
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
