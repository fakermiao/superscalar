/*
 * @Author: 苗金标
 * @Date: 2023-05-31 14:27:20
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 14:36:34
 * @Description: 
 * csr instruction must be executed after all instructions that is before it has been commited
 * 这样做确保csr的执行在正确的路径上而不会因为分支预测失败等原因错误，方便后续指令读取正确的csr值
 */
#pragma once

#include "../../common.h"
#include "../../component/fifo.h"
#include "../execute.h"
#include "../wb.h"
#include "priv.h"
#include "../../color.h"

namespace Supercore{
    
    class csr{
        public:
            uint32_t num;
            component::fifo<instStr> *issue_csr_fifo;
            component::fifo<instStr> *csr_wb_fifo;
            Priv& priv;
            csr(uint32_t num,component::fifo<instStr> *issue_csr_fifo,component::fifo<instStr> *csr_wb_fifo,Priv& priv) : 
            num(num),issue_csr_fifo(issue_csr_fifo),csr_wb_fifo(csr_wb_fifo),priv(priv){}
            execute_channel evaluate(wb_feedback_pack wb_feedback);

            void set_instInfo(instStr& inst,execute_channel& channel,uint64_t result){
                inst.rd_enable = true;
                inst.rd_value = result;
                channel.rd_enable = true;
                channel.rd_id = inst.rd_phy;
                channel.rd_value = result;
            }

            void csr_p(instStr& instInfo){
                printf("%scsr%s%d/%ld%s:\tpc:%lx,inst:%x\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst);
                // printf("\t\tbranch_jump:%d,branch_pc:%lx,result:%lx\n",instInfo.bru_jump,instInfo.bru_next_pc,instInfo.rd_value);
            }
    };
}