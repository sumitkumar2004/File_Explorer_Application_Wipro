#include "FileExplorer.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <ctime>
#include <sstream>

FileExplorer::FileExplorer() : show_hidden_files(false) {
    navigator = std::make_unique<Navigator>();
    file_ops = std::make_unique<FileOperations>(true);
}

FileExplorer::~FileExplorer() {
}

std::string FileExplorer::formatPermissions(mode_t mode) {
    std::string perms = "----------";

    // File type
    if (S_ISDIR(mode)) perms[0] = 'd';
    else if (S_ISLNK(mode)) perms[0] = 'l';
    else if (S_ISBLK(mode)) perms[0] = 'b';
    else if (S_ISCHR(mode)) perms[0] = 'c';
    else if (S_ISFIFO(mode)) perms[0] = 'p';
    else if (S_ISSOCK(mode)) perms[0] = 's';

    // Owner permissions
    if (mode & S_IRUSR) perms[1] = 'r';
    if (mode & S_IWUSR) perms[2] = 'w';
    if (mode & S_IXUSR) perms[3] = 'x';

    // Group permissions
    if (mode & S_IRGRP) perms[4] = 'r';
    if (mode & S_IWGRP) perms[5] = 'w';
    if (mode & S_IXGRP) perms[6] = 'x';

    // Other permissions
    if (mode & S_IROTH) perms[7] = 'r';
    if (mode & S_IWOTH) perms[8] = 'w';
    if (mode & S_IXOTH) perms[9] = 'x';

    // Special permissions
    if (mode & S_ISUID) {
        if (perms[3] == 'x') perms[3] = 's';
        else perms[3] = 'S';
    }
    if (mode & S_ISGID) {
        if (perms[6] == 'x') perms[6] = 's';
        else perms[6] = 'S';
    }
    if (mode & S_ISVTX) {
        if (perms[9] == 'x') perms[9] = 't';
        else perms[9] = 'T';
    }

    return perms;
}

std::string FileExplorer::formatFileSize(size_t size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size_d = static_cast<double>(size);

    while (size_d >= 1024.0 && unit < 4) {
        size_d /= 1024.0;
        unit++;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << size_d << " " << units[unit];
    return ss.str();
}

std::string FileExplorer::formatTime(time_t timestamp) {
    char buffer[80];
    struct tm* timeinfo = localtime(&timestamp);
    strftime(buffer, sizeof(buffer), "%b %d %H:%M", timeinfo);
    return std::string(buffer);
}

std::string FileExplorer::getFileType(mode_t mode, const std::string& path) {
    if (S_ISDIR(mode)) return "Directory";
    else if (S_ISLNK(mode)) return "Symbolic Link";
    else if (S_ISBLK(mode)) return "Block Device";
    else if (S_ISCHR(mode)) return "Character Device";
    else if (S_ISFIFO(mode)) return "FIFO";
    else if (S_ISSOCK(mode)) return "Socket";
    else return "Regular File";
}

bool FileExplorer::isHiddenFile(const std::string& name) {
    return !name.empty() && name[0] == '.';
}

std::vector<FileInfo> FileExplorer::listDirectory(const std::string& path) {
    std::vector<FileInfo> files;
    std::string target_path = path.empty() ? navigator->getCurrentPath() : path;

    DIR* dir = opendir(target_path.c_str());
    if (!dir) {
        std::cerr << "Error: Cannot open directory '" << target_path << "'" << std::endl;
        return files;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;

        // Skip hidden files if not showing them (but always show . and ..)
        if (!show_hidden_files && isHiddenFile(name) && name != "." && name != "..") {
            continue;
        }

        std::string full_path = target_path + "/" + name;
        struct stat file_stat;

        if (stat(full_path.c_str(), &file_stat) == 0) {
            FileInfo info;
            info.name = name;
            info.path = full_path;
            info.permissions = formatPermissions(file_stat.st_mode);
            info.size = file_stat.st_size;
            info.type = getFileType(file_stat.st_mode, full_path);
            info.modified_time = formatTime(file_stat.st_mtime);
            info.is_hidden = isHiddenFile(name);

            files.push_back(info);
        }
    }

    closedir(dir);

    // Sort files: directories first, then files, both alphabetically
    std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
        bool a_is_dir = a.type == "Directory";
        bool b_is_dir = b.type == "Directory";

        if (a_is_dir != b_is_dir) {
            return a_is_dir > b_is_dir;
        }

        return a.name < b.name;
    });

    return files;
}

bool FileExplorer::changeDirectory(const std::string& path) {
    if (navigator->navigateTo(path)) {
        chdir(navigator->getCurrentPath().c_str());
        return true;
    }
    return false;
}

std::string FileExplorer::getCurrentPath() const {
    return navigator->getCurrentPath();
}

void FileExplorer::displayDirectory(const std::vector<FileInfo>& files) {
    if (files.empty()) {
        std::cout << "Directory is empty or cannot be accessed." << std::endl;
        return;
    }

    std::cout << "\nDirectory listing for: " << navigator->getCurrentPath() << "\n";
    std::cout << std::string(80, '=') << "\n";

    // Print header
    std::cout << std::left << std::setw(20) << "Name"
              << std::setw(12) << "Size"
              << std::setw(12) << "Type"
              << std::setw(14) << "Modified"
              << "Permissions" << "\n";
    std::cout << std::string(80, '-') << "\n";

    // Print files
    for (const auto& file : files) {
        std::string display_name = file.name;
        if (file.type == "Directory") {
            display_name += "/";
        } else if (file.type == "Symbolic Link") {
            display_name += "@";
        }

        std::cout << std::left << std::setw(20) << display_name
                  << std::setw(12) << formatFileSize(file.size)
                  << std::setw(12) << file.type
                  << std::setw(14) << file.modified_time
                  << file.permissions << "\n";
    }

    std::cout << std::string(80, '=') << "\n";
    std::cout << "Total: " << files.size() << " items\n";
}

void FileExplorer::printCurrentDirectory() {
    std::cout << "Current directory: " << current_path << std::endl;
}

void FileExplorer::run() {
    std::string command;

    printCurrentDirectory();

    while (true) {
        std::cout << "\nFile Explorer> ";
        std::getline(std::cin, command);

        if (command.empty()) {
            continue;
        }

        if (command == "exit" || command == "quit") {
            std::cout << "Exiting File Explorer..." << std::endl;
            break;
        } else if (command == "help" || command == "?") {
            std::cout << "\nAvailable commands:\n";
            std::cout << "  ls, list           - List current directory contents\n";
            std::cout << "  cd [path]          - Change directory\n";
            std::cout << "  cd ..              - Go to parent directory\n";
            std::cout << "  cd ~               - Go to home directory\n";
            std::cout << "  cd -               - Go to previous directory\n";
            std::cout << "  back               - Go back in history\n";
            std::cout << "  forward            - Go forward in history\n";
            std::cout << "  pwd                - Print current directory path\n";
            std::cout << "  bookmark [name]    - Bookmark current directory\n";
            std::cout << "  goto [name]        - Go to bookmarked directory\n";
            std::cout << "  unmark [name]      - Remove bookmark\n";
            std::cout << "  bookmarks          - Show all bookmarks\n";
            std::cout << "  history            - Show navigation history\n";
            std::cout << "  cp [src] [dst]     - Copy file/directory\n";
            std::cout << "  mv [src] [dst]     - Move/rename file/directory\n";
            std::cout << "  rm [path]          - Delete file/directory\n";
            std::cout << "  mkdir [path]       - Create directory\n";
            std::cout << "  touch [file]       - Create empty file\n";
            std::cout << "  du [path]          - Show disk usage\n";
            std::cout << "  hidden             - Toggle hidden files display\n";
            std::cout << "  help, ?            - Show this help message\n";
            std::cout << "  exit, quit         - Exit file explorer\n";
        } else if (command == "ls" || command == "list") {
            auto files = listDirectory();
            displayDirectory(files);
        } else if (command.substr(0, 2) == "cd") {
            size_t space_pos = command.find(' ');
            if (space_pos != std::string::npos) {
                std::string path = command.substr(space_pos + 1);
                if (changeDirectory(path)) {
                    printCurrentDirectory();
                }
            } else {
                changeDirectory("~");
                printCurrentDirectory();
            }
        } else if (command == "pwd") {
            printCurrentDirectory();
        } else if (command == "back") {
            if (goBack()) {
                printCurrentDirectory();
            }
        } else if (command == "forward") {
            if (goForward()) {
                printCurrentDirectory();
            }
        } else if (command.substr(0, 8) == "bookmark") {
            size_t space_pos = command.find(' ');
            if (space_pos != std::string::npos) {
                std::string name = command.substr(space_pos + 1);
                addBookmark(name, getCurrentPath(), "User bookmark");
            } else {
                std::cout << "Usage: bookmark [name]" << std::endl;
            }
        } else if (command.substr(0, 4) == "goto") {
            size_t space_pos = command.find(' ');
            if (space_pos != std::string::npos) {
                std::string name = command.substr(space_pos + 1);
                if (goToBookmark(name)) {
                    printCurrentDirectory();
                }
            } else {
                std::cout << "Usage: goto [bookmark_name]" << std::endl;
            }
        } else if (command.substr(0, 6) == "unmark") {
            size_t space_pos = command.find(' ');
            if (space_pos != std::string::npos) {
                std::string name = command.substr(space_pos + 1);
                removeBookmark(name);
            } else {
                std::cout << "Usage: unmark [bookmark_name]" << std::endl;
            }
        } else if (command == "bookmarks") {
            showBookmarks();
        } else if (command == "history") {
            showHistory();
        } else if (command.substr(0, 2) == "cp") {
            size_t first_space = command.find(' ');
            if (first_space != std::string::npos) {
                size_t second_space = command.find(' ', first_space + 1);
                if (second_space != std::string::npos) {
                    std::string source = command.substr(first_space + 1, second_space - first_space - 1);
                    std::string destination = command.substr(second_space + 1);
                    copyFile(source, destination);
                } else {
                    std::cout << "Usage: cp [source] [destination]" << std::endl;
                }
            } else {
                std::cout << "Usage: cp [source] [destination]" << std::endl;
            }
        } else if (command.substr(0, 2) == "mv") {
            size_t first_space = command.find(' ');
            if (first_space != std::string::npos) {
                size_t second_space = command.find(' ', first_space + 1);
                if (second_space != std::string::npos) {
                    std::string source = command.substr(first_space + 1, second_space - first_space - 1);
                    std::string destination = command.substr(second_space + 1);
                    moveFile(source, destination);
                } else {
                    std::cout << "Usage: mv [source] [destination]" << std::endl;
                }
            } else {
                std::cout << "Usage: mv [source] [destination]" << std::endl;
            }
        } else if (command.substr(0, 2) == "rm") {
            size_t space_pos = command.find(' ');
            if (space_pos != std::string::npos) {
                std::string path = command.substr(space_pos + 1);
                deleteFile(path);
            } else {
                std::cout << "Usage: rm [path]" << std::endl;
            }
        } else if (command.substr(0, 5) == "mkdir") {
            size_t space_pos = command.find(' ');
            if (space_pos != std::string::npos) {
                std::string path = command.substr(space_pos + 1);
                createDirectory(path);
            } else {
                std::cout << "Usage: mkdir [path]" << std::endl;
            }
        } else if (command.substr(0, 5) == "touch") {
            size_t space_pos = command.find(' ');
            if (space_pos != std::string::npos) {
                std::string path = command.substr(space_pos + 1);
                createFile(path);
            } else {
                std::cout << "Usage: touch [file]" << std::endl;
            }
        } else if (command.substr(0, 2) == "du") {
            size_t space_pos = command.find(' ');
            if (space_pos != std::string::npos) {
                std::string path = command.substr(space_pos + 1);
                showDiskUsage(path);
            } else {
                showDiskUsage();
            }
        } else if (command == "hidden") {
            show_hidden_files = !show_hidden_files;
            std::cout << "Hidden files " << (show_hidden_files ? "shown" : "hidden") << std::endl;
        } else {
            std::cout << "Unknown command: " << command << std::endl;
            std::cout << "Type 'help' for available commands." << std::endl;
        }
    }
}

bool FileExplorer::directoryExists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

void FileExplorer::setShowHidden(bool show) {
    show_hidden_files = show;
}

bool FileExplorer::getShowHidden() const {
    return show_hidden_files;
}

bool FileExplorer::goBack() {
    if (navigator->goBack()) {
        chdir(navigator->getCurrentPath().c_str());
        return true;
    }
    return false;
}

bool FileExplorer::goForward() {
    if (navigator->goForward()) {
        chdir(navigator->getCurrentPath().c_str());
        return true;
    }
    return false;
}

bool FileExplorer::goToParent() {
    if (navigator->goToParent()) {
        chdir(navigator->getCurrentPath().c_str());
        return true;
    }
    return false;
}

bool FileExplorer::goToHome() {
    if (navigator->goToHome()) {
        chdir(navigator->getCurrentPath().c_str());
        return true;
    }
    return false;
}

bool FileExplorer::addBookmark(const std::string& name, const std::string& path, const std::string& description) {
    return navigator->addBookmark(name, path, description);
}

bool FileExplorer::removeBookmark(const std::string& name) {
    return navigator->removeBookmark(name);
}

bool FileExplorer::goToBookmark(const std::string& name) {
    if (navigator->goToBookmark(name)) {
        chdir(navigator->getCurrentPath().c_str());
        return true;
    }
    return false;
}

void FileExplorer::showBookmarks() {
    auto bookmarks = navigator->getBookmarks();
    if (bookmarks.empty()) {
        std::cout << "No bookmarks saved." << std::endl;
        return;
    }

    std::cout << "\nSaved Bookmarks:\n";
    std::cout << std::string(60, '-') << "\n";
    for (const auto& bookmark : bookmarks) {
        std::cout << std::left << std::setw(15) << bookmark.name
                  << std::setw(30) << bookmark.path
                  << bookmark.description << "\n";
    }
    std::cout << std::string(60, '-') << "\n";
}

void FileExplorer::showHistory() {
    auto back_history = navigator->getHistoryBack();
    auto forward_history = navigator->getHistoryForward();

    std::cout << "\nNavigation History:\n";

    if (!back_history.empty()) {
        std::cout << "\nBack History:\n";
        for (size_t i = 0; i < back_history.size(); i++) {
            std::cout << "  " << (i + 1) << ". " << back_history[i] << "\n";
        }
    }

    std::cout << "\nCurrent: " << navigator->getCurrentPath() << "\n";

    if (!forward_history.empty()) {
        std::cout << "\nForward History:\n";
        for (size_t i = 0; i < forward_history.size(); i++) {
            std::cout << "  " << (i + 1) << ". " << forward_history[i] << "\n";
        }
    }

    if (back_history.empty() && forward_history.empty()) {
        std::cout << "No navigation history available." << std::endl;
    }
}

bool FileExplorer::copyFile(const std::string& source, const std::string& destination) {
    OperationResult result = file_ops->copy(source, destination, true, true);
    if (result.success) {
        std::cout << "Copied '" << source << "' to '" << destination << "'" << std::endl;
        return true;
    } else {
        std::cout << "Error: " << result.message << std::endl;
        return false;
    }
}

bool FileExplorer::moveFile(const std::string& source, const std::string& destination) {
    OperationResult result = file_ops->move(source, destination);
    if (result.success) {
        std::cout << "Moved '" << source << "' to '" << destination << "'" << std::endl;
        return true;
    } else {
        std::cout << "Error: " << result.message << std::endl;
        return false;
    }
}

bool FileExplorer::deleteFile(const std::string& path) {
    std::cout << "Are you sure you want to delete '" << path << "'? (y/N): ";
    std::string confirmation;
    std::getline(std::cin, confirmation);

    if (confirmation == "y" || confirmation == "Y") {
        OperationResult result = file_ops->remove(path, true, false);
        if (result.success) {
            std::cout << "Deleted '" << path << "'" << std::endl;
            return true;
        } else {
            std::cout << "Error: " << result.message << std::endl;
            return false;
        }
    } else {
        std::cout << "Delete operation cancelled." << std::endl;
        return false;
    }
}

bool FileExplorer::createDirectory(const std::string& path) {
    OperationResult result = file_ops->createDirectory(path, true);
    if (result.success) {
        std::cout << "Created directory '" << path << "'" << std::endl;
        return true;
    } else {
        std::cout << "Error: " << result.message << std::endl;
        return false;
    }
}

bool FileExplorer::createFile(const std::string& path) {
    OperationResult result = file_ops->createFile(path);
    if (result.success) {
        std::cout << "Created file '" << path << "'" << std::endl;
        return true;
    } else {
        std::cout << "Error: " << result.message << std::endl;
        return false;
    }
}

void FileExplorer::showDiskUsage(const std::string& path) {
    std::string target_path = path.empty() ? getCurrentPath() : path;
    size_t total_size = 0;

    OperationResult result = file_ops->calculateSize(target_path, true, total_size);
    if (result.success) {
        std::cout << "Disk usage for '" << target_path << "': " << file_ops->formatFileSize(total_size) << std::endl;
    } else {
        std::cout << "Error: " << result.message << std::endl;
    }
}