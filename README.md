# RISC-V Single-Cycle SoC on Zynq-7000 with Automated HIL Verification

## 📖 Project Overview

This project implements a **Single-Cycle RISC-V Processor (RV32I)** integrated into a **Xilinx Zynq-7000 SoC**.

Unlike traditional simulation-only projects, this design features a robust **Hardware-in-the-Loop (HIL)** verification framework. By coordinating Python automation scripts with the Zynq Processing System (PS), the system enables dynamic instruction loading, execution, and result verification on the FPGA logic, solving the complexity of verifying custom processor IPs.

---

## 🏗️ System Architecture

**Data Flow:** `PC (Python) <--> UART <--> Zynq PS (Cortex-A9) <--> AXI4-Lite <--> RISC-V Core (PL)`

* **Processing System (PS):** Acts as the bridge and controller. It receives packets via UART and drives the AXI4-Lite Master interface to control the PL.
* **Programmable Logic (PL):** Contains the custom RISC-V Core, wrapped with an AXI4-Lite Slave interface for register-level control.

---

## ⚙️ Hardware Microarchitecture

The design adopts a **Single-Cycle RISC-V RV32I** architecture, meaning every instruction (Fetch, Decode, Execute, Memory, Writeback) completes within a single clock cycle. The datapath is divided into five key modules:

### 1. Instruction Fetch (IF)

* **PC Logic:** The Program Counter (PC) uses a **2-to-1 Multiplexer** to determine the next address.
* **Normal Flow:** Executes `PC + 4` by default using a dedicated adder.
* **Branch Flow:** When the branch condition is met (`Zero` Flag Asserted) and the instruction is a Branch, the PC switches to `PCTarget` (calculated by `PC + ImmExt`).


* **Instruction Memory:** Receives the PC address and outputs the 32-bit instruction code (`Instr`) to the decoder within the same cycle.

### 2. Decode & Register Read (ID)

* **Register File (RF):** Implemented with a **Dual Read Port** and **Single Write Port** architecture.
* `A1 (rs1)` and `A2 (rs2)` are driven directly by instruction fields to asynchronously read `RD1` and `RD2`.
* Writes occur on the **Positive Edge** of the clock, targeting the register specified by `A3 (rd)`.


* **Sign Extension Unit:** Expands the Immediate value from various instruction formats (I-Type, S-Type, B-Type) into a 32-bit signed integer (`ImmExt`) for ALU calculations or branch offsets.

### 3. Execution & ALU (EX)

* **ALU Muxing Logic:**
* **SrcA:** Comes directly from `RD1` of the Register File.
* **SrcB:** Selected via a 2-to-1 Mux; source can be `RD2` (R-Type) or `ImmExt` (I-Type/S-Type).


* **ALU Operation:** The Arithmetic Logic Unit handles addition, subtraction, logical operations (AND/OR), and comparisons (SLT). It generates the critical `Zero` signal used by the Control Unit for branch decisions.

### 4. Memory Access (MEM)

* **Data Memory:** Implements a synchronous write architecture.
* **Address:** Driven directly by the ALU calculation result (`ALUResult`).
* **Write Data:** Driven by `RD2` from the Register File (e.g., `sw` instruction).
* **Read Data:** Outputs data based on the address (e.g., `lw` instruction).



### 5. Write Back (WB)

* **Result Selection:** A critical **Result Mux** is placed at the end of the datapath.
* For arithmetic instructions (ADD/SUB/AND...), it selects `ALUResult`.
* For load instructions (LW), it selects `ReadData` to write back to the register file.



### 🧠 Control Unit Design

To drive the datapath, the Control Unit is designed as pure combinational logic, split into two levels:

1. **Main Decoder:** Parses the 7-bit `Opcode` to generate primary control signals:
* `RegWrite`: Enables register file write.
* `ALUSrc`: Selects the second ALU operand (Register vs. Immediate).
* `MemWrite`: Enables Data Memory write.
* `ResultSrc`: Determines write-back source (ALU vs. Memory).
* `Branch`: Flags the current instruction as a branch.


2. **ALU Decoder:** Combines the Main Decoder's `ALUOp` signal with the instruction's `Funct3` and `Funct7` fields to generate specific ALU Control codes (e.g., ADD, SUB, AND, OR, SLT).

---

## 🐛 Design Challenges & Engineering Solutions

During the SoC integration and verification phase, several critical engineering challenges were encountered and resolved. Below is the **Root Cause Analysis** and solution for each:

### 1. Control Signal Aliasing in ALU Decoder

* **The Issue:** During verification, the ALU incorrectly outputted `ADD` results when executing `SUB` (subtraction) instructions.
* **Root Cause Analysis:** According to the RISC-V RV32I standard, `ADD` and `SUB` share the same `Opcode` (0110011) and `Funct3` (000). The only difference lies in the 5th bit of `Funct7` (0 for ADD, 1 for SUB). The initial decoder logic used simplified conditions, ignoring `Funct7`, causing **Decoding Aliasing** where `SUB` was misinterpreted as the default `ADD`.
* **Resolution:** Refactored the `ALU_Decoder` module using behavioral `case` statements. Added mandatory bit-wise checks for `op[5]` and `funct7[5]` to ensure **decoding uniqueness** for R-Type operations.

### 2. Immediate Field Scrambling & Sign Extension

* **The Issue:** `BEQ` (Branch if Equal) tests failed because the PC jumped to incorrect memory addresses.
* **Root Cause Analysis:** To keep register ports fixed, RISC-V B-Type instructions perform **Bit Scrambling** on the Immediate value (e.g., Bit 11 is at instr[7], Bit 12 at instr[31]). The initial design treated the lower bits as a raw immediate without handling this scrambling or performing correct **Sign Extension** to 32-bits.
* **Resolution:** Implemented dedicated Muxing Logic in the `Sign_Extend` module. For B-Type instructions, the bits are manually concatenated according to ISA spec: `{32{instr[31]}, instr[7], instr[30:25], instr[11:8], 1'b0}`, resolving negative offset calculation errors.

### 3. Signal Integrity & Write Pulse Width Violation

* **The Issue:** "Ghost Instructions" appeared during system tests, where new instructions failed to write to Instruction Memory, causing the CPU to execute old, residual code.
* **Root Cause Analysis:** A **Timing Violation** occurred due to the speed difference between the Zynq PS (650MHz) and PL logic. When the C firmware toggled GPIOs to simulate AXI writes, the `WE` (Write Enable) **Pulse Width** was shorter than the Block RAM's **Setup Time** requirement, causing the latch to fail.
* **Resolution:**
* **Firmware Constraint:** Implemented timing control in the C firmware by introducing `usleep(1)` between `WE` Assert and De-assert states to widen the signal pulse.
* **Verification Safety:** Introduced a **Memory Sanitization** mechanism in the Python automation script, overwriting memory with `NOP` instructions before every load to ensure a clean test environment.



---

## 🧪 Automated Verification Suite

The project includes a Python-based verification framework located in `tests/`.

### Workflow

1. **Discovery:** Scans `tests/test_suite/hex/*.hex` for test cases.
2. **Sanitization:** Clears Instruction Memory (writes NOPs) to prevent ghost instructions.
3. **Loading:** Transmits machine code via UART to FPGA.
4. **Execution:** Releases Reset; CPU executes logic.
5. **Verification:** Reads Data Memory (0x2000) and compares against `.ans` golden references.

### Supported Tests

| Test Case | Description | Focus Area | Status |
| --- | --- | --- | --- |
| **01_add** | Basic Addition | ALU ADD, Reg Write | ✅ PASS |
| **02_sub** | Subtraction | ALU Decoder Fix | ✅ PASS |
| **03_branch** | Branch if Equal | PC Logic, Zero Flag | ✅ PASS |
| **04_fibonacci** | Fibonacci Seq | Dependency, Loop | ✅ PASS |
| **05_mult** | Multiplication | Algorithm, Memory Store | ✅ PASS |

---

## 📂 Repository Structure

```text
RISCV_Zynq_SoC/
├── hw/                     # Hardware Design
│   ├── src/
│   │   ├── ALU_Decoder.v   # [Critical] Fixed decoding logic
│   │   ├── Sign_Extend.v   # [Critical] Handles Immediate Scrambling
│   │   └── ...
│   └── system_wrapper.xsa  # Hardware Export
│
├── sw/                     # Embedded Firmware
│   └── src/
│       ├── helloworld.c    # [Critical] UART Monitor with Timing Control
│       └── lscript.ld      # Linker Script
│
└── tests/                  # Verification Suite
    ├── test_riscv.py       # Automation Script
    └── test_suite/         # Test vectors (Hex & Expected Answer)

```

---

## 👤 Author

**Pei-Sheng Ke**

* Master of Science in Electrical & Computer Engineering, The Ohio State University
* **Focus:** Digital IC Design, FPGA Verification, Computer Architecture

---

*Verified on Windows 11 / Vivado 2024.1 / Cora Z7S Board*
