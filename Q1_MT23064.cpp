#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <filesystem>
#include <chrono>
#include <regex>

namespace fs = std::filesystem;

class ShellCommand {
public:
    virtual void execute(const std::vector<std::string>& args) = 0;
};

class CdCommand : public ShellCommand {
public:
    void execute(const std::vector<std::string>& args) override {
        if (args.size() < 2) {
            std::cout << "Usage: cd <directory>\n";
            return;
        }

        const std::string& target = args[1];

        if (target == "--help") {
            //std::cout << "Usage: cd <directory> - Change current directory to specified <directory>.\n";
            std::cout <<"----------------------\n";
            std::cout << "       cd / - Change current directory to the root directory.\n";
            std::cout << "       cd .. - Move up one directory from the current location.\n";
            std::cout << "       cd \"dir\" - Change current directory to the specified directory named \"dir\".\n";
            std::cout << "       cd --help - Shows help message.\n";

        }

        else if (target == "/") {
            fs::current_path(fs::current_path().root_path());
        } else if (target == "..") {
            fs::current_path(fs::current_path().parent_path());
        } else {
            if (fs::exists(target) && fs::is_directory(target)) {
                fs::current_path(target);
            } else {
                std::cout << "Directory doesn't exist or is not accessible.\n";
            }
        }
    }
};

class MvCommand : public ShellCommand {
public:
    void execute(const std::vector<std::string>& args) override {
        if (args.empty()) {
            std::cout << "Usage: mv <options> <source> <destination>\n";
            return;
        }

        size_t argIndex = 1;
        bool interactive = false;
        bool force = false;
        bool recursive = false;

        // Parse options
        while (argIndex < args.size() && args[argIndex][0] == '-') {
            if (args[argIndex] == "-i") {
                interactive = true;
            } else if (args[argIndex] == "-f") {
                force = true;
            } else if (args[argIndex] == "-R") {
                recursive = true;
            } else if (args[argIndex] == "--help") {
                std::cout << "Usage: mv [OPTION] <source> <destination>\n"
                             "Options:\n"
                             "  -i    Interactive: Prompt before overwrite\n"
                             "  -f    Force: Overwrite without prompt\n"
                             "  -R    Recursive: Move directories recursively\n"
                             "  --help    Display this help and exit\n";
                return;
            } else {
                std::cout << "Unknown option: " << args[argIndex] << std::endl;
                return;
            }
            ++argIndex;
        }

        if (args.size() - argIndex != 2) {
            std::cout << "Usage: mv <source> <destination>\n";
            return;
        }

        const std::string& source = args[argIndex];
        const std::string& destination = args[argIndex + 1];

        if (!fs::exists(source)) {
            std::cout << "Source file/directory does not exist.\n";
            return;
        }

        try {
            if (fs::is_directory(source) && recursive) {
                fs::copy(source, destination, fs::copy_options::recursive);
                fs::remove_all(source);
                std::cout << "Successfully moved directory " << source << " to " << destination << std::endl;
            } else {
                if (force || (!fs::exists(destination) || interactive)) {
                    fs::rename(source, destination);
                    std::cout << "Successfully moved " << source << " to " << destination << std::endl;
                } else {
                    std::cout << "Destination file exists. Use -f to force or -i for interactive move.\n";
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cout << "Error moving/renaming file/directory: " << e.what() << std::endl;
        }
    }
};



class RmCommand : public ShellCommand {
public:
    void execute(const std::vector<std::string>& args) override {
        if (args.size() < 2) {
            std::cout << "Usage: rm <file1> [file2 ...]\n";
            return;
        }

        bool recursive = false;
        bool help = false;
        std::string extension = "";

        for (size_t i = 1; i < args.size(); ++i) {
            const std::string& option = args[i];

            if (option == "-R") {
                recursive = true;
            } else if (option == "-d") {
                removeEmptyDirectories();
            } else if (option == "--help") {
                help = true;
            } else if (option.length() > 1 && option[0] == '*' && option.find('.') != std::string::npos) {
                extension = option.substr(1);
                removeFilesWithExtension(extension);
            } else {
                const std::string& file_or_dir = option;

                if (!fs::exists(file_or_dir)) {
                    std::cout << "File/directory '" << file_or_dir << "' does not exist.\n";
                    continue;
                }

                try {
                    if (fs::is_directory(file_or_dir)) {
                        if (recursive) {
                            fs::remove_all(file_or_dir);
                            std::cout << "Directory '" << file_or_dir << "' and its contents removed.\n";
                        } else {
                            std::cout << "Cannot remove directory '" << file_or_dir << "'. Use -R to remove directories.\n";
                        }
                    } else {
                        fs::remove(file_or_dir);
                        std::cout << "File '" << file_or_dir << "' removed.\n";
                    }
                } catch (const fs::filesystem_error& e) {
                    std::cout << "Error removing file/directory '" << file_or_dir << "': " << e.what() << std::endl;
                }
            }
        }

        if (help) {
            std::cout << "Usage: rm <file1> [file2 ...]\n";
            std::cout << "Options:\n";
            std::cout << "-R: Remove directories and their contents recursively\n";
            std::cout << "-d: Remove only empty directories\n";
            std::cout << "--help: Show help message\n";
            std::cout << "*extension: Remove files with the specified extension\n";
        }
    }

private:
    void removeEmptyDirectories() const {
        for (const auto& entry : fs::directory_iterator(fs::current_path())) {
            if (fs::is_directory(entry) && fs::is_empty(entry)) {
                fs::remove(entry);
                std::cout << "Directory '" << entry.path().string() << "' removed.\n";
            }
        }
    }

    void removeFilesWithExtension(const std::string& extension) const {
        std::regex pattern(".*\\." + extension + "$");
        for (const auto& entry : fs::directory_iterator(fs::current_path())) {
            if (fs::is_regular_file(entry) && std::regex_match(entry.path().filename().string(), pattern)) {
                fs::remove(entry);
                std::cout << "File '" << entry.path().string() << "' removed.\n";
            }
        }
    }
};


class LsCommand : public ShellCommand {
public:
    void execute(const std::vector<std::string>& args) override {
        bool show_help = false;
        bool recursive = false;
        bool long_format = false;
        bool reverse_order = false;

        // Check for options
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "-r") {
                reverse_order = true;
            } else if (args[i] == "-l") {
                long_format = true;
            } else if (args[i] == "-R") {
                recursive = true;
            } else if (args[i] == "--help") {
                show_help = true;
            }
        }

        if (show_help) {
            std::cout <<"-----------------\n";
            std::cout << "-r: Reverse the order of listing\n";
            std::cout << "-l: Use a long listing format\n";
            std::cout << "-R: List subdirectories recursively\n";
            std::cout << "--help: Show help message\n";
            return;
        }

        std::vector<fs::directory_entry> entries;

    if (recursive) {
        for (const auto& entry : fs::recursive_directory_iterator(fs::current_path())) {
            entries.push_back(entry);
        }
    } else {
        for (const auto& entry : fs::directory_iterator(fs::current_path())) {
            entries.push_back(entry);
        }
    }
    

    if (reverse_order) {
        std::reverse(entries.begin(), entries.end());
    }

    for (const auto& entry : entries) {
        printEntry(entry, long_format);
    }
    }



private:
    void printPermissions(const fs::perms& p) const {
        std::string perms;

        perms += ((p & fs::perms::owner_read) != fs::perms::none) ? "r" : "-";
        perms += ((p & fs::perms::owner_write) != fs::perms::none) ? "w" : "-";
        perms += ((p & fs::perms::owner_exec) != fs::perms::none) ? "x" : "-";
        perms += ((p & fs::perms::group_read) != fs::perms::none) ? "r" : "-";
        perms += ((p & fs::perms::group_write) != fs::perms::none) ? "w" : "-";
        perms += ((p & fs::perms::group_exec) != fs::perms::none) ? "x" : "-";
        perms += ((p & fs::perms::others_read) != fs::perms::none) ? "r" : "-";
        perms += ((p & fs::perms::others_write) != fs::perms::none) ? "w" : "-";
        perms += ((p & fs::perms::others_exec) != fs::perms::none) ? "x" : "-";

        std::cout << perms << " ";
    }

   void printEntry(const fs::directory_entry& entry, bool long_format) const {
        if (long_format) {
            auto status = fs::status(entry);
            if (!fs::is_directory(status)) {
                auto perms = status.permissions();
                printPermissions(perms);

                auto cftime = fs::last_write_time(entry);
                std::time_t cftime_t = std::chrono::duration_cast<std::chrono::seconds>(cftime.time_since_epoch()).count();

                // std::time_t cftime_t = std::chrono::system_clock::to_time_t(
                //     std::chrono::time_point_cast<std::chrono::system_clock::duration>(cftime));

                std::cout << std::put_time(std::localtime(&cftime_t), "%c") << " ";
                std::cout << std::setw(10) << fs::file_size(entry) << " "
                          << entry.path().filename() << std::endl;
            } else {
                std::cout << entry.path().filename() << " is a directory." << std::endl;
            }
        } else {
            std::cout << entry.path().filename() << std::endl;
        }
    }

};

class CpCommand {
public:
    void execute(const std::vector<std::string>& args) {
        bool recursive = false;
        bool help = false;
        bool showVersion = false;
        bool backup = false;
        std::string source;
        std::string destination;

        for (size_t i = 1; i < args.size(); ++i) {
            const std::string& option = args[i];

            if (option == "-r") {
                recursive = true;
            } else if (option == "--help") {
                help = true;
            } else if (option == "--version") {
                showVersion = true;
            } else if (option == "-b") {
                backup = true;
            } else {
                if (source.empty()) {
                    source = option;
                } else if (destination.empty()) {
                    destination = option;
                }
            }
        }

        if (showVersion) {
            std::cout << "cp (Version 1.0)\n";
            return;
        }

        if (help || source.empty() || destination.empty()) {
            displayHelp();
            return;
        }

        if (!fs::exists(source)) {
            std::cout << "Source file does not exist.\n";
            return;
        }

        try {
            if (fs::is_directory(source)) {
                if (recursive) {
                    // Handle directory copy
                    if (!fs::exists(destination)) {
                        fs::create_directory(destination);
                    }

                    for (const auto& entry : fs::recursive_directory_iterator(source)) {
                        fs::copy(entry, destination / entry.path().filename(), fs::copy_options::overwrite_existing);
                    }
                    std::cout << "Successfully copied directory '" << source << "' to '" << destination << "'.\n";
                } else {
                    std::cout << "Use -r option to copy directories recursively.\n";
                }
            } else {
                // Handle file copy
                if (backup) {
                    if (fs::exists(destination)) {
                        std::string backupFileName = destination + "~";
                        fs::copy_file(destination, backupFileName, fs::copy_options::overwrite_existing);
                        std::cout << "Created backup file: " << backupFileName << std::endl;
                    }

                    fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
                    std::cout << "Successfully copied file '" << source << "' to '" << destination << "'.\n";
                } else {
                    fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
                    std::cout << "Successfully copied file '" << source << "' to '" << destination << "'.\n";
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cout << "Error copying file/directory: " << e.what() << std::endl;
        }
    }

private:
    void displayHelp() const {
        std::cout << "Options:\n";
        std::cout << "-r: Copy directories and their contents recursively\n";
        std::cout << "-b: Create backups of existing files\n";
        std::cout << "--help: Show help message\n";
        std::cout << "--version: Show version information\n";
    }
};




class Shell {
private:
    std::vector<std::string> splitCommand(const std::string& command) {
        std::istringstream iss(command);
        return {std::istream_iterator<std::string>{iss},
                std::istream_iterator<std::string>{}};
    }

    std::vector<std::string> extractOptions(std::vector<std::string>& args) {
        std::vector<std::string> options;
        auto it = std::remove_if(args.begin() + 1, args.end(),
                                 [&](const std::string& arg) {
                                     if (arg.substr(0, 1) == "-") {
                                         options.push_back(arg);
                                         return true;
                                     }
                                     return false;
                                 });
        args.erase(it, args.end());
        return options;
    }

public:
    void executeCommand(const std::string& command) {
    std::vector<std::string> args = splitCommand(command);

    if (args.empty()) {
        std::cout << "No command entered.\n";
        return;
    }

    std::string cmd = args[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd == "cd") {
        if (args.size() != 2) {
            std::cout << "Usage: cd <directory>\n";
            return;
        }
        CdCommand cd;
        cd.execute(args);
    } else if (cmd == "mv") {
        MvCommand mv;
        mv.execute(args);
    } else if (cmd == "rm") {
        RmCommand rm;
        rm.execute(args);
    } else if (cmd == "ls") {
        LsCommand ls;
        ls.execute(args);
    } else if (cmd == "cp") {
        CpCommand cp;
        cp.execute(args);
    } else {
        std::cout << "Command not recognized.\n";
    }
}

};

int main() {
    Shell shell;
    std::string input;

    std::cout << "Simple Shell - Enter a command (cd, mv, rm, ls, cp):" << std::endl;

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        shell.executeCommand(input);
        std::cout <<"-----------------\n";
    }

    return 0;
}