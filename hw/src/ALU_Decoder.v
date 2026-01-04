module ALU_Decoder(
    input [1:0] ALUOp,
    input [2:0] funct3,
    input [6:0] funct7,
    input [6:0] op,
    output reg [2:0] ALUControl
);

    always @(*) begin
        case (ALUOp)
            // ALUOp = 00: Load/Store instructions (LW/SW)
            2'b00: begin
                ALUControl = 3'b000;  // ADD for address calculation
            end
            
            // ALUOp = 01: Branch instructions (BEQ/BNE)
            2'b01: begin
                ALUControl = 3'b001;  // SUB for comparison
            end
            
            // ALUOp = 10: R-Type and I-Type arithmetic/logic instructions
            2'b10: begin
                case (funct3)
                    3'b000: begin  // ADD/SUB/ADDI
                        // SUB only if: R-Type (op[5]==1) AND funct7[5]==1
                        if (op[5] & funct7[5])
                            ALUControl = 3'b001;  // SUB
                        else
                            ALUControl = 3'b000;  // ADD or ADDI
                    end
                    
                    3'b010: begin  // SLT (Set Less Than)
                        ALUControl = 3'b101;
                    end
                    
                    3'b110: begin  // OR
                        ALUControl = 3'b011;
                    end
                    
                    3'b111: begin  // AND
                        ALUControl = 3'b010;
                    end
                    
                    default: begin
                        ALUControl = 3'b000;  // Default to ADD
                    end
                endcase
            end
            
            // Default case for undefined ALUOp values
            default: begin
                ALUControl = 3'b000;
            end
        endcase
    end

endmodule
