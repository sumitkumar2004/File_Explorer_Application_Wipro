#include "Navigator.h"
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sstream>

Navigator::Navigator() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        current_path = cwd;
    } else {
        current_path = "/";
    }
}

Navigator::~Navigator() {
}

bool Navigator::navigateTo(const std::string& path) {
    std::string target_path = getAbsolutePath(path);

    // Check if path exists and is a directory
    struct stat info;
    if (stat(target_path.c_str(), &info) != 0 || !S_ISDIR(info.st_mode)) {
        std::cerr << "Error: Path '" << target_path << "' does not exist or is not a directory." << std::endl;
        return false;
    }

    // Add current path to back history before navigating
    if (!current_path.empty()) {
        history_back.push(current_path);

        // Limit history size
        while (history_back.size() > MAX_HISTORY_SIZE) {
            history_back.pop();
        }
    }

    // Clear forward history when navigating to a new path
    while (!history_forward.empty()) {
        history_forward.pop();
    }

    current_path = normalizePath(target_path);
    return true;
}

bool Navigator::goBack() {
    if (history_back.empty()) {
        std::cout << "No previous directory in history." << std::endl;
        return false;
    }

    // Add current path to forward history
    history_forward.push(current_path);

    // Get previous path from back history
    current_path = history_back.top();
    history_back.pop();

    return true;
}

bool Navigator::goForward() {
    if (history_forward.empty()) {
        std::cout << "No forward history available." << std::endl;
        return false;
    }

    // Add current path to back history
    history_back.push(current_path);

    // Get next path from forward history
    current_path = history_forward.top();
    history_forward.pop();

    return true;
}

bool Navigator::goToParent() {
    std::string parent_path = getParentPath(current_path);
    return navigateTo(parent_path);
}

bool Navigator::goToHome() {
    std::string home_path = getHomeDirectory();
    return navigateTo(home_path);
}

std::string Navigator::getCurrentPath() const {
    return current_path;
}

void Navigator::addToHistory(const std::string& path) {
    if (!path.empty() && path != current_path) {
        history_back.push(current_path);

        // Limit history size
        while (history_back.size() > MAX_HISTORY_SIZE) {
            history_back.pop();
        }
    }
}

std::vector<std::string> Navigator::getHistoryBack() const {
    std::vector<std::string> history;
    std::stack<std::string> temp_stack = history_back;

    while (!temp_stack.empty()) {
        history.insert(history.begin(), temp_stack.top());
        temp_stack.pop();
    }

    return history;
}

std::vector<std::string> Navigator::getHistoryForward() const {
    std::vector<std::string> history;
    std::stack<std::string> temp_stack = history_forward;

    while (!temp_stack.empty()) {
        history.insert(history.begin(), temp_stack.top());
        temp_stack.pop();
    }

    return history;
}

void Navigator::clearHistory() {
    while (!history_back.empty()) {
        history_back.pop();
    }
    while (!history_forward.empty()) {
        history_forward.pop();
    }
}

bool Navigator::hasBack() const {
    return !history_back.empty();
}

bool Navigator::hasForward() const {
    return !history_forward.empty();
}

bool Navigator::addBookmark(const std::string& name, const std::string& path, const std::string& description) {
    // Check if bookmark with this name already exists
    for (const auto& bookmark : bookmarks) {
        if (bookmark.name == name) {
            std::cout << "Bookmark '" << name << "' already exists." << std::endl;
            return false;
        }
    }

    // Validate path
    std::string full_path = getAbsolutePath(path);
    struct stat info;
    if (stat(full_path.c_str(), &info) != 0 || !S_ISDIR(info.st_mode)) {
        std::cout << "Cannot bookmark invalid path: '" << full_path << "'" << std::endl;
        return false;
    }

    Bookmark new_bookmark;
    new_bookmark.name = name;
    new_bookmark.path = full_path;
    new_bookmark.description = description.empty() ? name : description;

    bookmarks.push_back(new_bookmark);
    std::cout << "Bookmark '" << name << "' added successfully." << std::endl;
    return true;
}

bool Navigator::removeBookmark(const std::string& name) {
    for (auto it = bookmarks.begin(); it != bookmarks.end(); ++it) {
        if (it->name == name) {
            bookmarks.erase(it);
            std::cout << "Bookmark '" << name << "' removed." << std::endl;
            return true;
        }
    }

    std::cout << "Bookmark '" << name << "' not found." << std::endl;
    return false;
}

bool Navigator::goToBookmark(const std::string& name) {
    for (const auto& bookmark : bookmarks) {
        if (bookmark.name == name) {
            return navigateTo(bookmark.path);
        }
    }

    std::cout << "Bookmark '" << name << "' not found." << std::endl;
    return false;
}

std::vector<Bookmark> Navigator::getBookmarks() const {
    return bookmarks;
}

Bookmark* Navigator::getBookmark(const std::string& name) {
    for (auto& bookmark : bookmarks) {
        if (bookmark.name == name) {
            return &bookmark;
        }
    }
    return nullptr;
}

std::string Navigator::normalizePath(const std::string& path) const {
    std::string normalized = path;

    // Replace multiple slashes with single slash
    while (normalized.find("//") != std::string::npos) {
        normalized.replace(normalized.find("//"), 2, "/");
    }

    // Remove trailing slash (except for root)
    if (normalized.length() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }

    return normalized;
}

std::string Navigator::getAbsolutePath(const std::string& path) const {
    if (path.empty()) {
        return current_path;
    }

    // Handle special cases
    if (path == "~") {
        return getHomeDirectory();
    }
    if (path == "-") {
        return hasBack() ? history_back.top() : current_path;
    }

    std::string absolute_path;

    // If path starts with /, it's already absolute
    if (path[0] == '/') {
        absolute_path = path;
    } else {
        // Relative path - join with current path
        absolute_path = current_path + "/" + path;
    }

    // Handle .. and . components
    std::vector<std::string> components;
    std::stringstream ss(absolute_path);
    std::string component;

    while (std::getline(ss, component, '/')) {
        if (component.empty() || component == ".") {
            continue;
        } else if (component == "..") {
            if (!components.empty() && components.back() != "..") {
                components.pop_back();
            } else if (components.empty()) {
                // Can't go above root
                components.push_back("..");
            }
        } else {
            components.push_back(component);
        }
    }

    // Reconstruct path
    std::string result;
    for (const auto& comp : components) {
        if (result.empty() || result.back() != '/') {
            result += "/" + comp;
        } else {
            result += comp;
        }
    }

    // Handle case where we end up with just "/"
    if (result.empty()) {
        result = "/";
    }

    return normalizePath(result);
}

std::string Navigator::getParentPath(const std::string& path) const {
    if (path.empty() || path == "/") {
        return "/";
    }

    size_t last_slash = path.find_last_of('/');
    if (last_slash == std::string::npos) {
        return "/";
    }

    if (last_slash == 0) {
        return "/";
    }

    return path.substr(0, last_slash);
}

std::string Navigator::getHomeDirectory() const {
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        return std::string(pw->pw_dir);
    }
    return "/"; // Fallback to root if home directory not found
}

void Navigator::setCurrentPath(const std::string& path) {
    current_path = normalizePath(path);
}