#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <string>
#include <vector>
#include <stack>

struct Bookmark {
    std::string name;
    std::string path;
    std::string description;
};

class Navigator {
private:
    std::stack<std::string> history_back;
    std::stack<std::string> history_forward;
    std::vector<Bookmark> bookmarks;
    std::string current_path;
    static const size_t MAX_HISTORY_SIZE = 100;

public:
    Navigator();
    ~Navigator();

    // Navigation methods
    bool navigateTo(const std::string& path);
    bool goBack();
    bool goForward();
    bool goToParent();
    bool goToHome();
    std::string getCurrentPath() const;

    // History management
    void addToHistory(const std::string& path);
    std::vector<std::string> getHistoryBack() const;
    std::vector<std::string> getHistoryForward() const;
    void clearHistory();
    bool hasBack() const;
    bool hasForward() const;

    // Bookmark management
    bool addBookmark(const std::string& name, const std::string& path, const std::string& description = "");
    bool removeBookmark(const std::string& name);
    bool goToBookmark(const std::string& name);
    std::vector<Bookmark> getBookmarks() const;
    Bookmark* getBookmark(const std::string& name);

    // Path utilities
    std::string normalizePath(const std::string& path) const;
    std::string getAbsolutePath(const std::string& path) const;
    std::string getParentPath(const std::string& path) const;
    std::string getHomeDirectory() const;

    // Current path management
    void setCurrentPath(const std::string& path);
};

#endif // NAVIGATOR_H