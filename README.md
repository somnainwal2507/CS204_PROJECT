# RISC-V Assembler

A simple assembler project developed for the Computer Architecture course. This tool translates RISC-V assembly language into machine code, demonstrating key principles in instruction translation and computer organization.

---

## Table of Contents

- [Overview](#overview)
- [Usage](#usage)
  - [Prerequisites](#prerequisites)
  - [Compilation](#compilation)
  - [Running the Assembler](#running-the-assembler)
- [Limitations](#limitations)
- [Further Scope of Improvement](#further-scope-of-improvement)
- [About Us](#about-us)


---

## Overview

This project takes an assembly file (`input.asm`) containing RISC-V code as input and produces a machine code file (`output.mc`) as output. The code is modularly divided into three main files:
- **format.cpp:** Handles the formatting and conversion logic for assembly instructions.
- **lookup.cpp:** Manages the opcode and instruction lookup functionality.
- **main.cpp:** Serves as the entry point and integrates all modules to perform the assembly process.

---

## Usage

### Prerequisites

- **C++ Compiler:** Ensure you have a C++ compiler installed (e.g., `g++` or `clang++`).
- **Command Line Usage:** Basic knowledge of terminal commands to compile and run the project.
- **bits/std++ Library:** This project requires the `bits/std++` library to be available. It is typically included in most modern C++ standard library distributions.


### Compilation

1. Open your terminal and navigate to the project directory.
2. Compile the source files using the following command:

   ```bash
      g++ main.cpp -o assembler
    ```
### Running the Assembler
1. Run the compiled executable:

   ```bash
      ./assembler
    ```
2. The program will read the `input.asm` file and generate the corresponding machine code in `output.mc`.

---

## Limitations
- `label :` is not supported as a label. It should be of the form `label:`
-

---

## Further Scope of Improvement

We can implement a proper lexer which can tokenize the input code in a much more systematic manner. This will help in handling edge cases and errors more efficiently.

---

## About Us
- Atharva Mahajan (2023CSB1104)
- Nachiket Avachat (2023CSB1106)
- Som Nainwal (2023CSB1163)
