#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <string>
#include <functional>
#include <sys/stat.h>

// Progress callback type for long operations
using ProgressCallback = std::function<void(size_t bytes_processed, size_t total_bytes)>;

struct OperationResult {
    bool success;
    std::string message;
    int error_code;

    OperationResult(bool s = false, const std::string& msg = "", int code = 0)
        : success(s), message(msg), error_code(code) {}
};

class FileOperations {
private:
    static const size_t BUFFER_SIZE = 8192;
    bool verbose_output;
    ProgressCallback progress_callback;

    // Helper methods
    bool copyFile(const std::string& source, const std::string& destination, bool preserve_attributes = true);
    bool copyDirectory(const std::string& source, const std::string& destination, bool preserve_attributes = true);
    bool removeDirectory(const std::string& path);
    bool confirmAction(const std::string& action, const std::string& target);
    void updateProgress(size_t current, size_t total);
    std::string formatFileSize(size_t size);

public:
    FileOperations(bool verbose = false);
    ~FileOperations();

    // Configuration
    void setVerbose(bool verbose);
    void setProgressCallback(ProgressCallback callback);

    // Copy operations
    OperationResult copy(const std::string& source, const std::string& destination, bool recursive = false, bool preserve_attributes = true);
    OperationResult copyMultiple(const std::vector<std::string>& sources, const std::string& destination, bool recursive = false);

    // Move/rename operations
    OperationResult move(const std::string& source, const std::string& destination);
    OperationResult moveMultiple(const std::vector<std::string>& sources, const std::string& destination);
    OperationResult rename(const std::string& old_name, const std::string& new_name);

    // Delete operations
    OperationResult remove(const std::string& path, bool recursive = false, bool force = false);
    OperationResult removeMultiple(const std::vector<std::string>& paths, bool recursive = false, bool force = false);
    OperationResult secureDelete(const std::string& path, int passes = 3);

    // Create operations
    OperationResult createDirectory(const std::string& path, bool create_parents = false);
    OperationResult createFile(const std::string& path, const std::string& content = "");
    OperationResult createSymbolicLink(const std::string& target, const std::string& link_path);

    // Size calculation
    OperationResult calculateSize(const std::string& path, bool recursive = true, size_t& size_out);

    // Comparison operations
    OperationResult compareFiles(const std::string& file1, const std::string& file2);
    OperationResult compareDirectories(const std::string& dir1, const std::string& dir2);

    // Batch operations
    OperationResult batchCopy(const std::vector<std::pair<std::string, std::string>>& copy_pairs);
    OperationResult batchMove(const std::vector<std::pair<std::string, std::string>>& move_pairs);

    // Utility methods
    bool exists(const std::string& path);
    bool isFile(const std::string& path);
    bool isDirectory(const std::string& path);
    bool isSymbolicLink(const std::string& path);
    size_t getFileSize(const std::string& path);
    std::string getMimeType(const std::string& path);
};

#endif // FILE_OPERATIONS_H