#ifndef QBS_H
#define QBS_H

#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>

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
            std::vector<std::string> includePaths;
            std::vector<std::string> flags;
            Compiler compiler;
            CxxVersion version;
            std::string projectName;

            // To handle variadic recursion
            void add_flags() {}
            void add_source_files() {}
            void add_include_paths() {}
        public:
            Build(std::string projectName,
                  CxxVersion version = CxxVersion::cpp20,
                  Compiler compiler = Compiler::gpp) {
                this->projectName = projectName;
                this->version = version;
                this->compiler = compiler;
            }

            void set_cxx_version(CxxVersion version) {
                this->version = version;
            }

            void add_source_file(std::string path) {
                this->sourceFiles.push_back(path);
            }

            template<typename... Args>
            void add_source_files(std::string first, Args... args) {
                this->add_source_file(first);
                this->add_source_files(args...);
            }

            void add_include_path(std::string path) {
                this->includePaths.push_back(path);
            }

            template<typename... Args>
            void add_include_paths(std::string first, Args... args) {
                this->add_include_path(first);
                this->add_include_paths(args...);
            }

            void add_flag(std::string flag) {
                this->flags.push_back(flag);
            }

            template<typename... Args>
            void add_flags(std::string first, Args... args) {
                this->add_flag(first);
                this->add_flags(args...);
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


                for (const auto &include : includePaths) {
                    std::string includeArg;
                    includeArg += "-I";
                    includeArg += include;
                    cmd.append(includeArg);
                }

                for (const auto &flag : flags) {
                    cmd.append(flag);
                }

                for (const auto &file : sourceFiles) {
                    cmd.append(file);
                }


                cmd.append_many("-o", projectName);


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
