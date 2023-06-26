/*
 * @Author: 苗金标
 * @Date: 2023-04-05 19:33:01
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 14:23:18
 * @Description: 
 */
#pragma once

#include "../../common.h"
#include "../../component/fifo.h"
#include "../execute.h"
#include "../wb.h"
#include "../../color.h"


namespace Supercore{
    class alu{
        public:
            uint32_t num;
            component::fifo<instStr> *issue_alu_fifo;
            component::fifo<instStr> *alu_wb_fifo;
            alu(uint32_t num,component::fifo<instStr> *issue_alu_fifo,component::fifo<instStr> *alu_wb_fifo) : num(num),
            issue_alu_fifo(issue_alu_fifo),alu_wb_fifo(alu_wb_fifo){}
            execute_channel evaluate(wb_feedback_pack wb_feedback);

            void alu_p(instStr& instInfo){
                if(cpu.cycle >= DIFFTEST_CYC){
                    printf("%salu%s%d/%ld%s:\t\tpc:%lx,inst:%x\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst);
                    printf("\t\trs1:%lx,rs2:%lx,result:%lx\n",instInfo.rs1_value,instInfo.rs2_value,instInfo.rd_value);
                }
            }
    };
}





