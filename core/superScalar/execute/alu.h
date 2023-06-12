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
/*
    add,sll,slt,sltu,xor_,srl,or_,and_,sub,sra,
    addw,subw,sllw,srlw,sraw,
    int64_t alu_exec(int64_t a,int64_t b,ALUOpType op){
    switch(op){
        case ALUOpType::add:
            return a + b;
        case ALUOpType::sll:
            return a << (b & 0x3f);
        case ALUOpType::slt:
            return (a < b);
        case ALUOpType::sltu:
            return ((uint64_t)a < (uint64_t)b);
        case ALUOpType::xor_:
            return a ^ b;
        case ALUOpType::srl:
            return (uint64_t)a >> (b & 0x3f);
        case ALUOpType::or_:
            return a | b;
        case ALUOpType::and_:
            return a & b;
        case ALUOpType::sub:
            return a - b;
        case ALUOpType::sra:
            return a >> (b & 0x3f);
        case ALUOpType::addw:
            return (int32_t)(a + b);
        case ALUOpType::subw:
            return (int32_t)((int32_t)a - (int32_t)b);
        case ALUOpType::sllw:
            return (int32_t)((int32_t)a << (b & 0x1f));
        case ALUOpType::srlw:
            return (int32_t)((uint64_t)(a & 0xffffffff) >> (b & 0x1f));
        case ALUOpType::sraw:
            return (int32_t)((int32_t)a >> (b & 0x1f));
        default:
            break;
    }
    return 0;
}
*/


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
                printf("%salu%s%d/%ld%s:\tpc:%lx,inst:%x\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst);
                printf("\t\trs1:%lx,rs2:%lx,result:%lx\n",instInfo.rs1_value,instInfo.rs2_value,instInfo.rd_value);
            }
    };
}





