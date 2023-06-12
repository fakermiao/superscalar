/*
 * @Author: 苗金标
 * @Date: 2023-06-01 10:20:05
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 14:34:08
 * @Description: 
 */
#include "mou.h"
namespace Supercore{
    execute_channel mou::evaluate(wb_feedback_pack wb_feedback){
        execute_channel exe_channel;
        memset(&exe_channel,0,sizeof(exe_channel));
        
        if(!(wb_feedback.enable && wb_feedback.flush)){
            if(!issue_mou_fifo->is_empty() && !mou_wb_fifo->is_full()){
                instStr instInfo;
                issue_mou_fifo->pop(&instInfo);
                if(instInfo.valid){
                    switch(instInfo.fuOpType.mouOp){
                        case MOUOpType::fence:{
                            break;
                        }
                        case MOUOpType::fencei:{
                            priv.fence_i();
                            break;
                        }
                        case MOUOpType::sfence_vma:{
                            if(!priv.sfence_vma(instInfo.rs1_value,instInfo.rs2_value & 0xffff)){
                                instInfo.has_execp = true;
                                instInfo.execp_id = exc_illegal_instr;
                            }
                            break;
                        }
                        default:
                            assert(0);
                    }
                    this->mou_p(instInfo);
                }
                mou_wb_fifo->push(instInfo);
            }
        }else{
            mou_wb_fifo->flush();
        }

        return exe_channel;
    }
}
