# File Explorer Application

A comprehensive console-based file explorer application in C++ that provides file management, navigation, and search functionality for Linux systems.

## Features

### Core Functionality
- **Directory Listing**: View files and directories with detailed information (permissions, size, type, modification time)
- **Navigation**: Advanced navigation with history, bookmarks, and quick navigation commands
- **File Operations**: Copy, move, delete, create files and directories with progress indication
- **Search**: Powerful file and content search with pattern matching and filters
- **Interactive Interface**: User-friendly command-line interface with help system

### Available Commands

#### Navigation Commands
- `ls`, `list` - List current directory contents
- `cd [path]` - Change directory
- `cd ..` - Go to parent directory
- `cd ~` - Go to home directory
- `cd -` - Go to previous directory
- `back` - Go back in navigation history
- `forward` - Go forward in navigation history
- `pwd` - Print current directory path

#### Bookmark System
- `bookmark [name]` - Bookmark current directory
- `goto [name]` - Go to bookmarked directory
- `unmark [name]` - Remove bookmark
- `bookmarks` - Show all bookmarks
- `history` - Show navigation history

#### File Operations
- `cp [source] [destination]` - Copy files/directories (recursive)
- `mv [source] [destination]` - Move/rename files/directories
- `rm [path]` - Delete files/directories (recursive, with confirmation)
- `mkdir [path]` - Create directory (with parents if needed)
- `touch [file]` - Create empty file
- `du [path]` - Show disk usage of path

#### Display Options
- `hidden` - Toggle hidden files display
- `help`, `?` - Show available commands
- `exit`, `quit` - Exit application

## Building the Application

### Prerequisites
- C++17 compatible compiler (GCC, Clang)
- CMake 3.10 or higher
- Linux operating system

### Build Instructions

```bash
# Clone or download the project
cd File_Explorer_Application

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
make

# Run the application
./bin/file_explorer
```

### Alternative Build (without CMake)

```bash
# Direct compilation with g++
g++ -std=c++17 -Iinclude -o file_explorer src/main.cpp src/FileExplorer.cpp src/Navigator.cpp src/FileOperations.cpp src/SearchEngine.cpp -pthread

# Run the application
./file_explorer
```

## Usage Examples

### Basic Navigation
```
File Explorer> ls                    # List current directory
File Explorer> cd Documents          # Change to Documents directory
File Explorer> cd ..                 # Go to parent directory
File Explorer> bookmark docs         # Bookmark current directory as "docs"
File Explorer> goto docs             # Go to bookmarked directory
```

### File Operations
```
File Explorer> cp file.txt backup/   # Copy file to backup directory
File Explorer> mv old.txt new.txt    # Rename file
File Explorer> mkdir new_folder      # Create new directory
File Explorer> touch new_file.txt    # Create empty file
File Explorer> du .                  # Show disk usage of current directory
```

### Search Operations
```
File Explorer> find *.cpp            # Find all C++ files (planned feature)
File Explorer> grep "function" .     # Search for text in files (planned feature)
```

## Project Structure

```
File_Explorer_Application/
├── CMakeLists.txt           # Build configuration
├── README.md               # This file
├── .gitignore              # Git ignore patterns
├── include/                # Header files
│   ├── FileExplorer.h      # Main file explorer class
│   ├── Navigator.h         # Navigation and history management
│   ├── FileOperations.h    # File manipulation operations
│   └── SearchEngine.h      # Search functionality
├── src/                    # Source files
│   ├── main.cpp            # Application entry point
│   ├── FileExplorer.cpp    # File explorer implementation
│   ├── Navigator.cpp       # Navigation implementation
│   ├── FileOperations.cpp  # File operations implementation
│   └── SearchEngine.cpp    # Search implementation
└── build/                  # Build output directory
```

## Implementation Details

### Day 1: Basic Structure and Directory Listing ✅
- Project setup with CMake build system
- Basic file explorer with directory listing
- File permissions, sizes, and type display
- Error handling for invalid paths

### Day 2: Navigation Features ✅
- Advanced navigation with history (back/forward)
- Bookmark system for frequently visited directories
- Path resolution and normalization
- Integration with main file explorer

### Day 3: File Manipulation ✅
- Copy, move, delete operations with progress tracking
- Directory creation with parent directory support
- File creation and disk usage calculation
- Safe operations with confirmation prompts

### Day 4: Search Functionality ✅
- File name pattern matching (glob and regex)
- Content search within files
- Search result display and export
- Size and date-based filtering

### Day 5: Permission Management (Planned)
- File permission viewing and modification
- Ownership management
- Security validation and warnings

## System Requirements

- **Operating System**: Linux (Ubuntu 18.04+, CentOS 7+, etc.)
- **Compiler**: GCC 7+ or Clang 6+ with C++17 support
- **Memory**: Minimum 64MB RAM
- **Disk Space**: 10MB for application
- **Permissions**: Read access to directories you want to explore, write access for file operations

## Error Handling

The application includes comprehensive error handling for:
- Invalid directory paths
- Permission denied errors
- File system errors
- Invalid user input
- Memory allocation failures

## Contributing

This project is designed as an educational capstone project. The codebase demonstrates:
- Object-oriented programming in C++
- Linux system programming
- File system operations
- User interface design
- Error handling and robustness

## License

This project is part of a Wipro capstone project and is intended for educational purposes.

## Future Enhancements

- Shell interface with command execution
- System monitoring capabilities
- Advanced file comparison
- Archive management (zip, tar operations)
- Network file system support
- GUI interface option
