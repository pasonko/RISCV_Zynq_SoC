#!/usr/bin/env python3
"""
RISC-V UART Test Automation Suite
Platform: Xilinx Zynq-7000 (Cora Z7S)
Protocol: Binary UART (115200 8N1)
Author: Automated Test Suite
Version: 2.0 - Full Verification Suite
"""

import serial
import struct
import time
import sys
import os
import glob
from pathlib import Path

# ========================================
# Protocol Configuration
# ========================================
SERIAL_PORT = 'COM4'
BAUDRATE = 115200
TIMEOUT = 1.0

# Command bytes
CMD_RESET = b'S'
CMD_RUN = b'R'
CMD_LOAD = b'L'
CMD_VERIFY = b'V'
ACK_BYTE = b'K'

# Test configuration
VERIFICATION_ADDRESS = 0x00002000
INSTRUCTION_START_ADDR = 0x00000000
RUN_TO_VERIFY_DELAY = 0.1  # Stability delay

# Memory clear configuration (新增這三行)
NOP_INSTR = 0x00000013  # addi x0, x0, 0 (RISC-V NOP instruction)
MEMORY_CLEAR_SIZE = 128  # Clear first 128 bytes (32 instructions)


# Directory structure
TEST_SUITE_DIR = 'test_suite'
HEX_DIR = os.path.join(TEST_SUITE_DIR, 'hex')
EXPECTED_DIR = os.path.join(TEST_SUITE_DIR, 'expected')

# ========================================
# Global Serial Port
# ========================================
ser = None

# ========================================
# UART Helper Functions
# ========================================

def open_serial_port(port_name):
    """Open serial port with specified settings"""
    try:
        port = serial.Serial(
            port=port_name,
            baudrate=BAUDRATE,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=TIMEOUT
        )
        return port
    except serial.SerialException as e:
        print(f"[ERROR] Failed to open serial port: {e}")
        sys.exit(1)

# ========================================
# Command Functions
# ========================================

def cmd_reset():
    """Send RESET command to put CPU into reset state"""
    try:
        ser.write(CMD_RESET)
        ack = ser.read(1)
        return ack == ACK_BYTE
    except serial.SerialException:
        return False

def cmd_run():
    """Send RUN command to release CPU from reset"""
    try:
        ser.write(CMD_RUN)
        ack = ser.read(1)
        return ack == ACK_BYTE
    except serial.SerialException:
        return False

def cmd_load(address, data):
    """Load a single instruction to memory"""
    try:
        payload = CMD_LOAD + struct.pack('<I', address) + struct.pack('<I', data)
        ser.write(payload)
        ack = ser.read(1)
        return ack == ACK_BYTE
    except serial.SerialException:
        return False

def cmd_verify(address):
    """Read data from memory at specified address"""
    try:
        payload = CMD_VERIFY + struct.pack('<I', address)
        ser.write(payload)
        response = ser.read(4)
        if len(response) == 4:
            return struct.unpack('<I', response)[0]
        return None
    except serial.SerialException:
        return None


def cmd_clear_memory(start_addr, length_bytes, verbose=False):
    """
    Clear a range of instruction memory by writing NOP instructions
    
    Args:
        start_addr (int): Starting address to clear
        length_bytes (int): Number of bytes to clear (must be multiple of 4)
        verbose (bool): Print detailed clearing progress
    
    Returns:
        bool: True if successful, False otherwise
    
    Note: This prevents "ghost instructions" from previous tests
    """
    if length_bytes % 4 != 0:
        print(f"[WARNING] length_bytes {length_bytes} not aligned to 4, rounding down")
        length_bytes = (length_bytes // 4) * 4
    
    if verbose:
        print(f"  [CLEAR] Clearing memory from 0x{start_addr:08X} to 0x{start_addr + length_bytes:08X}")
    
    num_instructions = length_bytes // 4
    current_addr = start_addr
    
    for i in range(num_instructions):
        if not cmd_load(current_addr, NOP_INSTR):
            print(f"[ERROR] Failed to clear address 0x{current_addr:08X}")
            return False
        
        if verbose and (i % 8 == 0 or i == num_instructions - 1):
            print(f"    Cleared {i + 1}/{num_instructions} instructions")
        
        current_addr += 4
    
    if verbose:
        print(f"  [CLEAR] Memory cleared: {num_instructions} NOPs written")
    
    return True



# ========================================
# File Parsing Functions
# ========================================

def parse_hex_line(line):
    """
    Parse a single line from hex file
    
    Args:
        line (str): Line from hex file
    
    Returns:
        int or None: Parsed instruction value, or None if line should be skipped
    """
    line = line.strip()
    
    if not line:
        return None
    
    # Remove comments
    if '#' in line:
        line = line.split('#')[0].strip()
    if '//' in line:
        line = line.split('//')[0].strip()
    
    if not line:
        return None
    
    try:
        instruction = int(line, 16)
        if instruction <= 0xFFFFFFFF:
            return instruction
    except ValueError:
        pass
    
    return None

def load_program_from_file(filename, start_addr=INSTRUCTION_START_ADDR, verbose=False):
    """
    Load RISC-V program from hex file
    
    Args:
        filename (str): Path to hex file
        start_addr (int): Starting memory address
        verbose (bool): Print detailed loading info
    
    Returns:
        tuple: (success: bool, instruction_count: int)
    """
    if not os.path.exists(filename):
        return False, 0
    
    current_addr = start_addr
    instruction_count = 0
    
    try:
        with open(filename, 'r') as f:
            for line in f:
                instruction = parse_hex_line(line)
                
                if instruction is None:
                    continue
                
                if not cmd_load(current_addr, instruction):
                    return False, instruction_count
                
                if verbose:
                    print(f"    0x{current_addr:08X} <- 0x{instruction:08X}")
                
                current_addr += 4
                instruction_count += 1
        
        return True, instruction_count
    
    except IOError:
        return False, 0

def read_expected_result(ans_file):
    """
    Read expected result from .ans file
    
    Args:
        ans_file (str): Path to .ans file
    
    Returns:
        int or None: Expected value, or None if error
    """
    try:
        with open(ans_file, 'r') as f:
            content = f.read().strip()
            
            # Remove comments
            if '#' in content:
                content = content.split('#')[0].strip()
            if '//' in content:
                content = content.split('//')[0].strip()
            
            # Try to parse as decimal or hex
            if content.startswith('0x') or content.startswith('0X'):
                return int(content, 16)
            else:
                return int(content, 10)
    except (IOError, ValueError):
        return None

# ========================================
# Test Discovery
# ========================================

def find_test_files():
    """
    Discover all test files in the test suite directory
    
    Returns:
        list: List of tuples (test_name, hex_path, ans_path)
    """
    test_cases = []
    
    # Check if directories exist
    if not os.path.exists(HEX_DIR):
        print(f"[ERROR] Hex directory not found: {HEX_DIR}")
        return test_cases
    
    if not os.path.exists(EXPECTED_DIR):
        print(f"[ERROR] Expected directory not found: {EXPECTED_DIR}")
        return test_cases
    
    # Find all .hex files
    hex_files = glob.glob(os.path.join(HEX_DIR, '*.hex'))
    hex_files.sort()  # Ensure consistent ordering
    
    for hex_path in hex_files:
        # Extract base name (e.g., "01_add" from "01_add.hex")
        base_name = os.path.splitext(os.path.basename(hex_path))[0]
        
        # Construct corresponding .ans file path
        ans_path = os.path.join(EXPECTED_DIR, f"{base_name}.ans")
        
        # Check if .ans file exists
        if os.path.exists(ans_path):
            test_cases.append((base_name, hex_path, ans_path))
        else:
            print(f"[WARNING] No .ans file found for: {base_name}")
    
    return test_cases

# ========================================
# Test Execution
# ========================================

def run_single_test(test_name, hex_path, ans_path, verbose=False):
    """
    Run a single test case with memory clear safety
    
    Args:
        test_name (str): Name of the test
        hex_path (str): Path to hex file
        ans_path (str): Path to expected result file
        verbose (bool): Print detailed execution info
    
    Returns:
        dict: Test result with keys 'passed', 'expected', 'actual', 'error'
    """
    result = {
        'name': test_name,
        'passed': False,
        'expected': None,
        'actual': None,
        'error': None,
        'instructions': 0
    }
    
    # Read expected result
    expected = read_expected_result(ans_path)
    if expected is None:
        result['error'] = "Failed to read .ans file"
        return result
    result['expected'] = expected
    
    # Step 1: Reset CPU
    if not cmd_reset():
        result['error'] = "Reset command failed"
        return result
    
    if verbose:
        print(f"  [1] Reset complete")
    
    # Step 2: Clear instruction memory (SAFETY MECHANISM)
    if verbose:
        print(f"  [2] Clearing instruction memory...")
    
    if not cmd_clear_memory(INSTRUCTION_START_ADDR, MEMORY_CLEAR_SIZE, verbose=verbose):
        result['error'] = "Memory clear failed"
        return result
    
    if verbose:
        print(f"  [2] Memory cleared successfully")
    
    # Step 3: Load program
    success, instr_count = load_program_from_file(hex_path, verbose=verbose)
    if not success:
        result['error'] = "Program loading failed"
        return result
    result['instructions'] = instr_count
    
    if verbose:
        print(f"  [3] Loaded {instr_count} instructions")
    
    # Step 4: Run CPU
    if not cmd_run():
        result['error'] = "Run command failed"
        return result
    
    if verbose:
        print(f"  [4] CPU started")
    
    # Step 5: Wait for execution
    time.sleep(RUN_TO_VERIFY_DELAY)
    
    # Step 6: Verify result
    actual = cmd_verify(VERIFICATION_ADDRESS)
    if actual is None:
        result['error'] = "Verification read failed"
        return result
    result['actual'] = actual
    
    if verbose:
        print(f"  [5] Read result: {actual}")
    
    # Step 7: Compare
    result['passed'] = (actual == expected)
    
    return result


def run_test_suite(verbose=False):
    """
    Run all tests in the test suite
    
    Args:
        verbose (bool): Print detailed execution info
    
    Returns:
        tuple: (passed_count, total_count)
    """
    print("=" * 70)
    print("  RISC-V Automated Verification Suite")
    print("=" * 70)
    print(f"Serial Port:   {SERIAL_PORT} @ {BAUDRATE} baud")
    print(f"Hex Directory: {HEX_DIR}")
    print(f"Ans Directory: {EXPECTED_DIR}")
    print("=" * 70)
    
    # Discover tests
    test_cases = find_test_files()
    
    if not test_cases:
        print("\n[ERROR] No test cases found!")
        return 0, 0
    
    print(f"\nDiscovered {len(test_cases)} test case(s)\n")
    
    # Run all tests
    results = []
    
    for i, (test_name, hex_path, ans_path) in enumerate(test_cases, 1):
        print(f"[{i}/{len(test_cases)}] Running test: {test_name}")
        
        if verbose:
            print(f"  Hex file: {hex_path}")
            print(f"  Ans file: {ans_path}")
        
        result = run_single_test(test_name, hex_path, ans_path, verbose=verbose)
        results.append(result)
        
        # Print result
        if result['passed']:
            print(f"  [PASS] Result: {result['actual']} ✓")
        else:
            if result['error']:
                print(f"  [FAIL] Error: {result['error']}")
            else:
                print(f"  [FAIL] Expected: {result['expected']}, Got: {result['actual']} ✗")
        
        print()
    
    # Print summary
    print("=" * 70)
    print("  Test Summary")
    print("=" * 70)
    
    passed_count = sum(1 for r in results if r['passed'])
    total_count = len(results)
    
    # Detailed results table
    print(f"\n{'Test Name':<20} {'Status':<10} {'Expected':<12} {'Actual':<12}")
    print("-" * 70)
    
    for result in results:
        status = "[PASS] ✓" if result['passed'] else "[FAIL] ✗"
        expected_str = str(result['expected']) if result['expected'] is not None else "N/A"
        actual_str = str(result['actual']) if result['actual'] is not None else "N/A"
        
        print(f"{result['name']:<20} {status:<10} {expected_str:<12} {actual_str:<12}")
    
    print("-" * 70)
    print(f"\nTotal Passed: {passed_count} / {total_count}")
    
    if passed_count == total_count:
        print("Result: ALL TESTS PASSED! ✓✓✓")
    else:
        print(f"Result: {total_count - passed_count} test(s) failed")
    
    print("=" * 70)
    
    return passed_count, total_count

# ========================================
# Entry Point
# ========================================

if __name__ == "__main__":
    # Parse command line options
    verbose = '--verbose' in sys.argv or '-v' in sys.argv
    
    print(f"[CONFIG] Verbose mode: {'ON' if verbose else 'OFF'}\n")
    
    # Open serial port
    try:
        ser = open_serial_port(SERIAL_PORT)
        print(f"[INFO] Serial port opened: {SERIAL_PORT}\n")
        time.sleep(0.5)  # Stabilize
        
        # Run test suite
        passed, total = run_test_suite(verbose=verbose)
        
        # Close serial port
        ser.close()
        print("\n[INFO] Serial port closed")
        
        # Exit with appropriate code
        sys.exit(0 if passed == total else 1)
        
    except KeyboardInterrupt:
        print("\n[INFO] Test suite interrupted by user")
        if ser and ser.is_open:
            ser.close()
        sys.exit(2)
        
    except Exception as e:
        print(f"\n[ERROR] Unexpected error: {e}")
        if ser and ser.is_open:
            ser.close()
        sys.exit(1)
