module Data_Memory(
    // Port A (CPU Side)
    input clk,
    input rst,
    input WE,
    input [31:0] A,
    input [31:0] WD,
    output reg [31:0] RD,
    
    // Port B (System Side - Synchronous)
    input we_b,
    input [31:0] addr_b,
    input [31:0] din_b,
    output reg [31:0] dout_b
);

    reg [31:0] mem [1023:0];
    
// Port A: 同步讀寫 (Read-first mode)
    always @(posedge clk) begin
        if (~rst) begin
            RD <= 32'd0;  // 輸出端的同步重設（位於 BRAM 陣列外部）
        end else begin
            RD <= mem[A[31:2]];  // 同步讀取
            if (WE) begin
                mem[A[31:2]] <= WD;  // 寫入
            end
        end
    end
    
    
    // Port B: Synchronous read/write (byte address to word address conversion)
    always @(posedge clk) begin
        if (we_b) begin
            mem[addr_b[31:2]] <= din_b;
        end
        dout_b <= mem[addr_b[31:2]];
    end
    
    initial begin
        mem[28] = 32'h00000020;
    end

endmodule
