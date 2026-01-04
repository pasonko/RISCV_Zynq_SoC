module Single_Cycle_Top(
    input clk,
    input rst,
    output wire [31:0] PC_Top,
    
    // Port B Interface (System/AXI Side)
    input [31:0] addr_b,      // Shared address bus
    input [31:0] din_b,       // Shared data input bus
    input we_b,                // Write enable for Port B
    output wire [31:0] dout_b
);

    wire [31:0] RD_Instr, RD1_Top, Imm_Ext_Top, ALUResult, ReadData, PCPlus4, RD2_Top, SrcB, Result;
    wire RegWrite, MemWrite, ALUSrc, ResultSrc;
    wire [1:0] ImmSrc;
    wire [2:0] ALUControl_Top;
    
    // ========================================
    // RESET OVERRIDE LOGIC (Traffic Control)
    // ========================================
    // Constraint: CPU MUST NOT run while external system is writing
    // When we_b = 1 (Writing), cpu_rst = 0 (Force CPU into reset)
    // When we_b = 0 (Idle), cpu_rst = rst (Follow external reset)
    wire cpu_rst;
    assign cpu_rst = rst && (~we_b);
    
    // ========================================
    // MEMORY MAPPING (Address Decoding)
    // ========================================
    // If addr_b < 0x2000: Access Instruction Memory
    // If addr_b >= 0x2000: Access Data Memory
    wire sel_instr_mem = (addr_b < 32'h2000);
    wire sel_data_mem = (addr_b >= 32'h2000);
    
    wire we_b_instr = we_b && sel_instr_mem;
    wire we_b_data = we_b && sel_data_mem;

    // ========================================
    // CPU CORE COMPONENTS
    // ========================================
    PC_Module PC(
        .clk(clk),
        .rst(cpu_rst),        // Use cpu_rst instead of rst
        .PC(PC_Top),
        .PC_Next(PCPlus4)
    );

    PC_Adder PC_Adder(
        .a(PC_Top),
        .b(32'd4),
        .c(PCPlus4)
    );
    
    Instruction_Memory Instruction_Memory(
        // Port A (CPU) - Uses cpu_rst for safety
        .rst(cpu_rst),        // Use cpu_rst instead of rst
        .A(PC_Top),
        .RD(RD_Instr),
        // Port B (System)
        .clk(clk),
        .we_b(we_b_instr),
        .addr_b(addr_b),
        .din_b(din_b),
        .dout_b()             // Not used, can be left unconnected
    );

    Register_File Register_File(
        .clk(clk),
        .rst(cpu_rst),        // Use cpu_rst instead of rst
        .WE3(RegWrite),
        .WD3(Result),
        .A1(RD_Instr[19:15]),
        .A2(RD_Instr[24:20]),
        .A3(RD_Instr[11:7]),
        .RD1(RD1_Top),
        .RD2(RD2_Top)
    );

    Sign_Extend Sign_Extend(
        .In(RD_Instr),
        .ImmSrc(ImmSrc[0]),
        .Imm_Ext(Imm_Ext_Top)
    );

    Mux Mux_Register_to_ALU(
        .a(RD2_Top),
        .b(Imm_Ext_Top),
        .s(ALUSrc),
        .c(SrcB)
    );

    ALU ALU(
        .A(RD1_Top),
        .B(SrcB),
        .Result(ALUResult),
        .ALUControl(ALUControl_Top),
        .OverFlow(),
        .Carry(),
        .Zero(),
        .Negative()
    );

    Control_Unit_Top Control_Unit_Top(
        .Op(RD_Instr[6:0]),
        .RegWrite(RegWrite),
        .ImmSrc(ImmSrc),
        .ALUSrc(ALUSrc),
        .MemWrite(MemWrite),
        .ResultSrc(ResultSrc),
        .Branch(),
        .funct3(RD_Instr[14:12]),
        .funct7(RD_Instr[31:25]),
        .ALUControl(ALUControl_Top)
    );

    Data_Memory Data_Memory(
        // Port A (CPU) - Uses cpu_rst for safety
        .clk(clk),
        .rst(cpu_rst),        // Use cpu_rst instead of rst
        .WE(MemWrite),
        .WD(RD2_Top),
        .A(ALUResult),
        .RD(ReadData),
        // Port B (System)
        .we_b(we_b_data),
        .addr_b(addr_b),
        .din_b(din_b),
        .dout_b(dout_b)             // Not used, can be left unconnected
    );

    Mux Mux_DataMemory_to_Register(
        .a(ALUResult),
        .b(ReadData),
        .s(ResultSrc),
        .c(Result)
    );

endmodule
