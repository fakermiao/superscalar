/*
 * @Author: 苗金标
 * @Date: 2023-04-06 10:17:37
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 14:33:09
 * @Description: 
 */
#pragma once

#include "../../common.h"
#include "../../component/fifo.h"
#include "../execute.h"
#include "../wb.h"
#include <climits>
#include "../../color.h"
    // mul,mulh,mulhsu,mulhu,div,divu,rem,remu,
    // mulw,divw,divuw,remw,remuw 

namespace Supercore{
    class mdu{
        public:
            uint32_t num;
            component::fifo<instStr> *issue_mdu_fifo;
            component::fifo<instStr> *mdu_wb_fifo;
            mdu(uint32_t num,component::fifo<instStr> *issue_mdu_fifo,component::fifo<instStr> *mdu_wb_fifo) : 
                num(num),issue_mdu_fifo(issue_mdu_fifo),mdu_wb_fifo(mdu_wb_fifo){}
            execute_channel evaluate(wb_feedback_pack wb_feedback);

            void mdu_p(instStr& instInfo){
                if(cpu.cycle >= DIFFTEST_CYC)
                    printf("%smdu%s%d/%ld%s:\t\tpc:%lx,inst:%x\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst);
            }
    };
}





























    