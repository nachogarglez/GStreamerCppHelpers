![CI Status](https://github.com/nachogarglez/GStreamerCppHelpers/actions/workflows/ci.yml/badge.svg)

# GStreamerCppHelpers

**GStreamerCppHelpers** is a set of lightweight C++ headers designed to simplify working with GStreamer from C++, without requiring full C++ bindings or introspection.

## Who Is This For?

This library is intended for C++ developers who need to work with GStreamer but prefer not to use complete C++ binding libraries (such as `gstreamermm`) or rely on GObject introspection.

### Features

- Header-only: no need to compile or install anything beyond the headers themselves.
- Zero external dependencies, aside from GStreamer.
- No use of `.gir` introspection files or generated wrappers.

> ⚠️ **Note for Visual Studio users**:  
> Make sure your CMake setup properly enables C++17. You may need to explicitly add:
> ```cmake
> target_compile_options(<YOUR_TARGET> PRIVATE "/Zc:__cplusplus")
> ```

## Available Components

- [**`GstPtr<>`**](GstPtr/README.md)  
  A smart pointer for managing GStreamer object lifetimes, wrapping `ref/unref` in a safe, RAII-style interface.  
  It provides functionality similar to `std::shared_ptr`, but tailored for GStreamer types.

## Building the Project

This library is header-only, so building is only required for running tests.

### Prerequisites

- `clang-tidy` ≥ 10  
- Python ≥ 3.8  
- Conan ≥ 2.0  
  (the build system will automatically detect or generate a default profile if needed)

### Build Steps

```bash
cmake -Bbuild -DCONAN_BUILD_MISSING=ON
cmake --build build
```

# Using as a Conan Dependency

A Conan 2.x recipe is provided under `/conan_recipe`.

1. Export the Recipe

```bash
cd conan_recipe
conan export . --name gstreamercpphelpers --version=0.0.3
```
⚠️ Make sure your Conan profile is configured with C++17 or higher.


2. Add the Dependency to Your conanfile.txt or Similar

```bash
[requires]
gstreamercpphelpers/0.0.3
```

3.  Link with CMake

```cmake
find_package(gstreamercpphelpers REQUIRED)
target_link_libraries(<YOUR TARGET> gstreamercpphelpers::gstreamercpphelpers)
```

