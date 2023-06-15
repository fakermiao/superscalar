/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:42:08
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 19:39:11
 * @Description: 
 */
/*
 * @Author: 苗金标
 * @Date: 2023-04-07 15:14:09
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-24 17:17:31
 * @Description: 
 */
#include "wb.h"
namespace Supercore{

    void wb::set_rob_item(component::rob_item& rob_item_t,const instStr& inst){
        rob_item_t.complete         = true;
        rob_item_t.has_execp        = inst.has_execp;
        rob_item_t.execp_id         = inst.execp_id;
        rob_item_t.execp_value      = inst.execp_value;
        rob_item_t.rd_valid         = inst.rd_id;
        rob_item_t.rd_id            = inst.rd_phy;
        rob_item_t.rd_value         = inst.rd_value;
        
        rob_item_t.predicted        = inst.predicted;
        rob_item_t.predicted_jump   = inst.predicted_jump;
        rob_item_t.predicted_next_pc= inst.predicted_next_pc;
        rob_item_t.checkpoint_id_valid =  inst.checkpoint_id_valid;
        rob_item_t.checkpoint_id    = inst.checkpoint_id;
        rob_item_t.bru_jump         = inst.bru_jump;
        rob_item_t.bru_next_pc      = inst.bru_next_pc;
    }

    void wb::interrupt_handler(){
        uint32_t tail_id;
        uint32_t head_id;
        assert(rob->get_front_id(&head_id));
        assert(rob->get_tail_id(&tail_id));

        do{
            auto item = rob->get_item(tail_id);
            if(item.OPreg_v){
                rat->restore_map_sync(item.Preg,item.OPreg);
                phy_regfile->write_sync(item.Preg,0,false);
                phy_regfile->set_valid_sync(item.OPreg,true);
            }
        }while((tail_id != head_id) && (rob->get_prev_id(tail_id,&tail_id)));
        printf("interrupt handler\n");
        rob->flush();
        cp->flush();
        store_buffer->flush();
    } 

    void wb::execption_handler(){
        uint32_t tail_id;
        uint32_t head_id;
        assert(rob->get_front_id(&head_id));
        assert(rob->get_tail_id(&tail_id));

        do{
            auto item = rob->get_item(tail_id);
            if(item.OPreg_v){
                rat->restore_map_sync(item.Preg,item.OPreg);
                phy_regfile->write_sync(item.Preg,0,false);
                phy_regfile->set_valid_sync(item.OPreg,true);
            }
        }while((tail_id != head_id) && (rob->get_prev_id(tail_id,&tail_id)));
        printf("execption handler\n");
        rob->flush();
        cp->flush();
        store_buffer->flush();
    }

    wb_feedback_pack wb::evaluate(){
        wb_feedback_pack feed_pack;
        memset(&feed_pack,0,sizeof(feed_pack));

        for(uint32_t i = 0;i < ALU_UNIT_NUM;i++){
            while(!alu_wb_fifo[i]->is_empty()){
                instStr instInfo;
                alu_wb_fifo[i]->pop(&instInfo);
                auto rob_item_t = rob->get_item(instInfo.rob_id);
                this->set_rob_item(rob_item_t,instInfo);
                rob->set_item(instInfo.rob_id,rob_item_t);
            }
        }
        for(uint32_t i = 0;i < BRU_UNIT_NUM;i++){
            while(!bru_wb_fifo[i]->is_empty()){
                instStr instInfo;
                bru_wb_fifo[i]->pop(&instInfo);
                auto rob_item_t = rob->get_item(instInfo.rob_id);
                this->set_rob_item(rob_item_t,instInfo);
                rob->set_item(instInfo.rob_id,rob_item_t);
            }
        }
        for(uint32_t i = 0;i < CSR_UNIT_NUM;i++){
            while(!csr_wb_fifo[i]->is_empty()){
                instStr instInfo;
                csr_wb_fifo[i]->pop(&instInfo);
                auto rob_item_t = rob->get_item(instInfo.rob_id);
                this->set_rob_item(rob_item_t,instInfo);
                rob->set_item(instInfo.rob_id,rob_item_t);
            }
        }
        for(uint32_t i = 0;i < LSU_UNIT_NUM;i++){
            while(!lsu_wb_fifo[i]->is_empty()){
                instStr instInfo;
                lsu_wb_fifo[i]->pop(&instInfo);
                auto rob_item_t = rob->get_item(instInfo.rob_id);
                this->set_rob_item(rob_item_t,instInfo);
                rob->set_item(instInfo.rob_id,rob_item_t);
            }
        }
        for(uint32_t i = 0;i < MDU_UNIT_NUM;i++){
            while(!mdu_wb_fifo[i]->is_empty()){
                instStr instInfo;
                mdu_wb_fifo[i]->pop(&instInfo);
                auto rob_item_t = rob->get_item(instInfo.rob_id);
                this->set_rob_item(rob_item_t,instInfo);
                rob->set_item(instInfo.rob_id,rob_item_t);
            }
        }
        for(uint32_t i = 0;i < MOU_UNIT_NUM;i++){
            while(!mou_wb_fifo[i]->is_empty()){
                instStr instInfo;
                mou_wb_fifo[i]->pop(&instInfo);
                auto rob_item_t = rob->get_item(instInfo.rob_id);
                this->set_rob_item(rob_item_t,instInfo);
                rob->set_item(instInfo.rob_id,rob_item_t);
            }
        }
        component::rob_item rob_item;
        memset(&rob_item,0,sizeof(rob_item));
        uint32_t commit_num = 0;

        uint32_t arch10_feedback = 0;
        uint32_t arch10_feedback_v = false;
        uint64_t arch10_feedback_value = 0;
        while(!rob->is_empty() && commit_num < WB_WIDTH){
            rv_exc_code int_code = priv.check_and_raise_int();
            if(int_code != exc_custom_ok && (!priv.need_trap())){
                feed_pack.enable = true;
                feed_pack.flush  = true;
                feed_pack.next_pc = priv.get_trap_pc();
                feed_pack.inter  = true;
                this->interrupt_handler();
                priv.post_exc();
            }else{
                rob->get_front(&rob_item);
                if(rob_item.complete){
                    if(rob_item.inst_v){
                        if(rob_item.has_execp){
                            if(rob_item.execp_id == exc_breakpoint){
                                csr_cause_def cause(exc_breakpoint);
                                priv.raise_trap(rob_item.pc,cause);
                            }else if(rob_item.execp_id == exc_ecall_from_u){
                                uint32_t phy_17 = 0,phy_10 = 0;
                                if(!rat->get_phy_id(17,&phy_17)){
                                    phy_17 = 0;
                                }
                                if(!rat->get_phy_id(10,&phy_10)){
                                    phy_10 = 0;
                                }
                                if(rv_test && phy_regfile->read_data_valid(phy_17) && (phy_regfile->read_data(phy_17) == 93)){
                                    if(arch10_feedback_v){
                                        if(arch10_feedback_value == 0){
                                            printf("***Test Pass***\n");
                                            exit(0);
                                        }else{
                                            printf("---Test Failed with Value 0x%lx\n",phy_regfile->read_data(phy_10));
                                            exit(0);
                                        }
                                    }
                                    if(phy_regfile->read_data_valid(phy_10) && (phy_regfile->read_data(phy_10) == 0)){
                                        printf("***Test Pass***\n");
                                        exit(0);
                                    }else{
                                        printf("---Test Failed with Value 0x%lx\n",phy_regfile->read_data(phy_10));
                                        exit(0);
                                    }
                                }else{
                                    priv.ecall(rob_item.pc);
                                }
                            }else if(rob_item.execp_id == exc_instr_misalign){
                                csr_cause_def cause(rob_item.execp_id);
                                priv.raise_trap(rob_item.pc,cause,rob_item.execp_value);
                            }else if(rob_item.execp_id == exc_illegal_instr){
                                csr_cause_def cause(rob_item.execp_id);
                                priv.raise_trap(rob_item.pc,cause,rob_item.inst);
                            }else{
                                csr_cause_def cause(rob_item.execp_id);
                                priv.raise_trap(rob_item.pc,cause,rob_item.execp_value);
                            }
                        }
                        if(rob_item.rd_valid){
                            feed_pack.wb_channel[commit_num].rd_enable = true;
                            feed_pack.wb_channel[commit_num].rd_id     = rob_item.rd_id;
                            feed_pack.wb_channel[commit_num].rd_value  = rob_item.rd_value;
                            phy_regfile->write_sync(rob_item.Preg,rob_item.rd_value,true);
                            
                            {
                                //for riscv test
                                if(!rat->get_phy_id(10,&arch10_feedback))
                                    arch10_feedback = 0;
                                if(arch10_feedback == rob_item.Preg){
                                    arch10_feedback_v = true;
                                    arch10_feedback_value = rob_item.rd_value;
                                }
                            }

                            //for debug
                            uint32_t arch_id = rat->get_arch_id(rob_item.Preg);
                            cpu.gpr[arch_id] = rob_item.rd_value;

                            if(rob_item.OPreg_v){
                                phy_regfile->write_sync(rob_item.OPreg,0,false);
                                rat->release_map_sync(rob_item.OPreg);
                            }
                        }
                        if(priv.need_trap()){
                            this->execption_handler();
                            feed_pack.flush = true;
                            feed_pack.enable = true;
                            feed_pack.next_pc = priv.get_trap_pc();
                        }
                        if(!rob_item.has_execp){
                            if(rob_item.fu_type == FuType::lsu){
                                switch(rob_item.fuOpType.lsuOp){
                                    case LSUOpType::lb: case LSUOpType::lh: case LSUOpType::lw: case LSUOpType::ld: case LSUOpType::lbu: 
                                    case LSUOpType::lhu: case LSUOpType::lwu: case LSUOpType::lr: case LSUOpType::lrw:
                                        break;
                                    case LSUOpType::sb: case LSUOpType::sh: case LSUOpType::sw: case LSUOpType::sd:
                                    case LSUOpType::sc: case LSUOpType::scw: case LSUOpType::amoswap: case LSUOpType::amoadd:
                                    case LSUOpType::amoxor: case LSUOpType::amoand: case LSUOpType::amoor: case LSUOpType::amomin:
                                    case LSUOpType::amomax: case LSUOpType::amominu: case LSUOpType::amomaxu: case LSUOpType::amoswapw:
                                    case LSUOpType::amoaddw: case LSUOpType::amoxorw: case LSUOpType::amoandw: case LSUOpType::amoorw:
                                    case LSUOpType::amominw: case LSUOpType::amomaxw: case LSUOpType::amominuw: case LSUOpType::amomaxuw:{
                                        component::store_buffer_item store_item;
                                        // memset(&store_item,0,sizeof(store_item));
                                        store_buffer->print();
                                        store_buffer->pop(&store_item);
                                        assert(store_item.enable);
                                        // printf("write pc:%lx,addr:%lx,value:%ld\n",store_item.pc,store_item.addr,store_item.data);
                                        priv.va_write(store_item.addr,store_item.size,(unsigned char*)&store_item.data);
                                        break;
                                    }
                                    default:
                                        assert(0);
                                }
                            }else if(rob_item.fu_type == FuType::bru){
                                if(rob_item.predicted){   
                                    if((rob_item.bru_jump == rob_item.predicted_jump) && ((rob_item.bru_next_pc == rob_item.predicted_next_pc) ||
                                        (!rob_item.predicted_jump))){
                                            // printf("branch predicted hit\n");
                                        bp->update_prediction(rob_item.pc,rob_item.inst,rob_item.bru_jump,rob_item.bru_next_pc,true,0);
                                        cp->pop();
                                    }else if(rob_item.checkpoint_id_valid){
                                        // printf("branch predicted miss\n");
                                        auto cp_t = cp->get_item(rob_item.checkpoint_id);
                                        bp->update_prediction(rob_item.pc,rob_item.inst,rob_item.bru_jump,rob_item.bru_next_pc,false,cp_t.global_history);
                                        if(rob_item.OPreg_v){
                                            // printf("hello:%d\n",rob_item.OPreg);
                                            rat->cp_release_map(cp_t,rob_item.OPreg);
                                        }
                                        uint32_t _cnt = 0;
                                        for(uint32_t i = 0;i < PHY_REG_NUM;i++){
                                            if(!rat->cp_get_visible(cp_t,i)){
                                                rat->cp_set_valid(cp_t,i,false);
                                                phy_regfile->cp_set_data_valid(cp_t,i,false);
                                            }else{
                                                phy_regfile->cp_set_data_valid(cp_t,i,true);
                                                _cnt++;
                                            }
                                        }
                                        assert(_cnt == 32);//32个寄存器，0号寄存器始终映射为0

                                        
                                        rat->restore_sync(cp_t);
                                        phy_regfile->restore_sync(cp_t);
                                        cp->flush();
                                        // rat->flush();
                                        rob->flush();
                                        store_buffer->flush();
                                        feed_pack.enable = true;
                                        feed_pack.flush = true;
                                        feed_pack.bru_flush = true;
                                        feed_pack.bru_next_pc = rob_item.bru_jump ? rob_item.bru_next_pc : (rob_item.pc + 4);
                                    }else{
                                        assert(false);
                                    }
                                }else{
                                    cp->flush();
                                    // rat->flush();
                                    rob->flush();
                                    store_buffer->flush();
                                    feed_pack.enable = true;
                                    feed_pack.flush  = true;
                                    feed_pack.bru_flush = true;
                                    feed_pack.bru_next_pc = rob_item.bru_jump ? rob_item.bru_next_pc : (rob_item.pc + 4);
                                }
                            }else if((rob_item.fu_type == FuType::csr) || (rob_item.fu_type == FuType::mou)){
                                feed_pack.flush = true;
                                feed_pack.enable = true;
                                feed_pack.next_pc = rob_item.pc + 4;
                            }
                        }
                            
                        priv.post_exc();
                    }else{
                        csr_cause_def cause(rob_item.execp_id);
                        uint64_t tval = (rob_item.execp_id == exc_illegal_instr) ? (rob_item.inst) : (rob_item.pc);
                        priv.raise_trap(rob_item.pc,cause,tval);
                        feed_pack.flush = true;
                        feed_pack.enable = true;
                        feed_pack.next_pc = priv.get_trap_pc();
                        priv.post_exc();
                    }
                    this->wb_p(commit_num,rob_item);
                    rob->pop(&rob_item);
                    commit_num++;
                }
                else{
                    break;
                }
            }
        }
        return feed_pack;
    }
}
