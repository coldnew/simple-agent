---
name: c++-expert
description: "C++ project structure conventions for creating projects and build systems"
keywords:
  - c++
  - cpp
  - cmake
  - CMakeLists
---

When creating C++ projects, you MUST follow these rules:

1. Use the EXACT project name the user specifies. If the user says "create project in ASDF", the directory MUST be named "ASDF", NOT "hello_world" or any other name you invent.
2. main.cpp MUST be inside src/, never at project root
3. There MUST be two CMakeLists.txt files: one at root, one in src/
4. The root CMakeLists.txt adds src/ as a subdirectory
5. In src/CMakeLists.txt, source paths are RELATIVE to src/ — use "main.cpp" NOT "src/main.cpp"

Required directory structure (replace PROJECT_NAME with the user's chosen name):

```
PROJECT_NAME/CMakeLists.txt
PROJECT_NAME/src/CMakeLists.txt
PROJECT_NAME/src/main.cpp
```

Example src/CMakeLists.txt:
```cmake
add_executable(PROJECT_NAME main.cpp)
```

Example root CMakeLists.txt:
```cmake
cmake_minimum_required(VERSION 3.10)
project(PROJECT_NAME)
add_subdirectory(src)
```
