#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <string>
#include <vector>
#include <regex>
#include <sys/stat.h>

struct SearchCriteria {
    std::string name_pattern;
    std::string content_pattern;
    size_t min_size = 0;
    size_t max_size = SIZE_MAX;
    time_t modified_after = 0;
    time_t modified_before = 0;
    bool case_sensitive = true;
    bool use_regex = false;
    bool search_content = false;
    bool recursive = true;
    bool include_hidden = false;
};

struct SearchResult {
    std::string path;
    std::string name;
    std::string type;
    size_t size;
    std::vector<std::pair<int, std::string>> content_matches; // line number, matching line
};

class SearchEngine {
private:
    bool verbose_output;
    std::vector<std::regex> compiled_patterns;

    bool matchesCriteria(const std::string& path, const struct stat& file_stat, const SearchCriteria& criteria);
    bool matchesNamePattern(const std::string& name, const SearchCriteria& criteria);
    std::vector<std::pair<int, std::string>> searchInFile(const std::string& file_path, const SearchCriteria& criteria);
    void searchInDirectory(const std::string& dir_path, const SearchCriteria& criteria, std::vector<SearchResult>& results);
    std::string globToRegex(const std::string& glob_pattern);

public:
    SearchEngine(bool verbose = false);
    ~SearchEngine();

    // Configuration
    void setVerbose(bool verbose);

    // Search operations
    std::vector<SearchResult> findFiles(const std::string& search_path, const SearchCriteria& criteria);
    std::vector<SearchResult> searchContent(const std::string& search_path, const std::string& pattern, bool case_sensitive = true, bool use_regex = false);

    // Convenience methods
    std::vector<SearchResult> findByName(const std::string& search_path, const std::string& pattern, bool case_sensitive = true);
    std::vector<SearchResult> findBySize(const std::string& search_path, size_t min_size, size_t max_size = SIZE_MAX);
    std::vector<SearchResult> findByType(const std::string& search_path, const std::string& type); // "file", "dir", "link", etc.

    // Display methods
    void displayResults(const std::vector<SearchResult>& results);
    void saveResults(const std::vector<SearchResult>& results, const std::string& output_file);

    // Pattern utilities
    std::string sanitizePattern(const std::string& pattern);
    bool isValidPattern(const std::string& pattern);
};

#endif // SEARCH_ENGINE_H