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
}wb_feedback_pack;
namespace core{
    class wb{
        public:
            component::fifo<instStr> *execute_wb_fifo;
            Priv& priv;
            bool rv_test;
            component::baseBp *bp;
            component::checkpoint_buffer *cp;

            wb(component::fifo<instStr> *execute_wb_fifo,Priv& priv,bool rv_test,component::baseBp *bp,component::checkpoint_buffer *cp):
                execute_wb_fifo(execute_wb_fifo),priv(priv),rv_test(rv_test),bp(bp),cp(cp){}

            virtual wb_feedback_pack evaluate();
    };
}

