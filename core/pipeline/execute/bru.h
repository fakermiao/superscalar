/*
 * @Author: 苗金标
 * @Date: 2023-04-03 19:20:57
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-09 16:26:26
 * @Description: 
 */
#pragma once

#include "../../common.h"
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