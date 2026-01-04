module Instruction_Memory(
    // Port A (CPU Side - Combinational Read)
    input rst,
    input [31:0] A,
    output [31:0] RD,
    
    // Port B (System Side - Synchronous)
    input clk,
    input we_b,
    input [31:0] addr_b,
    input [31:0] din_b,
    output reg [31:0] dout_b
);

    reg [31:0] mem [1023:0];
    
    // Port A: Combinational read with reset protection
    // rst = 1: Run/Action, rst = 0: Reset (Active High Reset)
    assign RD = (~rst) ? {32{1'b0}} : mem[A[31:2]];
    
    // Port B: Synchronous read/write (byte address to word address conversion)
    always @(posedge clk) begin
        if (we_b) begin
            mem[addr_b[31:2]] <= din_b;
        end
        dout_b <= mem[addr_b[31:2]];
    end
    
    initial begin
        $readmemh("memfile.hex", mem);
    end

endmodule
