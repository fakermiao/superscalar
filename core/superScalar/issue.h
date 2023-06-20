/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:41:26
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-07 16:26:59
 * @Description: 
 */
#pragma once
#include "../component/fifo.h"
#include "../component/issue_queue.h"
#include "../component/regfile.h"
#include "../component/checkpoint_buffer.h"
#include "rename_issue.h"
#include "../common.h"
#include "../config.h"
#include "wb.h"
#include "execute/execute.h"
#include "super_config.h"
#include "../color.h"
/*
************************************
note 1.由于访存的特殊性，可以考虑实现两个发射队列，一个用于访存指令另一个用于其它指令，访存指令按顺序发射且每周期发射一条
note 2.由于分支预测的存在，要执行的访存指令可能在分支预测错误的路径上因此不能立刻改变内存结果，对于原子指令要同时写寄存器的指令情况更加复杂
************************************
*/
namespace Supercore{
    class issue{
        public:
            issue(component::issue_queue<instStr> &issue_q,component::fifo<rename_issue_pack> *rename_issue_fifo,component::fifo<instStr> **issue_alu_fifo,component::fifo<instStr> **issue_bru_fifo,
                component::fifo<instStr> **issue_csr_fifo,component::fifo<instStr> **issue_lsu_fifo,component::fifo<instStr> **issue_mdu_fifo,
                component::fifo<instStr> **issue_mou_fifo,component::regfile<uint64_t> *phy_regfile,component::checkpoint_buffer *cp) : 
                issue_q(issue_q),rename_issue_fifo(rename_issue_fifo),issue_alu_fifo(issue_alu_fifo),
                issue_bru_fifo(issue_bru_fifo),issue_csr_fifo(issue_csr_fifo),issue_lsu_fifo(issue_lsu_fifo),issue_mdu_fifo(issue_mdu_fifo),
                issue_mou_fifo(issue_mou_fifo),phy_regfile(phy_regfile),cp(cp){}
            component::issue_queue<instStr>& issue_q;
            component::fifo<rename_issue_pack> *rename_issue_fifo;
            component::fifo<instStr> **issue_alu_fifo;
            component::fifo<instStr> **issue_bru_fifo;
            component::fifo<instStr> **issue_csr_fifo;
            component::fifo<instStr> **issue_lsu_fifo;
            component::fifo<instStr> **issue_mdu_fifo;
            component::fifo<instStr> **issue_mou_fifo;

            component::regfile<uint64_t> *phy_regfile;
            component::checkpoint_buffer *cp;

            uint32_t debug_issue_num;
            virtual void evaluate(wb_feedback_pack wb_feedback_pack_t,exe_feedback_t exe_feedback);

            void issue_p(uint32_t num,instStr& instInfo,uint32_t unit_num){
                printf("%sissue%s%d/%ld%s:\tpc:%lx,inst:%x,rob_id:%d,uint_num:%d\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst,
                        instInfo.rob_id,unit_num);
                printf("\t\trs1_valid:%d,rs1_phy:%d--rs2_valid:%d,rs2_phy:%d--rd_valid:%d,rd_phy:%d\n",instInfo.rs1_valid,instInfo.rs1_phy,instInfo.rs2_valid,
                instInfo.rs2_phy,instInfo.rd_valid,instInfo.rd_phy);
            }
    };
}

