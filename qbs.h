#ifndef QBS_H
#define QBS_H

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
        std::string cmdBase;
    };

    /**
     * @brief Contains predefined compilers
     *
     */
    namespace Compilers {
        const Compiler_t GCC = { .cmdBase = "gcc" };
        const Compiler_t GPP = { .cmdBase = "g++" };
        const Compiler_t CLANG = { .cmdBase = "clang" };
    }

    /**
     * @class Std_t
     * @brief Type for defining custom standard versions
     *        Major cxx versions taken from https://en.cppreference.com/w/cpp/language/history
     * 
     */
    struct Std_t {
        std::string versionFlag;
        std::string extension;
    };

    /**
     * @brief Contains predefined standard versions
     *
     */
    namespace Stds {
        const Std_t CXX11 = { .versionFlag = "-std=c++11", .extension = "cpp" };
        const Std_t CXX14 = { .versionFlag = "-std=c++14", .extension = "cpp" };
        const Std_t CXX17 = { .versionFlag = "-std=c++17", .extension = "cpp" };
        const Std_t CXX20 = { .versionFlag = "-std=c++20", .extension = "cpp" };
        const Std_t CXX23 = { .versionFlag = "-std=c++23", .extension = "cpp" };
    }
    enum BuildType { EXE, LIB };
    enum FetchType { GIT, HTTP };

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
             * @param fetchType The target FetchType (http, git)
             * @return Status code from attempting to fetch file
             */
            int fetch(std::string url, FetchType fetchType = FetchType::HTTP);

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
                    return std::system(this->string().c_str());
                });
            }

            /**
             * @brief Runs the command synchronously, redirecting cout and cerr to a file
             *        Then reads that file and returns lines.
             *
             * @todo Make operation less expensive
             *
             * @return The return code and output of cmd ran
             */
            std::pair<int, std::vector<std::string>> run_capture_output() {
                auto tempPath = fs::temp_directory_path().string();
                if (tempPath.back() != '/') tempPath += '/';
                const std::string TEMP_FILE_NAME = tempPath + "cmd.txt";

                #ifndef _WIN32
                    this->append("&>", TEMP_FILE_NAME);
                #else
                    this->append("*>", TEMP_FILE_NAME);
                #endif

                int ret = this->run();

                if (ret != 0) {
                    return {ret, std::vector<std::string>()};
                }

                std::vector<std::string> lines = Utils::file_read_all(TEMP_FILE_NAME);

                std::system(("rm -rf " + TEMP_FILE_NAME).c_str());

                return {ret, lines};
            }
    };

    namespace Utils {
            std::vector<std::string> split_string(std::string str, char delim) {
                std::vector<std::string> tokens;
                std::stringstream inputStream(str);
                std::string curr_token;

                while (std::getline(inputStream, curr_token, delim)) {
                    tokens.push_back(curr_token);
                }


                return tokens;
            }

            int fetch(std::string url, FetchType fetchType) {
                Cmd cmd;
                std::string mode;

                switch (fetchType) {
                    case HTTP:
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

            int make_dir_if_not_exists(std::string path) {
                auto dir = fs::directory_entry(path);

                if (dir.exists()) return 0;

                Cmd cmd;
                
                cmd.append("mkdir", "-p", path);

                return cmd.run();
            }

            std::vector<std::string> file_read_all(std::string path) {
                std::vector<std::string> lines;
                std::string buf;
                std::ifstream file(path);
                
                while (std::getline(file, buf)) lines.push_back(buf);

                return lines;
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
            Compiler_t compiler;
            Std_t std;
            BuildType buildType;
            bool parallel, exportCompile;
            std::string projectName;
            std::string outputDir;

            void prep_compile_cmd(Cmd *cmd, std::string extension) {
            }

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
             *        buildType = BuildType::exe
             *        outputDir = "./output/"
             *        parallel = true
             *
             * @param projectName The name of the project. Used as final executable name
             */
            Build(std::string projectName) {
                this->projectName = projectName;
                this->std = Stds::CXX20;
                this->compiler = Compilers::GPP;
                this->buildType = BuildType::EXE;
                this->outputDir = "./output/";
                this->parallel = true;

                Utils::make_dir_if_not_exists(outputDir);
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
             * @brief Rebuilds the build source file and runs the new executable
             *        if any changes have been detected to the source file
             *
             * @param argc From main function
             * @param argv From main function
             * @param FILE_NAME The file name of the source file
             *        Recommended to pass __FILE__ as the value
             */
            void enable_rebuild_self(const int argc, char **argv, const std::string FILE_NAME) {
                assert (argc >= 1 && "Malformed cli arguments");

                const fs::path self = fs::path(FILE_NAME);
                const fs::path exe = fs::path(argv[0]);

                const auto selfWriteTime = fs::last_write_time(self).time_since_epoch().count();
                const auto exeWriteTime = fs::last_write_time(exe).time_since_epoch().count();

                if (selfWriteTime > exeWriteTime) {
                    Cmd cmd;
                    cmd.append("g++", FILE_NAME, "-o", self.stem().string());
                    cmd.run();

                    cmd.set_length(0);
                    cmd.append("./" + self.stem().string());
                    cmd.run();
                    std::exit(0);
                }
            }

            void enable_export_compile_commands() {
                this->exportCompile = true;
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
             * @brief Clears the build completely to defaults
             *        compiler = Compilers::GPP
             *        std = Stds::CXX20
             *        buildType = BuildType::exe
             *        outputDir = "./output/"
             *        parallel = true
             *
             * @param projectName the new name for the project
             */
            void clear(std::string projectName) {
                this->sourceFiles.clear();
                this->includeDirs.clear();
                this-> linkDirs.clear();
                this->linkFiles.clear();
                this->flags.clear();
                this->compiler = Compilers::GPP;
                this->std = Stds::CXX20;
                this->buildType = BuildType::EXE;
                this->projectName = projectName;
                this->outputDir = "./output/";
                this->parallel = true;
            }


            /**
             * @brief Build the project
             *
             * @return Status codes summed up from build commands
             */
            int build() {
                int ret = 0;
                std::vector<std::string> oFiles;
                std::vector<std::future<int>> results;
                std::vector<Cmd*> cmds;
                std::ofstream exportFile;
                const std::string ROOT_DIR = fs::current_path();

                if (this->exportCompile) {
                    exportFile.open("compile_commands.json");

                    exportFile << "[\n";
                }
                
                
                for (const auto &src : this->sourceFiles) {
                    Cmd *cmd = new Cmd();
                    auto srcPath = fs::path(src);
                    std::string ext = srcPath.extension();
                    std::string fileName = srcPath.stem();
                    const std::string OUTPUT_FILE = this->outputDir + fileName + ".o";

                    cmd->append(this->compiler.cmdBase);

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

                    if (this->std.extension == ext) {
                        cmd->append(this->std.versionFlag);
                    }

                    cmd->append("-c", "-o", OUTPUT_FILE, src);

                    oFiles.push_back(OUTPUT_FILE);

                    if (this->exportCompile) {
                        exportFile << "\t{\n"
                                   << "\t\t\"directory\": \"" << ROOT_DIR << "\",\n"
                                   << "\t\t\"command\": \"" << cmd->string() << "\",\n"
                                   << "\t\t\"file\": \"" << src << "\",\n"
                                   << "\t\t\"output\": \"" << OUTPUT_FILE << "\",\n"
                                   << "\t},\n";
                    }

                    if (this->parallel) {
                        results.push_back(cmd->run_async());
                        cmds.push_back(cmd);
                    } else {
                        ret += cmd->run();
                        delete cmd;
                    }
                }

                if (this->exportCompile) {
                    exportFile << "]";
                    exportFile.close();
                }

                if (this->parallel) {
                    assert(results.size() == cmds.size() && "Results and Cmds differ in size");
                
                    for (int i = 0; i < results.size(); ++i) {
                        ret += results[i].get();
                        delete cmds[i];
                    }
                }

                Cmd cmd;
                
                if (this->buildType == BuildType::EXE) {
                    cmd.append(this->compiler.cmdBase);
                    
                    for (const auto &o : oFiles) {
                        cmd.append(o);
                    }

                    cmd.append("-o", this->outputDir + this->projectName);

                    ret += cmd.run();
                } else if (buildType == BuildType::LIB) {
                    cmd.append("ar", "rvs", this->projectName + ".a");

                    for (const auto &o : oFiles) {
                        cmd.append(o);
                    }

                    ret += cmd.run();
                } else {
                    throw std::runtime_error("UNREACHABLE: How did you get here?");
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
                this->runArgs.push_back(arg);
                return build_and_run(args...);
            }

            
            /**
             * @brief Build the project then run the final executable WITHOUT any cli arguments
             *        DOES NOT WORK IF BuildType IS LIBRARY
             *
             * @return Status code from build or final executable if built properly
             */
            int build_and_run() {
                if (this->buildType == BuildType::LIB) {
                    throw std::logic_error("Cannot run a library");
                }

                int buildCode = this->build();
            
                if (buildCode != 0) {
                    return buildCode;
                }
                    
                std::string exe = this->outputDir + projectName;

                for (const auto &arg : this->runArgs) {
                    exe += " ";
                    exe += arg;
                }

                Cmd cmd(exe);

                return cmd.run();
            }
    };



}



#endif //QBS_H
