/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:40:24
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-07 09:34:07
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

namespace Supercore{
    class decode{
        public:
            component::fifo<fetch_decode_pack> *fetch_decode_fifo;
            component::fifo<decode_rename_pack> *decode_rename_fifo;
            decode(component::fifo<fetch_decode_pack> *fetch_decode_fifo,component::fifo<decode_rename_pack> *decode_rename_fifo):
                fetch_decode_fifo(fetch_decode_fifo),decode_rename_fifo(decode_rename_fifo) {}
            virtual void evaluate(wb_feedback_pack wb_feedback_pack_t);

            void decode_p(uint32_t num,instStr& instInfo){
                if(cpu.cycle >= DIFFTEST_CYC){
                    printf("%sdecode%s%d/%ld%s:\tpc:%lx,inst:%x,valid:%d\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst,instInfo.valid);
                    printf("\t\trs1_valid:%d,rs1_id:%d--rs2_valid:%d,rs2_id:%d--rd_valid:%d,rd_id:%d\n",instInfo.rs1_valid,instInfo.rs1_id,instInfo.rs2_valid,
                        instInfo.rs2_id,instInfo.rd_valid,instInfo.rd_id);
                }
            }
    };
}