# Self-Made Virtual Machine

## Introduction

The# Compiler Ecosystem

The Compiler Ecosystem is a comprehensive toolchain designed for converting VASM (Virtual Assembly) code into executable binaries, along with providing a virtual machine and disassembler for bytecode management. This repository encompasses various components aimed at streamlining the development and execution of VASM programs.

## Repository Structure

- **bin/**
  - **compiler/**: 
    - Contains the binary `vtx`, which is responsible for converting VASM code directly into x86-64 Linux NASM assembly code.
  - **non-nanboxed/**: 
    - Houses the `virt_mach` and `devasm` executables that operate on non-nanboxed values.
  - **nan-boxed/**: 
    - Contains the `virt_mach` and `devasm` executables that utilize nan-boxed values in their stack implementations.

- **virt_mach**: 
  - A versatile virtual machine capable of generating VM bytecode from VASM files and executing said bytecode directly.

- **devasm**: 
  - A disassembler designed to convert compiled VM bytecode back into a human-readable format.

- **vpp**: 
  - The VASM preprocessor, which prepares VASM source files for compilation.

- **src/**: 
  - Contains all source code, including the implementation of the compiler, virtual machine, and related components.

- **examples/**: 
  - A collection of examples showcasing the capabilities and usage of the compiler and virtual machine, which can be built and executed for demonstration purposes.

- **lib/**: 
  - The standard library for VASM files, providing essential functions and routines for VASM programs.

- **vasm/**: 
  - Contains the implementation of a Visual Studio Code extension specifically for `.vasm` files, enhancing the development experience with syntax highlighting and code snippets.

## Usage

The exact usage of the executables and their options will be detailed in a subsequent section. 

## Contributing

Contributions to the Compiler Ecosystem are welcome. Please follow the standard procedures for pull requests and issue reporting.

## License

This project is licensed under the [MIT License](LICENSE).

