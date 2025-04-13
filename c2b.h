#ifndef C2B_H
#define C2B_H

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
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
namespace c2b {
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

    namespace TermColors {
        const std::string RED = "\033[0;31m";
        const std::string GREEN = "\033[0;32m";
        const std::string YELLOW = "\033[0;33m";
        const std::string BLUE = "\033[0;34m";
        const std::string MAGENTA = "\033[0;35m";
        const std::string CYAN = "\033[0;36m";
        const std::string WHITE = "\033[0;37m";
        const std::string RESET = "\033[0m";
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

    /**
     * @class Logger
     * @brief A class that logs messages to a stream
     *
     */
    class Logger {
        private:
            std::ostream &stream;
        public:
            Logger(std::ostream &stream) : stream(stream) {}

            enum Level { INFO, ERROR, WARNING, FATAL };

            /**
             * @brief Logs a message to the provided stream
             *
             * @param level The level to log
             * @param msg The message to log
             */
            void log(Level level, std::string msg);
            
            /**
             * @brief Log a message to the provided stream with info level
             *
             * @param msg The message to log
             */
            void log_info(std::string msg);

            /**
             * @brief Log a message to the provided stream with error level
             *
             * @param msg The message to log
             */
            void log_error(std::string msg);

            /**
             * @brief Log a message to the provided stream with warning level
             *
             * @param msg The message to log
             */
            void log_warning(std::string msg);

            /**
             * @brief Log a message to the provided stream with fatal level
             *        and exit the program with the provided exit code
             *
             * @param msg The message to log
             * @param exit_code The exit code to exit with
             */
            void log_fatal(std::string msg,  int exit_code = 1);
    };

    namespace Loggers {
        extern Logger stdout;
        extern Logger stderr;
    }

    /**
     * @class Cmd
     * @brief A class that runs shell commands
     *
     * @note This class does not support stdin. 
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
                
                Loggers::stdout.log_info(cmd);
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
             * @brief Clears the cmd
             *
             */
            void clear();

            /**
             * @brief Get the current length of the Cmd
             *
             * @return The current length
             */
            size_t get_length() const;

            /**
             * @brief Get a string representation of the Cmd
             *
             * @return String representation
             */
            std::string string() const;
            
            /**
             * @brief Runs the cmd in current user shell without any output
             *
             * @return status code from the cmd
             */
            int run();

            /**
             * @brief Runs a cmd asynchronously without any output
             *
             * @return A future with the return code of the cmd
             */
            std::future<int> run_async();

            /**
             * @brief Runs the command synchronously, capturing stdout & stderr
             *
             *
             * @return The return code, stdout, and stderr of cmd ran
             */
            std::tuple<int, std::vector<std::string>, std::vector<std::string>> run_capture_output();

            /**
             * @brief Runs the command asynchronouly, capturing stdout & stderr
             *
             * @return A future with the return code, stdout, and stderr of the cmd
             */
            std::future<std::tuple<int, std::vector<std::string>, std::vector<std::string>>> run_async_capture_output();

            std::future<int> run_async_redirect_output(std::ostream &std_stream = std::cout, std::ostream &err_stream = std::cerr);

            int run_redirect_output(std::ostream &std_stream = std::cout, std::ostream &err_stream = std::cerr);
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
            void set_parallel(bool parallel);

            /**
             * @brief Set whether to incrementally build project or not
             *
             * @param incremental the incremental value
             */
            void set_incremental(bool incremental);

            /**
             * @brief Set the cxx version
             *
             * @param version The target CxxVersion
             */
            void set_std(Std_t std);

            /**
             * @brief Set the compiler type (Currently clang, g++ supported)
             *
             * @param compiler The target Compiler 
             */
            void set_compiler(Compiler_t compiler);

            /**
             * @brief Sets the build type (Executable, or library)
             *
             * @param build_type The target BuildType
             */
            void set_build_type(BuildType build_type);

            /**
             * @brief Sets the output directory
             *
             * @param path The target output path
             */
            void set_output_dir(std::string path);

            /**
             * @brief Set whether to export compile commands or not
             *
             * @param export_compile the export value
             */
            void set_export_compile_commands(bool export_compile);

            /**
             * @brief Append "Wall" and "Wextra" flags
             */
            void enable_warnings();

            /**
             * @brief Rebuilds the build source file and runs the new executable
             *        if any changes have been detected to the source file
             *
             * @param argc From main function
             * @param argv From main function
             * @param FILE_NAME The file name of the source file
             *        Recommended to pass __FILE__ as the value
             */
            static void rebuild_self(const int argc, char **argv, const std::string FILE_NAME);


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
            void append_source_dir(std::string path, bool recursive = true);

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
            void clear(std::string project_name);


            /**
             * @brief Build the project
             *
             * @return Status codes summed up from build commands
             */
            int build(); 

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
            int build_and_run(); 
    };
}

#endif //C2B_H
