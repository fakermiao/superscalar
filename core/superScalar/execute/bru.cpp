/*
 * @Author: 苗金标
 * @Date: 2023-05-31 10:13:28
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 14:21:08
 * @Description: 
 * The conditional branch instructions will generate an instruction-address-misaligned exception if the
 * target address is not aligned to a four-byte boundary and the branch condition evaluates to true.
 * If the branch condition evaluates to false, the instruction-address-misaligned exception will not be
 * raised.
 */
#include "bru.h"
#include "../../color.h"
namespace Supercore{
    execute_channel bru::evaluate(wb_feedback_pack wb_feedback){
        execute_channel exe_channel;
        memset(&exe_channel,0,sizeof(exe_channel));
        
        if(!(wb_feedback.enable && wb_feedback.flush)){
            if(!issue_bru_fifo->is_empty() && !bru_wb_fifo->is_full()){
                instStr instInfo;
                this->issue_bru_fifo->pop(&instInfo);
                // printf("bru\trs1:%lx,rs2:%lx\n",instInfo.rs1_value,instInfo.rs2_value);
                if(instInfo.valid){
                    bool jump = false;
                    uint64_t jump_pc = 0;
                    switch(instInfo.fuOpType.bruOp){
                        case BRUOpType::jal:{
                            jump = true;
                            jump_pc = instInfo.pc + instInfo.imm;
                            break;
                        }
                        case BRUOpType::jalr:{
                            jump = true;
                            jump_pc = (instInfo.rs1_value + instInfo.imm) ^ 1;
                            break;
                        }
                        case BRUOpType::beq:{
                            jump = (instInfo.rs1_value == instInfo.rs2_value);
                            jump_pc = instInfo.pc + instInfo.imm;
                            break;
                        }
                        case BRUOpType::bne:{
                            jump = (instInfo.rs1_value != instInfo.rs2_value);
                            jump_pc = instInfo.pc + instInfo.imm;
                            break;
                        }
                        case BRUOpType::blt:{
                            jump = (instInfo.rs1_value < instInfo.rs2_value);
                            jump_pc = instInfo.pc + instInfo.imm;
                            break;
                        }
                        case BRUOpType::bge:{
                            jump = (instInfo.rs1_value >= instInfo.rs2_value);
                            jump_pc = instInfo.pc + instInfo.imm;
                            break;
                        }
                        case BRUOpType::bltu:{
                            jump = ((uint64_t)instInfo.rs1_value < (uint64_t)instInfo.rs2_value);
                            jump_pc = instInfo.pc + instInfo.imm;
                            break;
                        }
                        case BRUOpType::bgeu:{
                            jump = ((uint64_t)instInfo.rs1_value >= (uint64_t)instInfo.rs2_value);
                            jump_pc = instInfo.pc + instInfo.imm;
                            break;
                        }
                        default:{
                            printf("%sno such bru type pc:%lx,inst:%x\033[0m\n",FORMATRED,instInfo.pc,instInfo.inst);
                            assert(0);
                        }
                    }    
                    //异常检查
                    jump_pc &= ~1;
                    if(instInfo.fuOpType.bruOp == BRUOpType::jal){
                        if(jump_pc % 4){
                            instInfo.has_execp  = true;
                            instInfo.execp_id   = exc_instr_misalign;
                            instInfo.execp_value = jump_pc;
                            instInfo.rd_enable  = false;
                            exe_channel.rd_enable = false;
                            printf("jal execption\n");
                        }else{
                            instInfo.rd_enable = true;
                            instInfo.rd_value  = instInfo.pc + 4;
                            exe_channel.rd_enable = true;
                            exe_channel.rd_id = instInfo.rd_phy;
                            exe_channel.rd_value = instInfo.pc + 4;
                        }
                    }else if(instInfo.fuOpType.bruOp == BRUOpType::jalr){
                        if(jump_pc % 4){
                            instInfo.has_execp = true;
                            instInfo.execp_id  = exc_instr_misalign;
                            instInfo.execp_value = jump_pc;
                            instInfo.rd_enable = false;
                            exe_channel.rd_enable = false;
                            printf("jalr execption pc:%lx\n",jump_pc);
                        }else{
                            instInfo.rd_enable = true;
                            instInfo.rd_value  = instInfo.pc + 4;
                            exe_channel.rd_enable = true;
                            exe_channel.rd_id  = instInfo.rd_phy;
                            exe_channel.rd_value = instInfo.pc + 4;
                        }
                    }else{
                        if(jump && (jump_pc % 4)){
                            instInfo.has_execp = true;
                            instInfo.execp_id  = exc_instr_misalign;
                            instInfo.execp_value = jump_pc;
                            printf("bru execption\n");
                        }
                    }
                    instInfo.bru_jump = jump;
                    instInfo.bru_next_pc = jump_pc;

                    this->bru_p(instInfo);
                }
                this->bru_wb_fifo->push(instInfo);
            }
        }else{
            bru_wb_fifo->flush();
        }
        return exe_channel;
    }
}