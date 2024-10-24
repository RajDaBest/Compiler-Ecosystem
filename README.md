# 🔧 Compiler Ecosystem

<div align="center">

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![Platform](https://img.shields.io/badge/platform-linux--64-blue)
![Language](https://img.shields.io/badge/language-C11-brightgreen)
![Architecture](https://img.shields.io/badge/arch-x86__64-red)
![Optimization](https://img.shields.io/badge/optimization-O3%20%2B%20AVX2-orange)
</div>

## 📖 Overview

The Compiler Ecosystem is a sophisticated toolchain designed for converting self-developed VASM (Virtual Assembly) language code into executable binaries or bytecode. It provides a complete development environment for VASM programming, from source to execution.

### 🎯 Key Components

- 🖥️ **Virtual Machine**: 
  - Compiles VASM → VM bytecode 
  - Emulates bytecode execution
  - Configurable stack and memory sizes
  - Support for both nan-boxed and standard implementations

- 🔍 **Disassembler**: 
  - Converts VM bytecode → VASM
  - Human-readable output format
  - Supports both binary versions

- 🛠️ **Compiler/Interpreter**: 
  - Direct VASM → x86-64 Linux NASM conversion
  - Optimized output assembly
  - Platform-specific optimizations

- 🔄 **Preprocessor**: 
  - Parses preprocessing tokens in VASM files
  - Library inclusion support
  - Macro processing
  - Conditional compilation

- 📚 **Standard Library**: 
  - Provides `vstdlib.hasm`
  - Common routines and utilities
  - Optimized implementations

- 🎨 **VS Code Extension**: 
  - "VASM Assembly Language (for VM) Support"
  - Syntax highlighting
  - Code snippets
  - Integration with toolchain

### 🔄 Implementation Variants

#### Nan-boxed Version
- Uses NaN-boxing for value representation
- Optimized memory usage
- Currently unsupported
- Suitable for memory-constrained environments

#### Non-nanboxed Version
- Standard implementation
- Actively maintained
- Better debugging support
- More straightforward value representation

## 📂 Repository Structure

```
.
├── bin/
│   ├── compiler/         # Contains vtx binary (VASM→NASM compiler)
│   ├── non-nanboxed/    # Standard implementation binaries
│   │   ├── virtmach     # Virtual Machine
│   │   ├── devasm       # Disassembler
│   │   └── vpp          # Preprocessor
│   └── nan-boxed/       # Nan-boxed implementation binaries
├── src/                 # Source code
│   ├── nan_boxed/       # Nan-boxed implementation sources
│   ├── non_nanboxed/    # Standard implementation sources
│   └── Compiler-Backend/# NASM compiler implementation
├── examples/            # Usage examples and tests
├── lib/                # Standard library sources
└── vasm/              # VS Code extension source
```

## 🛠️ Building from Source

### Compiler Optimization Flags

```makefile
CFLAGS=-Wall -Wextra -std=c11 -pedantic -O3 -march=native -ffast-math 
       -funroll-loops -ftree-vectorize -finline-functions 
       -floop-nest-optimize -mavx2 -mfma -mfpmath=sse -flto 
       -fno-math-errno -fno-signed-zeros -pg -g
```

#### Flag Explanations
- **Standard Compliance**:
  - `-std=c11`: Use C11 standard
  - `-pedantic`: Strict standard compliance
  - `-Wall -Wextra`: Enable comprehensive warnings

- **Optimization Levels**:
  - `-O3`: Highest optimization level
  - `-march=native`: CPU-specific optimizations
  - `-ffast-math`: Aggressive floating-point optimizations
  - `-funroll-loops`: Loop unrolling
  - `-ftree-vectorize`: Auto-vectorization
  - `-finline-functions`: Aggressive function inlining
  - `-floop-nest-optimize`: Loop nest optimizations

- **SIMD/Vector Support**:
  - `-mavx2`: Enable AVX2 instructions
  - `-mfma`: Enable FMA instructions
  - `-mfpmath=sse`: Use SSE for floating-point math

- **Additional Optimizations**:
  - `-flto`: Link-time optimization
  - `-fno-math-errno`: Skip errno in math functions
  - `-fno-signed-zeros`: Ignore sign of zero

- **Debug Support**:
  - `-pg`: Enable profiling
  - `-g`: Include debug symbols

### Available Make Targets

#### Non-nanboxed Binaries
```bash
# Build virtual machine with optimizations
make virtmach    

# Build disassembler
make devasm      

# Build preprocessor
make vpp         

# Build VASM→NASM compiler
make compiler    

# Clean non-nanboxed builds
make clean       
```

#### Nan-boxed Binaries *(unsupported)*
```bash
# Build nan-boxed virtual machine
make nan_virtmach    

# Build nan-boxed disassembler
make nan_devasm      

# Build nan-boxed preprocessor
make nan_vpp         

# Clean nan-boxed builds
make nan_clean       
```

## 🚀 Usage

### Virtual Machine (`virtmach`)
```bash
./virtmach --action <asm|run|pp> [options] <input> [output]
```

#### Options:
- **Action Selection**:
  - `--action asm`: Assemble VASM to bytecode
  - `--action run`: Execute bytecode
  - `--action pp`: Preprocess VASM file

- **Memory Configuration**:
  - `--stack-size <n>`: Set VM stack size
  - `--program-capacity <n>`: Set program memory size
  - `--static-size <n>`: Set static memory size

- **Library Management**:
  - `--lib <path>`: Add library search path
  - `--vlib-ignore`: Ignore VLIB environment

- **Execution Control**:
  - `--limit <n>`: Limit instruction count
  - `--debug`: Enable debug output

- **Preprocessor Options**:
  - `--save-vpp [file]`: Save preprocessed output
  - `--vpp`: Enable preprocessor

### Disassembler (`devasm`)
```bash
./devasm <input.vm>
```

Converts VM bytecode back to human-readable VASM format.

### VASM Preprocessor (`vpp`)
```bash
vpp [OPTIONS] <input_file> [output_file]
```
Configure the environment variable VLIB in your bash session to have the path to the
VASM standard library file.

#### VPP Options:
- **Library Management**:
  - `--lib <path>`: Add library search path(s)
  - `--vlib-ignore`: Ignore VLIB environment variable
  - Multiple `--lib` flags supported

- **Input/Output**:
  - `<input_file>`: Source VASM file (required)
  - `[output_file]`: Output file (optional)
    - Defaults to `<input>.vpp` if not specified

#### VPP Features:
- **Include System**:
  - Supports `%include` directives
  - Recursive include resolution
  - Library path searching

- **Macro Processing**:
  - Macro definitions
  - Macro expansion
  - Nested macros

- **Conditional Compilation**:
  - `%ifdef/%ifndef`
  - `%else/%elif`
  - `%endif`

- **File Management**:
  - Automatic include guards
  - Source file tracking
  - Error location reporting

### Example Workflow
```bash
# 1. Preprocess VASM file with library support
./vpp --lib /usr/local/lib/vasm input.vasm input.vpp

# 2. Compile preprocessed file to bytecode
./virtmach --action asm \
          --stack-size 1024 \
          --program-capacity 2048 \
          input.vpp output.vm

# 3. Execute bytecode with debug info
./virtmach --action run \
          --debug \
          --limit 1000000 \
          output.vm

# 4. Disassemble bytecode for verification
./devasm output.vm > output.vasm
```

## 🔧 Technical Specifications

### Virtual Machine
- **Stack Implementation**:
  - Configurable size
  - Growth direction: descending
  - Alignment: 8-byte

- **Memory Model**:
  - Separate instruction/data space
  - Static allocation support
  - Dynamic memory management

- **Instruction Set**:
  - RISC-like design
  - Fixed instruction width
  - Zero-overhead abstraction

### Compiler Backend
- **Target**: x86-64 Linux
- **Assembly Format**: NASM
- **Optimizations**:
  - Register allocation
  - Instruction scheduling
  - Peephole optimizations

## 🤝 Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines
- Maintain optimization flags
- Add tests for new features
- Document public interfaces
- Follow existing code style
- Update relevant documentation

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---
<div align="center">
Made with ❤️ for the VASM community

*Performance-Focused Virtual Assembly Development*
</div>