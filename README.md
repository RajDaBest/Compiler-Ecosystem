# 🔧 Compiler Ecosystem

<div align="center">

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![Platform](https://img.shields.io/badge/platform-linux--64-blue)
![Language](https://img.shields.io/badge/language-C11-brightgreen)
![Architecture](https://img.shields.io/badge/arch-x86__64-red)
![Optimization](https://img.shields.io/badge/optimization-O3%20%2B%20AVX2-orange)
</div>

## 📖 Overview

This Compiler Toolchain is a sophisticated toolchain designed for converting self-developed VASM (Virtual Assembly) language code into executable binaries or bytecode. It provides a complete development environment for VASM programming, from source to execution.

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
  - `--debug`: Enable step-debugging; Instructions in the source code are executed one-by-one by pressing return

- **Preprocessor Options**:
  - `--save-vpp [file]`: Save preprocessed output
  - `--vpp`: Enable preprocessor

### Disassembler (`devasm`)
```bash
./devasm <input.vm>
```

Converts VM bytecode back to human-readable VASM format.

### VPP (VASM Preprocessor)

### Overview
VPP is the preprocessor for VASM files, providing macro capabilities, file inclusion, and conditional compilation features.

### Features

#### Include System
- Syntax: `%include "filename"`
- Supports recursive include resolution
- Library path searching via `--lib` option
- Environment variable `VLIB` for standard library location

#### Macro System
- Define macros: `%define MACRO_NAME value`
- Macro expansion in code
- Support for nested macros
- Complex macro definitions with parameters

#### Conditional Compilation
```vasm
%ifdef MACRO_NAME
    ; code when MACRO_NAME is defined
%else
    ; code when MACRO_NAME is not defined
%endif

%ifndef MACRO_NAME
    ; code when MACRO_NAME is not defined
%endif
```

#### Command Line Usage
```bash
vpp [OPTIONS] <input_file> [output_file]
```

#### Options
- `--lib <path>`: Add library search path
- `--vlib-ignore`: Ignore VLIB environment variable
- Multiple `--lib` flags supported

#### Environment Configuration
- Set `VLIB` environment variable to specify standard library location:
```bash
export VLIB=/path/to/vstdlib.hasm
```

#### Error Handling
- Source file tracking
- Line number preservation
- Detailed error messages with location information
- Automatic include guard detection

#### Best Practices
1. Use include guards to prevent multiple inclusion
2. Organize libraries in standard locations
3. Use meaningful macro names
4. Document macro dependencies
5. Keep preprocessing directives at file level when possible

#### Example Workflow
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

# VASM Language Reference

## Overview
VASM (Virtual Assembly) is a stack-based assembly language designed for a virtual machine environment, combining low-level control with high-level safety features.

## Instruction Set

### Data Movement Instructions
- `spush <value>`: Push signed integer onto stack
- `fpush <value>`: Push floating-point number onto stack
- `upush <value>`: Push unsigned integer onto stack
- `pop`: Remove top element from stack
- `pop_at <position>`: Remove element at specific position

### Stack Manipulation
- `rdup <offset>`: Duplicate element relative to stack top
- `adup <position>`: Duplicate element at absolute position
- `rswap <offset>`: Swap top with relative position
- `aswap <position>`: Swap top with absolute position
- `empty`: Check if stack is empty (pushes 0 or 1)

### Arithmetic Operations
#### Integer Operations
- `splus`: Add signed integers (dest = dest + src)
- `uplus`: Add unsigned integers
- `sminus`: Subtract signed integers
- `uminus`: Subtract unsigned integers
- `smult`: Multiply signed integers
- `umult`: Multiply unsigned integers
- `sdiv`: Divide signed integers
- `udiv`: Divide unsigned integers

#### Floating-Point Operations
- `fplus`: Add floating-point numbers
- `fminus`: Subtract floating-point numbers
- `fmult`: Multiply floating-point numbers
- `fdiv`: Divide floating-point numbers

### Memory Operations
#### Load Instructions
- `zeload8`: Load 8 bits with zero extension
- `zeload16`: Load 16 bits with zero extension
- `zeload32`: Load 32 bits with zero extension
- `load64`: Load 64 bits
- `seload8`: Load 8 bits with sign extension
- `seload16`: Load 16 bits with sign extension
- `seload32`: Load 32 bits with sign extension

#### Store Instructions
- `store8`: Store 8 bits to memory
- `store16`: Store 16 bits to memory
- `store32`: Store 32 bits to memory
- `store64`: Store 64 bits to memory

### Control Flow
- `jmp <address>`: Unconditional jump
- `ujmp_if <address>`: Jump if top of stack is non-zero
- `fjmp_if <address>`: Jump if float is not zero (ε = 1e-9)
- `call <address>`: Function call
- `ret`: Return from function
- `halt`: Stop execution
- `native <function_id>`: Call native function

### Comparison Operations
#### Integer Comparisons
- `equ`: Unsigned equality
- `eqs`: Signed equality
- `geu`: Unsigned greater or equal
- `ges`: Signed greater or equal
- `leu`: Unsigned less or equal
- `les`: Signed less or equal
- `gu`: Unsigned greater than
- `gs`: Signed greater than
- `lu`: Unsigned less than
- `ls`: Signed less than

#### Floating-Point Comparisons
- `eqf`: Float equality
- `gef`: Float greater or equal
- `lef`: Float less or equal
- `gf`: Float greater than
- `lf`: Float less than

### Type Conversion
- `utf`: Unsigned to float
- `stf`: Signed to float
- `ftu`: Float to unsigned
- `fts`: Float to signed
- `stu`: Signed to unsigned
- `uts`: Unsigned to signed

### Bitwise Operations
- `and`: Bitwise AND
- `or`: Bitwise OR
- `not`: Bitwise NOT
- `lsr`: Logical shift right
- `asr`: Arithmetic shift right
- `sl`: Shift left

### Directives
- `.text`: Code section
- `.data`: Data section
- `.byte`: Define byte
- `.string`: Define string
- `.double`: Define double
- `.word`: Define word (16-bit)
- `.doubleword`: Define doubleword (32-bit)
- `.quadword`: Define quadword (64-bit)

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

# ⚠️ Technical Cautions

## Stack Management
- **Implicit Register Behavior**: The `swap` instruction effectively converts the stack top into an implicit register. Values can be promoted to this position and restored using consecutive `swap` operations.
  
- **Bounds Verification**:
  - Stack overflow/underflow conditions are not validated during direct x86-64 compilation and execution
  - Memory access beyond stack boundaries will trigger OS-level segmentation faults during direct x86-64 compilation and execution 
  - Stack operation validation occurs only during VM bytecode execution, not in native compilation

## Control Flow Considerations
- **Program Termination**:
  - The `halt` instruction is mandatory for proper program termination
  - Absence of `halt` results in segmentation fault during direct x86-64 compilation due to instruction pointer overflow, and results in an error during VM bytecode conversion stage only
  - `halt` uses stack top value as exit code; empty stack during `halt` triggers segmentation fault during direct x86-64 compilation, and in a stack underflow trap during VM bytecode execution
  - Control flow must return to OS through proper termination sequence during direct x86-64 compilation

## Jump Instructions
- **Label Requirements**:
  - VM execution and x86-64 compilation supports both label-based and absolute instruction addressing

## Best Practices
1. Always terminate programs with `halt` instruction and valid stack state
2. Implement application-level bounds checking when required
3. Consider using VM execution mode for development/testing to leverage built-in safety checks
4. Monitor stack depth during program design to prevent overflow conditions

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