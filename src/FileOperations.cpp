#include "FileOperations.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include <iomanip>

FileOperations::FileOperations(bool verbose) : verbose_output(verbose) {
}

FileOperations::~FileOperations() {
}

void FileOperations::setVerbose(bool verbose) {
    verbose_output = verbose;
}

void FileOperations::setProgressCallback(ProgressCallback callback) {
    progress_callback = callback;
}

bool FileOperations::copyFile(const std::string& source, const std::string& destination, bool preserve_attributes) {
    std::ifstream src(source, std::ios::binary);
    if (!src) {
        if (verbose_output) std::cerr << "Error: Cannot open source file: " << source << std::endl;
        return false;
    }

    std::ofstream dest(destination, std::ios::binary);
    if (!dest) {
        if (verbose_output) std::cerr << "Error: Cannot create destination file: " << destination << std::endl;
        return false;
    }

    // Get file size for progress tracking
    src.seekg(0, std::ios::end);
    size_t file_size = src.tellg();
    src.seekg(0, std::ios::beg);

    char buffer[BUFFER_SIZE];
    size_t total_copied = 0;

    while (src.read(buffer, BUFFER_SIZE)) {
        dest.write(buffer, src.gcount());
        total_copied += src.gcount();
        updateProgress(total_copied, file_size);
    }

    // Write remaining bytes
    if (src.gcount() > 0) {
        dest.write(buffer, src.gcount());
        total_copied += src.gcount();
        updateProgress(total_copied, file_size);
    }

    src.close();
    dest.close();

    if (preserve_attributes) {
        struct stat file_stat;
        if (stat(source.c_str(), &file_stat) == 0) {
            chmod(destination.c_str(), file_stat.st_mode);
        }
    }

    return true;
}

bool FileOperations::copyDirectory(const std::string& source, const std::string& destination, bool preserve_attributes) {
    DIR* dir = opendir(source.c_str());
    if (!dir) {
        if (verbose_output) std::cerr << "Error: Cannot open source directory: " << source << std::endl;
        return false;
    }

    // Create destination directory
    if (mkdir(destination.c_str(), 0755) != 0 && errno != EEXIST) {
        if (verbose_output) std::cerr << "Error: Cannot create destination directory: " << destination << std::endl;
        closedir(dir);
        return false;
    }

    struct dirent* entry;
    bool success = true;

    while ((entry = readdir(dir)) != nullptr && success) {
        std::string name = entry->d_name;

        // Skip . and ..
        if (name == "." || name == "..") {
            continue;
        }

        std::string source_path = source + "/" + name;
        std::string dest_path = destination + "/" + name;

        struct stat file_stat;
        if (stat(source_path.c_str(), &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                success = copyDirectory(source_path, dest_path, preserve_attributes);
            } else {
                success = copyFile(source_path, dest_path, preserve_attributes);
            }
        } else {
            success = false;
        }
    }

    closedir(dir);

    if (preserve_attributes && success) {
        struct stat source_stat;
        if (stat(source.c_str(), &source_stat) == 0) {
            chmod(destination.c_str(), source_stat.st_mode);
        }
    }

    return success;
}

OperationResult FileOperations::copy(const std::string& source, const std::string& destination, bool recursive, bool preserve_attributes) {
    if (!exists(source)) {
        return OperationResult(false, "Source does not exist: " + source, ENOENT);
    }

    if (isDirectory(source)) {
        if (!recursive) {
            return OperationResult(false, "Source is a directory but recursive copy not specified", EISDIR);
        }
        bool success = copyDirectory(source, destination, preserve_attributes);
        if (success) {
            return OperationResult(true, "Directory copied successfully");
        } else {
            return OperationResult(false, "Failed to copy directory", errno);
        }
    } else {
        bool success = copyFile(source, destination, preserve_attributes);
        if (success) {
            return OperationResult(true, "File copied successfully");
        } else {
            return OperationResult(false, "Failed to copy file", errno);
        }
    }
}

OperationResult FileOperations::move(const std::string& source, const std::string& destination) {
    if (!exists(source)) {
        return OperationResult(false, "Source does not exist: " + source, ENOENT);
    }

    // Try rename first (fast path)
    if (rename(source.c_str(), destination.c_str()) == 0) {
        return OperationResult(true, "File/directory moved successfully");
    }

    // If rename fails (different filesystem), fall back to copy+delete
    if (errno == EXDEV) {
        OperationResult copy_result = copy(source, destination, true, true);
        if (copy_result.success) {
            OperationResult delete_result = remove(source, true, true);
            if (delete_result.success) {
                return OperationResult(true, "File/directory moved successfully (copy+delete)");
            } else {
                return OperationResult(false, "Moved but failed to delete source", delete_result.error_code);
            }
        } else {
            return copy_result;
        }
    }

    return OperationResult(false, "Failed to move file/directory", errno);
}

OperationResult FileOperations::remove(const std::string& path, bool recursive, bool force) {
    if (!exists(path)) {
        return force ? OperationResult(true, "Path does not exist (forced)") :
                       OperationResult(false, "Path does not exist: " + path, ENOENT);
    }

    if (isDirectory(path)) {
        if (!recursive) {
            return OperationResult(false, "Path is a directory but recursive delete not specified", EISDIR);
        }
        return removeDirectory(path) ? OperationResult(true, "Directory removed successfully") :
                                      OperationResult(false, "Failed to remove directory", errno);
    } else {
        if (unlink(path.c_str()) == 0) {
            return OperationResult(true, "File removed successfully");
        } else {
            return OperationResult(false, "Failed to remove file", errno);
        }
    }
}

bool FileOperations::removeDirectory(const std::string& path) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return false;
    }

    struct dirent* entry;
    bool success = true;

    while ((entry = readdir(dir)) != nullptr && success) {
        std::string name = entry->d_name;

        // Skip . and ..
        if (name == "." || name == "..") {
            continue;
        }

        std::string full_path = path + "/" + name;

        struct stat file_stat;
        if (stat(full_path.c_str(), &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                success = removeDirectory(full_path);
            } else {
                success = (unlink(full_path.c_str()) == 0);
            }
        } else {
            success = false;
        }
    }

    closedir(dir);

    if (success) {
        success = (rmdir(path.c_str()) == 0);
    }

    return success;
}

OperationResult FileOperations::createDirectory(const std::string& path, bool create_parents) {
    if (create_parents) {
        std::filesystem::path fs_path(path);
        std::error_code ec;
        if (std::filesystem::create_directories(fs_path, ec)) {
            return OperationResult(true, "Directory created successfully");
        } else {
            return OperationResult(false, "Failed to create directory: " + ec.message(), ec.value());
        }
    } else {
        if (mkdir(path.c_str(), 0755) == 0) {
            return OperationResult(true, "Directory created successfully");
        } else {
            return OperationResult(false, "Failed to create directory", errno);
        }
    }
}

OperationResult FileOperations::createFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file) {
        return OperationResult(false, "Failed to create file", errno);
    }

    if (!content.empty()) {
        file << content;
    }

    file.close();
    return OperationResult(true, "File created successfully");
}

OperationResult FileOperations::calculateSize(const std::string& path, bool recursive, size_t& size_out) {
    size_out = 0;

    if (!exists(path)) {
        return OperationResult(false, "Path does not exist", ENOENT);
    }

    if (isFile(path)) {
        struct stat file_stat;
        if (stat(path.c_str(), &file_stat) == 0) {
            size_out = file_stat.st_size;
            return OperationResult(true, "File size calculated");
        }
        return OperationResult(false, "Failed to get file size", errno);
    }

    if (isDirectory(path) && recursive) {
        DIR* dir = opendir(path.c_str());
        if (!dir) {
            return OperationResult(false, "Cannot open directory", errno);
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;

            std::string full_path = path + "/" + name;
            size_t item_size;
            OperationResult result = calculateSize(full_path, recursive, item_size);
            if (!result.success) {
                closedir(dir);
                return result;
            }
            size_out += item_size;
        }

        closedir(dir);
    }

    return OperationResult(true, "Size calculated successfully");
}

void FileOperations::updateProgress(size_t current, size_t total) {
    if (progress_callback) {
        progress_callback(current, total);
    }
}

bool FileOperations::exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool FileOperations::isFile(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

bool FileOperations::isDirectory(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

bool FileOperations::isSymbolicLink(const std::string& path) {
    struct stat buffer;
    return (lstat(path.c_str(), &buffer) == 0 && S_ISLNK(buffer.st_mode));
}

size_t FileOperations::getFileSize(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        return buffer.st_size;
    }
    return 0;
}

std::string FileOperations::formatFileSize(size_t size) {
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

// Stub implementations for remaining methods
OperationResult FileOperations::copyMultiple(const std::vector<std::string>& sources, const std::string& destination, bool recursive) {
    return OperationResult(false, "Not implemented yet", ENOSYS);
}

OperationResult FileOperations::moveMultiple(const std::vector<std::string>& sources, const std::string& destination) {
    return OperationResult(false, "Not implemented yet", ENOSYS);
}

OperationResult FileOperations::rename(const std::string& old_name, const std::string& new_name) {
    return OperationResult(false, "Not implemented yet", ENOSYS);
}

OperationResult FileOperations::removeMultiple(const std::vector<std::string>& paths, bool recursive, bool force) {
    return OperationResult(false, "Not implemented yet", ENOSYS);
}

OperationResult FileOperations::secureDelete(const std::string& path, int passes) {
    return OperationResult(false, "Not implemented yet", ENOSYS);
}

OperationResult FileOperations::createSymbolicLink(const std::string& target, const std::string& link_path) {
    return OperationResult(false, "Not implemented yet", ENOSYS);
}

OperationResult FileOperations::compareFiles(const std::string& file1, const std::string& file2) {
    return OperationResult(false, "Not implemented yet", ENOSYS);
}

OperationResult FileOperations::compareDirectories(const std::string& dir1, const std::string& dir2) {
    return OperationResult(false, "Not implemented yet", ENOSYS);
}

OperationResult FileOperations::batchCopy(const std::vector<std::pair<std::string, std::string>>& copy_pairs) {
    return OperationResult(false, "Not implemented yet", ENOSYS);
}

OperationResult FileOperations::batchMove(const std::vector<std::pair<std::string, std::string>>& move_pairs) {
    return OperationResult(false, "Not implemented yet", ENOSYS);
}

std::string FileOperations::getMimeType(const std::string& path) {
    return "application/octet-stream";
}