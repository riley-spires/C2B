#include "c2b.h"

namespace c2b {
    void Logger::log(c2b::Logger::Level level, std::string msg) {
        switch (level) {
            case INFO:
                log_info(msg);
                break;
            case ERROR:
                log_error(msg);
                break;
            case WARNING:
                log_warning(msg);
                break;
            case FATAL:
                log_fatal(msg);
                break;
            default:
                throw std::invalid_argument("Unknown log level");
        }
    }

    void Logger::log_info(std::string msg) {
        if (this->color) {
            this->stream << c2b::TermColors::GREEN;
        }
        this->stream << "[INFO] ";
        if (this->color) {
            this->stream << c2b::TermColors::RESET;
        }

        this->stream << msg << std::endl;
    }

    void Logger::log_error(std::string msg) {
        if (this->color) {
            this->stream << c2b::TermColors::RED;
        }
        this->stream << "[ERROR] ";
        if (this->color) {
            this->stream << c2b::TermColors::RESET;
        }
        this->stream << msg << std::endl;
    }

    void Logger::log_warning(std::string msg) {
        if (this->color) {
            this->stream << c2b::TermColors::YELLOW;
        }
        this->stream << "[WARNING] ";
        if (this->color) {
            this->stream << c2b::TermColors::RESET;
        }

        this->stream << msg << std::endl;
    }

    void Logger::log_fatal(std::string msg, int exit_code) {
        if (this->color) {
            this->stream << c2b::TermColors::RED;
        }
        this->stream << "[FATAL] " << msg;
        if (this->color) {
            this->stream << c2b::TermColors::RESET;
        }

        if (this->color) {
            this->stream << c2b::TermColors::RESET;
        }

        std::exit(exit_code);
    }

    namespace Loggers {
        Logger stdout(std::cout);
        Logger stderr(std::cerr);
    }

    void Cmd::clear() {
        this->length = 0;
        this->args.clear();
    }

    size_t Cmd::get_length() const {
        return this->length;
    }

    std::string Cmd::string() const {
        std::string sb;
        for (const auto &arg : args) {
            sb += arg;
            sb += " ";
        }

        return sb;
    }

    int Cmd::run() {
        return this->run_async().get();
    }

    std::future<int> Cmd::run_async() {
        this->print();

        return std::async([this]() -> int {
            int dev_null = open("/dev/null", O_WRONLY);
            pid_t pid = fork();

            if (pid == 0) {
                dup2(dev_null, STDOUT_FILENO);
                dup2(dev_null, STDERR_FILENO);

                close(dev_null);
                const char *shell = getenv("SHELL");
                execl(shell, shell,"-c", this->string().c_str(), nullptr);

                perror("execl");
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                int status;
                close(dev_null);

                waitpid(pid, &status, 0);
                return WEXITSTATUS(status);
            } else {
                perror("fork");
                exit(EXIT_FAILURE);
            }
        });
    }

    std::tuple<int, std::vector<std::string>, std::vector<std::string>> Cmd::run_capture_output() {
        return this->run_async_capture_output().get();
    }

    std::future<std::tuple<int, std::vector<std::string>, std::vector<std::string>>> Cmd::run_async_capture_output() {
        this->print();

        return std::async([this]() -> std::tuple<int, std::vector<std::string>, std::vector<std::string>> {
            std::vector<std::string> stdout, stderr;
            int stdout_pipe[2], stderr_pipe[2];

            pipe(stdout_pipe);
            pipe(stderr_pipe);
            pid_t pid = fork();

            if (pid == 0) {
                dup2(stdout_pipe[1], STDOUT_FILENO);
                close(stdout_pipe[0]);

                dup2(stderr_pipe[1], STDERR_FILENO);
                close(stderr_pipe[0]);

                const char *shell = getenv("SHELL");
                execl(shell, shell, "-c", this->string().c_str(), nullptr);

                perror("execl");
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                close(stdout_pipe[1]);
                close(stderr_pipe[1]);
                
                std::array<char, 128> buffer;
                ssize_t count;

                while ((count = read(stdout_pipe[0], buffer.data(), buffer.size() - 1)) > 0) {
                    buffer[count] = '\0';
                    std::stringstream ss(buffer.data());
                    std::string line;
                    while (std::getline(ss, line)) {
                        stderr.push_back(line);
                    }
                }
                close(stdout_pipe[0]);
        

                while ((count = read(stderr_pipe[0], buffer.data(), buffer.size() - 1)) > 0) {
                    buffer[count] = '\0';
                    std::stringstream ss(buffer.data());
                    std::string line;
                    while (std::getline(ss, line)) {
                        stderr.push_back(line);
                    }
                }
                close(stderr_pipe[0]);

                int status;
                waitpid(pid, &status, 0);
                return {WEXITSTATUS(status), stdout, stderr};
            } else {
                perror("fork");
                exit(EXIT_FAILURE);
            }

        });
    }

    int Cmd::run_redirect_output(std::ostream &std_stream, std::ostream &err_stream) {
        return this->run_async_redirect_output(std_stream, err_stream).get();
    }

    std::future<int> Cmd::run_async_redirect_output(std::ostream &std_stream, std::ostream &err_stream) {
        this->print();

        return std::async([this, &std_stream, &err_stream]() -> int{
            int stdout_pipe[2], stderr_pipe[2];

            pipe(stdout_pipe);
            pipe(stderr_pipe);
            pid_t pid = fork();

            if (pid == 0) {
                dup2(stdout_pipe[1], STDOUT_FILENO);
                close(stdout_pipe[0]);

                dup2(stderr_pipe[1], STDERR_FILENO);
                close(stderr_pipe[0]);

                const char *shell = getenv("SHELL");
                execl(shell, shell, "-c", this->string().c_str(), nullptr);

                perror("execl");
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                close(stdout_pipe[1]);
                close(stderr_pipe[1]);
                
                std::array<char, 2048> buffer;
                ssize_t count;

                while ((count = read(stdout_pipe[0], buffer.data(), buffer.size() - 1)) > 0) {
                    buffer[count] = '\0';
                    std_stream << buffer.data();
                }
                close(stdout_pipe[0]);


                while ((count = read(stderr_pipe[0], buffer.data(), buffer.size() - 1)) > 0) {
                    buffer[count] = '\0';
                    err_stream << buffer.data();
                }
                close(stderr_pipe[0]);

                int status;
                waitpid(pid, &status, 0);
                return WEXITSTATUS(status);
            } else {
                perror("fork");
                exit(EXIT_FAILURE);
            }

        });
    }

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
                cmd.clear();
                
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
    }

    void Build::set_parallel(bool parallel) {
        this->parallel = parallel;
    }

    void Build::set_incremental(bool incremental) {
        this->incremental = incremental;
    }

    void Build::set_std(c2b::Std_t std) {
        this->std = std;
    }

    void Build::set_compiler(Compiler_t compiler) {
        this->compiler = compiler;
    }

    void Build::set_build_type(BuildType build_type) {
        this->build_type = build_type;
    }

    void Build::set_output_dir(std::string path) {
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

    void Build::set_export_compile_commands(bool export_compile) {
        this->export_compile = export_compile;
    }

    void Build::enable_warnings() {
        this->append_flag("Wall", "Wextra");
    }

    void Build::rebuild_self(const int argc, char **argv, const std::string FILE_NAME) {
        assert (argc >= 1 && "Malformed cli arguments");

        if (Utils::file_older(FILE_NAME, argv[0]) == 2) {
            Cmd cmd;
            cmd.append("g++","-lc2b", FILE_NAME, "-o", argv[0]);
            cmd.run();

            cmd.clear();
            cmd.append(std::string(argv[0]));
            std::exit(cmd.run_redirect_output());
        }
    }

    void Build::append_source_dir(std::string path, bool recursive) {
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


    void Build::clear(std::string project_name) {
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

    
    int Build::build() {
        Utils::make_dir_if_not_exists(output_dir + "oFiles/");
        std::vector<std::string> o_files;
        std::vector<std::future<Cmd::Captured_Output_t>> results;
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
                    results.push_back(cmd->run_async_capture_output());
                    cmds.push_back(cmd);
                } else {
                    auto result = cmd->run_capture_output();
                    delete cmd;
                    if (std::get<0>(result) != 0) {
                        for (const auto &line : std::get<2>(result)) {
                            std::cerr << line << std::endl;
                        }
                    }

                    return std::get<0>(result);
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
                auto result = results[i].get();
                delete cmds[i];

                if (std::get<0>(result) != 0) {
                    for (const auto &line : std::get<2>(result)) {
                        std::cerr << line << std::endl;
                    }
                    return std::get<0>(result);
                }
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

                auto result = cmd.run_capture_output();
                if (std::get<0>(result) != 0) {
                    for (const auto &line : std::get<2>(result)) {
                        std::cerr << line << std::endl;
                    }
                    return std::get<0>(result);
                }   
            } else if (build_type == BuildType::LIB) {
                cmd.append("ar", "rvs", this->output_dir + "lib" + this->project_name + ".a");

                for (const auto &o : o_files) {
                    cmd.append(o);
                }

                auto result = cmd.run_capture_output();
                if (std::get<0>(result) != 0) {
                    for (const auto &line : std::get<2>(result)) {
                        std::cerr << line << std::endl;
                    }
                    return std::get<0>(result);
                }
            } else {
                throw std::runtime_error("UNREACHABLE: How did you get here?");
            }
        } else {
            Loggers::stdout.log_info("Target " + project_name + " already up to date");
        }


        return 0;
    }

    int Build::build_and_run() {
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

        auto result = cmd.run_redirect_output();

        return result;
    }
}
