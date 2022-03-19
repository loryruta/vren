---
title: "How to build"
---

### Requirements

- CMake >= 3.20.x
- C++ >= 20
- vcpkg

### Building on Linux / Windows

Clone the repository and navigate into it:
```
git clone https://github.com/loryruta/vren
cd vren
```

Use CMake to build it, don't forget to specify the vcpkg home directory:
```
mkdir build
cd build
cmake -B .. -DCMAKE_TOOLCHAIN_FILE=<path to vcpkg home>
cmake --build .
```

To test that everything went fine, you can run the demo:
```
cd build/vren_demo
./vren_demo
```
