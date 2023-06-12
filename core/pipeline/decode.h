/*
 * @Author: 苗金标
 * @Date: 2023-04-03 20:42:31
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-04-11 15:12:17
 * @Description: 
 */
#pragma once
#include "../common.h"
#include "../component/fifo.h"
#include "wb.h"
#include "fetch_decode.h"
#include "decode_issue.h"

// static uint64_t immI(uint32_t i) { return SEXT(BITS(i, 31, 20), 12); }
// static uint64_t immU(uint32_t i) { return SEXT(BITS(i, 31, 12), 20) << 12; }
// static uint64_t immS(uint32_t i) { return SEXT(BITS(i, 31, 25), 7) << 5 | BITS(i, 11, 7); }
// static uint64_t immJ(uint32_t i) { return SEXT(BITS(i, 31, 31), 1) << 20 | BITS(i, 19, 12) << 12 | BITS(i, 20, 20) << 11 | BITS(i, 30, 21) << 1;}
// static uint64_t immB(uint32_t i) { return SEXT(BITS(i, 31, 31), 1) << 12 | BITS(i, 7, 7) << 11 | BITS(i, 30, 25) << 5 | BITS(i, 11, 8) << 1;}

namespace core{
    class decode{
        public:
            component::fifo<fetch_decode_pack> *fetch_decode_fifo;
            component::fifo<decode_issue_pack> *decode_issue_fifo;
            decode(component::fifo<fetch_decode_pack> *fetch_decode_fifo,component::fifo<decode_issue_pack> *decode_issue_fifo):
                fetch_decode_fifo(fetch_decode_fifo),decode_issue_fifo(decode_issue_fifo) {}
            virtual void evaluate(wb_feedback_pack wb_feedback_pack_t);
    };
}