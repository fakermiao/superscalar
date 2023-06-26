/*
 * @Author: 苗金标
 * @Date: 2023-04-03 19:20:57
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 14:25:03
 * @Description: 
 */
#pragma once

#include "../../common.h"
#include "../../component/fifo.h"
#include "../execute.h"
#include "../wb.h"
#include "../../color.h"
namespace Supercore{
/* jal,jalr,beq,bne,blt,bge,bltu,bgeu */
typedef struct bru_feedback_pack{
    bool     enable;
    bool     jump;
    uint64_t next_pc;
}bru_feedback_pack;

inline void bru_exec(instStr &inst,bru_feedback_pack *feed_pack){
    feed_pack->enable = true;
    switch(inst.fuOpType.bruOp){
        case BRUOpType::jal:{
            feed_pack->jump = true;
            feed_pack->next_pc = inst.pc + inst.imm;
            break;
        }
        case BRUOpType::jalr:{
            feed_pack->jump = true;
            feed_pack->next_pc = inst.rs1_value + inst.imm;
            break;
        }
        case BRUOpType::beq:{
            feed_pack->jump = (inst.rs1_value == inst.rs2_value);
            feed_pack->next_pc = inst.pc + inst.imm;
            break;        
        }
        case BRUOpType::bne:{
            feed_pack->jump = (inst.rs1_value != inst.rs2_value);
            feed_pack->next_pc = inst.pc + inst.imm;
            break;
        }
        case BRUOpType::blt:{
            feed_pack->jump = (inst.rs1_value < inst.rs2_value);
            feed_pack->next_pc = inst.pc + inst.imm;
            break;
        }
        case BRUOpType::bge:{
            feed_pack->jump = (inst.rs1_value >= inst.rs2_value);
            feed_pack->next_pc = inst.pc + inst.imm;
            break;
        }
        case BRUOpType::bltu:{
            feed_pack->jump = ((uint64_t)inst.rs1_value < (uint64_t)inst.rs2_value);
            feed_pack->next_pc = inst.pc + inst.imm;
            break;
        }
        case BRUOpType::bgeu:{
            feed_pack->jump = ((uint64_t)inst.rs1_value >= (uint64_t)inst.rs2_value);
            feed_pack->next_pc = inst.pc + inst.imm;
            break;
        }
    }
    
}

    class bru{
        public:
            uint32_t num;
            component::fifo<instStr> *issue_bru_fifo;
            component::fifo<instStr> *bru_wb_fifo;

            bru(uint32_t num,component::fifo<instStr> *issue_bru_fifo,component::fifo<instStr> *bru_wb_fifo) : 
            num(num),issue_bru_fifo(issue_bru_fifo),bru_wb_fifo(bru_wb_fifo){}
            execute_channel evaluate(wb_feedback_pack wb_feedback);
            
            void bru_p(instStr& instInfo){
                if(cpu.cycle >= DIFFTEST_CYC){
                    printf("%sbru%s%d/%ld%s:\t\tpc:%lx,inst:%x\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst);
                    printf("\t\tbranch_jump:%d,branch_pc:%lx,result:%lx\n",instInfo.bru_jump,instInfo.bru_next_pc,instInfo.rd_value);
                }
            }      
    };
}













