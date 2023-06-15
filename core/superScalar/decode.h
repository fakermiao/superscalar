/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:40:24
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-07 09:34:07
 * @Description: 
 */
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
#include "decode_rename.h"

#include "../color.h"

// static uint64_t immI(uint32_t i) { return SEXT(BITS(i, 31, 20), 12); }
// static uint64_t immU(uint32_t i) { return SEXT(BITS(i, 31, 12), 20) << 12; }
// static uint64_t immS(uint32_t i) { return SEXT(BITS(i, 31, 25), 7) << 5 | BITS(i, 11, 7); }
// static uint64_t immJ(uint32_t i) { return SEXT(BITS(i, 31, 31), 1) << 20 | BITS(i, 19, 12) << 12 | BITS(i, 20, 20) << 11 | BITS(i, 30, 21) << 1;}
// static uint64_t immB(uint32_t i) { return SEXT(BITS(i, 31, 31), 1) << 12 | BITS(i, 7, 7) << 11 | BITS(i, 30, 25) << 5 | BITS(i, 11, 8) << 1;}

namespace Supercore{
    class decode{
        public:
            component::fifo<fetch_decode_pack> *fetch_decode_fifo;
            component::fifo<decode_rename_pack> *decode_rename_fifo;
            decode(component::fifo<fetch_decode_pack> *fetch_decode_fifo,component::fifo<decode_rename_pack> *decode_rename_fifo):
                fetch_decode_fifo(fetch_decode_fifo),decode_rename_fifo(decode_rename_fifo) {}
            virtual void evaluate(wb_feedback_pack wb_feedback_pack_t);

            void decode_p(uint32_t num,instStr& instInfo){
                printf("%sdecode%s%d/%ld%s:\tpc:%lx,inst:%x,valid:%d\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst,instInfo.valid);
                printf("\t\trs1_valid:%d,rs1_id:%d--rs2_valid:%d,rs2_id:%d--rd_valid:%d,rd_id:%d\n",instInfo.rs1_valid,instInfo.rs1_id,instInfo.rs2_valid,
                instInfo.rs2_id,instInfo.rd_valid,instInfo.rd_id);
            }
    };
}