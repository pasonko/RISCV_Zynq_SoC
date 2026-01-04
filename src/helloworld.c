/******************************************************************************
* Filename: helloworld.c
* Description: UART-to-AXI Bridge for RISC-V IP Control (Binary Protocol)
* Platform: Xilinx Zynq-7000 (Cora Z7S)
* UART: 115200 baud, 8N1, Polling Mode
******************************************************************************/

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xuartps_hw.h"
#include "sleep.h"              // Added for usleep() delay function

// ========================================
// RISC-V IP Configuration
// ========================================
#ifdef XPAR_RISCV_AXI_WRAPPER_0_BASEADDR
    #define RISC_V_BASE_ADDR    XPAR_RISCV_AXI_WRAPPER_0_BASEADDR
#else
    #define RISC_V_BASE_ADDR    0x43C00000
#endif

// RISC-V IP Register Offsets (AXI4-Lite)
#define REG_RESET_OFFSET        0x00    // Write: Reset Control
#define REG_ADDR_OFFSET         0x04    // Write: Address Bus
#define REG_DIN_OFFSET          0x08    // Write: Data Input
#define REG_WE_OFFSET           0x0C    // Write: Write Enable
#define REG_DOUT_OFFSET         0x10    // Read:  Data Output

// Control values
#define CPU_RESET               0x00000000
#define CPU_RUN                 0x00000001
#define WE_ENABLE               0x00000001
#define WE_DISABLE              0x00000000

// ========================================
// UART Configuration
// ========================================
#ifndef STDIN_BASEADDRESS
    #ifdef XPAR_XUARTPS_0_BASEADDR
        #define UART_BASEADDR   XPAR_XUARTPS_0_BASEADDR
    #else
        #define UART_BASEADDR   0xE0000000  // Default PS UART0
    #endif
#else
    #define UART_BASEADDR       STDIN_BASEADDRESS
#endif

// ========================================
// Protocol Command Definitions
// ========================================
#define CMD_RESET               'S'     // Stop/Reset CPU
#define CMD_RUN                 'R'     // Run CPU
#define CMD_LOAD                'L'     // Load instruction/data
#define CMD_VERIFY              'V'     // Verify/Read memory
#define ACK_BYTE                'K'     // Acknowledgment

// ========================================
// UART Helper Functions (Binary, Little-Endian)
// ========================================

/**
 * @brief Receive a single byte from UART (blocking)
 * @return Received byte
 */
u8 uart_read_byte(void)
{
    // Wait until data is available
    while (!XUartPs_IsReceiveData(UART_BASEADDR)) {
        // Polling loop
    }
    return (u8)XUartPs_RecvByte(UART_BASEADDR);
}

/**
 * @brief Send a single byte via UART (blocking)
 * @param data Byte to send
 */
void uart_write_byte(u8 data)
{
    // Wait until transmit FIFO is not full
    while (XUartPs_IsTransmitFull(UART_BASEADDR)) {
        // Polling loop
    }
    XUartPs_SendByte(UART_BASEADDR, data);
}

/**
 * @brief Read 4-byte u32 from UART in Little-Endian format
 * @return 32-bit value
 */
u32 uart_read_u32(void)
{
    u32 value = 0;

    // Receive LSB first (Little-Endian)
    value  = (u32)uart_read_byte();         // Byte 0 (LSB)
    value |= (u32)uart_read_byte() << 8;    // Byte 1
    value |= (u32)uart_read_byte() << 16;   // Byte 2
    value |= (u32)uart_read_byte() << 24;   // Byte 3 (MSB)

    return value;
}

/**
 * @brief Write 4-byte u32 to UART in Little-Endian format
 * @param value 32-bit value to send
 */
void uart_write_u32(u32 value)
{
    // Send LSB first (Little-Endian)
    uart_write_byte((u8)(value & 0xFF));         // Byte 0 (LSB)
    uart_write_byte((u8)((value >> 8) & 0xFF));  // Byte 1
    uart_write_byte((u8)((value >> 16) & 0xFF)); // Byte 2
    uart_write_byte((u8)((value >> 24) & 0xFF)); // Byte 3 (MSB)
}

// ========================================
// Command Handler Functions
// ========================================

/**
 * @brief CMD_RESET ('S'): Put RISC-V CPU into reset state
 */
void handle_reset(void)
{
    Xil_Out32(RISC_V_BASE_ADDR + REG_RESET_OFFSET, CPU_RESET);
    uart_write_byte(ACK_BYTE);  // Send 'K'
}

/**
 * @brief CMD_RUN ('R'): Release RISC-V CPU from reset
 */
void handle_run(void)
{
    Xil_Out32(RISC_V_BASE_ADDR + REG_RESET_OFFSET, CPU_RUN);
    uart_write_byte(ACK_BYTE);  // Send 'K'
}

/**
 * @brief CMD_LOAD ('L'): Write instruction/data to memory with pulse widening
 * Protocol: 'L' + 4 bytes (Address) + 4 bytes (Data)
 * Response: 'K' (ACK)
 *
 * Note: Added software delays to ensure FPGA logic captures the WE pulse
 */
void handle_load(void)
{
    u32 address, data;

    // Receive address (4 bytes, little-endian)
    address = uart_read_u32();

    // Receive data (4 bytes, little-endian)
    data = uart_read_u32();

    // Write to memory via AXI with pulse widening
    Xil_Out32(RISC_V_BASE_ADDR + REG_ADDR_OFFSET, address);
    Xil_Out32(RISC_V_BASE_ADDR + REG_DIN_OFFSET, data);

    // Pulse write enable with delays for stability
    Xil_Out32(RISC_V_BASE_ADDR + REG_WE_OFFSET, WE_ENABLE);
    usleep(1);  // Hold WE high for 1 microsecond (pulse width)

    Xil_Out32(RISC_V_BASE_ADDR + REG_WE_OFFSET, WE_DISABLE);
    usleep(1);  // Wait 1 microsecond before next operation (setup time)

    // Send acknowledgment
    uart_write_byte(ACK_BYTE);  // Send 'K'
}

/**
 * @brief CMD_VERIFY ('V'): Read data from memory
 * Protocol: 'V' + 4 bytes (Address)
 * Response: 4 bytes (Data at address)
 */
void handle_verify(void)
{
    u32 address, data;

    // Receive address (4 bytes, little-endian)
    address = uart_read_u32();

    // Set address bus
    Xil_Out32(RISC_V_BASE_ADDR + REG_ADDR_OFFSET, address);

    // Read data from read-only register
    data = Xil_In32(RISC_V_BASE_ADDR + REG_DOUT_OFFSET);

    // Send data back to PC (4 bytes, little-endian)
    uart_write_u32(data);
}

// ========================================
// Main Function
// ========================================

int main(void)
{
    u8 command;

    init_platform();

    // Print startup message (only at boot, not during command processing)
    xil_printf("\r\n");
    xil_printf("============================================\r\n");
    xil_printf("  RISC-V UART-to-AXI Bridge v1.0\r\n");
    xil_printf("============================================\r\n");
    xil_printf("RISC-V Base: 0x%08X\r\n", RISC_V_BASE_ADDR);
    xil_printf("UART Base:   0x%08X\r\n", UART_BASEADDR);
    xil_printf("Protocol:    Binary, 115200 8N1\r\n");
    xil_printf("Commands:    S=Reset, R=Run, L=Load, V=Verify\r\n");
    xil_printf("Status:      Ready for commands...\r\n");
    xil_printf("============================================\r\n\r\n");

    // Initialize RISC-V to reset state
    Xil_Out32(RISC_V_BASE_ADDR + REG_RESET_OFFSET, CPU_RESET);

    // ========================================
    // Main Command Loop (Infinite)
    // ========================================
    while (1)
    {
        // Wait for and receive command byte
        command = uart_read_byte();

        // Dispatch command to appropriate handler
        switch (command)
        {
            case CMD_RESET:     // 'S' - Stop/Reset
                handle_reset();
                break;

            case CMD_RUN:       // 'R' - Run
                handle_run();
                break;

            case CMD_LOAD:      // 'L' - Load
                handle_load();
                break;

            case CMD_VERIFY:    // 'V' - Verify
                handle_verify();
                break;

            default:
                // Unknown command - ignore or send error
                // (Optional: uart_write_byte('E'); for error indication)
                break;
        }
    }

    // This line is never reached
    cleanup_platform();
    return 0;
}
