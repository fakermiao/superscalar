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
    /*
int64_t mdu_exec(int64_t& a,int64_t& b,MDUOpType op){
        int64_t result = 0;
        // printf("mdu\t---");
        switch(op){
            case MDUOpType::mul:{
                result = a * b;
                break;
            }
            case MDUOpType::mulh:{
                result = ((__int128_t)a * (__int128_t)b) >> 64;
                break;
            }
            case MDUOpType::mulhu:{
                
                result = (static_cast<__uint128_t>(static_cast<uint64_t>(a)) * static_cast<__uint128_t>(static_cast<uint64_t>(b))) >> 64;
                break;
            }
            case MDUOpType::mulhsu:{
                result = (static_cast<__int128_t>(a) * static_cast<__uint128_t>(static_cast<uint64_t>(b))) >> 64;
                break;
            }
            case MDUOpType::mulw:{
                result = (int32_t)a * (int32_t)b;
                break;
            }
            case MDUOpType::div:{
                if(b == 0) result = -1;
                else if(a == LONG_MIN && b == -1) result = LONG_MIN;
                else result = a / b;
                break;
            }
            case MDUOpType::divu:{
                if(b == 0) result = ULONG_MAX;
                else result = ((uint64_t)a / (uint64_t)b);
                break;
            }
            case MDUOpType::rem:{
                if(b == 0) result = a;
                else if(a == LONG_MIN && b == -1) result = 0;
                else result = a % b;
                break;
            }
            case MDUOpType::remu:{
                if(b == 0) result = a;
                else result = (uint64_t)a % (uint64_t)b;
                break;
            }
            case MDUOpType::divw:{
                a = (int32_t)a;
                b = (int32_t)b;

                if(b == 0) result = -1;
                else if(a == INT_MIN && b == -1) result = INT_MIN;
                else result = SEXT(((int32_t)a / (int32_t)b),32);
                break;
            }
            case MDUOpType::divuw:{
                a = (int32_t)a;
                b = (int32_t)b;
                if(b == 0) result = ULLONG_MAX;
                else result = SEXT(((uint32_t)a / (uint32_t)b),32);
                break;
            }
            case MDUOpType::remw:{
                a = (int32_t)a;
                b = (int32_t)b;
                if(b == 0) result = a;
                else if(a == LONG_MIN && b == -1) result = 0;
                else result = a % b;
                break;
            }
            case MDUOpType::remuw:{
                a = (int32_t)a;
                b = (int32_t)b;
                if(b == 0) result = a;
                else result = SEXT((uint32_t)a % (uint32_t)b,32);
                break; 
            }
            default:
                assert(false);
        }
        return result;
    }
    */
    

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
                printf("%smdu%s%d/%ld%s:\t\tpc:%lx,inst:%x\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst);
                // printf("\t\tbranch_jump:%d,branch_pc:%lx,result:%lx\n",instInfo.bru_jump,instInfo.bru_next_pc,instInfo.rd_value);
            }
    };
}





























    