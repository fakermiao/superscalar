/*
 * @Author: 苗金标
 * @Date: 2023-04-03 20:43:00
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-28 20:09:37
 * @Description: 
 */
#pragma once
#include "../component/fifo.h"
#include "decode_issue.h"
#include "../common.h"
#include "../config.h"
#include "wb.h"
#include "execute/execute.h"

namespace core{
    class issue{
        public:
            issue(component::fifo<decode_issue_pack> *decode_issue_fifo,component::fifo<instStr> *issue_execute_fifo);
                // decode_issue_fifo(decode_issue_fifo),issue_execute_fifo(issue_execute_fifo),issue_q(component::fifo<instStr>(ISSUE_QUEUE_SIZE))
                // {}
            component::fifo<instStr> issue_q;
            component::fifo<decode_issue_pack> *decode_issue_fifo;
            component::fifo<instStr> *issue_execute_fifo;
            virtual void evaluate(wb_feedback_pack wb_feedback_pack_t,exe_feedback_t exe_feedback);
    };
}

