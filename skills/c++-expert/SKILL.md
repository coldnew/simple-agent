---
name: c++-expert
description: "C++ project structure conventions for creating projects and build systems"
keywords:
  - c++
  - cpp
  - cmake
  - CMakeLists
---

When creating C++ projects, you MUST use this exact structure:

```
<project>/CMakeLists.txt
<project>/src/CMakeLists.txt
<project>/src/main.cpp
```

Rules:
1. main.cpp MUST be inside src/, never at project root
2. There MUST be two CMakeLists.txt files: one at root, one in src/
3. The root CMakeLists.txt adds src/ as a subdirectory
4. In src/CMakeLists.txt, source paths are RELATIVE to src/ — use "main.cpp" NOT "src/main.cpp"

Example src/CMakeLists.txt:
```cmake
add_executable(myapp main.cpp)
```

Example root CMakeLists.txt:
```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp)
add_subdirectory(src)
```
