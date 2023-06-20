/*
 * @Author: 苗金标
 * @Date: 2023-04-05 16:38:52
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-06 15:37:06
 * @Description: 
 */
#pragma once
#include "../../common.h"
#include "../../config.h"
#include "../../component/fifo.h"
#include "../../component/memory.h"
#include "../wb.h"
#include "../execute.h"
#include "alu.h"
#include "bru.h"
#include "mdu.h"
#include "priv.h"
#include "bru.h"
#include "../super_config.h"

namespace Supercore{
struct exe_feedback_t{
    bool     rd_valid;
    uint32_t rd_id;
    uint64_t rd_value;
    Supercore::execute_channel  exe_channel[EXECUTE_UNIT_NUM];
};
    class execute{
        public:
            component::fifo<instStr> *issue_execute_fifo;
            component::fifo<instStr> *execute_wb_fifo;
            Priv& priv;
            bool test;

            execute(component::fifo<instStr> *issue_execute_fifo,component::fifo<instStr> *execute_wb_fifo,Priv& priv):
                issue_execute_fifo(issue_execute_fifo),execute_wb_fifo(execute_wb_fifo),priv(priv),test(true) {}
            bru_feedback_pack evaluate(wb_feedback_pack wb_feedback_pack_t,exe_feedback_t& exe_feedback);
        private:
            rv_exc_code mem_read(uint64_t start_addr,uint64_t size,uint8_t *buffer);
            rv_exc_code mem_write(uint64_t start_addr,uint64_t size,uint8_t *buffer);
    };
}
