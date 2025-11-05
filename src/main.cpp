#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include <unistd.h>
#include <limits>
#include "FileExplorer.h"

void printUsage() {
    std::cout << "File Explorer Application\n";
    std::cout << "Usage: file_explorer [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help     Show this help message\n";
    std::cout << "  -v, --version  Show version information\n";
    std::cout << "\nModes:\n";
    std::cout << "  1. File Explorer - Navigate and manage files\n";
    std::cout << "  2. Shell        - Execute commands with piping\n";
    std::cout << "  3. Monitor      - System monitoring tool\n\n";
}

void printVersion() {
    std::cout << "File Explorer Application v1.0.0\n";
    std::cout << "A comprehensive console-based file management tool\n";
}

void showMainMenu() {
    std::cout << "\n=== File Explorer Application ===\n";
    std::cout << "Please select a mode:\n";
    std::cout << "1. File Explorer\n";
    std::cout << "2. Shell Interface\n";
    std::cout << "3. System Monitor\n";
    std::cout << "4. Help\n";
    std::cout << "5. Exit\n";
    std::cout << "Enter your choice (1-5): ";
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage();
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printVersion();
            return 0;
        }
    }

    std::cout << "Welcome to File Explorer Application!\n";

    // Create file explorer instance
    std::unique_ptr<FileExplorer> fileExplorer = std::make_unique<FileExplorer>();

    int choice;
    while (true) {
        showMainMenu();
        std::cin >> choice;

        // Clear input buffer
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1:
                std::cout << "\n=== File Explorer Mode ===\n";
                fileExplorer->run();
                break;
            case 2:
                std::cout << "\nShell interface coming soon...\n";
                break;
            case 3:
                std::cout << "\nSystem monitor coming soon...\n";
                break;
            case 4:
                printUsage();
                break;
            case 5:
                std::cout << "Goodbye!\n";
                return 0;
            default:
                std::cout << "Invalid choice. Please try again.\n";
                break;
        }
    }

    return 0;
}