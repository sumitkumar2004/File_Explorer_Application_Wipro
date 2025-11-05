#include "SearchEngine.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>

SearchEngine::SearchEngine(bool verbose) : verbose_output(verbose) {
}

SearchEngine::~SearchEngine() {
}

void SearchEngine::setVerbose(bool verbose) {
    verbose_output = verbose;
}

std::vector<SearchResult> SearchEngine::findFiles(const std::string& search_path, const SearchCriteria& criteria) {
    std::vector<SearchResult> results;

    if (verbose_output) {
        std::cout << "Searching in: " << search_path << std::endl;
        if (!criteria.name_pattern.empty()) {
            std::cout << "Name pattern: " << criteria.name_pattern << std::endl;
        }
        if (criteria.search_content && !criteria.content_pattern.empty()) {
            std::cout << "Content pattern: " << criteria.content_pattern << std::endl;
        }
    }

    searchInDirectory(search_path, criteria, results);

    if (verbose_output) {
        std::cout << "Found " << results.size() << " matches." << std::endl;
    }

    return results;
}

void SearchEngine::searchInDirectory(const std::string& dir_path, const SearchCriteria& criteria, std::vector<SearchResult>& results) {
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) {
        if (verbose_output) {
            std::cerr << "Cannot open directory: " << dir_path << std::endl;
        }
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;

        // Skip hidden files if not included
        if (!criteria.include_hidden && !name.empty() && name[0] == '.') {
            continue;
        }

        // Skip . and ..
        if (name == "." || name == "..") {
            continue;
        }

        std::string full_path = dir_path + "/" + name;
        struct stat file_stat;

        if (stat(full_path.c_str(), &file_stat) == 0) {
            if (matchesCriteria(full_path, file_stat, criteria)) {
                SearchResult result;
                result.path = full_path;
                result.name = name;
                result.size = file_stat.st_size;

                if (S_ISDIR(file_stat.st_mode)) {
                    result.type = "Directory";
                } else if (S_ISLNK(file_stat.st_mode)) {
                    result.type = "Symbolic Link";
                } else if (S_ISREG(file_stat.st_mode)) {
                    result.type = "File";
                } else {
                    result.type = "Other";
                }

                // Search content if requested and it's a regular file
                if (criteria.search_content && S_ISREG(file_stat.st_mode)) {
                    result.content_matches = searchInFile(full_path, criteria);
                }

                results.push_back(result);
            }

            // Recursively search subdirectories
            if (criteria.recursive && S_ISDIR(file_stat.st_mode)) {
                searchInDirectory(full_path, criteria, results);
            }
        }
    }

    closedir(dir);
}

bool SearchEngine::matchesCriteria(const std::string& path, const struct stat& file_stat, const SearchCriteria& criteria) {
    std::string name = path.substr(path.find_last_of('/') + 1);

    // Check name pattern
    if (!criteria.name_pattern.empty() && !matchesNamePattern(name, criteria)) {
        return false;
    }

    // Check size criteria
    if (file_stat.st_size < criteria.min_size || file_stat.st_size > criteria.max_size) {
        return false;
    }

    // Check modification time
    if (criteria.modified_after > 0 && file_stat.st_mtime < criteria.modified_after) {
        return false;
    }
    if (criteria.modified_before > 0 && file_stat.st_mtime > criteria.modified_before) {
        return false;
    }

    return true;
}

bool SearchEngine::matchesNamePattern(const std::string& name, const SearchCriteria& criteria) {
    if (criteria.use_regex) {
        try {
            std::regex pattern(criteria.name_pattern,
                criteria.case_sensitive ? std::regex::ECMAScript : std::regex::ECMAScript | std::regex::icase);
            return std::regex_search(name, pattern);
        } catch (const std::regex_error& e) {
            if (verbose_output) {
                std::cerr << "Invalid regex pattern: " << e.what() << std::endl;
            }
            return false;
        }
    } else {
        // Convert glob pattern to regex
        std::string regex_pattern = globToRegex(criteria.name_pattern);
        try {
            std::regex pattern(regex_pattern,
                criteria.case_sensitive ? std::regex::ECMAScript : std::regex::ECMAScript | std::regex::icase);
            return std::regex_search(name, pattern);
        } catch (const std::regex_error& e) {
            if (verbose_output) {
                std::cerr << "Invalid pattern: " << e.what() << std::endl;
            }
            return false;
        }
    }
}

std::vector<std::pair<int, std::string>> SearchEngine::searchInFile(const std::string& file_path, const SearchCriteria& criteria) {
    std::vector<std::pair<int, std::string>> matches;

    std::ifstream file(file_path);
    if (!file) {
        return matches;
    }

    std::regex pattern;
    try {
        pattern = std::regex(criteria.content_pattern,
            criteria.case_sensitive ? std::regex::ECMAScript : std::regex::ECMAScript | std::regex::icase);
    } catch (const std::regex_error& e) {
        if (verbose_output) {
            std::cerr << "Invalid content regex: " << e.what() << std::endl;
        }
        return matches;
    }

    std::string line;
    int line_number = 1;

    while (std::getline(file, line)) {
        if (std::regex_search(line, pattern)) {
            matches.emplace_back(line_number, line);
        }
        line_number++;
    }

    return matches;
}

std::vector<SearchResult> SearchEngine::searchContent(const std::string& search_path, const std::string& pattern, bool case_sensitive, bool use_regex) {
    SearchCriteria criteria;
    criteria.content_pattern = pattern;
    criteria.case_sensitive = case_sensitive;
    criteria.use_regex = use_regex;
    criteria.search_content = true;
    criteria.recursive = true;

    return findFiles(search_path, criteria);
}

std::vector<SearchResult> SearchEngine::findByName(const std::string& search_path, const std::string& pattern, bool case_sensitive) {
    SearchCriteria criteria;
    criteria.name_pattern = pattern;
    criteria.case_sensitive = case_sensitive;
    criteria.recursive = true;

    return findFiles(search_path, criteria);
}

std::vector<SearchResult> SearchEngine::findBySize(const std::string& search_path, size_t min_size, size_t max_size) {
    SearchCriteria criteria;
    criteria.min_size = min_size;
    criteria.max_size = max_size;
    criteria.recursive = true;

    return findFiles(search_path, criteria);
}

std::vector<SearchResult> SearchEngine::findByType(const std::string& search_path, const std::string& type) {
    SearchCriteria criteria;
    criteria.recursive = true;

    // We'll filter by type after getting all results
    auto all_results = findFiles(search_path, criteria);
    std::vector<SearchResult> filtered_results;

    for (const auto& result : all_results) {
        if ((type == "file" && result.type == "File") ||
            (type == "dir" && result.type == "Directory") ||
            (type == "link" && result.type == "Symbolic Link")) {
            filtered_results.push_back(result);
        }
    }

    return filtered_results;
}

void SearchEngine::displayResults(const std::vector<SearchResult>& results) {
    if (results.empty()) {
        std::cout << "No matches found." << std::endl;
        return;
    }

    std::cout << "\nSearch Results (" << results.size() << " found):\n";
    std::cout << std::string(80, '=') << "\n";

    for (const auto& result : results) {
        std::cout << result.path;
        if (result.type == "Directory") {
            std::cout << "/";
        }
        std::cout << " [" << result.type << "]";

        if (result.type == "File") {
            const char* units[] = {"B", "KB", "MB", "GB", "TB"};
            int unit = 0;
            double size_d = static_cast<double>(result.size);

            while (size_d >= 1024.0 && unit < 4) {
                size_d /= 1024.0;
                unit++;
            }

            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << size_d << " " << units[unit];
            std::cout << " (" << ss.str() << ")";
        }

        std::cout << "\n";

        // Show content matches if available
        if (!result.content_matches.empty()) {
            std::cout << "  Content matches:\n";
            for (const auto& match : result.content_matches) {
                std::cout << "    Line " << match.first << ": " << match.second << "\n";
            }
            std::cout << "\n";
        }
    }

    std::cout << std::string(80, '=') << "\n";
}

std::string SearchEngine::globToRegex(const std::string& glob_pattern) {
    std::string regex;
    regex.reserve(glob_pattern.length() * 2);

    for (char c : glob_pattern) {
        switch (c) {
            case '*':
                regex += ".*";
                break;
            case '?':
                regex += ".";
                break;
            case '.':
            case '^':
            case '$':
            case '+':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '|':
            case '\\':
                regex += "\\";
                regex += c;
                break;
            default:
                regex += c;
                break;
        }
    }

    return regex;
}


void SearchEngine::saveResults(const std::vector<SearchResult>& results, const std::string& output_file) {
    std::ofstream file(output_file);
    if (!file) {
        std::cerr << "Error: Cannot create output file: " << output_file << std::endl;
        return;
    }

    file << "Search Results (" << results.size() << " found):\n";
    file << std::string(80, '=') << "\n";

    for (const auto& result : results) {
        file << result.path << " [" << result.type << "]";
        if (result.type == "File") {
            const char* units[] = {"B", "KB", "MB", "GB", "TB"};
            int unit = 0;
            double size_d = static_cast<double>(result.size);

            while (size_d >= 1024.0 && unit < 4) {
                size_d /= 1024.0;
                unit++;
            }

            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << size_d << " " << units[unit];
            file << " (" << ss.str() << ")";
        }
        file << "\n";

        if (!result.content_matches.empty()) {
            file << "  Content matches:\n";
            for (const auto& match : result.content_matches) {
                file << "    Line " << match.first << ": " << match.second << "\n";
            }
            file << "\n";
        }
    }

    file.close();
    std::cout << "Results saved to: " << output_file << std::endl;
}

std::string SearchEngine::sanitizePattern(const std::string& pattern) {
    // Basic pattern sanitization
    if (pattern.empty()) return "*";
    return pattern;
}

bool SearchEngine::isValidPattern(const std::string& pattern) {
    try {
        if (pattern.empty()) return true;
        std::regex test(pattern);
        return true;
    } catch (const std::regex_error&) {
        return false;
    }
}