/******************************************************************************
* Filename: helloworld.c
* Description: RISC-V Calculation & Store Verification Test with Read-Only Register
******************************************************************************/

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xil_io.h"

// Base address from xparameters.h or fallback
#ifdef XPAR_RISCV_AXI_WRAPPER_0_BASEADDR
    #define RISC_V_BASE_ADDR    XPAR_RISCV_AXI_WRAPPER_0_BASEADDR
#else
    #define RISC_V_BASE_ADDR    0x43C00000
#endif

// AXI Register offsets
#define REG_RESET_OFFSET        0x00    // Write: Reset Control
#define REG_ADDR_OFFSET         0x04    // Write: Address Bus
#define REG_DIN_OFFSET          0x08    // Write: Data Input
#define REG_WE_OFFSET           0x0C    // Write: Write Enable
#define REG_DOUT_OFFSET         0x10    // Read:  Data Output (Read-Only)

// Control values
#define CPU_RESET               0x00000000
#define CPU_RUN                 0x00000001
#define WE_ENABLE               0x00000001
#define WE_DISABLE              0x00000000

// Test parameters
#define DATA_MEM_ADDR           0x00002000
#define EXPECTED_VALUE          8

// Helper function to write instruction to instruction memory
void write_instruction(u32 address, u32 instruction)
{
    Xil_Out32(RISC_V_BASE_ADDR + REG_ADDR_OFFSET, address);
    Xil_Out32(RISC_V_BASE_ADDR + REG_DIN_OFFSET, instruction);
    Xil_Out32(RISC_V_BASE_ADDR + REG_WE_OFFSET, WE_ENABLE);
    Xil_Out32(RISC_V_BASE_ADDR + REG_WE_OFFSET, WE_DISABLE);
}

// Software delay loop (volatile to prevent optimization)
void delay_cycles(void)
{
    volatile int i;
    for (i = 0; i < 1000000; i++) {
        // Wait loop
    }
}

int main()
{
    u32 read_value;

    init_platform();

    xil_printf("\r\n");
    xil_printf("============================================\r\n");
    xil_printf("  RISC-V Calculation & Store Test\r\n");
    xil_printf("  Task: Calculate 5 + 3 = 8\r\n");
    xil_printf("  Store to 0x2000 and Verify via 0x10\r\n");
    xil_printf("============================================\r\n");
    xil_printf("Base Address: 0x%08X\r\n\r\n", RISC_V_BASE_ADDR);

    // ========================================
    // RESET PHASE
    // ========================================
    xil_printf("[1] Reset Phase: Holding CPU in reset...\r\n");
    Xil_Out32(RISC_V_BASE_ADDR + REG_RESET_OFFSET, CPU_RESET);
    xil_printf("    Status: CPU is in RESET state\r\n\r\n");

    // ========================================
    // PROGRAM PHASE
    // ========================================
    xil_printf("[2] Program Phase: Loading instruction sequence...\r\n");

    // Instruction 0 @ 0x0000: addi x1, x0, 5 -> x1 = 5
    xil_printf("    [0x0000] addi x1, x0, 5\r\n");
    write_instruction(0x00000000, 0x00500093);

    // Instruction 1 @ 0x0004: addi x2, x0, 3 -> x2 = 3
    xil_printf("    [0x0004] addi x2, x0, 3\r\n");
    write_instruction(0x00000004, 0x00300113);

    // Instruction 2 @ 0x0008: add x3, x1, x2 -> x3 = 8
    xil_printf("    [0x0008] add x3, x1, x2\r\n");
    write_instruction(0x00000008, 0x002081B3);

    // Build address 0x2000 without LUI
    xil_printf("    Building address 0x2000 without LUI:\r\n");

    // Instruction 3 @ 0x000C: addi x4, x0, 1024 -> x4 = 0x400
    xil_printf("      [0x000C] addi x4, x0, 1024  -> x4 = 0x0400\r\n");
    write_instruction(0x0000000C, 0x40000213);

    // Instruction 4 @ 0x0010: add x4, x4, x4 -> x4 = 0x800
    xil_printf("      [0x0010] add x4, x4, x4     -> x4 = 0x0800\r\n");
    write_instruction(0x00000010, 0x00420233);

    // Instruction 5 @ 0x0014: add x4, x4, x4 -> x4 = 0x1000
    xil_printf("      [0x0014] add x4, x4, x4     -> x4 = 0x1000\r\n");
    write_instruction(0x00000014, 0x00420233);

    // Instruction 6 @ 0x0018: add x4, x4, x4 -> x4 = 0x2000
    xil_printf("      [0x0018] add x4, x4, x4     -> x4 = 0x2000\r\n");
    write_instruction(0x00000018, 0x00420233);

    // Instruction 7 @ 0x001C: sw x3, 0(x4) -> Store 8 to Mem[0x2000]
    xil_printf("    [0x001C] sw x3, 0(x4)\r\n");
    write_instruction(0x0000001C, 0x00322023);

    xil_printf("    Status: 8 instructions loaded successfully\r\n\r\n");

    // ========================================
    // RUN PHASE
    // ========================================
    xil_printf("[3] Run Phase: Starting CPU execution...\r\n");
    Xil_Out32(RISC_V_BASE_ADDR + REG_RESET_OFFSET, CPU_RUN);
    xil_printf("    Status: CPU is RUNNING\r\n\r\n");

    // ========================================
    // WAIT FOR EXECUTION
    // ========================================
    xil_printf("[4] Wait Phase: Allowing CPU to complete execution...\r\n");
    delay_cycles();
    xil_printf("    Status: Delay completed\r\n\r\n");

    // ========================================
    // VERIFICATION PHASE (CRUCIAL)
    // ========================================
    xil_printf("[5] Verification Phase: Reading back from Data Memory...\r\n");

    // Set address bus to point to Data Memory at 0x2000
    xil_printf("    Writing Address = 0x%08X to Offset 0x04\r\n", DATA_MEM_ADDR);
    Xil_Out32(RISC_V_BASE_ADDR + REG_ADDR_OFFSET, DATA_MEM_ADDR);

    // Read the data from the read-only register at offset 0x10
    xil_printf("    Reading from Offset 0x10 (Read-Only Register)...\r\n");
    read_value = Xil_In32(RISC_V_BASE_ADDR + REG_DOUT_OFFSET);

    xil_printf("    Read Value    = 0x%08X (%d decimal)\r\n", read_value, read_value);
    xil_printf("    Expected Value = 0x%08X (%d decimal)\r\n\r\n",
               EXPECTED_VALUE, EXPECTED_VALUE);

    // ========================================
    // RESULT
    // ========================================
    xil_printf("============================================\r\n");
    if (read_value == EXPECTED_VALUE) {
        xil_printf("  TEST RESULT: SUCCESS!\r\n");
        xil_printf("  Read back value: %d\r\n", read_value);
        xil_printf("  The RISC-V core correctly:\r\n");
        xil_printf("    - Calculated 5 + 3 = 8\r\n");
        xil_printf("    - Stored to memory address 0x2000\r\n");
        xil_printf("    - Read-only register verified\r\n");
    } else {
        xil_printf("  TEST RESULT: FAILURE!\r\n");
        xil_printf("  Expected %d but got %d\r\n", EXPECTED_VALUE, read_value);
        xil_printf("  Possible issues:\r\n");
        xil_printf("    - CPU did not execute correctly\r\n");
        xil_printf("    - Memory write failed\r\n");
        xil_printf("    - Address mapping error\r\n");
    }
    xil_printf("============================================\r\n\r\n");

    cleanup_platform();
    return 0;
}
