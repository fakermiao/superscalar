/*
 * @Author: 苗金标
 * @Date: 2023-04-05 19:33:01
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-09 16:26:16
 * @Description: 
 */
#pragma once

#include "../../common.h"
/*
    add,sll,slt,sltu,xor_,srl,or_,and_,sub,sra,
    addw,subw,sllw,srlw,sraw,
*/
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