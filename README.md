![workflow](https://github.com/nachogarglez/GStreamerCppHelpers/actions/workflows/ci.yml/badge.svg)

# GStreamerCppHelpers

Headers to simplify GStreamer usage from C++.

## For Whom Is This Useful?

Those who need to use GStreamer from C++, but do not want to use some complete C++ bindings for GStreamer.

These headers do not have external dependencies (other than GStreamer itself) and do not use introspection files neither.

## Currently available

* [GstPtr <>](GstPtr/README.md)

  A specialized SmartPointer for GStreamer objects.

# How to build

This library is header-only, therefore you only need to build it to run the tests.

* Pre-requisites:
  * clang-tidy  >= 10
  * Python >= 3.6

```bash
cmake -Bbuild -DCONAN_BUILD_MISSING=ON
cmake --build build
```

# How to use as a Conan dependency

You can add this header as a Conan dependency in a few steps:

* Export the recipe

```shell
cd conan_recipe
export conan gstreamercpphelpers/0.0.1@
```

* Include the dependency in your conanfile.txt or equivalent

```script
[requires]
   gstreamercpphelpers/0.0.1
```

* Use in your CMakeLists.txt

```CMake
find_package(gstreamercpphelpers)
target_link_libraries(${PROJECT_NAME} gstreamercpphelpers::gstreamercpphelpers)
```
