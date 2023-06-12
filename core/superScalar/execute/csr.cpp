/*
 * @Author: 苗金标
 * @Date: 2023-05-31 16:04:50
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 14:39:58
 * @Description: 
 */
#include "csr.h"
namespace Supercore{
    execute_channel csr::evaluate(wb_feedback_pack wb_feedback){
        execute_channel exe_channel;
        memset(&exe_channel,0,sizeof(exe_channel));

        if(!(wb_feedback.enable && wb_feedback.flush)){
            if(!issue_csr_fifo->is_empty() && !csr_wb_fifo->is_full()){
                instStr instInfo;
                this->issue_csr_fifo->pop(&instInfo);
                if(instInfo.valid){
                    int16_t csr_index = BITS(instInfo.imm,12,0);
                    uint64_t csr_result;
                    switch(instInfo.fuOpType.csrOp){
                        case CSROpType::ecall:{
                            instInfo.has_execp = true;
                            instInfo.execp_id  = exc_ecall_from_u;
                            break;
                        }
                        case CSROpType::ebreak:{
                            instInfo.has_execp = true;
                            instInfo.execp_id = exc_breakpoint;
                            break;
                        }
                        case CSROpType::sret:{
                            instInfo.valid = priv.sret();
                            break;
                        }
                        case CSROpType::mret:{
                            instInfo.valid = priv.mret();
                            break;
                        }
                        case CSROpType::wfi:{
                            break;
                        }
                        case CSROpType::csrrw:{
                            instInfo.valid = priv.check_op_permission_check(csr_index,instInfo.rs1_id != 0);
                            if(instInfo.valid) instInfo.valid = priv.csr_read(csr_index,csr_result);
                            if(instInfo.valid) instInfo.valid = priv.csr_write(csr_index,instInfo.rs1_value);
                            if(instInfo.valid && instInfo.rd_id){
                                this->set_instInfo(instInfo,exe_channel,csr_result);
                            }
                            break;
                        }
                        case CSROpType::csrrs:{
                            instInfo.valid = priv.check_op_permission_check(csr_index,instInfo.rs1_id != 0);
                            if(instInfo.valid) instInfo.valid = priv.csr_read(csr_index,csr_result);
                            if(instInfo.valid && instInfo.rs1_id) instInfo.valid = priv.csr_write(csr_index,csr_result | instInfo.rs1_value);
                            if(instInfo.valid && instInfo.rd_id){
                                this->set_instInfo(instInfo,exe_channel,csr_result);
                            }
                            break;
                        }
                        case CSROpType::csrrc:{
                            instInfo.valid = priv.check_op_permission_check(csr_index,instInfo.rs1_id != 0);
                            if(instInfo.valid) instInfo.valid = priv.csr_read(csr_index,csr_result);
                            if(instInfo.valid && instInfo.rs1_id) instInfo.valid = priv.csr_write(csr_index,csr_result & (~instInfo.rs1_value));
                            if(instInfo.valid && instInfo.rd_id){
                                this->set_instInfo(instInfo,exe_channel,csr_result);
                            }
                            break;
                        }
                        case CSROpType::csrrwi:{
                            instInfo.valid = priv.check_op_permission_check(csr_index,instInfo.rs1_id != 0);
                            if(instInfo.valid) instInfo.valid = priv.csr_read(csr_index,csr_result);
                            if(instInfo.valid) instInfo.valid = priv.csr_write(csr_index,instInfo.rs1_id);
                            if(instInfo.valid && instInfo.rd_id){
                                this->set_instInfo(instInfo,exe_channel,csr_result);
                            }
                            break;
                        }
                        case CSROpType::csrrsi:{
                            instInfo.valid = priv.check_op_permission_check(csr_index,instInfo.rs1_id != 0);
                            if(instInfo.valid) instInfo.valid = priv.csr_read(csr_index,csr_result);
                            if(instInfo.valid) instInfo.valid = priv.csr_write(csr_index,csr_result | instInfo.rs1_id);
                            if(instInfo.valid && instInfo.rd_id){
                                this->set_instInfo(instInfo,exe_channel,csr_result);
                            }
                            break;
                        }
                        case CSROpType::csrrci:{
                            instInfo.valid = priv.check_op_permission_check(csr_index,instInfo.rs1_id != 0);
                            if(instInfo.valid) instInfo.valid = priv.csr_read(csr_index,csr_result);
                            if(instInfo.valid) instInfo.valid = priv.csr_write(csr_index,csr_result & (~instInfo.rs1_id));
                            if(instInfo.valid && instInfo.rd_id){
                                this->set_instInfo(instInfo,exe_channel,csr_result);
                            }
                            break;
                        }
                    }
                    if(!instInfo.valid){
                        instInfo.has_execp = true;
                        instInfo.execp_id  = exc_illegal_instr;
                    }
                    this->csr_p(instInfo);
                }
                this->csr_wb_fifo->push(instInfo);
            }
        }else{
            csr_wb_fifo->flush();
        }
        return exe_channel;
    }
}