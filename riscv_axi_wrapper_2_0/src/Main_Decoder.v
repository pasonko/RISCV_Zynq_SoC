module Main_Decoder(Op,RegWrite,ImmSrc,ALUSrc,MemWrite,ResultSrc,Branch,ALUOp);
    input [6:0]Op;
    output RegWrite,ALUSrc,MemWrite,ResultSrc,Branch;
    output [1:0]ImmSrc,ALUOp;

    // 修正 1: RegWrite 加入 I-Type ALU (0010011)
    assign RegWrite = (Op == 7'b0000011 |    // lw
                       Op == 7'b0110011 |    // R-type (add, sub...)
                       Op == 7'b0010011) ? 1'b1 : 1'b0;  // I-type ALU (addi...)

    // ImmSrc 對 I-Type 使用預設值 2'b00，已正確
    assign ImmSrc = (Op == 7'b0100011) ? 2'b01 :  // S-type (sw)
                    (Op == 7'b1100011) ? 2'b10 :  // B-type (beq)
                                         2'b00 ;  // I-type (lw, addi...)

    // 修正 2: ALUSrc 加入 I-Type ALU (0010011)
    assign ALUSrc = (Op == 7'b0000011 |    // lw
                     Op == 7'b0100011 |    // sw
                     Op == 7'b0010011) ? 1'b1 : 1'b0;  // I-type ALU (addi...)

    assign MemWrite = (Op == 7'b0100011) ? 1'b1 : 1'b0;  // sw
    
    assign ResultSrc = (Op == 7'b0000011) ? 1'b1 : 1'b0; // lw

    assign Branch = (Op == 7'b1100011) ? 1'b1 : 1'b0;    // beq

    // 選項 A: 簡單版 (僅支援 addi)
    assign ALUOp = (Op == 7'b0110011) ? 2'b10 :  // R-type
                   (Op == 7'b1100011) ? 2'b01 :  // Branch
                                        2'b00 ;  // 其他 (加法)

    // 選項 B: 完整版 (支援所有 I-Type ALU 指令)
    // assign ALUOp = (Op == 7'b0110011) ? 2'b10 :  // R-type
    //                (Op == 7'b0010011) ? 2'b10 :  // I-type ALU (andi, ori...)
    //                (Op == 7'b1100011) ? 2'b01 :  // Branch
    //                                     2'b00 ;  // 其他

endmodule
