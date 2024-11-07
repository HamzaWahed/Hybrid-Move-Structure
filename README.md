# Hybrid Move Structure

The code can be compiled using the following commands:

```bash
git clone git@github.com:HamzaWahed/Hybrid-Move-Structure.git
cd Hybrid-Move-Structure
mkdir build
cd build
cmake ..
make
```

**WARNING:** The Zig build system is not linking the sdsl-lite lib, so ignore the
commands below and use `cmake` instead.

Clone and install sdsl-lite in the lib directory using the following commands:

```bash
cd lib
git clone https://github.com/simongog/sdsl-lite.git
cd sdsl-lite
./install.sh
```

Code can be compiled using the command:

```bash
zig build
```

The executable is found in the directory:

```bash
zig-out/bin
```
