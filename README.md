# Avendish processor template

This provides a basic, canonical template for making objects with [Avendish](https://github.com/celtera/avendish).

Note that some libraries are needed to access the various back-ends. They can be passed to CMake.

These instructions are mostly useful on Windows / macOS as on Linux one can just install the packages from the OS package manager.

For instance, on Arch Linux:

```bash
$ sudo pacman -S vst3sdk pd pybind11
```

will install things.

To see a complete build procedure, one can refer to the [Github actions workflows](.github/workflows/), which compile 
the project on clean virtual machines.

## Python
 
[Get it there](https://github.com/pybind/pybind11) and pass to cmake:
```cpp
-Dpybind11_DIR=path/to/pybind11
```

## VST 3 SDK

[Get it there](https://github.com/steinbergmedia/vst3sdk) and pass to cmake:

```cpp
-DVST3_SDK_ROOT=path/to/vst3
```

## Max SDK

[Get it there](https://cycling74.com/downloads/sdk) and pass to cmake:
```cmake
-DAVND_MAXSDK_PATH=path/to/maxsdk
```

## PureData

[Get it there](https://github.com/pure-data/pure-data), build it, and pass to cmake:

```cmake
-DCMAKE_PREFIX_PATH=path/to/the/folder/containing/m_pd.h
```

## TODO

* Support importing them with a package manager
* Audio support for standalone