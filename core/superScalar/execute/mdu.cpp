/*
 * @Author: 苗金标
 * @Date: 2023-05-31 20:32:14
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 14:32:10
 * @Description: 
 */
#include "mdu.h"

namespace Supercore{
    execute_channel mdu::evaluate(wb_feedback_pack wb_feedback){
        execute_channel exe_channel;
        memset(&exe_channel,0,sizeof(exe_channel));

        if(!(wb_feedback.enable && wb_feedback.flush)){
            if(!issue_mdu_fifo->is_empty() && !mdu_wb_fifo->is_full()){
                instStr instInfo;
                issue_mdu_fifo->pop(&instInfo);
                if(instInfo.valid){
                    int64_t src1 = instInfo.rs1_value;
                    int64_t src2 = instInfo.rs2_valid ? instInfo.rs2_value : instInfo.imm;
                    int64_t mdu_result;
                    switch(instInfo.fuOpType.mduOp){
                        case MDUOpType::mul:{
                            mdu_result = src1 * src2;
                            break;
                        }
                        case MDUOpType::mulh:{
                            mdu_result = ((__int128_t)src1 * (__int128_t)src2) >> 64;
                            break;
                        }
                        case MDUOpType::mulhu:{
                            mdu_result = (static_cast<__uint128_t>(static_cast<uint64_t>(src1)) * static_cast<__uint128_t>(static_cast<uint64_t>(src2))) >> 64;
                            break;
                        }
                        case MDUOpType::mulhsu:{
                            mdu_result = (static_cast<__int128_t>(src1) * static_cast<__uint128_t>(static_cast<uint64_t>(src2))) >> 64;
                            break;
                        }
                        case MDUOpType::mulw:{
                            mdu_result = (int32_t)src1 * (int32_t)src2;
                            break;
                        }
                        case MDUOpType::div:{
                            if(src2 == 0) mdu_result = -1;
                            else if(src1 == LONG_MIN && src1 == -1) mdu_result = LONG_MIN;
                            else mdu_result = src1 / src2;
                            break; 
                        }
                        case MDUOpType::divu:{
                            if(src2 == 0) mdu_result = ULONG_MAX;
                            else mdu_result = ((uint64_t)src1 / (uint64_t)src2);
                            break;
                        }
                        case MDUOpType::rem:{
                            if(src2 == 0) mdu_result = src1;
                            else if(src1 == LONG_MIN && src2 == -1) mdu_result = 0;
                            else mdu_result = src1 % src2;
                            break;
                        }
                        case MDUOpType::remu:{
                            if(src2 == 0) mdu_result = src1;
                            else mdu_result = (uint64_t)src1 % (uint64_t)src2;
                            break;
                        }
                        case MDUOpType::divw:{
                            src1 = (int32_t)src1;
                            src2 = (int32_t)src2;
                            if(src2 == 0) mdu_result = -1;
                            else if(src1 == INT_MIN && src2 == -1) mdu_result = INT_MIN;
                            else mdu_result = SEXT(((int32_t)src1 / (int32_t)src2),32);
                            break;
                        }
                        case MDUOpType::divuw:{
                            src1 = (int32_t)src1;
                            src2 = (int32_t)src2;
                            if(src2 == 0) mdu_result = ULLONG_MAX;
                            else mdu_result = SEXT(((uint32_t)src1 / (uint32_t)src2),32);
                            break;
                        }
                        case MDUOpType::remw:{
                            src1 = (int32_t)src1;
                            src2 = (int32_t)src2;
                            if(src2 == 0) mdu_result = src1;
                            else if(src1 == LONG_MIN && src2 == -1) mdu_result = 0;
                            else mdu_result = src1 % src2;
                            break;
                        }
                        case MDUOpType::remuw:{
                            src1 = (int32_t)src1;
                            src2 = (int32_t)src2;
                            if(src2 == 0) mdu_result = src1;
                            else mdu_result = SEXT((uint32_t)src1 % (uint32_t)src2,32);
                            break;
                        }
                        default:
                            assert(0);
                    }
                    instInfo.rd_enable    = true;
                    instInfo.rd_value     = mdu_result;
                    exe_channel.rd_enable = true;
                    exe_channel.rd_id     = instInfo.rd_phy;
                    exe_channel.rd_value  = mdu_result;

                    this->mdu_p(instInfo);
                }
                mdu_wb_fifo->push(instInfo);
            }
        }
        return exe_channel;
    }
}
