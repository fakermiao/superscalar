/*
 * @Author: 苗金标
 * @Date: 2023-05-25 14:56:05
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-26 09:40:38
 * @Description: 
 */
#pragma once
#include "../common.h"
#include "../component/checkpoint_buffer.h"
#include "../component/rat.h"
#include "../component/rob.h"
#include "decode_rename.h"
#include "rename_issue.h"
#include "wb.h"

namespace core{
    class rename{
        public:
            component::fifo<decode_rename_pack> *decode_rename_fifo;
            component::fifo<rename_issue_pack> *rename_issue_fifo;
            component::rat *rat;
            component::rob *rob;
            component::checkpoint_buffer *cp;
            rename(component::fifo<decode_rename_pack> *decode_rename_fifo,component::fifo<rename_issue_pack> *rename_issue_fifo,component::rat *rat,
                    component::rob *rob,component::checkpoint_buffer *cp) : decode_rename_fifo(decode_rename_fifo),rename_issue_fifo(rename_issue_fifo),
                    rat(rat),rob(rob),cp(cp){}
            void evaluate(wb_feedback_pack wb_feedback_pack_t);
    };
}
