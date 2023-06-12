/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:40:49
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 17:39:12
 * @Description: 
 */
/*
 * @Author: 苗金标
 * @Date: 2023-03-31 20:40:22
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-24 10:43:01
 * @Description: 
 */
#pragma once
#include <queue>
#include "../common.h"
#include "../component/memory.h"
#include "../component/fifo.h"
#include "fetch_decode.h"
#include "execute/bru.h"
#include "wb.h"
#include "execute/priv.h"
#include "../component/bp/baseBp.h"
#include "../component/checkpoint_buffer.h"
#include "../cache/cache_config.h"

#include "../color.h"

namespace Supercore{
    class fetch{
        public:
            uint64_t pc;
            uint64_t npc;
            Priv& priv;
            component::memory*  mem;
            bool     jump_wait;
            component::fifo<fetch_decode_pack> *fetch_decode_fifo;
            component::baseBp *bp;
            component::checkpoint_buffer *cp;

            fetch(uint64_t npc,Priv& priv,component::memory* mem,component::fifo<fetch_decode_pack> *fetch_decode_fifo,component::baseBp *bp,component::checkpoint_buffer *cp):
                npc(npc),priv(priv),mem(mem),fetch_decode_fifo(fetch_decode_fifo),bp(bp),cp(cp){
                    this->pc = npc;
                    this->jump_wait = false;

            }
            void evaluate(bru_feedback_pack bru_feedback_pack_t,wb_feedback_pack wb_feedback_pack_t);

            void fetch_p(uint32_t num,instStr& instInfo){
                printf("%sfetch%s%d/%ld%s:\tpc:%lx,inst:%08x\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst);
                printf("\t\tpredicted:%d,pred_jump:%d,pred_pc:%lx\n",instInfo.predicted,instInfo.predicted_jump,instInfo.predicted_next_pc);
            }
    };
}
