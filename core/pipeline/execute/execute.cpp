/*
 * @Author: 苗金标
 * @Date: 2023-04-05 17:06:00
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-24 17:18:07
 * @Description: 
 */
#include "execute.h"
#include "alu.h"
#include "bru.h"
#include "mdu.h"
/*
主要设置目的寄存器的值和跳转反馈
*/
namespace core{
    
    bru_feedback_pack execute::evaluate(wb_feedback_pack wb_feedback_pack_t,exe_feedback_t& exe_feedback){
        bru_feedback_pack bru_feedback_pack_t;
        bru_feedback_pack_t.enable = false;
        bru_feedback_pack_t.jump   = false;
        bru_feedback_pack_t.next_pc= 0;
        
        if(!(wb_feedback_pack_t.enable && wb_feedback_pack_t.flush)){
                exe_feedback.rd_valid = false;
                exe_feedback.rd_value = 0;
                exe_feedback.rd_value = 0;
            if(!issue_execute_fifo->is_empty() && !execute_wb_fifo->is_full()){
                instStr instInfo;
                this->issue_execute_fifo->pop(&instInfo);
                    // printf("EXECUTE\tpc:%lx,inst:%x,valid:%d,rs1_id:%d,rs2_id:%d,rs1:%lx,rs2:%lx,has_exp:%d,exp_id:%d\n",instInfo.pc,instInfo.inst,instInfo.valid,instInfo.rs1_id,
                    //     instInfo.rs2_id,instInfo.rs1_value,instInfo.rs2_value,instInfo.has_execp,instInfo.execp_id);
                exe_feedback.rd_valid = instInfo.rd_valid;
                exe_feedback.rd_id     = instInfo.rd_id;
                exe_feedback.rd_value  = 0;
                instInfo.bru_jump = false;
                instInfo.bru_next_pc = 0;
                if(instInfo.valid){
                    switch(instInfo.fuType){
                        case FuType::alu:{
                            instInfo.rd_enable = true;
                            switch(instInfo.fuOpType.aluOp){
                                case ALUOpType::lui:{
                                    instInfo.rd_value = instInfo.imm;
                                    break;
                                }
                                case ALUOpType::auipc:{
                                    // printf("EXECUTE\tinst:%x,pc:%lx,imm:%lx\n",instInfo.inst,instInfo.pc,instInfo.imm);
                                    instInfo.rd_value = (instInfo.imm) + instInfo.pc;
                                    break;
                                }
                                default:{
                                    int64_t src1 = instInfo.rs1_value;
                                    int64_t src2 = instInfo.rs2_valid ? instInfo.rs2_value : instInfo.imm;
                                    instInfo.rd_value = alu_exec(src1,src2,instInfo.fuOpType.aluOp);
                                    // printf("EXECUTE\tinst:%x,rs1_id:%d,rs2_id:%d,rs1:%lx,rs2:%lx,rd:%lx,imm:%lx\n",instInfo.inst,instInfo.rs1_id,instInfo.rs2_id,instInfo.rs1_value,instInfo.rs2_value,instInfo.rd_value,instInfo.imm);
                                    break;
                                }
                            }
                            break;
                        }
                        case FuType::bru:{
                            //printf("EXECUTE\tinst:%x,rs1_id:%d,rs2_id%d,rs1:%lx,rs2:%lx\n",instInfo.inst,instInfo.rs1_id,instInfo.rs2_id,instInfo.rs1_value,instInfo.rs2_value);
                            bru_feedback_pack_t.enable = false;

                            bru_exec(instInfo,&bru_feedback_pack_t);
                            if(instInfo.fuOpType.bruOp == BRUOpType::jal){
                                // instInfo.rd_enable = true;
                                // instInfo.rd_value  = instInfo.pc + 4;
                                if(bru_feedback_pack_t.next_pc & 1) bru_feedback_pack_t.next_pc ^= 1;
                                if(bru_feedback_pack_t.next_pc % 4){
                                    instInfo.has_execp = true;
                                    instInfo.execp_id  = exc_instr_misalign;
                                    instInfo.execp_value = bru_feedback_pack_t.next_pc;
                                    instInfo.rd_enable = false;
                                }else{
                                    instInfo.rd_enable = true;
                                    instInfo.rd_value  = instInfo.pc + 4;
                                }
                            }else if(instInfo.fuOpType.bruOp == BRUOpType::jalr){
                                // instInfo.rd_enable = true;
                                // instInfo.rd_value  = instInfo.pc + 4;
                                if(bru_feedback_pack_t.next_pc & 1) bru_feedback_pack_t.next_pc ^= 1;
                                if(bru_feedback_pack_t.next_pc % 4){
                                    instInfo.has_execp = true;
                                    instInfo.execp_id  = exc_instr_misalign;
                                    instInfo.execp_value = bru_feedback_pack_t.next_pc;
                                    instInfo.rd_enable = false;
                                }else{
                                    instInfo.rd_enable = true;
                                    instInfo.rd_value  = instInfo.pc + 4;
                                }
                            }else{
                                if(bru_feedback_pack_t.jump){
                                    if(bru_feedback_pack_t.next_pc % 4){
                                        instInfo.has_execp = true;
                                        instInfo.execp_id  = exc_instr_misalign;
                                        instInfo.execp_value = bru_feedback_pack_t.next_pc;
                                    }
                                }
                            }
                            instInfo.bru_jump = bru_feedback_pack_t.jump;
                            instInfo.bru_next_pc = bru_feedback_pack_t.next_pc;
                            // printf("EXECUTE\tfeedback enable:%d,feedback pc:%lx,feedback jump:%d,before pc:%lx,imm:%lx\n",bru_feedback_pack_t.enable,bru_feedback_pack_t.next_pc,
                            // bru_feedback_pack_t.jump,instInfo.pc,instInfo.imm);
                            break;
                        }
                        /*csr读写相关的6条指令（由于csr一致性？？？）需要冲刷流水线但是还未实现------------------------,
                            wfi没有实现*/
                        case FuType::csr:{
                            int16_t csr_index = BITS(instInfo.imm,12,0);
                            bru_feedback_pack_t.enable = true;
                            bru_feedback_pack_t.jump = false;
                            bru_feedback_pack_t.next_pc = 0;
                            switch(instInfo.fuOpType.csrOp){
                                case CSROpType::ecall:{
                                    /*由于ecall指令要用到寄存器R[17]判断是程序结束还是调用因此在写回阶段处理而不是执行阶段，因为执行阶段
                                    可能读不到正确的寄存器值*/
                                    instInfo.has_execp = true;
                                    instInfo.execp_id  = exc_ecall_from_u;
                                    break;
                                }
                                case CSROpType::ebreak:{
                                    instInfo.has_execp = true;
                                    instInfo.execp_id  = exc_breakpoint;
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
                                    uint64_t csr_result;
                                    if(instInfo.valid) instInfo.valid = priv.csr_read(csr_index,csr_result);
                                    if(instInfo.valid) instInfo.valid = priv.csr_write(csr_index,instInfo.rs1_value);
                                    // printf("EXECUTE\tmtvec:%lx\n",priv.get_mtvec());
                                    if(instInfo.valid && instInfo.rd_id){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = csr_result;
                                    }
                                    break;
                                }
                                case CSROpType::csrrs:{
                                    instInfo.valid = priv.check_op_permission_check(csr_index,instInfo.rs1_id != 0);
                                    uint64_t csr_result;
                                    if(instInfo.valid) instInfo.valid = priv.csr_read(csr_index,csr_result);
                                    if(instInfo.valid && instInfo.rs1_id) instInfo.valid = priv.csr_write(csr_index,csr_result | instInfo.rs1_value);
                                    if(instInfo.valid && instInfo.rd_id){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = csr_result;
                                    }
                                    break;
                                }
                                case CSROpType::csrrc:{
                                    instInfo.valid = priv.check_op_permission_check(csr_index,instInfo.rs1_id != 0);
                                    uint64_t csr_result;
                                    if(instInfo.valid) instInfo.valid = priv.csr_read(csr_index,csr_result);
                                    if(instInfo.valid && instInfo.rs1_id) instInfo.valid = priv.csr_write(csr_index,csr_result & (~instInfo.rs1_value));
                                    if(instInfo.valid && instInfo.rd_id){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = csr_result;
                                    } 
                                    break;
                                }
                                case CSROpType::csrrwi:{
                                    instInfo.valid = priv.check_op_permission_check(csr_index,instInfo.rs1_id != 0);
                                    uint64_t csr_result;
                                    if(instInfo.valid) priv.csr_read(csr_index,csr_result);
                                    if(instInfo.valid) priv.csr_write(csr_index,instInfo.rs1_id);
                                    if(instInfo.valid && instInfo.rd_id){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = csr_result;
                                    }
                                    break;
                                }   
                                case CSROpType::csrrsi:{
                                    instInfo.valid = priv.check_op_permission_check(csr_index,instInfo.rs1_id != 0);
                                    uint64_t csr_result;
                                    if(instInfo.valid) instInfo.valid = priv.csr_read(csr_index,csr_result);
                                    if(instInfo.valid && instInfo.rs1_id) instInfo.valid = priv.csr_write(csr_index,csr_result | instInfo.rs1_id);
                                    if(instInfo.valid && instInfo.rd_id){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = csr_result;
                                    }
                                    break;
                                }
                                case CSROpType::csrrci:{
                                    instInfo.valid = priv.check_op_permission_check(csr_index,instInfo.rs1_id != 0);
                                    uint64_t csr_result;
                                    if(instInfo.valid) instInfo.valid = priv.csr_read(csr_index,csr_result);
                                    if(instInfo.valid && instInfo.rs1_id) instInfo.valid = priv.csr_write(csr_index,csr_result & (~instInfo.rs1_id));
                                    if(instInfo.valid && instInfo.rd_id){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = csr_result;
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                        case FuType::lsu:{
                            instInfo.rd_enable = true;
                            switch(instInfo.fuOpType.lsuOp){
                                /*lb,lh,lw,ld,lbu,lhu,lwu,sb,sh,sw,sd
                                lr ,sc ,amoswap ,amoadd ,amoxor ,amoand ,amoxor ,amoand ,amoor ,amomin ,amomax ,amominu ,amomaxu,
                                lrw,scw,amoswapw,amoaddw,amoxorw,amoandw,amoxorw,amoandw,amoorw,amominw,amomaxw,amominuw,amomaxuw
                                原子指令amo所有指令都需要进行修改
                                */
                                case LSUOpType::lb:{
                                    int8_t buf;
                                    // priv.mem->read(instInfo.rs1_value + instInfo.imm,(void*)&buf,1);
                                    // instInfo.rd_enable = true;
                                    // instInfo.rd_value  = buf;
                                    rv_exc_code exp_id = mem_read(instInfo.rs1_value + instInfo.imm,1,(unsigned char*)&buf);
                                    if(exp_id == exc_custom_ok){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = buf;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exp_id;
                                    }
                                    break;
                                }
                                case LSUOpType::lh:{
                                    int16_t buf;
                                    // priv.mem->read(instInfo.rs1_value + instInfo.imm,(void*)&buf,2);
                                    // instInfo.rd_enable = true;
                                    // instInfo.rd_value  = buf;
                                    rv_exc_code exp_id = mem_read(instInfo.rs1_value + instInfo.imm,2,(unsigned char*)&buf);
                                    if(exp_id == exc_custom_ok){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = buf;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exp_id;
                                    }
                                    break;
                                }
                                case LSUOpType::lw:{
                                    int32_t buf;
                                    // priv.mem->read(instInfo.rs1_value + instInfo.imm,(void*)&buf,4);
                                    // instInfo.rd_enable = true;
                                    // instInfo.rd_value  = buf;
                                    rv_exc_code exp_id = mem_read(instInfo.rs1_value + instInfo.imm,4,(unsigned char*)&buf);
                                    if(exp_id == exc_custom_ok){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = buf;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exp_id;
                                    }
                                    break;
                                }
                                case LSUOpType::ld:{
                                    int64_t buf;
                                    rv_exc_code exp_id = mem_read(instInfo.rs1_value + instInfo.imm,8,(unsigned char*)&buf);
                                    if(exp_id == exc_custom_ok){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = buf;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exp_id;
                                    }
                                    break;
                                }
                                case LSUOpType::lbu:{
                                    uint8_t buf;
                                    // priv.mem->read(instInfo.rs1_value + instInfo.imm,(void*)&buf,1);
                                    // instInfo.rd_enable = true;
                                    // instInfo.rd_value  = buf;
                                    rv_exc_code exp_id = mem_read(instInfo.rs1_value + instInfo.imm,1,(unsigned char*)&buf);
                                    if(exp_id == exc_custom_ok){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = buf;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exp_id;
                                    }
                                    break;
                                }
                                case LSUOpType::lhu:{
                                    uint16_t buf;
                                    // priv.mem->read(instInfo.rs1_value + instInfo.imm,(void*)&buf,2);
                                    // instInfo.rd_enable = true;
                                    // instInfo.rd_value  = buf;
                                    rv_exc_code exp_id = mem_read(instInfo.rs1_value + instInfo.imm,2,(unsigned char*)&buf);
                                    if(exp_id == exc_custom_ok){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = buf;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exp_id;
                                    }
                                    break;
                                }
                                case LSUOpType::lwu:{
                                    uint32_t buf;
                                    // priv.mem->read(instInfo.rs1_value + instInfo.imm,(void*)&buf,4);
                                    // instInfo.rd_enable = true;
                                    // instInfo.rd_value  = buf;
                                    rv_exc_code exp_id = mem_read(instInfo.rs1_value + instInfo.imm,4,(unsigned char*)&buf);
                                    if(exp_id == exc_custom_ok){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = buf;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exp_id;
                                    }
                                    break;
                                }
                                case LSUOpType::sb:{
                                    // priv.mem->write(instInfo.rs1_value + instInfo.imm,(void*)&instInfo.rs2_value,1);
                                    rv_exc_code exp_id = mem_write(instInfo.rs1_value + instInfo.imm,1,(unsigned char*)&instInfo.rs2_value);
                                    if(exp_id == exc_custom_ok){
                                        break;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exp_id;
                                        break;
                                    }
                                }
                                case LSUOpType::sh:{
                                    // priv.mem->write(instInfo.rs1_value + instInfo.imm,(void*)&instInfo.rs2_value,2);
                                    rv_exc_code exp_id = mem_write(instInfo.rs1_value + instInfo.imm,2,(unsigned char*)&instInfo.rs2_value);
                                    if(exp_id == exc_custom_ok){
                                        break;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exp_id;
                                        break;
                                    }
                                    break;
                                }
                                case LSUOpType::sw:{
                                    // priv.mem->write(instInfo.rs1_value + instInfo.imm,(void*)&instInfo.rs2_value,4);
                                    rv_exc_code exp_id = mem_write(instInfo.rs1_value + instInfo.imm,4,(unsigned char*)&instInfo.rs2_value);
                                    if(exp_id == exc_custom_ok){
                                        break;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exp_id;
                                        break;
                                    }
                                    break;
                                }
                                case LSUOpType::sd:{
                                    // priv.mem->write(instInfo.rs1_value + instInfo.imm,(void*)&instInfo.rs2_value,8);
                                    rv_exc_code exp_id = mem_write(instInfo.rs1_value + instInfo.imm,8,(unsigned char*)&instInfo.rs2_value);
                                    if(exp_id == exc_custom_ok){
                                        break;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exp_id;
                                        break;
                                    }
                                    break;
                                }
                                case LSUOpType::lr:{
                                    if(instInfo.rs2_id != 0){
                                        instInfo.has_execp = true;
                                        instInfo.execp_id  = exc_illegal_instr;
                                    }else{
                                        int64_t result;
                                        rv_exc_code exc = priv.va_lr(instInfo.rs1_value,8,(uint8_t*)&result);
                                        if(exc == exc_custom_ok){
                                            instInfo.rd_enable = true;
                                            instInfo.rd_value  = result;
                                        }else{
                                            instInfo.has_execp = true;
                                            instInfo.execp_id  = exc;
                                        }
                                    }
                                    break;
                                }
                                case LSUOpType::lrw:{
                                    if(instInfo.rs2_id != 0){
                                        instInfo.has_execp = true;
                                        instInfo.execp_id  = exc_illegal_instr;
                                    }else{
                                        int32_t result;
                                        rv_exc_code exc = priv.va_lr(instInfo.rs1_value,4,(uint8_t*)&result);
                                        if(exc == exc_custom_ok){
                                            instInfo.rd_enable = true;
                                            instInfo.rd_value  = result;
                                        }else{
                                            instInfo.has_execp = true;
                                            instInfo.execp_id  = exc;
                                        }
                                    }
                                    break;
                                }
                                case LSUOpType::sc:{
                                    bool result;
                                    rv_exc_code exc = priv.va_sc(instInfo.rs1_value,8,(uint8_t*)&instInfo.rs2_value,result);
                                    if(exc == exc_custom_ok){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value  = result;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id  = exc;
                                    }
                                    break;
                                }
                                case LSUOpType::scw:{
                                    bool result;
                                    rv_exc_code exc = priv.va_sc(instInfo.rs1_value,4,(uint8_t*)&instInfo.rs2_value,result);
                                    if(exc == exc_custom_ok){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value  = result;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id  = exc;
                                    }
                                    break;
                                }
                                case LSUOpType::amoswap: case LSUOpType::amoadd: case LSUOpType::amoxor: case LSUOpType::amoand: case LSUOpType::amoor:
                                case LSUOpType::amomin: case LSUOpType::amomax: case LSUOpType::amominu: case LSUOpType::amomaxu:{
                                    int64_t result;
                                    rv_exc_code exc = priv.va_amo(instInfo.rs1_value,8,instInfo.fuOpType.lsuOp,instInfo.rs2_value,result);
                                    if(exc == exc_custom_ok){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = result;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exc;
                                    }
                                    break;
                                }
                                case LSUOpType::amoswapw: case LSUOpType::amoaddw: case LSUOpType::amoxorw: case LSUOpType::amoandw: case LSUOpType::amoorw:
                                case LSUOpType::amominw: case LSUOpType::amomaxw: case LSUOpType::amominuw: case LSUOpType::amomaxuw:{
                                    int64_t result;
                                    rv_exc_code exc = priv.va_amo(instInfo.rs1_value,4,instInfo.fuOpType.lsuOp,instInfo.rs2_value,result);
                                    if(exc == exc_custom_ok){
                                        instInfo.rd_enable = true;
                                        instInfo.rd_value = result;
                                    }else{
                                        instInfo.has_execp = true;
                                        instInfo.execp_id = exc;
                                    }
                                    break;
                                }

                                default:
                                    assert(false);
                                    break;
                            }
                            break;
                        }
                        case FuType::mdu:{
                            instInfo.rd_enable = true;
                            int64_t src1 = instInfo.rs1_value;
                            int64_t src2 = instInfo.rs2_valid ? instInfo.rs2_value : instInfo.imm;
                            instInfo.rd_value = mdu_exec(src1,src2,instInfo.fuOpType.mduOp);
                            break;
                        }
                        case FuType::mou:{
                            /* fence fencei sfence_vma*/
                            switch(instInfo.fuOpType.mouOp){
                                case MOUOpType::fence:
                                    bru_feedback_pack_t.enable = true;
                                    bru_feedback_pack_t.jump = false;
                                    bru_feedback_pack_t.next_pc = 0;
                                    break;
                                case MOUOpType::fencei:
                                    priv.fence_i();
                                    bru_feedback_pack_t.enable = true;
                                    bru_feedback_pack_t.jump = false;
                                    bru_feedback_pack_t.next_pc = 0;
                                    break;
                                case MOUOpType::sfence_vma:
                                    if(!priv.sfence_vma(instInfo.rs1_value,instInfo.rs2_value & 0xffff)){
                                        instInfo.has_execp = true;
                                        instInfo.execp_id  = exc_illegal_instr;
                                        bru_feedback_pack_t.enable = true;
                                        bru_feedback_pack_t.jump = false;
                                        bru_feedback_pack_t.next_pc = 0;
                                    }else{
                                        bru_feedback_pack_t.enable = true;
                                        bru_feedback_pack_t.jump   = false;
                                        bru_feedback_pack_t.next_pc= 0;
                                    }
                                    break;
                            }
                            break;
                        }
                    }
                    
                    if(!instInfo.valid){
                        instInfo.has_execp = true;
                        instInfo.execp_id = exc_illegal_instr;
                    }
                }
                this->execute_wb_fifo->push(instInfo);
            }
        }else{
            this->execute_wb_fifo->flush();
        }

        return bru_feedback_pack_t;
    }

    rv_exc_code execute::mem_read(uint64_t start_addr,uint64_t size,uint8_t *buffer){
        if(start_addr % size != 0)
            return exc_load_misalign;
        return priv.va_read(start_addr,size,buffer);
    }

    rv_exc_code execute::mem_write(uint64_t start_addr,uint64_t size,uint8_t *buffer){
        if(start_addr % size != 0)
            return exc_store_misalign;
        return priv.va_write(start_addr,size,buffer);
    }
}
