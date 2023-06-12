
/*
 * @Author: 苗金标
 * @Date: 2023-04-05 11:23:49
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 20:57:41
 * @Description: 
 */
#include "issue.h"

namespace Supercore{
    void issue::evaluate(wb_feedback_pack wb_feedback_pack_t,exe_feedback_t exe_feedback){
        /*目前只考虑果壳的顺序单发情况，多发的情况考虑后面再处理（涉及到一个周期内译码的指令不能全部放入发射队列要暂停的情况）*/
        this->debug_issue_num = 0;
        if(!(wb_feedback_pack_t.enable && wb_feedback_pack_t.flush)){

            //setp1.确定可以发射的指令,放入对应功能单元的fifo
            std::vector<uint32_t> issued_list;
            if(!issue_q.is_empty()){
                uint32_t id = 0;
                issue_q.get_front_id(&id);
                auto first_id = id;

                bool alu_unit_used[ALU_UNIT_NUM];
                bool bru_unit_used[BRU_UNIT_NUM];
                bool csr_unit_used[CSR_UNIT_NUM];
                bool lsu_unit_used[LSU_UNIT_NUM];
                bool mdu_unit_used[MDU_UNIT_NUM];
                bool mou_unit_used[MOU_UNIT_NUM];
                for(uint32_t i = 0;i < ALU_UNIT_NUM;i++){
                    alu_unit_used[i] = issue_alu_fifo[i]->is_full();
                }
                for(uint32_t i = 0;i < BRU_UNIT_NUM;i++){
                    bru_unit_used[i] = issue_bru_fifo[i]->is_full();
                }
                for(uint32_t i = 0;i < CSR_UNIT_NUM;i++){
                    csr_unit_used[i] = issue_csr_fifo[i]->is_full();
                }
                for(uint32_t i = 0;i < LSU_UNIT_NUM;i++){
                    lsu_unit_used[i] = issue_lsu_fifo[i]->is_full();
                }
                for(uint32_t i = 0;i < MDU_UNIT_NUM;i++){
                    mdu_unit_used[i] = issue_mdu_fifo[i]->is_full();
                }
                for(uint32_t i = 0;i < MOU_UNIT_NUM;i++){
                    mou_unit_used[i] = issue_mou_fifo[i]->is_full();
                }
                //遍历发射队列找到可以发射的指令
                do{
                    printf("issue\tid:%d\n",id);
                    instStr cur_inst = issue_q.get_item(id);
                    bool issued = false;
                    bool rs1_ready = false;
                    bool rs2_ready = false;
                    if(cur_inst.rs1_valid){
                        if(cur_inst.rs1_enable)
                            rs1_ready = true;
                    }else{
                        rs1_ready = true;
                        cur_inst.rs1_enable = true;
                        cur_inst.rs1_value  = 0;
                    }
                    if(cur_inst.rs2_valid){
                        if(cur_inst.rs2_enable)
                            rs2_ready = true;
                    }else{
                        rs2_ready = true;
                        cur_inst.rs2_enable = true;
                        cur_inst.rs2_value  = 0;
                    }
                    //处理反馈，可以不需要
                    for(uint32_t i = 0;i < EXECUTE_UNIT_NUM;i++){
                        if(exe_feedback.exe_channel[i].rd_enable){
                            if(!rs1_ready && (cur_inst.rs1_phy == exe_feedback.exe_channel[i].rd_id)){
                                rs1_ready = true;
                                cur_inst.rs1_enable = true;
                                cur_inst.rs1_value = exe_feedback.exe_channel[i].rd_value;
                            }
                            if(!rs2_ready && (cur_inst.rs2_phy == exe_feedback.exe_channel[i].rd_id)){
                                rs2_ready = true;
                                cur_inst.rs2_enable = true;
                                cur_inst.rs2_value = exe_feedback.exe_channel[i].rd_value;
                            }
                        }
                    }
                    for(uint32_t i = 0;i < EXECUTE_UNIT_NUM;i++){
                        if(wb_feedback_pack_t.wb_channel[i].rd_enable){
                            if(!rs1_ready && (cur_inst.rs1_phy == wb_feedback_pack_t.wb_channel[i].rd_id)){
                                rs1_ready = true;
                                cur_inst.rs1_enable = true;
                                cur_inst.rs1_value  = wb_feedback_pack_t.wb_channel[i].rd_value;
                            }
                            if(!rs2_ready && (cur_inst.rs1_phy == wb_feedback_pack_t.wb_channel[i].rd_id)){
                                rs2_ready = true;
                                cur_inst.rs2_enable = true;
                                cur_inst.rs2_value  = wb_feedback_pack_t.wb_channel[i].rd_value;
                            }
                        }
                    }
                    if(rs1_ready && rs2_ready){
                        uint32_t unit_num = 0;
                        component::fifo<instStr> **unit_fifo = NULL;
                        switch(cur_inst.fuType){
                            case FuType::alu:{
                                while((unit_num < ALU_UNIT_NUM) && (alu_unit_used[unit_num])){unit_num++;}
                                issued = unit_num < ALU_UNIT_NUM;
                                alu_unit_used[unit_num] = true;
                                unit_fifo = issue_alu_fifo;
                                break;
                            }
                            case FuType::bru:{
                                while((unit_num < BRU_UNIT_NUM) && (bru_unit_used[unit_num])){unit_num++;}
                                issued = unit_num < BRU_UNIT_NUM;
                                bru_unit_used[unit_num] = true;
                                unit_fifo = issue_bru_fifo;
                                break;
                            }
                            case FuType::csr:{
                                while((unit_num < CSR_UNIT_NUM) && (csr_unit_used[unit_num])){unit_num++;}
                                issued = unit_num < CSR_UNIT_NUM;
                                csr_unit_used[unit_num] = true;
                                unit_fifo = issue_csr_fifo;
                                break;
                            }
                            case FuType::lsu:{
                                while((unit_num < LSU_UNIT_NUM) && (lsu_unit_used[unit_num])){unit_num++;}
                                issued = unit_num < LSU_UNIT_NUM;
                                lsu_unit_used[unit_num] = true;
                                unit_fifo = issue_lsu_fifo;
                                break;
                            }
                            case FuType::mdu:{
                                while((unit_num < MDU_UNIT_NUM) && (mdu_unit_used[unit_num])){unit_num++;}
                                issued = unit_num < MDU_UNIT_NUM;
                                mdu_unit_used[unit_num] = true;
                                unit_fifo = issue_mdu_fifo;
                                break;
                            }
                            case FuType::mou:{
                                while((unit_num < MOU_UNIT_NUM) && (mou_unit_used[unit_num])){unit_num++;}
                                issued = unit_num < MOU_UNIT_NUM;
                                mou_unit_used[unit_num] = true;
                                unit_fifo = issue_mou_fifo;
                                break;
                            }
                        }
                        if(issued){
                            issued_list.push_back(id);
                            unit_fifo[unit_num]->push(cur_inst);
                            this->issue_p(debug_issue_num,cur_inst,unit_num);
                            this->debug_issue_num++;
                        }
                    }
                }while(issue_q.get_next_id(id,&id) && (id != first_id) && (issued_list.size() < ISSUE_WIDTH));
                this->issue_q.compress_sync(issued_list);
            }

            //step2.接收exectue、wb执行返回唤醒issue_q队列的指令
            uint32_t cur_id = 0;
            if(issue_q.get_front_id(&cur_id)){
                auto first_id = cur_id;
                auto modified = false;
                do{
                    auto cur_inst = issue_q.get_item(cur_id);
                    for(uint32_t i = 0;i < EXECUTE_UNIT_NUM;i++){
                        if(exe_feedback.exe_channel[i].rd_enable){
                            if(cur_inst.rs1_valid && !cur_inst.rs1_enable && (cur_inst.rs1_phy == exe_feedback.exe_channel[i].rd_id)){
                                cur_inst.rs1_enable = true;
                                cur_inst.rs1_value  = exe_feedback.exe_channel[i].rd_value;
                                modified = true;
                            }
                            if(cur_inst.rs2_valid && !cur_inst.rs2_enable && (cur_inst.rs2_phy == exe_feedback.exe_channel[i].rd_id)){
                                cur_inst.rs2_enable = true;
                                cur_inst.rs2_value = exe_feedback.exe_channel[i].rd_value;
                                modified = true;
                            }
                        }
                        if(wb_feedback_pack_t.wb_channel[i].rd_enable){
                            if(cur_inst.rs1_valid && cur_inst.rs1_enable && (cur_inst.rs1_phy == wb_feedback_pack_t.wb_channel[i].rd_id)){
                                cur_inst.rs1_enable = true;
                                cur_inst.rs1_value = wb_feedback_pack_t.wb_channel[i].rd_value;
                                modified = true;
                            }
                            if(cur_inst.rs2_valid && cur_inst.rs2_enable && (cur_inst.rs2_phy == wb_feedback_pack_t.wb_channel[i].rd_id)){
                                cur_inst.rs2_enable = true;
                                cur_inst.rs2_value = wb_feedback_pack_t.wb_channel[i].rd_value;
                                modified = true;
                            }
                        }
                        if(modified){
                            issue_q.set_item_sync(cur_id,cur_inst);
                        }
                    }
                }while(issue_q.get_next_id(cur_id,&cur_id) && (cur_id != first_id));
            }
            
            //step3.rename阶段发送的指令放入issue_q队列,目的寄存器对应的物理寄存器设置为false
            if(!rename_issue_fifo->is_empty()){
                rename_issue_pack rev_pack;
                this->rename_issue_fifo->get_front(&rev_pack);
                uint32_t num = 0;
                while(num < RENAME_WIDTH){
                    if(rev_pack.rename_issue[num].enable)
                        num++;
                    else
                        break;
                }
                if(num < issue_q.get_space()){
                    this->rename_issue_fifo->pop(&rev_pack);
                    for(uint32_t i = 0;i < num;i++){
                        if(rev_pack.rename_issue[i].valid && rev_pack.rename_issue[i].predicted && rev_pack.rename_issue[i].checkpoint_id_valid){
                            component::checkpoint_t cp_t = cp->get_item(rev_pack.rename_issue[i].checkpoint_id);
                            phy_regfile->save(cp_t);
                            cp->set_item(rev_pack.rename_issue[i].checkpoint_id,cp_t);
                        }
                        //读取物理寄存器,设置目的物理寄存器堆无效化
                        if(rev_pack.rename_issue[i].rs1_valid){
                            auto rs1_addr = rev_pack.rename_issue[i].rs1_phy;
                            if(phy_regfile->read_data_valid(rs1_addr)){
                                rev_pack.rename_issue[i].rs1_enable = true;
                                rev_pack.rename_issue[i].rs1_value  = phy_regfile->read_data(rs1_addr);
                            }
                        }
                        if(rev_pack.rename_issue[i].rs2_valid){
                            auto rs2_addr = rev_pack.rename_issue[i].rs2_phy;
                            if(phy_regfile->read_data_valid(rs2_addr)){
                                rev_pack.rename_issue[i].rs2_enable = true;
                                rev_pack.rename_issue[i].rs2_value  = phy_regfile->read_data(rs2_addr);
                            }
                        }
                        if(rev_pack.rename_issue[i].rd_valid && (rev_pack.rename_issue[i].rd_id != 0)){
                            phy_regfile->set_valid(rev_pack.rename_issue[i].rd_phy,false);
                        }
                        for(uint32_t j = 0;j < EXECUTE_UNIT_NUM;j++){
                            if(exe_feedback.exe_channel[j].rd_enable){
                                if(rev_pack.rename_issue[i].rs1_valid && !rev_pack.rename_issue[i].rs1_enable && 
                                (rev_pack.rename_issue[i].rs1_phy == exe_feedback.exe_channel[j].rd_id)){
                                    rev_pack.rename_issue[i].rs1_enable = true;
                                    rev_pack.rename_issue[i].rs1_value = exe_feedback.exe_channel[j].rd_value;
                                }
                                if(rev_pack.rename_issue[i].rs2_valid && !rev_pack.rename_issue[i].rs2_enable &&
                                    (rev_pack.rename_issue[i].rs2_phy == exe_feedback.exe_channel[j].rd_id)){
                                        rev_pack.rename_issue[i].rs2_enable = true;
                                        rev_pack.rename_issue[i].rs2_value = exe_feedback.exe_channel[j].rd_value;
                                }
                            }

                            if(wb_feedback_pack_t.wb_channel[j].rd_enable){
                                if(rev_pack.rename_issue[i].rs1_valid && !rev_pack.rename_issue[i].rs1_enable && 
                                (rev_pack.rename_issue[i].rs1_phy == wb_feedback_pack_t.wb_channel[j].rd_id)){
                                    rev_pack.rename_issue[i].rs1_enable = true;
                                    rev_pack.rename_issue[i].rs1_value  = wb_feedback_pack_t.wb_channel[j].rd_value;
                                }
                                if(rev_pack.rename_issue[i].rs2_valid && !rev_pack.rename_issue[i].rs2_enable &&
                                (rev_pack.rename_issue[i].rs2_phy == wb_feedback_pack_t.wb_channel[j].rd_id)){
                                    rev_pack.rename_issue[i].rs1_enable = true;
                                    rev_pack.rename_issue[i].rs2_value  = wb_feedback_pack_t.wb_channel[j].rd_value;
                                }
                            }
                        }
                        issue_q.push(rev_pack.rename_issue[i]);
                    }
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
            for(uint32_t i = 0;i < ALU_UNIT_NUM;i++){
                issue_alu_fifo[i]->flush();
            }
            for(uint32_t i = 0;i < BRU_UNIT_NUM;i++){
                issue_bru_fifo[i]->flush();
            }
            for(uint32_t i = 0;i < CSR_UNIT_NUM;i++){
                issue_csr_fifo[i]->flush();
            }
            for(uint32_t i = 0;i < LSU_UNIT_NUM;i++){
                issue_lsu_fifo[i]->flush();
            }
            for(uint32_t i = 0;i < MDU_UNIT_NUM;i++){
                issue_mdu_fifo[i]->flush();
            }
            for(uint32_t i = 0;i < MOU_UNIT_NUM;i++){
                issue_mou_fifo[i]->flush();
            }
            issue_q.flush();
            // issue_execute_fifo->flush();
        }
    }
}