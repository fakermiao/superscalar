/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:40:08
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 20:56:58
 * @Description: 
 */
/*
 * @Author: 苗金标
 * @Date: 2023-04-03 21:01:51
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-24 17:18:42
 * @Description: 
 */
#include "decode.h"

namespace Supercore{
    void decode::evaluate(wb_feedback_pack wb_feedback_pack_t){
        if(!(wb_feedback_pack_t.enable && wb_feedback_pack_t.flush)){
            if(!this->fetch_decode_fifo->is_empty() && !this->decode_rename_fifo->is_full()){
                fetch_decode_pack rev_pack;
                decode_rename_pack send_pack;
                this->fetch_decode_fifo->pop(&rev_pack);
                memset(&send_pack,0,sizeof(send_pack));

                for(uint32_t i = 0;i < FETCH_WIDTH;i++){
                    if(rev_pack.fetch_decode[i].enable){
                        /*需要译码产生imm，rs1_valid,rs1_id,rs2_valid,rs2_id,rd_valid信号*/
                        rv_instr* inst = (rv_instr *)&rev_pack.fetch_decode[i].inst;
                        auto op_data = rev_pack.fetch_decode[i].inst;
                        auto imm_i = (op_data >> 20) & 0xfff;
                        auto imm_s = (((op_data >> 7) & 0x1f)) | (((op_data >> 25) & 0x7f) << 5);
                        auto imm_b = (((op_data >> 8) & 0x0f) << 1) | (((op_data >> 25) & 0x3f) << 5) | (((op_data >> 7) & 0x01) << 11) | (((op_data >> 31) & 0x01) << 12);
                        auto imm_u = op_data & (~0xfff);
                        auto imm_j = (((op_data >> 12) & 0xff) << 12) | (((op_data >> 20) & 0x01) << 11) | (((op_data >> 21) & 0x3ff) << 1) | (((op_data >> 31) & 0x01) << 20);
                        
                        send_pack.decode_issue[i].enable = true;
                        send_pack.decode_issue[i].valid  = rev_pack.fetch_decode[i].valid;
                        send_pack.decode_issue[i].pc     = rev_pack.fetch_decode[i].pc;
                        send_pack.decode_issue[i].inst   = rev_pack.fetch_decode[i].inst;
                        send_pack.decode_issue[i].has_execp = rev_pack.fetch_decode[i].has_execp;
                        send_pack.decode_issue[i].execp_id = rev_pack.fetch_decode[i].execp_id;
                        send_pack.decode_issue[i].debug_pc = rev_pack.fetch_decode[i].debug_pc;
                        send_pack.decode_issue[i].predicted = rev_pack.fetch_decode[i].predicted;
                        send_pack.decode_issue[i].predicted_jump = rev_pack.fetch_decode[i].predicted_jump;
                        send_pack.decode_issue[i].predicted_next_pc = rev_pack.fetch_decode[i].predicted_next_pc;
                        send_pack.decode_issue[i].checkpoint_id_valid = rev_pack.fetch_decode[i].checkpoint_id_valid;
                        send_pack.decode_issue[i].checkpoint_id = rev_pack.fetch_decode[i].checkpoint_id;
                        send_pack.decode_issue[i].issued = rev_pack.fetch_decode[i].issued;
                        if(send_pack.decode_issue[i].valid){
                        switch(inst->r_type.opcode){
                            case 0x37:{//lui
                                send_pack.decode_issue[i].imm       = sign_extend(imm_u,32);
                                send_pack.decode_issue[i].rs1_valid = false;
                                send_pack.decode_issue[i].rs1_id    = 0;
                                send_pack.decode_issue[i].rs2_valid = false;
                                send_pack.decode_issue[i].rs2_id    = 0;
                                send_pack.decode_issue[i].rd_valid  = true;
                                send_pack.decode_issue[i].rd_id     = inst->u_type.rd;
                                send_pack.decode_issue[i].fuType    = FuType::alu;
                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::lui;
                                break;
                            }
                            case 0x17:{//auipc
                                send_pack.decode_issue[i].imm       = sign_extend(imm_u,32);
                                send_pack.decode_issue[i].rs1_valid = false;
                                send_pack.decode_issue[i].rs1_id    = 0;
                                send_pack.decode_issue[i].rs2_valid = false;
                                send_pack.decode_issue[i].rs2_id    = 0;
                                send_pack.decode_issue[i].rd_valid  = true;
                                send_pack.decode_issue[i].rd_id     = inst->u_type.rd;
                                send_pack.decode_issue[i].fuType    = FuType::alu;
                                send_pack.decode_issue[i].fuOpType.aluOp  = ALUOpType::auipc;
                                break;
                            }
                            case 0x6f:{//jal
                                send_pack.decode_issue[i].imm       = sign_extend(imm_j,21);
                                send_pack.decode_issue[i].rs1_valid = false;
                                send_pack.decode_issue[i].rs1_id    = 0;
                                send_pack.decode_issue[i].rs2_valid = false;
                                send_pack.decode_issue[i].rs2_id    = 0;
                                send_pack.decode_issue[i].rd_valid  = true;
                                send_pack.decode_issue[i].rd_id     = inst->j_type.rd;
                                send_pack.decode_issue[i].fuType    = FuType::bru;
                                send_pack.decode_issue[i].fuOpType.bruOp = BRUOpType::jal;
                                break;
                            }
                            case 0x67:{//jalr
                                send_pack.decode_issue[i].imm       = sign_extend(imm_i,12);
                                send_pack.decode_issue[i].rs1_valid = true;
                                send_pack.decode_issue[i].rs1_id    = inst->i_type.rs1;
                                send_pack.decode_issue[i].rs2_valid = false;
                                send_pack.decode_issue[i].rs2_id    = 0;
                                send_pack.decode_issue[i].rd_valid  = true;
                                send_pack.decode_issue[i].rd_id     = inst->i_type.rd;
                                send_pack.decode_issue[i].fuType    = FuType::bru;
                                send_pack.decode_issue[i].fuOpType.bruOp = BRUOpType::jalr;
                                break;
                            }
                            case 0x63:{//beq bne blt bge bltu bgeu
                                send_pack.decode_issue[i].imm       = sign_extend(imm_b,13);
                                send_pack.decode_issue[i].rs1_valid = true;
                                send_pack.decode_issue[i].rs1_id    = inst->b_type.rs1;
                                send_pack.decode_issue[i].rs2_valid = true;
                                send_pack.decode_issue[i].rs2_id    = inst->b_type.rs2;
                                send_pack.decode_issue[i].fuType    = FuType::bru;
                                send_pack.decode_issue[i].rd_valid  = false;
                                send_pack.decode_issue[i].rd_id     = 0;
                                switch(inst->b_type.funct3){
                                    case 0x0:
                                        send_pack.decode_issue[i].fuOpType.bruOp = BRUOpType::beq;
                                        break;
                                    case 0x1:
                                        send_pack.decode_issue[i].fuOpType.bruOp = BRUOpType::bne;
                                        break;
                                    case 0x4:
                                        send_pack.decode_issue[i].fuOpType.bruOp = BRUOpType::blt;
                                        break;
                                    case 0x5:
                                        send_pack.decode_issue[i].fuOpType.bruOp = BRUOpType::bge;
                                        break;
                                    case 0x6:
                                        send_pack.decode_issue[i].fuOpType.bruOp = BRUOpType::bltu;
                                        break;
                                    case 0x7:
                                        send_pack.decode_issue[i].fuOpType.bruOp = BRUOpType::bgeu;
                                        break;
                                    default://invalid,需要触发异常
                                        send_pack.decode_issue[i].valid = false;
                                        send_pack.decode_issue[i].has_execp = true;
                                        send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                        break;
                                }
                                break;
                            }
                            case 0x03:{//lb,lh,lw,ld,lbu,lhu,lwu
                                send_pack.decode_issue[i].imm       = sign_extend(imm_i,12);
                                send_pack.decode_issue[i].rs1_valid = true;
                                send_pack.decode_issue[i].rs1_id    = inst->i_type.rs1;
                                send_pack.decode_issue[i].rs2_valid = false;
                                send_pack.decode_issue[i].rs2_id    = 0;
                                send_pack.decode_issue[i].rd_valid  = true;
                                send_pack.decode_issue[i].rd_id     = inst->i_type.rd;
                                send_pack.decode_issue[i].fuType    = FuType::lsu;
                                switch(inst->i_type.funct3){
                                    case 0x0://lb
                                        send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::lb;
                                        break;
                                    case 0x1://lh
                                        send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::lh;
                                        break;
                                    case 0x2://lw
                                        send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::lw;
                                        break;
                                    case 0x3://ld
                                        send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::ld;
                                        break;
                                    case 0x4://lbu
                                        send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::lbu;
                                        break;
                                    case 0x5://lhu
                                        send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::lhu;
                                        break;
                                    case 0x6://lwu
                                        send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::lwu;
                                        break;
                                    default://invalid 
                                        // printf("DECODE\tinst:%x\n",send_pack.decode_issue[i].inst);
                                        send_pack.decode_issue[i].valid = false;
                                        send_pack.decode_issue[i].has_execp = true;
                                        send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                        break;
                                }
                                break;
                            }
                            case 0x23:{//sb,sh,sw,sd
                                send_pack.decode_issue[i].imm       = sign_extend(imm_s,12);
                                send_pack.decode_issue[i].rs1_valid = true;
                                send_pack.decode_issue[i].rs1_id    = inst->s_type.rs1;
                                send_pack.decode_issue[i].rs2_valid = true;
                                send_pack.decode_issue[i].rs2_id    = inst->s_type.rs2;
                                send_pack.decode_issue[i].rd_valid  = false;
                                send_pack.decode_issue[i].rd_id     = 0;
                                send_pack.decode_issue[i].fuType    = FuType::lsu;
                                switch(inst->s_type.funct3){
                                    case 0x0:
                                        send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::sb;
                                        break;
                                    case 0x1:
                                        send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::sh;
                                        break;
                                    case 0x2:
                                        send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::sw;
                                        break;
                                    case 0x3:
                                        send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::sd;
                                        break;
                                    default:
                                        send_pack.decode_issue[i].valid = false;
                                        send_pack.decode_issue[i].has_execp = true;
                                        send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                        break;
                                }
                                break;
                            }
                            case 0x13:{//OPIMM
                                send_pack.decode_issue[i].imm       = sign_extend(imm_i,12);
                                send_pack.decode_issue[i].rs1_valid = true;
                                send_pack.decode_issue[i].rs1_id    = inst->i_type.rs1;
                                send_pack.decode_issue[i].rs2_valid = false;
                                send_pack.decode_issue[i].rs2_id    = 0;
                                send_pack.decode_issue[i].rd_valid  = true;
                                send_pack.decode_issue[i].rd_id     = inst->i_type.rd;
                                send_pack.decode_issue[i].fuType    = FuType::alu;
                                switch(inst->i_type.funct3){
                                    case 0x0:
                                        send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::add;
                                        break;
                                    case 0x2:
                                        send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::slt;
                                        break;
                                    case 0x3:
                                        send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::sltu;
                                        break;
                                    case 0x4:
                                        send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::xor_;
                                        break;
                                    case 0x6:
                                        send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::or_;
                                        break;
                                    case 0x7:
                                        send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::and_;
                                        break;
                                    case 0x1:
                                        send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::sll;
                                        break;
                                    case 0x5:
                                        if((inst->r_type.funct7 >> 1) == 0x00)
                                            send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::srl;
                                        else if((inst->r_type.funct7 >> 1) == 0x10)
                                            send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::sra;
                                        else{
                                            send_pack.decode_issue[i].valid = false;
                                            send_pack.decode_issue[i].has_execp = true;
                                            send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                        }
                                        break;
                                    default:
                                        send_pack.decode_issue[i].valid = false;
                                        send_pack.decode_issue[i].has_execp = true;
                                        send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                        break;
                                }   
                                break;
                            }
                            case 0x1B:{//OPIMM32
                                send_pack.decode_issue[i].imm       = sign_extend(imm_i,12);
                                send_pack.decode_issue[i].rs1_valid = true;
                                send_pack.decode_issue[i].rs1_id    = inst->i_type.rs1;
                                send_pack.decode_issue[i].rs2_valid = false;
                                send_pack.decode_issue[i].rs2_id    = 0;
                                send_pack.decode_issue[i].rd_valid  = true;
                                send_pack.decode_issue[i].rd_id     = inst->i_type.rd;
                                send_pack.decode_issue[i].fuType    = FuType::alu;
                                switch(inst->i_type.funct3){
                                    case 0x0:
                                        send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::addw;
                                        break;
                                    case 0x1:
                                        send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::sllw;
                                        break;
                                    case 0x5:
                                        if(inst->r_type.funct7 == 0x00)
                                            send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::srlw;
                                        else if(inst->r_type.funct7 == 0x20)
                                            send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::sraw;
                                        else{
                                            send_pack.decode_issue[i].valid = false;
                                            send_pack.decode_issue[i].has_execp = true;
                                            send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                        }
                                        break;
                                    default:
                                        send_pack.decode_issue[i].valid = false;
                                        send_pack.decode_issue[i].has_execp = true;
                                        send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                        break;
                                }
                                break;
                            }
                            case 0x33:{//OP
                                send_pack.decode_issue[i].imm       = 0;
                                send_pack.decode_issue[i].rs1_valid = true;
                                send_pack.decode_issue[i].rs1_id    = inst->r_type.rs1;
                                send_pack.decode_issue[i].rs2_valid = true;
                                send_pack.decode_issue[i].rs2_id    = inst->r_type.rs2;
                                send_pack.decode_issue[i].rd_valid  = true;
                                send_pack.decode_issue[i].rd_id     = inst->r_type.rd;
                                send_pack.decode_issue[i].fuType    = FuType::alu;
                                switch(inst->r_type.funct7){
                                    case 0x00:{
                                        switch(inst->r_type.funct3){
                                            case 0x0:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::add;
                                                break;
                                            case 0x1:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::sll;
                                                break;
                                            case 0x2:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::slt;
                                                break;
                                            case 0x3:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::sltu;
                                                break;
                                            case 0x4:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::xor_;
                                                break;
                                            case 0x5:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::srl;
                                                break;
                                            case 0x6:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::or_;
                                                break;
                                            case 0x7:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::and_;
                                                break;
                                            default:
                                                send_pack.decode_issue[i].valid = false;
                                                send_pack.decode_issue[i].has_execp = true;
                                                send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                            break;
                                        }
                                        break;
                                    }
                                    case 0x20:{
                                        switch(inst->r_type.funct3){
                                            case 0x0:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::sub;
                                                break;
                                            case 0x5:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::sra;
                                                break;
                                            default:
                                                send_pack.decode_issue[i].valid = false;
                                                send_pack.decode_issue[i].has_execp = true;
                                                send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                                break;
                                        }
                                        break;
                                    }
                                    case 0x01:{//mdu
                                        send_pack.decode_issue[i].fuType = FuType::mdu;
                                        switch(inst->r_type.funct3){
                                            case 0x0:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::mul;
                                                break;
                                            case 0x1:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::mulh;
                                                break;
                                            case 0x2:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::mulhsu;
                                                break;
                                            case 0x3:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::mulhu;
                                                break;
                                            case 0x4:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::div;
                                                break;
                                            case 0x5:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::divu;
                                                break;
                                            case 0x6:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::rem;
                                                break;
                                            case 0x7:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::remu;
                                                break;
                                            default:
                                                send_pack.decode_issue[i].valid = false;
                                                send_pack.decode_issue[i].has_execp = true;
                                                send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                        }
                                        break;
                                    }
                                    default:
                                        send_pack.decode_issue[i].valid = false;
                                        send_pack.decode_issue[i].has_execp = true;
                                        send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                        break;
                                }
                                break;
                            }
                            case 0x3B:{//OP32
                                send_pack.decode_issue[i].imm       = 0;
                                send_pack.decode_issue[i].rs1_valid = true;
                                send_pack.decode_issue[i].rs1_id    = inst->r_type.rs1;
                                send_pack.decode_issue[i].rs2_valid = true;
                                send_pack.decode_issue[i].rs2_id    = inst->r_type.rs2;
                                send_pack.decode_issue[i].rd_valid  = true;
                                send_pack.decode_issue[i].rd_id     = inst->r_type.rd;
                                send_pack.decode_issue[i].fuType    = FuType::alu;
                                switch(inst->r_type.funct7){
                                    case 0x00:{
                                        switch(inst->r_type.funct3){
                                            case 0x0:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::addw;
                                                break;
                                            case 0x1:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::sllw;
                                                break;
                                            case 0x5:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::srlw;
                                                break;
                                            default:
                                                send_pack.decode_issue[i].valid = false;
                                                send_pack.decode_issue[i].has_execp = true;
                                                send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                                break;
                                        }
                                        break;
                                    }
                                    case 0x20:{
                                        switch(inst->r_type.funct3){
                                            case 0x0:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::subw;
                                                break;
                                            case 0x5:
                                                send_pack.decode_issue[i].fuOpType.aluOp = ALUOpType::sraw;
                                                break;
                                            default:
                                                send_pack.decode_issue[i].valid = false;
                                                send_pack.decode_issue[i].has_execp = true;
                                                send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                                break;
                                        }
                                        break;
                                    }
                                    case 0x01:{
                                        send_pack.decode_issue[i].fuType = FuType::mdu;
                                        switch(inst->r_type.funct3){
                                            case 0x0:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::mulw;
                                                break;
                                            case 0x4:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::divw;
                                                break;
                                            case 0x5:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::divuw;
                                                break;
                                            case 0x6:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::remw;
                                                break;
                                            case 0x7:
                                                send_pack.decode_issue[i].fuOpType.mduOp = MDUOpType::remuw;
                                                break;
                                            default:
                                                send_pack.decode_issue[i].valid = false;
                                                send_pack.decode_issue[i].has_execp = true;
                                                send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                                break;
                                        }
                                        break;
                                    }
                                    default:
                                        send_pack.decode_issue[i].valid = false;
                                        send_pack.decode_issue[i].has_execp = true;
                                        send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                        break;
                                }
                                break;
                            }
                            case 0x2f:{//AMO
                                send_pack.decode_issue[i].imm       = 0;
                                send_pack.decode_issue[i].rs1_valid = true;
                                send_pack.decode_issue[i].rs1_id    = inst->r_type.rs1;
                                send_pack.decode_issue[i].rs2_valid = true;
                                send_pack.decode_issue[i].rs2_id    = inst->r_type.rs2;
                                send_pack.decode_issue[i].rd_valid  = true;
                                send_pack.decode_issue[i].rd_id     = inst->r_type.rd;
                                send_pack.decode_issue[i].fuType    = FuType::lsu;
                                if(inst->r_type.funct3 == 0x2){
                                    switch(inst->r_type.funct7 >> 2){
                                        case 0x02:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::lrw;
                                            break;
                                        case 0x03:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::scw;
                                            break;
                                        case 0x01:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amoswapw;
                                            break;
                                        case 0x00:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amoaddw;
                                            break;
                                        case 0x04:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amoxorw;
                                            break;
                                        case 0x0C:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amoandw;
                                            break;
                                        case 0x08:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amoorw;
                                            break;
                                        case 0x10:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amominw;
                                            break;
                                        case 0x14:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amomaxw;
                                            break;
                                        case 0x18:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amominuw;
                                            break;
                                        case 0x1C:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amomaxuw;
                                            break;
                                        default:
                                            send_pack.decode_issue[i].valid = false;
                                            send_pack.decode_issue[i].has_execp = true;
                                            send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                            break;
                                    }
                                }else if(inst->r_type.funct3 == 0x03){
                                    switch(inst->r_type.funct7 >> 2){
                                        case 0x02:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::lr;
                                            break;
                                        case 0x03:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::sc;
                                            break;
                                        case 0x01:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amoswap;
                                            break;
                                        case 0x00:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amoadd;
                                            break;
                                        case 0x04:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amoxor;
                                            break;
                                        case 0x0C:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amoand;
                                            break;
                                        case 0x08:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amoor;
                                            break;
                                        case 0x10:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amomin;
                                            break;
                                        case 0x14:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amomax;
                                            break;
                                        case 0x18:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amominu;
                                            break;
                                        case 0x1C:
                                            send_pack.decode_issue[i].fuOpType.lsuOp = LSUOpType::amomaxu;
                                            break;
                                        default:
                                            send_pack.decode_issue[i].valid = false;
                                            send_pack.decode_issue[i].has_execp = true;
                                            send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                            break;
                                    }
                                }else{
                                    send_pack.decode_issue[i].valid = false;
                                    send_pack.decode_issue[i].has_execp = true;
                                    send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                }
                                break;
                            }
                            case 0x0F:{//FENCE
                                send_pack.decode_issue[i].imm       = imm_i;
                                send_pack.decode_issue[i].rs1_valid = true;
                                send_pack.decode_issue[i].rs1_id    = inst->i_type.rs1;
                                send_pack.decode_issue[i].rs2_valid = false;
                                send_pack.decode_issue[i].rs2_id    = 0;
                                send_pack.decode_issue[i].rd_valid  = true;
                                send_pack.decode_issue[i].rd_id     = inst->i_type.rd;
                                send_pack.decode_issue[i].fuType    = FuType::mou;
                                switch(inst->i_type.funct3){
                                    case 0x0:
                                        send_pack.decode_issue[i].fuOpType.mouOp = MOUOpType::fence;
                                        break;
                                    case 0x1:
                                        send_pack.decode_issue[i].fuOpType.mouOp = MOUOpType::fencei;
                                        break;
                                    default:
                                        send_pack.decode_issue[i].valid = false;
                                        send_pack.decode_issue[i].has_execp = true;
                                        send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                        break;
                                }
                                break;
                            }
                            case 0x73:{//SYSTEM
                                send_pack.decode_issue[i].imm       = imm_i;
                                send_pack.decode_issue[i].rs1_valid = true;
                                send_pack.decode_issue[i].rs1_id    = inst->i_type.rs1;
                                send_pack.decode_issue[i].rs2_valid = false;
                                send_pack.decode_issue[i].rs2_id    = 0;
                                send_pack.decode_issue[i].rd_valid  = true;
                                send_pack.decode_issue[i].rd_id     = inst->i_type.rd;
                                send_pack.decode_issue[i].fuType    = FuType::csr;
                                switch(inst->i_type.funct3){
                                    case 0x0:{
                                        send_pack.decode_issue[i].rd_valid = false;
                                        uint64_t rs2 = (inst->i_type.imm12) & ((1<<5)-1);
                                        switch(((inst->i_type.imm12) & ((1 << 12)-1)) >> 5){
                                            case 0x00:{//ecall ebreak
                                                if(inst->i_type.rs1 == 0 && inst->i_type.rd == 0){
                                                    switch((inst->i_type.imm12) & ((1<<5)-1)){
                                                        case 0x0://ecall
                                                            send_pack.decode_issue[i].fuOpType.csrOp = CSROpType::ecall;
                                                            break;
                                                        case 0x1:
                                                            send_pack.decode_issue[i].fuOpType.csrOp = CSROpType::ebreak;
                                                            break;
                                                        default:
                                                            send_pack.decode_issue[i].valid = false;
                                                            send_pack.decode_issue[i].has_execp = true;
                                                            send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                                            break;
                                                    }
                                                }else{
                                                    send_pack.decode_issue[i].valid = false;
                                                    send_pack.decode_issue[i].has_execp = true;
                                                    send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                                }
                                                break;
                                            }
                                            case 0x08:{//sret wfi
                                                if(inst->r_type.rs1 == 0 && inst->r_type.rd == 0){
                                                    switch((inst->i_type.imm12) & ((1<<5)-1)){
                                                        case 0x02:
                                                            send_pack.decode_issue[i].fuOpType.csrOp = CSROpType::sret;
                                                            break;
                                                        case 0x05:
                                                            send_pack.decode_issue[i].fuOpType.csrOp = CSROpType::wfi;
                                                            break;
                                                        default:
                                                            send_pack.decode_issue[i].valid = false;
                                                            send_pack.decode_issue[i].has_execp = true;
                                                            send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                                            break;
                                                    }
                                                }else{
                                                    send_pack.decode_issue[i].valid = false;
                                                    send_pack.decode_issue[i].has_execp = true;
                                                    send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                                }
                                                break;
                                            }
                                            case 0x18:{
                                                if(rs2 == 0x02 && inst->r_type.rs1 == 0 && inst->r_type.rd == 0){
                                                    send_pack.decode_issue[i].fuOpType.csrOp = CSROpType::mret;
                                                }else{
                                                    send_pack.decode_issue[i].valid = false;
                                                    send_pack.decode_issue[i].has_execp = true;
                                                    send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                                }
                                                break;
                                            }
                                            case 0x09:{
                                                send_pack.decode_issue[i].rs2_valid = true;
                                                send_pack.decode_issue[i].rs2_id    = inst->r_type.rs2;
                                                send_pack.decode_issue[i].fuType = FuType::mou;
                                                send_pack.decode_issue[i].fuOpType.mouOp = MOUOpType::sfence_vma;
                                                break;
                                            }
                                            default:
                                                send_pack.decode_issue[i].valid = false;
                                                send_pack.decode_issue[i].has_execp = true;
                                                send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                                break;
                                        }
                                        break;
                                    }
                                    case 0x1:{
                                        send_pack.decode_issue[i].fuOpType.csrOp = CSROpType::csrrw;
                                        break;
                                    }
                                    case 0x2:{
                                        send_pack.decode_issue[i].fuOpType.csrOp = CSROpType::csrrs;
                                        break;
                                    }
                                    case 0x3:{
                                        send_pack.decode_issue[i].fuOpType.csrOp = CSROpType::csrrc;
                                        break;
                                    }
                                    case 0x5:{
                                        send_pack.decode_issue[i].fuOpType.csrOp = CSROpType::csrrwi;
                                        break;
                                    }
                                    case 0x6:{
                                        send_pack.decode_issue[i].fuOpType.csrOp = CSROpType::csrrsi;
                                        break;
                                    }
                                    case 0x7:{
                                        send_pack.decode_issue[i].fuOpType.csrOp = CSROpType::csrrci;
                                        break;
                                    }
                                    default:{
                                            send_pack.decode_issue[i].valid = false;
                                            send_pack.decode_issue[i].has_execp = true;
                                            send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                            break;
                                    }
                                }
                                break;
                            }
                            default:{
                                send_pack.decode_issue[i].valid = false;
                                send_pack.decode_issue[i].has_execp = true;
                                send_pack.decode_issue[i].execp_id = exc_illegal_instr;
                                break;
                            }
                        }
                        }
                                                this->decode_p(i,send_pack.decode_issue[i]);
                    }else{
                        send_pack.decode_issue[i].enable = rev_pack.fetch_decode[i].enable;
                        send_pack.decode_issue[i].pc = rev_pack.fetch_decode[i].pc;
                        send_pack.decode_issue[i].has_execp = rev_pack.fetch_decode[i].has_execp;
                        send_pack.decode_issue[i].execp_id  = rev_pack.fetch_decode[i].execp_id;
                    }
                    // printf("DECODE\tpc:%lx,inst:%x,has_exp:%d,exp_id:%d\n",send_pack.decode_issue[i].pc,send_pack.decode_issue[i].inst,
                    //     send_pack.decode_issue[i].has_execp,*(uint32_t*)&send_pack.decode_issue[i].execp_id);
                }
                this->decode_rename_fifo->push(send_pack);
            }
        }else{
            decode_rename_fifo->flush();
        }
    }
}
