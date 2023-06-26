/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:40:44
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 17:20:15
 * @Description: 
 */
#include "fetch.h"
namespace Supercore{
    void fetch::evaluate(bru_feedback_pack bru_feedback_pack_t,wb_feedback_pack wb_feedback_pack_t){
        if(!(wb_feedback_pack_t.enable && wb_feedback_pack_t.flush))
        {
            if(jump_wait){
                if(bru_feedback_pack_t.enable){
                    this->jump_wait = false;
                    if(bru_feedback_pack_t.jump){
                        this->pc = bru_feedback_pack_t.next_pc;
                    }
                }
            }else if(!fetch_decode_fifo->is_full()){
                if(pc % 4){
                    this->jump_wait = true;
                    fetch_decode_pack send_pack;
                    memset(&send_pack,0,sizeof(send_pack));
                    instStr fetch_info;
                    fetch_info.enable = true;
                    fetch_info.pc     = this->pc;
                    fetch_info.inst   = 0;
                    fetch_info.valid  = false;
                    fetch_info.has_execp = true;
                    fetch_info.execp_id  = exc_instr_misalign;
                    fetch_info.debug_pc = this->pc + 4;

                    send_pack.fetch_decode[0] = fetch_info;
                    this->fetch_decode_fifo->push(send_pack);
                }else{
                    fetch_decode_pack send_pack;
                    memset(&send_pack,0,sizeof(send_pack));
                    //一个周期内取到的指令应该属于同一个cacheline
                    uint32_t offset = pc % L1I_SZLINE;
                    uint32_t len    = std::min(FETCH_WIDTH,(L1D_SZLINE - offset) / 4);
                    
                    for(uint32_t pos = 0;pos < len;pos++){
                        instStr fetch_info;
                        memset(&fetch_info,0,sizeof(fetch_info));
                        uint32_t inst_v = 0;
                        rv_exc_code exp_id = priv.va_if(this->pc,4,(uint8_t*)&inst_v);
                        if(exp_id == exc_custom_ok){
                            fetch_info.valid = true;
                            fetch_info.has_execp = false;
                        }else{
                            fetch_info.has_execp = true;
                            fetch_info.execp_id  = exp_id;
                            fetch_info.valid = false;
                        }

                        fetch_info.enable = true;
                        fetch_info.pc     = this->pc;
                        fetch_info.inst   = inst_v;
                        this->jump_wait   = ((inst_v & 0x7f) == 0x6f) || ((inst_v & 0x7f) == 0x67) || ((inst_v & 0x7f) == 0x63) || 
                                            ((inst_v & 0x7f) == 0x0f) || ((inst_v & 0x7f) == 0x73);
                        bool jump = ((inst_v & 0x7f) == 0x6f) || ((inst_v & 0x7f) == 0x67) || ((inst_v & 0x7f) == 0x63);

                        if(jump){
                            bool jump_result;
                            uint64_t jump_next_pc;
                            if(bp->get_prediction(this->pc,inst_v,&jump_result,&jump_next_pc)){
                                component::checkpoint_t cp_t;
                                bp->save(cp_t,this->pc);
                                bp->update_prediction_guess(this->pc,inst_v,jump_result);
                                fetch_info.checkpoint_id_valid = cp->push(cp_t,&fetch_info.checkpoint_id);
                                
                                if(!fetch_info.checkpoint_id_valid){
                                    //checkpoint buffer 已满
                                    this->jump_wait = true;
                                    this->pc = this->pc + 4;
                                }else{
                                    fetch_info.predicted = true;
                                    fetch_info.predicted_jump = jump_result;
                                    fetch_info.predicted_next_pc = jump_next_pc;
                                    this->jump_wait = false;
                                    this->pc = jump_next_pc;
                                }
                            }else{
                                this->jump_wait = true;
                                this->pc = this->pc + 4;
                            }
                        }else{
                            this->pc = this->pc + 4;
                        }
                        this->fetch_p(pos,fetch_info);
                        send_pack.fetch_decode[pos] = fetch_info;
                        if(this->jump_wait || (fetch_info.predicted && fetch_info.predicted_jump))
                            break;
                    }
                    this->fetch_decode_fifo->push(send_pack);
                }
            }
        }else{
            this->jump_wait = false;
            this->fetch_decode_fifo->flush();
            if(wb_feedback_pack_t.bru_flush){
                this->pc = wb_feedback_pack_t.bru_next_pc;
            }else{
                this->pc = wb_feedback_pack_t.next_pc;
            }
        }

    }
}