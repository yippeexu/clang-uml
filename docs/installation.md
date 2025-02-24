# Installation

<!-- toc -->

* [Distribution packages](#distribution-packages)
  * [Ubuntu](#ubuntu)
  * [Fedora](#fedora)
  * [Conda](#conda)
  * [Windows](#windows)
* [Building from source](#building-from-source)
  * [Linux](#linux)
  * [macos](#macos)
  * [Windows](#windows-1)
    * [Visual Studio native build](#visual-studio-native-build)
* [Shell autocompletion scripts](#shell-autocompletion-scripts)
  * [Bash](#bash)
  * [Zsh](#zsh)

<!-- tocstop -->

### Distribution packages

#### Ubuntu

```bash
# Currently supported Ubuntu versions are Focal, Jammy and Lunar
sudo add-apt-repository ppa:bkryza/clang-uml
sudo apt update
sudo apt install clang-uml
```

#### Fedora

```bash
# Fedora 36
wget https://github.com/bkryza/clang-uml/releases/download/0.4.1/clang-uml-0.4.1-1.fc36.x86_64.rpm
sudo dnf install ./clang-uml-0.4.1-1.fc36.x86_64.rpm

# Fedora 37
wget https://github.com/bkryza/clang-uml/releases/download/0.4.1/clang-uml-0.4.1-1.fc37.x86_64.rpm
sudo dnf install ./clang-uml-0.4.1-1.fc37.x86_64.rpm

# Fedora 38
wget https://github.com/bkryza/clang-uml/releases/download/0.4.1/clang-uml-0.4.1-1.fc38.x86_64.rpm
sudo dnf install ./clang-uml-0.4.1-1.fc38.x86_64.rpm
```

#### Conda

```bash
conda config --add channels conda-forge
conda config --set channel_priority strict
conda install -c bkryza/label/clang-uml clang-uml
```

#### Windows

Download and run the latest Windows installer from
[Releases page](https://github.com/bkryza/clang-uml/releases).

### Building from source

#### Linux
First make sure that you have the following dependencies installed:

```bash
# Ubuntu (clang version will vary depending on Ubuntu version)
apt install ccache cmake libyaml-cpp-dev clang-12 libclang-12-dev libclang-cpp12-dev
```

Then proceed with building the sources:

```bash
git clone https://github.com/bkryza/clang-uml
cd clang-uml
# Please note that top level Makefile is just a convenience wrapper for CMake
make release
release/src/clang-uml --help

# To build using a specific installed version of LLVM use:
LLVM_VERSION=16 make release
# or specify path to a specific llvm-config binary, e.g.:
LLVM_CONFIG_PATH=/usr/bin/llvm-config-16 make release

# Optionally, to install in default prefix
make install
# or to install in custom prefix
make install DESTDIR=/opt/clang-uml
# or simply
export PATH=$PATH:$PWD/release
```

#### macos

```bash
brew install ccache cmake llvm yaml-cpp

export CC=/usr/local/opt/llvm/bin/clang
export CCX=/usr/local/opt/llvm/bin/clang++
LLVM_VERSION=16 make release
```

#### Windows

##### Visual Studio native build

These steps present how to build and use `clang-uml` natively using Visual Studio only.

First, install the following dependencies manually:

* [Python 3](https://www.python.org/downloads/windows/)
* [Git](https://git-scm.com/download/win)
* [CMake](https://cmake.org/download/)
* [Visual Studio](https://visualstudio.microsoft.com/vs/community/)

> All the following steps should be invoked in `Developer PowerShell for VS`.

Create installation directory for `clang-uml` and its dependencies:

```bash
# This is where clang-uml binary and its dependencies will be installed after build
# If you change this path, adapt all consecutive steps
mkdir C:\clang-uml
# This directory will be removed after build
mkdir C:\clang-uml-tmp
cd C:\clang-uml-tmp
```

Build and install `yaml-cpp`:

```bash
git clone https://github.com/jbeder/yaml-cpp
cd yaml-cpp
git checkout yaml-cpp-0.7.0
cd ..
cmake -S .\yaml-cpp\ -B .\yaml-cpp-build\ -DCMAKE_INSTALL_PREFIX="C:\clang-uml" -Thost=x64
cd yaml-cpp-build
msbuild .\INSTALL.vcxproj -maxcpucount /p:Configuration=Release
```

Build and install `LLVM`:

```bash 
pip install psutil
# Update the LLVM branch if necessary
git clone --branch llvmorg-15.0.6 --depth 1 https://github.com/llvm/llvm-project.git llvm
cmake -S .\llvm\llvm -B llvm-build -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_INSTALL_PREFIX="C:\clang-uml" -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD=X86 -Thost=x64
cd llvm-build
msbuild .\INSTALL.vcxproj -maxcpucount /p:Configuration=Release
```

Build and install `clang-uml`:

```bash
git clone https://github.com/bkryza/clang-uml
cmake -S .\clang-uml\ -B .\clang-uml-build\ -DCMAKE_INSTALL_PREFIX="C:\clang-uml" -DCMAKE_PREFIX_PATH="C:\clang-uml" -DBUILD_TESTS=OFF -Thost=x64
cd clang-uml-build
msbuild .\INSTALL.vcxproj -maxcpucount /p:Configuration=Release
```

Check if `clang-uml` works:

```bash
cd C:\clang-uml
bin\clang-uml.exe --version
```
It should produce something like:
```bash
clang-uml 0.4.1
Copyright (C) 2021-2023 Bartek Kryza <bkryza@gmail.com>
Linux x86_64 6.2.0-36-generic
Built against LLVM/Clang libraries version: 17.0.3
Using LLVM/Clang libraries version: Ubuntu clang version 17.0.3 (++20231010073202+37b79e779f44-1~exp1~20231010073304.52)

```

Finally, remove the temporary build directory:

```bash
rm -r C:\clang-uml-tmp
```

### Shell autocompletion scripts
For `Linux` and `macos`, Bash and Zsh autocomplete scripts are available, and
if `clang-uml` is installed from a distribution package they should work
out of the box. When installing `clang-uml` from sources the files need to be
installed manually. The completion scripts are available in directory:
* [`packaging/autocomplete`](./packaging/autocomplete)

#### Bash
The `clang-uml` script can be either directly loaded to the
current Bash session using:

```shell
source clang-uml
```

or the script can be copied to `/usr/share/bash-completion/completions/`
or `/etc/bash_completion.d` on Linux or to `/usr/local/etc/bash_completion.d` on
`macos` with `Homebrew`.

```shell
sudo cp packaging/autocomplete/clang-uml /usr/share/bash-completion/completions/
```

Make sure autocompletion is enabled in your `~/.bashrc` or `~/.bash_profile`:

```shell
if [ -f /etc/bash_completion ]; then
  . /etc/bash_completion
fi
```

On OSX you might need to install `bash-completion` using Homebrew:
```shell
brew install bash-completion
```
Make sure to the following lines are uncommented in the `~/.bashrc`:

```shell
if [ -f $(brew --prefix)/etc/bash_completion ]; then
  . $(brew --prefix)/etc/bash_completion
fi
```

#### Zsh
In Zsh, the `_clang-uml` Zsh completion file must be copied to one of the
folders under `$FPATH` variable, and the Zsh terminal should be reopened.

For testing, `_clang-uml` completion function can be updated without
restarting Zsh:

```shell
# Copy _clang-uml somewhere under $FPATH
$ unfunction _clang-uml
$ autoload -U _clang-uml
```

