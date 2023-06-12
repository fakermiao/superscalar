/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:42:28
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 15:49:27
 * @Description: 
 */
/*
 * @Author: 苗金标
 * @Date: 2023-04-03 18:58:37
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-24 14:51:58
 * @Description: 
 */
#pragma once
#include "../common.h"
#include "../component/fifo.h"
#include "execute/priv.h"
#include "../component/bp/baseBp.h"
#include "../component/checkpoint_buffer.h"
#include "../component/rat.h"
#include "../component/rob.h"
#include "../component/regfile.h"
#include "../component/store_buffer.h"
#include "super_config.h"
#include "../color.h"

namespace Supercore{
typedef struct channel{
    bool     rd_enable;
    uint32_t rd_id;
    int64_t  rd_value;
}channel;
typedef struct wb_feedback_pack{
    bool     stepOne;
    bool     enable;
    bool     flush;
    bool     inter;
    bool     bru_flush;
    uint64_t bru_next_pc;
    uint64_t next_pc;
    uint64_t debug_pc;
    
    bool     rd_enable;
    uint32_t rd_id;
    int64_t  rd_value;
    channel  wb_channel[EXECUTE_UNIT_NUM];
}wb_feedback_pack;

    class wb{
        public:
            Priv& priv;
            bool rv_test;
            component::baseBp *bp;
            component::checkpoint_buffer *cp;
            component::fifo<instStr> **alu_wb_fifo;
            component::fifo<instStr> **bru_wb_fifo;
            component::fifo<instStr> **csr_wb_fifo;
            component::fifo<instStr> **lsu_wb_fifo;
            component::fifo<instStr> **mdu_wb_fifo;
            component::fifo<instStr> **mou_wb_fifo;

            component::rat *rat;
            component::rob  *rob;
            component::regfile<uint64_t> *phy_regfile;
            component::store_buffer *store_buffer;


            std::queue<instStr> commit_q;

            wb(Priv& priv,bool rv_test,component::baseBp *bp,component::checkpoint_buffer *cp,
                component::fifo<instStr> **alu_wb_fifo,component::fifo<instStr> **bru_wb_fifo,component::fifo<instStr> **csr_wb_fifo,
                component::fifo<instStr> **lsu_wb_fifo,component::fifo<instStr> **mdu_wb_fifo,component::fifo<instStr> **mou_wb_fifo,
                component::rat *rat,component::rob *rob,component::regfile<uint64_t> *phy_regfile,component::store_buffer *store_buffer) :
                priv(priv),rv_test(rv_test),bp(bp),cp(cp),alu_wb_fifo(alu_wb_fifo),bru_wb_fifo(bru_wb_fifo),
                csr_wb_fifo(csr_wb_fifo),lsu_wb_fifo(lsu_wb_fifo),mdu_wb_fifo(mdu_wb_fifo),mou_wb_fifo(mou_wb_fifo),rat(rat),rob(rob),
                phy_regfile(phy_regfile),store_buffer(store_buffer){}

            void interrupt_handler();
            void execption_handler();

            void set_rob_item(component::rob_item& rob_item_t,const instStr& inst);

            virtual wb_feedback_pack evaluate();

            void wb_p(uint32_t num,component::rob_item& item){
                printf("%sWB%s%d/%ld%s:\t\tpc:%lx,inst:%x\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,item.pc,item.inst);
                printf("\t\trd:%d,rd_value:%lx\n",item.Preg,item.rd_value);
                // printf("\t\tpredicted:%d,pred_jump:%d,pred_pc:%lx\n",instInfo.predicted,instInfo.predicted_jump,instInfo.predicted_next_pc);
            }
    };
}

