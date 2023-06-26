/*
 * @Author: 苗金标
 * @Date: 2023-05-29 16:52:43
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-07 09:42:48
 * @Description: 
 */
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
#include "../color.h"

namespace Supercore{
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

            void rename_p(uint32_t num,instStr& instInfo){
                if(cpu.cycle > DIFFTEST_CYC){
                printf("%srename%s%d/%ld%s:\tpc:%lx,inst:%x,rob_id:%d\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst,instInfo.rob_id);
                printf("\t\trs1_valid:%d,rs1_phy:%d--rs2_valid:%d,rs2_phy:%d--rd_valid:%d,rd_phy:%d\n",instInfo.rs1_valid,instInfo.rs1_phy,instInfo.rs2_valid,
                instInfo.rs2_phy,instInfo.rd_valid,instInfo.rd_phy);
                }
            }
    };
}
