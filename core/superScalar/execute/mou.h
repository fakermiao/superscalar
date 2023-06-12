/*
 * @Author: 苗金标
 * @Date: 2023-05-31 14:27:41
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 14:33:47
 * @Description: 
 */
#pragma once
#include "../../common.h"
#include "../../component/fifo.h"
#include "../execute.h"
#include "../wb.h"
#include "priv.h"
#include "../../color.h"

namespace Supercore{
    class mou{
        public:
            uint32_t num;
            component::fifo<instStr> *issue_mou_fifo;
            component::fifo<instStr> *mou_wb_fifo;
            Priv& priv;

            mou(uint32_t num,component::fifo<instStr> *issue_mou_fifo,component::fifo<instStr> *mou_wb_fifo,Priv& priv) :
            num(num),issue_mou_fifo(issue_mou_fifo),mou_wb_fifo(mou_wb_fifo),priv(priv){}
            execute_channel evaluate(wb_feedback_pack wb_feedback);

            void mou_p(instStr& instInfo){
                printf("%smou%s%d/%ld%s:\tpc:%lx,inst:%x\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst);
                // printf("\t\tbranch_jump:%d,branch_pc:%lx,result:%lx\n",instInfo.bru_jump,instInfo.bru_next_pc,instInfo.rd_value);
            }    
    };
}
