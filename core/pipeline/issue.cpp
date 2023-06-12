/*
 * @Author: 苗金标
 * @Date: 2023-04-05 11:23:49
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-28 20:05:40
 * @Description: 
 */
#include "issue.h"

namespace core{
    issue::issue(component::fifo<decode_issue_pack> *decode_issue_fifo,component::fifo<instStr> *issue_execute_fifo):issue_q(component::fifo<instStr>(ISSUE_QUEUE_SIZE)){
        this->decode_issue_fifo = decode_issue_fifo;
        this->issue_execute_fifo = issue_execute_fifo;
    }
    void issue::evaluate(wb_feedback_pack wb_feedback_pack_t,exe_feedback_t exe_feedback){
        /*目前只考虑果壳的顺序单发情况，多发的情况考虑后面再处理（涉及到一个周期内译码的指令不能全部放入发射队列要暂停的情况）*/
        if(!(wb_feedback_pack_t.enable && wb_feedback_pack_t.flush)){
            if(!issue_q.is_full() && !decode_issue_fifo->is_empty()){
                decode_issue_pack rev_pack;
                this->decode_issue_fifo->pop(&rev_pack);
                this->issue_q.push(rev_pack.decode_issue[0]);
            }
            if(wb_feedback_pack_t.rd_enable){
                uint32_t rd_id = wb_feedback_pack_t.rd_id;
                cpu.gpr[rd_id] = rd_id ? wb_feedback_pack_t.rd_value : 0;
                cpu.gpr_v[rd_id] = true;
            }
            if(!issue_q.is_empty()){
                instStr instInfo;
                if(issue_q.get_front(&instInfo)){
                    //  printf("ISSUE\tinst:%x,valid:%d,rs1_id:%d,rs2_id:%d,rs1:%lx,rs2:%lx,has_exp:%d,exp_id:%d\n",instInfo.inst,instInfo.valid,instInfo.rs1_id,instInfo.rs2_id,
                    //         instInfo.rs1_value,instInfo.rs2_value,instInfo.has_execp,instInfo.execp_id);
                    if(instInfo.rs1_valid){
                        if(exe_feedback.rd_valid && (exe_feedback.rd_id == instInfo.rs1_id)){
                            instInfo.rs1_enable = false;
                        }   
                        else if(cpu.gpr_v[instInfo.rs1_id]){
                            instInfo.rs1_enable = true;
                            instInfo.rs1_value = cpu.gpr[instInfo.rs1_id];
                        }
                    }else{
                        instInfo.rs1_enable = true;
                        instInfo.rs1_value = 0;
                    }
                    if(instInfo.rs2_valid){
                        if(exe_feedback.rd_valid && (exe_feedback.rd_id == instInfo.rs2_id)){
                            instInfo.rs2_enable = false;
                        }
                        else if(cpu.gpr_v[instInfo.rs2_id]){
                            instInfo.rs2_enable = true;
                            instInfo.rs2_value = cpu.gpr[instInfo.rs2_id];
                        }
                    }else{
                        instInfo.rs2_enable = true;
                        instInfo.rs2_value  = 0;
                    }
                    if(instInfo.rs1_enable && instInfo.rs2_enable){
                        // printf("ISSUE\tinst:%x,valid:%d,rs1_id:%d,rs2_id%d,rs1:%lx,rs2:%lx,has_exp:%d,exp_id:%d\n",instInfo.inst,instInfo.valid,instInfo.rs1_id,instInfo.rs2_id,
                        //     instInfo.rs1_value,instInfo.rs2_value,instInfo.has_execp,instInfo.execp_id);
                        issue_execute_fifo->push(instInfo);
                        issue_q.pop(&instInfo);
                        if(instInfo.rd_valid && instInfo.rd_id){
                            cpu.gpr_v[instInfo.rd_id] = false;
                        }
                    }
                }else{
                    assert(false);
                }
            }
        }else{
            for(int i = 0;i < 32;i++){
                cpu.gpr_v[i] = true;
            }
            if(wb_feedback_pack_t.rd_enable){
                uint32_t rd_id = wb_feedback_pack_t.rd_id;
                cpu.gpr[rd_id] = rd_id ? wb_feedback_pack_t.rd_value : 0;
                cpu.gpr_v[rd_id] = true;
            }
            issue_q.flush();
            issue_execute_fifo->flush();
        }
    }
}