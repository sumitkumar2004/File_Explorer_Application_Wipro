#ifndef FILE_EXPLORER_H
#define FILE_EXPLORER_H

#include <string>
#include <vector>
#include <sys/stat.h>
#include "Navigator.h"
#include "FileOperations.h"

struct FileInfo {
    std::string name;
    std::string path;
    std::string permissions;
    size_t size;
    std::string type;
    std::string modified_time;
    bool is_hidden;
};

class FileExplorer {
private:
    std::unique_ptr<Navigator> navigator;
    std::unique_ptr<FileOperations> file_ops;
    bool show_hidden_files;

    std::string formatPermissions(mode_t mode);
    std::string formatFileSize(size_t size);
    std::string formatTime(time_t timestamp);
    std::string getFileType(mode_t mode, const std::string& path);
    bool isHiddenFile(const std::string& name);

public:
    FileExplorer();
    ~FileExplorer();

    // Basic directory operations
    std::vector<FileInfo> listDirectory(const std::string& path = "");
    bool changeDirectory(const std::string& path);
    std::string getCurrentPath() const;

    // Navigation methods (delegated to Navigator)
    bool goBack();
    bool goForward();
    bool goToParent();
    bool goToHome();
    bool addBookmark(const std::string& name, const std::string& path, const std::string& description = "");
    bool removeBookmark(const std::string& name);
    bool goToBookmark(const std::string& name);
    void showBookmarks();
    void showHistory();

    // File operations
    bool copyFile(const std::string& source, const std::string& destination);
    bool moveFile(const std::string& source, const std::string& destination);
    bool deleteFile(const std::string& path);
    bool createDirectory(const std::string& path);
    bool createFile(const std::string& path);
    void showDiskUsage(const std::string& path = "");

    // Display methods
    void displayDirectory(const std::vector<FileInfo>& files);
    void printCurrentDirectory();

    // Main application loop
    void run();

    // Utility methods
    bool directoryExists(const std::string& path);
    void setShowHidden(bool show);
    bool getShowHidden() const;
};

#endif // FILE_EXPLORER_H