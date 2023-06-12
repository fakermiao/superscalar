/*
 * @Author: 苗金标
 * @Date: 2023-05-30 19:58:16
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 14:16:30
 * @Description: 
 */
#include "alu.h"
#include "../../color.h"
namespace Supercore{
    execute_channel alu::evaluate(wb_feedback_pack wb_feedpack){
        execute_channel exe_channel;
        memset(&exe_channel,0,sizeof(exe_channel));
        if(!(wb_feedpack.enable && wb_feedpack.flush)){
            if(!issue_alu_fifo->is_empty() && !alu_wb_fifo->is_full()){
                instStr instInfo;
                this->issue_alu_fifo->pop(&instInfo);
                if(instInfo.valid){
                    instInfo.rd_enable = true;
                    int64_t alu_result = 0;
                    int64_t src1 = instInfo.rs1_value;
                    int64_t src2 = instInfo.rs2_valid ? instInfo.rs2_value : instInfo.imm;
                    switch(instInfo.fuOpType.aluOp){
                        case ALUOpType::lui:{
                            alu_result = instInfo.imm;
                            break;
                        }
                        case ALUOpType::auipc:{
                            alu_result = instInfo.imm + instInfo.pc;
                            break;
                        }
                        case ALUOpType::add:{
                            alu_result = src1 + src2;
                            break;
                        }
                        case ALUOpType::sll:{
                            alu_result = src1 << (src2 & 0x3f);
                            break;
                        }
                        case ALUOpType::slt:{
                            alu_result = src1 < src2;
                            break;
                        }
                        case ALUOpType::sltu:{
                            alu_result = ((uint64_t)src1 < (uint64_t)src2);
                            break;
                        }
                        case ALUOpType::xor_:{
                            alu_result = src1 ^ src2;
                            break;
                        }
                        case ALUOpType::srl:{
                            alu_result = ((uint64_t)src1 >> (src2 & 0x3f));
                            break;
                        }
                        case ALUOpType::or_:{
                            alu_result = src1 | src2;
                            break;
                        }
                        case ALUOpType::and_:{
                            alu_result = src1 & src2;
                            break;
                        }
                        case ALUOpType::sub:{
                            alu_result = src1 - src2;
                            break;
                        }
                        case ALUOpType::sra:{
                            alu_result = src1 >> (src2 & 0x3f);
                            break;
                        }
                        case ALUOpType::addw:{
                            alu_result = (int32_t)((int32_t)src1 - (int32_t)src2);
                            break;
                        }
                        case ALUOpType::subw:{
                            alu_result = (int32_t)((int32_t)src1 - (int32_t)src2);
                            break;
                        }
                        case ALUOpType::sllw:{
                            alu_result = (int32_t)((int32_t)src1 << (src2 & 0x1f));
                            break;
                        }
                        case ALUOpType::srlw:{
                            alu_result = (int32_t)(uint64_t(src1 & 0xffffffff) >> (src2 & 0x1f));
                            break;
                        }
                        case ALUOpType::sraw:{
                            alu_result = (int32_t)((int32_t)src1 >> (src2 & 0x1f));
                            break;
                        }
                        default:{
                            printf("%sno such alu type pc:%lx,inst:%x\033[0m\n",FORMATRED,instInfo.pc,instInfo.inst);
                            assert(0);
                            break;
                        }
                    }


                    instInfo.rd_enable      = true;
                    instInfo.rd_value       = alu_result;
                    exe_channel.rd_enable   = true;
                    exe_channel.rd_id       = instInfo.rd_phy;
                    exe_channel.rd_value    = alu_result;

                    this->alu_p(instInfo);
                }
                this->alu_wb_fifo->push(instInfo);
            }
        }else{
            alu_wb_fifo->flush();
        }
        return exe_channel;
    }
}