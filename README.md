# Objectives
## - Demonstrate modern C++ error handling using std::expected (C++23) + std::variant + std::visit.

## - Provide unit tests (GoogleTest) that verify error propagation through a simple processing pipeline.

## - Target environment: Ubuntu on Windows Subsystem for Linux (WSL).

# - Tech Stack

- Language: C++23

- Unit test framework: GoogleTest (GTest)

- Build system: CMake (with FetchContent to pull GTest)

- Compilers : g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0

# - Directory Structure

ModernCppErrorHandling/
├─ src/
│  ├─ pipeline.hpp          # Declarations (Config/Errors + API)
│  └─ pipeline.cpp          # Definitions (LoadConfig/ValidateData/ProcessData)
├─ tests/
│  └─ test_config_read_error.cpp   # GTest: verifies ConfigReadError propagation
├─ CMakeLists.txt
└─ README.md


 Copyright [2025] [Smart Surgery Technology Co.]
