/*
 * @Author: 苗金标
 * @Date: 2023-05-25 17:26:54
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-26 19:31:10
 * @Description: 
 * 重命名阶段主要工作如下：
 *      1.逻辑寄存器重命名（考虑超标量同一周期多条指令的相关性）
 *      2.分发，将指令相关信息放入重排序缓存
 *      3.保存状态，检测指令如果是分支预测指令将rat相关信息保存checkpoint
 * 0613:fix rs2 dependcy bug
 */
#include "rename.h"
namespace Supercore{
    void rename::evaluate(wb_feedback_pack wb_feedback_pack_t){
        if(!(wb_feedback_pack_t.enable && wb_feedback_pack_t.flush)){
            if(!decode_rename_fifo->is_empty() && !rename_issue_fifo->is_full()){
                component::checkpoint_t brs_cp;
                memset(&brs_cp,0,sizeof(brs_cp));
                
                rat->save(brs_cp);

                uint32_t phy_need_cnt = 0;
                uint32_t rob_need_cnt = 0;
                uint32_t free_phy_list[RENAME_WIDTH];

                decode_rename_pack rev_pack;
                decode_rename_fifo->get_front(&rev_pack);
                rename_issue_pack send_pack;
                memset(&send_pack,0,sizeof(send_pack));
                for(uint32_t i = 0;i < RENAME_WIDTH; i++){
                    memcpy(&send_pack.rename_issue[i],&rev_pack.decode_issue[i],sizeof(send_pack.rename_issue[i]));
                    //valid表示是否有异常，因此对于valid为false的指令也需要rob表项以便精确异常
                    if(rev_pack.decode_issue[i].enable){
                        rob_need_cnt++;
                    }
                    if(rev_pack.decode_issue[i].enable && rev_pack.decode_issue[i].rd_valid && (rev_pack.decode_issue[i].rd_id != 0))//
                        phy_need_cnt++;
                }

                if((rat->get_free_phy_id(phy_need_cnt,free_phy_list) >= phy_need_cnt) && (rob->get_free_space() >= rob_need_cnt)){
                    component::rob_item rob_item_t[RENAME_WIDTH];
                    //map from rat、rob
                    for(uint32_t i = 0,j = 0;i < RENAME_WIDTH; i++){
                        memset(&rob_item_t[i],0,sizeof(rob_item_t[i]));
                        if(rev_pack.decode_issue[i].enable){
                            rob_item_t[i].complete = false;
                            rob_item_t[i].Areg     = rev_pack.decode_issue[i].rd_id;
                            rob_item_t[i].pc       = rev_pack.decode_issue[i].pc;
                            rob_item_t[i].inst     = rev_pack.decode_issue[i].inst;
                            rob_item_t[i].inst_v   = rev_pack.decode_issue[i].valid;
                            rob_item_t[i].has_execp= rev_pack.decode_issue[i].has_execp;
                            rob_item_t[i].execp_id = rev_pack.decode_issue[i].execp_id;
                            rob_item_t[i].fu_type  = rev_pack.decode_issue[i].fuType;
                            memcpy(&rob_item_t[i].fuOpType,&rev_pack.decode_issue[i].fuOpType,sizeof(rob_item_t[i].fuOpType));

                            if(rev_pack.decode_issue[i].rd_valid && (rev_pack.decode_issue[i].rd_id != 0)){
                                rob_item_t[i].OPreg_v = rat->get_phy_id(rob_item_t[i].Areg,&rob_item_t[i].OPreg);
                                rob_item_t[i].Preg = free_phy_list[j++];
                                send_pack.rename_issue[i].rd_phy = rob_item_t[i].Preg;
                                //have bugs
                                rat->set_map_sync(rob_item_t[i].Areg,rob_item_t[i].Preg);
                                rat->cp_set_map(brs_cp,rob_item_t[i].Areg,rob_item_t[i].Preg);//更新最新映射关系，set_map_sync有延迟
                            }else{
                                rob_item_t[i].OPreg_v   = false;
                                rob_item_t[i].OPreg     = 0;
                                rob_item_t[i].Preg      = 0; 
                                send_pack.rename_issue[i].rd_phy = 0;
                            }

                            if(rev_pack.decode_issue[i].valid && rev_pack.decode_issue[i].predicted && rev_pack.decode_issue[i].checkpoint_id_valid){
                                component::checkpoint_t cp_t,cp_s;
                                brs_cp.clone(cp_t);
                                cp_s = cp->get_item(rev_pack.decode_issue[i].checkpoint_id);
                                cp_t.global_history = cp_s.global_history;
                                cp_t.local_history  = cp_s.local_history;
                                cp->set_item_sync(rev_pack.decode_issue[i].checkpoint_id,cp_t);
                            }
                        }
                    }
                    //find source register map
                    for(uint32_t i = 0;i < RENAME_WIDTH;i++){
                        if(rev_pack.decode_issue[i].enable){
                            if(rev_pack.decode_issue[i].rs1_valid && (rev_pack.decode_issue[i].rs1_id != 0)){
                                assert(rat->get_phy_id(rev_pack.decode_issue[i].rs1_id,&send_pack.rename_issue[i].rs1_phy));
                            }
                            if(rev_pack.decode_issue[i].rs2_valid && (rev_pack.decode_issue[i].rs2_id != 0)){
                                assert(rat->get_phy_id(rev_pack.decode_issue[i].rs2_id,&send_pack.rename_issue[i].rs2_phy));
                            }
                        }
                    }

                    //find dependence and solve it
                    //dest register dependency
                    for(uint32_t i = 1;i < RENAME_WIDTH;i++){
                        if(rev_pack.decode_issue[i].enable){
                            if(rev_pack.decode_issue[i].rd_valid && (rev_pack.decode_issue[i].rd_id != 0)){
                                for(uint32_t j = 0;j < i;j++){
                                    if(rev_pack.decode_issue[j].rd_valid && (rev_pack.decode_issue[j].rd_id != 0)){
                                        if(rev_pack.decode_issue[i].rd_id == rev_pack.decode_issue[j].rd_id)
                                            rob_item_t[i].OPreg = rob_item_t[j].Preg;
                                    }
                                }
                            }
                        }
                    }

                    //souce register dependency
                    for(uint32_t i = 1;i < RENAME_WIDTH;i++){
                        if(rev_pack.decode_issue[i].enable){
                            for(uint32_t j = 0;j < i;j++){
                                if(rev_pack.decode_issue[j].enable && rev_pack.decode_issue[j].rd_valid){
                                    if(rev_pack.decode_issue[i].rs1_valid && (rev_pack.decode_issue[i].rs1_id != 0)
                                        && (rev_pack.decode_issue[i].rs1_id == rev_pack.decode_issue[j].rd_id)){
                                        send_pack.rename_issue[i].rs1_phy = rob_item_t[j].Preg;
                                    }

                                    if(rev_pack.decode_issue[i].rs2_valid && (rev_pack.decode_issue[i].rs2_id != 0)
                                        && (rev_pack.decode_issue[i].rs2_id == rev_pack.decode_issue[j].rd_id)){
                                            send_pack.rename_issue[i].rs2_phy = rob_item_t[j].Preg;
                                    }
                                }
                            }
                        }
                    }

                    for(uint32_t i = 0;i < RENAME_WIDTH;i++){
                        if(rev_pack.decode_issue[i].enable){
                            //write rob
                            assert(rob->push(rob_item_t[i],&send_pack.rename_issue[i].rob_id));
                            // if(cpu.cycle > 326630)
                            //     printf("rename cycle:%ld, enable:%d\n",cpu.cycle,rob->is_empty());
                            this->rename_p(i,send_pack.rename_issue[i]);
                        }
                    }


                    rename_issue_fifo->push(send_pack);
                    decode_rename_fifo->pop(&rev_pack);

                }else{
                    if(rat->get_free_phy_id(phy_need_cnt,free_phy_list) < phy_need_cnt){
                        //rat space not enough
                    }else if(rob->get_free_space() < rob_need_cnt){
                        //rob space not enough
                    }
                }
            }
        }else{
            rename_issue_fifo->flush();
        }
    }
}