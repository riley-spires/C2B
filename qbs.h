#ifndef QBS_H
#define QBS_H

#include <array>
#include <cassert>
#include <cmath>
#include <fstream>
#include <future>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <format>

namespace fs = std::filesystem;

// Forward declare Utils, types, and consts to be used throughout rest of file
namespace qbs {
    /**
     * @class Compiler_t
     * @brief Type for defining custom compilers
     *
     */
    struct Compiler_t {
        std::string cmd_base;
    };

    /**
     * @brief Contains predefined compilers
     *
     */
    namespace Compilers {
        const Compiler_t GCC = { .cmd_base = "gcc" };
        const Compiler_t GPP = { .cmd_base = "g++" };
        const Compiler_t CLANG = { .cmd_base = "clang" };
    }

    /**
     * @class Std_t
     * @brief Type for defining custom standard versions
     *        Major cxx versions taken from https://en.cppreference.com/w/cpp/language/history
     * 
     */
    struct Std_t {
        std::string version_flag;
        std::string extension;
    };

    /**
     * @brief Contains predefined standard versions
     *
     */
    namespace Stds {
        const Std_t CXX11 = { .version_flag = "-std=c++11", .extension = "cpp" };
        const Std_t CXX14 = { .version_flag = "-std=c++14", .extension = "cpp" };
        const Std_t CXX17 = { .version_flag = "-std=c++17", .extension = "cpp" };
        const Std_t CXX20 = { .version_flag = "-std=c++20", .extension = "cpp" };
        const Std_t CXX23 = { .version_flag = "-std=c++23", .extension = "cpp" };
    }
    enum BuildType { EXE, LIB };
    enum FetchType { GIT, HTTP };
    enum OS { WIN, MAC, LINUX, UNKNOWN };
    enum Arch { X86, X64, ARM64, ARM32, UKNOWN };

    namespace Utils {
            /**
             * @brief Splits the string based on the delimeter provided
             *
             * @param str The string to be split
             * @param delim The character to split the string with
             * @return A vector of strings containing each substring from the split
             */
            std::vector<std::string> split_string(std::string str, char delim = ' ');

            /**
             * @brief Fetches a file from a url
             *        DEPENDS ON WGET OR CURL
             *
             * @param url The url to receive the file
             * @param output_path path to desired output (including extension)
             * @param fetch_type The target FetchType (http, git)
             * @return Status code from attempting to fetch file or 45 if output_path already exists
             */
            int fetch(std::string url, std::string output_path, FetchType fetch_type = FetchType::HTTP);

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
            int decompress(std::string path);

            /**
             * @brief Makes a directory at the provided path if that directory does not exist
             *
             * @param path Path to the directory to be made
             * @return Return code of `mkdir` Command
             */
            int make_dir_if_not_exists(std::string path);

            /**
             * @brief Read all lines of the provided file
             *
             * @param path Path to the file to read
             * @return All lines of the provided file
             */
            std::vector<std::string> file_read_all(std::string path);

            /**
             * @brief Gives which file is older
             *
             * @param path1 path to the first file
             * @param path2 path to the second file
             *
             * @return 0 if path1 and path2 have same modify datetime
             *         1 if path1 is older than path2
             *         2 if path2 is older than path1
             *         3 if one of the paths do not exist
             *
             */
            int file_older(std::string path1, std::string path2);

            /**
             * @brief Tells if a file exists
             *
             * @param path Path to file
             * @return true if file exists, false if file does not exist
             */
            bool file_exists(std::string path);
            
            /**
             * @brief Gets the type of operating system
             *
             * @return The type of operating system
             */
            OS get_os();

            /**
             * @brief Gets the type of architecture of the system
             *
             * @return The type of architecture of the system
             */
            Arch get_arch();
            
    }
}

namespace qbs {
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
            void append() {}

            void print() {
                std::string cmd = this->string();
                auto split = Utils::split_string(cmd);
                cmd = "";
                for (int i = 0; i < split.size() - 1; ++i) {
                    cmd += split[i] + " ";
                }   
                
                std::cout << "[INFO] " << cmd << std::endl;
            }
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

                sb += "2>&1";

                return sb;
            }
            
            /**
             * @brief Runs the cmd in current user shell
             *
             * @return status code from the cmd
             */
            int run() {
                return this->run_async().get();
            }

            /**
             * @brief Runs a cmd asynchronously
             *
             * @return A future with the return code of the cmd
             */
            std::future<int> run_async() {
                this->print();

                return std::async([this]() {
                    FILE *stream = popen(this->string().c_str(), "r");
                    assert(stream != nullptr && "Out of ram");
                    std::array<char, 128> buffer;

                    while (fgets(buffer.data(), buffer.size(), stream) != nullptr) {}

                    return pclose(stream);
                });
            }

            /**
             * @brief Runs the command synchronously, capturing stdout & stderr
             *
             *
             * @return The return code and output of cmd ran
             */
            std::pair<int, std::vector<std::string>> run_capture_output() {
                return this->run_async_capture_output().get();
            }

            /**
             * @brief Runs the command asynchronouly, capturing stdout & stderr
             *
             * @return A ftuture with the return code and stdout/stderr of the cmd
             */
            std::future<std::pair<int, std::vector<std::string>>> run_async_capture_output() {
                this->print();

                return std::async([this]() -> std::pair<int, std::vector<std::string>> {
                    FILE *stream = popen(this->string().c_str(), "r");
                    
                    assert(stream != nullptr && "Out of ram");
                    std::array<char, 128> buffer;
                    std::string stdout;

                    while (fgets(buffer.data(), buffer.size(), stream) != nullptr) {
                        stdout += buffer.data();
                    }

                    return {pclose(stream), Utils::split_string(stdout, '\n')};
                });
            }
    };

    namespace Utils {
            std::vector<std::string> split_string(std::string str, char delim) {
                std::vector<std::string> tokens;
                std::stringstream input_stream(str);
                std::string curr_token;

                while (std::getline(input_stream, curr_token, delim)) {
                    tokens.push_back(curr_token);
                }


                return tokens;
            }

            int fetch(std::string url, std::string output_path, FetchType fetch_type) {
                if (fs::directory_entry(output_path).exists()) {
                    return 45;
                }

                Cmd cmd;

                FILE *stream = popen("wget > /dev/null 2>&1", "r");
                assert(stream != nullptr && "Out of ram");
                std::array<char, 128> buffer;
                while (fgets(buffer.data(), buffer.size(), stream) != nullptr) {}

                int wget_code = pclose(stream);

                switch (fetch_type) {
                    case HTTP:
                            if (wget_code == 127) {
                                cmd.append("curl", "-o");
                            } else {
                                cmd.append("wget", "-O");
                            }

                            cmd.append(output_path, url);
                        break;
                    case GIT:
                        cmd.append("git", "clone", url);
                        break;
                    default:
                        throw std::invalid_argument("Unknown fetch type");
                }


                return cmd.run();
            }

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
                    std::string file_name = file.relative_path().string() + file.filename().string();
                    if (ext == ".gz") {
                        cmd.append("gzip", "-dkf", file);
                    } else if (ext == ".tar") {
                        cmd.append("tar", "-xf", file);
                    } else if (ext == ".zip") {
                        cmd.append("unzip", file);
                    } else if (ext == ".rar") {
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

            int make_dir_if_not_exists(std::string path) {
                auto dir = fs::directory_entry(path);

                if (dir.exists()) return 0;

                Cmd cmd;
                
                cmd.append("mkdir", "-p", path);

                return cmd.run();
            }

            // TODO: Move file functions into nested namespace

            std::vector<std::string> file_read_all(std::string path) {
                std::vector<std::string> lines;
                std::string buf;
                std::ifstream file(path);
                
                while (std::getline(file, buf)) lines.push_back(buf);

                return lines;
            }

            int file_older(std::string path1, std::string path2) {
                const fs::path path1Path = fs::path(path1);
                const fs::path path2Path = fs::path(path2);
        
                if (!fs::directory_entry(path1Path).exists() ||
                    !fs::directory_entry(path2Path).exists()) {
                    return 2;
                }

                const auto path1_write_time = fs::last_write_time(path1Path).time_since_epoch().count();
                const auto path2_write_time = fs::last_write_time(path2Path).time_since_epoch().count();

                if (path1_write_time > path2_write_time) {
                    return 2;
                } else if (path2_write_time > path1_write_time) {
                    return 1;
                } else {
                    return 0;
                }
            }

            bool file_exists(std::string path) {
                return fs::directory_entry(path).exists();
            }

            OS get_os() {
                #if defined(_WIN32) || defined(_WIN64)
                    return OS::WIN;
                #elif defined(__linux__)
                    return OS::LINUX;
                #elif defined(__APPLE__) || defined(__MACH__)
                    return OS::MAC;
                #else
                    return OS::UNKNOWN;
                #endif
            }

            Arch get_arch() {
                #if defined(_M_X64) || defined(__x86_64__)
                    return Arch::X64;
                #elif defined(_M_IX86) || defined(__i386__)
                    return Arch::X86;
                #elif defined(_M_ARM64) || defined(__aarch64__)
                    return Arch::ARM64;
                #elif defined(_M_ARM) || defined(__arm__)
                    return Arch::ARM32;
                #else
                    return Arch::UNKNOWN;
                #endif
            }

    };

    /**
     * @class Build
     * @brief Build a whole project with this api
     *
     */
    class Build {
        private:
            std::vector<std::string> source_files;
            std::vector<std::string> include_dirs;
            std::vector<std::string> link_dirs;
            std::vector<std::string> link_files;
            std::vector<std::string> flags;
            std::vector<std::string> run_args;
            Compiler_t compiler;
            Std_t std;
            BuildType build_type;
            bool parallel, export_compile, incremental;
            std::string project_name;
            std::string output_dir;

            // To handle variadic recursion
            void append_flag() {}
            void append_include_dir() {}
            void append_link_dir() {}
            void append_source_file() {}
            void append_link_file() {}

        public:

            /**
             * @brief Build constructor defaults:
             *        std = Stds::CXX20
             *        compiler = Compilers::GPP
             *        build_type = BuildType::exe
             *        output_dir = "./build/"
             *        parallel = true
             *        incremental = true
             *        export_compile = true
             *
             * @param project_name The name of the project. Used as final executable name
             */
            Build(std::string project_name) {
                this->project_name = project_name;
                this->std = Stds::CXX20;
                this->compiler = Compilers::GPP;
                this->build_type = BuildType::EXE;
                this->output_dir = "./build/";
                this->parallel = true;
                this->incremental = true;
                this->export_compile = true;
            }

            /**
             * @brief Set whether to build parallel or not
             *
             * @param parallel The parallel value
             */
            void set_parallel(bool parallel) {
                this->parallel = parallel;
            }

            /**
             * @brief Set whether to incrementally build project or not
             *
             * @param incremental the incremental value
             */
            void set_incremental(bool incremental) {
                this->incremental = incremental;
            }

            /**
             * @brief Set the cxx version
             *
             * @param version The target CxxVersion
             */
            void set_std(Std_t std) {
                this->std = std;
            }

            /**
             * @brief Set the compiler type (Currently clang, g++ supported)
             *
             * @param compiler The target Compiler 
             */
            void set_compiler(Compiler_t compiler) {
                this->compiler = compiler;
            }

            /**
             * @brief Sets the build type (Executable, or library)
             *
             * @param build_type The target BuildType
             */
            void set_build_type(BuildType build_type) {
                this->build_type = build_type;
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

                this->output_dir = path;
            }

            /**
             * @brief Set whether to export compile commands or not
             *
             * @param export_compile the export value
             */
            void set_export_compile_commands(bool export_compile) {
                this->export_compile = export_compile;
            }

            /**
             * @brief Append "Wall" and "Wextra" flags
             */
            void enable_warnings() {
                this->append_flag("Wall", "Wextra");
            }

            /**
             * @brief Rebuilds the build source file and runs the new executable
             *        if any changes have been detected to the source file
             *
             * @param argc From main function
             * @param argv From main function
             * @param FILE_NAME The file name of the source file
             *        Recommended to pass __FILE__ as the value
             */
            static void rebuild_self(const int argc, char **argv, const std::string FILE_NAME) {
                assert (argc >= 1 && "Malformed cli arguments");

                if (Utils::file_older(FILE_NAME, argv[0]) == -1) {
                    Cmd cmd;
                    cmd.append("g++", FILE_NAME, "-o", argv[0]);
                    cmd.run();

                    cmd.set_length(0);
                    cmd.append("./" + std::string(argv[0]));
                    std::exit(cmd.run());
                }
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

                std::string file_ext = fs::path(path).extension();

                if (file_ext != ".cpp" && file_ext != ".c" && file_ext != ".cc")
                    return;
                
                this->source_files.push_back(path);

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

                this->include_dirs.push_back(path);

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

                this->link_dirs.push_back(path);

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
                this->link_files.push_back(first);

                this->append_link_file(args...);
            }

            /**
             * @brief Clears the build completely to defaults
             *        compiler = Compilers::GPP
             *        std = Stds::CXX20
             *        build_type = BuildType::exe
             *        output_dir = "./build/"
             *        parallel = true
             *        incremental = true
             *        export_compile = true
             *
             * @param project_name the new name for the project
             */
            void clear(std::string project_name) {
                this->source_files.clear();
                this->include_dirs.clear();
                this-> link_dirs.clear();
                this->link_files.clear();
                this->flags.clear();
                this->compiler = Compilers::GPP;
                this->std = Stds::CXX20;
                this->build_type = BuildType::EXE;
                this->project_name = project_name;
                this->output_dir = "./build/";
                this->parallel = true;
                this->incremental = true;
                this->export_compile = true;
            }


            /**
             * @brief Build the project
             *
             * @return Status codes summed up from build commands
             */
            int build() {
                Utils::make_dir_if_not_exists(output_dir + "oFiles/");

                int ret = 0;
                std::vector<std::string> o_files;
                std::vector<std::future<int>> results;
                std::vector<Cmd*> cmds;
                std::ofstream export_file;
                const std::string ROOT_DIR = fs::current_path();
                bool build_final_product = false;

                if (this->export_compile) {
                    export_file.open(output_dir + "compile_commands.json");

                    export_file << "[\n";
                }
                
                
                for (const auto &src : this->source_files) {
                    Cmd *cmd = new Cmd();
                    auto src_path = fs::path(src);
                    std::string ext = src_path.extension();
                    std::string file_name = src_path.stem();
                    const std::string OUTPUT_FILE = this->output_dir + "oFiles/" + file_name + ".o";


                    cmd->append(this->compiler.cmd_base);

                    for (const auto &include : include_dirs) {
                        std::string include_arg;
                        include_arg += "-I";
                        include_arg += include;
                        cmd->append(include_arg);
                    }

                    for (const auto &link_dir : link_dirs) {
                        std::string link_dir_arg;
                        link_dir_arg += "-L";
                        link_dir_arg += link_dir;
                        cmd->append(link_dir_arg);
                    }

                    for (const auto &link_file : link_files) {
                        std::string link_file_arg;
                        link_file_arg += "-l";
                        link_file_arg += link_file;
                        cmd->append(link_file_arg);
                    }

                    for (const auto &flag : flags) {
                        cmd->append(flag);
                    }

                    if (this->std.extension == ext) {
                        cmd->append(this->std.version_flag);
                    }

                    cmd->append("-c", "-o", OUTPUT_FILE, src);

                    o_files.push_back(OUTPUT_FILE);

                    if (this->export_compile) {
                        export_file << "\t{\n"
                                   << "\t\t\"directory\": \"" << ROOT_DIR << "\",\n"
                                   << "\t\t\"command\": \"" << cmd->string() << "\",\n"
                                   << "\t\t\"file\": \"" << src << "\",\n"
                                   << "\t\t\"output\": \"" << OUTPUT_FILE << "\",\n"
                                   << "\t},\n";
                    }

                    if (this->incremental && Utils::file_older(src, OUTPUT_FILE) != 1) {
                        if (this->parallel) {
                            results.push_back(cmd->run_async());
                            cmds.push_back(cmd);
                        } else {
                            ret += cmd->run();
                            delete cmd;
                        }

                        build_final_product = true;
                    } else {
                        delete cmd;
                    }
                }

                if (this->export_compile) {
                    export_file << "]";
                    export_file.close();
                }

                if (this->parallel) {
                    assert(results.size() == cmds.size() && "Results and Cmds differ in size");
                
                    for (int i = 0; i < results.size(); ++i) {
                        ret += results[i].get();
                        delete cmds[i];
                    }
                }

                if (build_final_product) {
                    Cmd cmd;
                    
                    if (this->build_type == BuildType::EXE) {
                        cmd.append(this->compiler.cmd_base);
                        
                        for (const auto &o : o_files) {
                            cmd.append(o);
                        }

                        for (const auto &L : this->link_dirs) {
                            cmd.append("-L", L);
                        }

                        for (const auto &l : this->link_files) {
                            cmd.append("-l", l);
                        }

                        cmd.append("-o", this->output_dir + this->project_name);

                        ret += cmd.run();
                    } else if (build_type == BuildType::LIB) {
                        cmd.append("ar", "rvs", this->output_dir + "lib" + this->project_name + ".a");

                        for (const auto &o : o_files) {
                            cmd.append(o);
                        }

                        ret += cmd.run();
                    } else {
                        throw std::runtime_error("UNREACHABLE: How did you get here?");
                    }
                } else {
                    std::cout << "[INFO] Target " << project_name << " already up to date" << std::endl;
                }


                return ret;
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
                this->run_args.push_back(arg);
                return build_and_run(args...);
            }

            
            /**
             * @brief Build the project then run the final executable WITHOUT any cli arguments
             *        DOES NOT WORK IF BuildType IS LIBRARY
             *
             * @return Status code from build or final executable if built properly
             */
            int build_and_run() {
                if (this->build_type == BuildType::LIB) {
                    throw std::logic_error("Cannot run a library");
                }

                int build_code = this->build();
            
                if (build_code != 0) {
                    return build_code;
                }
                    
                std::string exe = this->output_dir + project_name;

                for (const auto &arg : this->run_args) {
                    exe += " ";
                    exe += arg;
                }

                Cmd cmd(exe);

                auto result = cmd.run_capture_output();

                for (const auto &line : result.second) {
                    std::cout << line << std::endl;
                }

                return result.first;
            }
    };



}



#endif //QBS_H
