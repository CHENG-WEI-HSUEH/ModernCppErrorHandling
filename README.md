# Objectives <br />

- Demonstrate modern C++ error handling using std::expected (C++23) + std::variant + std::visit. <br />

- Provide unit tests (GoogleTest) that verify error propagation through a simple processing pipeline. <br />

- Target environment: Ubuntu on Windows Subsystem for Linux (WSL). <br />

# Tech Stack <br />

- Language: C++23 <br />

- Unit test framework: GoogleTest (GTest) <br />

- Build system: CMake (with FetchContent to pull GTest) <br />

- Compilers : g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0 <br />

# Files <br />

- UnitTest/Basic.cpp : is the file aligned with the guidelines of the official document requirements to establish a scenario to trigger ConfigReadError...

- UnitTest/Advanced.cpp : is the file further testing and exploring the functions, expected, variant, and visit by GoogleTest; the testing error types include FileNotFoundError, PermissionError, IOError...

- Config.cpp & Config.h & main.cpp : are the official example for the functions, expected, variant, and visit.

 Copyright [2025] [Smart Surgery Technology Co.]
