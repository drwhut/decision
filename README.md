# Decision Compiler

![](https://github.com/drwhut/decision/workflows/Decision/badge.svg)
![](https://github.com/drwhut/decision/workflows/Decision%20%28Development%29/badge.svg)

## Documentation

The documentation is being hosted on GitHub pages:

* [Decision User Manual](https://drwhut.github.io/decision/user)
* [Decision Developer Manual](https://drwhut.github.io/decision/developer)

It is also available to build from the project. See the
[docs/README.md](docs/README.md) file for more information.

## Branches

* `master` (0.3.0): The stable branch, which contains the "safest" version to
use.
* `development`: The development branch, which contains the latest and greatest
features, but may not be stable as it is being actively worked on by developers.

## Compiling

The build process uses **CMake**. For **Windows**, you will need Visual Studio
installed with Visual C++ support. For **Linux**, you will need build tools
installed like gcc and make.

### Windows (Win32)

```bash
mkdir build
cd build
cmake -DCOMPILER_32=ON ..
cmake --build . --config Debug
cmake --build . --config Release
```
If you have Visual Studio, this will generate a Visual Studio solution. You can
either run CMake's "--build" argument as above, or you can open the solution
yourself and build it from Visual Studio directly.

If you choose to run Decision directly from Visual Studio, you should set the
*decision* project to be the start-up project as well.
Right-click on the *decision* project in the solution explorer, and click
"Set as StartUp Project".

### Windows (x64)

```bash
mkdir build
cd build
cmake -DCOMPILER_32=OFF -A x64 ..
cmake --build . --config Debug
cmake --build . --config Release
```
By default, CMake will not generate a x64 version. Because of this, you need to
provide an extra argument to specify you want to build for the x64 architecture.

Same as above, if you choose to run Decision from Visual Studio, you should
set the *decision* project to be the start-up project as well. Right-click on
the *decision* project in the solution explorer, and click "Set as StartUp
Project".

### Linux

```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

By default, CMake installs Decision with a prefix of `/usr/local`. If you want
to change the prefix, add this argument when creating the build files:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
```

### Options

#### 32-Bit Mode

If you want the interpreter and compiler to store data in 32-bits, add this
argument:

```bash
cmake -DCOMPILER_32=ON ..
```

**NOTE:** This option will only work properly on 32-bit architectures! Applying
this option on 64-bit architectures will not work since it will not be able to
store full 64-bit pointers.

#### Shared Library

By default, a static library for the compiler is generated (.lib/.a). If you
want to generate a shared library instead (.dll/.so), add this argument:

```bash
cmake -DCOMPILER_SHARED=ON ..
```

#### Enable C API Tests

If you want to test Decision's C API, add this argument:

```bash
cmake -DCOMPILER_C_TESTS=ON ..
```

Note that this option will generate a lot more executables than usual.
See [tests/README.md](tests/README.md) for more details.

## License

Decision
Copyright (C) 2019-2020  Benjamin Beddows

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.