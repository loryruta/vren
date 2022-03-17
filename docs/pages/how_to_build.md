---
title: "How to build"
---

### Requirements

- CMake >= 3.20.x
- C++ >= 20
- vcpkg

### Building on Linux

Clone the repository and navigate into it:
```
git clone https://github.com/loryruta/vren
cd vren
```

Install the dependencies using vcpkg:
```
vcpkg install .
```

Use CMake to build it:
```
mkdir build
cd build
cmake ..
cmake -B .
cd ..
```

To test that everything went fine, you can run the demo:
```
cd build/vren_demo
./vren_demo
```

### Building on Windows
### Building on MacOS
