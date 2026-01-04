
module Register_File(clk,rst,WE3,WD3,A1,A2,A3,RD1,RD2,R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16, R17, R18, R19, R20, R21, R22, R23, R24, R25, R26, R27, R28, R29, R30, R31);

    input clk,rst,WE3;
    input [4:0]A1,A2,A3;
    input [31:0]WD3;

    output [31:0]RD1,RD2;
    output wire [31:0] R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16, R17, R18, R19, R20, R21, R22, R23, R24, R25, R26, R27, R28, R29, R30, R31;
    
    integer i;
  

    reg [31:0] Register [31:0];
    
    assign R0  = Register[0];
    assign R1  = Register[1];
    assign R2  = Register[2];
    assign R3  = Register[3];
    assign R4  = Register[4];
    assign R5  = Register[5];
    assign R6  = Register[6];
    assign R7  = Register[7];
    assign R8  = Register[8];
    assign R9  = Register[9];
    assign R10 = Register[10];
    assign R11 = Register[11];
    assign R12 = Register[12];
    assign R13 = Register[13];
    assign R14 = Register[14];
    assign R15 = Register[15];
    assign R16 = Register[16];
    assign R17 = Register[17];
    assign R18 = Register[18];
    assign R19 = Register[19];
    assign R20 = Register[20];
    assign R21 = Register[21];
    assign R22 = Register[22];
    assign R23 = Register[23];
    assign R24 = Register[24];
    assign R25 = Register[25];
    assign R26 = Register[26];
    assign R27 = Register[27];
    assign R28 = Register[28];
    assign R29 = Register[29];
    assign R30 = Register[30];
    assign R31 = Register[31];
       
    always @ (posedge clk)
    begin
        if(WE3)
            Register[A3] <= WD3;
    end

    assign RD1 = (~rst) ? 32'd0 : Register[A1];
    assign RD2 = (~rst) ? 32'd0 : Register[A2];

    initial begin   
        Register[0] = 0;   
 
        for (i = 1; i < 32; i = i + 1) 
            Register[i] = 1;   

    end

endmodule