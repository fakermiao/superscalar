/*
 * @Author: 苗金标
 * @Date: 2023-05-12 09:29:47
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-16 15:33:06
 * @Description: 
 */
/*
1.分支预测两个要素：方向（跳转、不跳转）、目标地址（直接跳转以立即数的形式给出一个相对于PC的偏移值、间接跳转目标地址
来自于一个通用寄存器（除了CALL/Return指令外一般的处理器都不会推荐使用间接跳转类型的分支指令））

2.分支指令的判断：在取指阶段快速解码分辨是否是分支指令、在指令从L2 Cache写入到I-Cache之前快速解码（又称预解码）
    设计中使用取指阶段快速解码
 ///发生自修改时要将分支预测器清空

3.分支指令的方向预测
    > 基于两位饱和计数器的分支预测（初始状态一般选择strongly not taken或者weakly not taken）（正确率有极限值现代处理器不会直接使用这种方法）
    > PHT(Pattern History Table)存放两位饱和计数器的值（此时是PC对应的饱和计数器值，可以使用hash压缩PC
        长度减少PHT表项的个数），PHT的更新在乱序中在提交阶段要离开流水线时更新，但在顺序中执行阶段就能更新
    > 基于局部历史的分支预测
    > BHR(Branch History Register)记录分支指令过去的历史状态，使用BHR寻址PHT，所有PHT表项使用同一个饱和计数器
        所有分支指令的BHR组合一起称为分支历史寄存器表（BHRT或BHT），BHT、PHT都使用PC的一部分来寻址
        --> 将PC值进行哈希处理之后得到固定的长度寻址BHT可以一定程度解决BHT重名问题
        --> 将寻址得到的BHR和PC值得一部分进行拼接寻址PHT可以一定程度解决PHT重名问题（所有分支指令使用同一个PHT）
    
    > 基于全局历史的分支预测
        --> GHR(Global History Register,全局历史寄存器)记录所有分支指令最近执行情况，GHR和PC做一定处理后寻址PHT
    > 竞争的分支预测
        --> CPHT(choice PHT)由两位饱和计数器组成，一种预测方法两次预测失败同时另一种预测方法两次预测成功时状态机转到使用另一个
            分支预测方法的状态，转换机制为P1正确P2错误计数器减1，P1错误P2正确计数器加1，预测结果一样计数器保持不变（饱和状态00用P1
            预测，饱和状态11用P2预测）
        --> CPHT寻址使用PC和GHR寄存器进行一定的运算处理的方式进行

    > 分支预测更新
        --> GHR更新有三个时间点：取指令阶段、流水线执行阶段、提交阶段，普通处理器可以使用执行阶段更新但超标量处理器
            在执行阶段和取指阶段间可能存在很多指令因此使用在取指令阶段更新GHR的方法
            --> 取指阶段更新预测失败时的恢复方法：1.提交阶段修复法，提交阶段放置一个GHR，当分支指令退休时将结果更新到这个GHR
                分支预测失败时将这个GHR写到取指阶段的GHR中
            --> checkpoint修复法，取指阶段更新GHR的同时保存旧的GHR
            --> 乱序执行的流水线两种方法都需要，
        --> 由于GHR利用了指令之间的相关性所以取指阶段更新最好，但BHR没有因此可以在指令退休时更新BHR
        --> 一般分支指令退休时更新PHT
4.分支指令的目标地址预测
    直接跳转目标地址固定只需要记录就可以
    > 直接跳转类型的分支预测
        --> BTB(Branch Target Buffer),使用Cache的形式存储分支预测目标地址
        --> BTB缺失的处理：暂停执行直到计算出目标地址或继续执行使用顺序PC值取指令
    > 间接跳转类型的分支预测
        --> CALL/Return指令的分支预测：CALL指令地址固定可以使用BTB对目标地址预测，Return指令使用RAS（Return Address Stack）
            CALL预测时将下一条指令地址存入RAS中，可以在BTB的表项中增加一项记录分支指令类型以判断是否是CALL指令
        --> 对于类似递归调用连续调用同一个子程序的情况可以使用带计数器的RAS
        --> 其它类型的分支指令预测暂时可以忽略

5.分支预测失败的恢复
。。。。。。。。。read book 138（150）页
两种方法:利用ROB恢复和checkpoint恢复，使用checkpoint恢复存在由于乱序执行流水线中部分指令需要冲刷部分指令在正确路径上不需要冲刷的情况，
        这种情况可以使用对分支指令进行编号来处理，分支指令之后的指令得到这个编号直到下一个分支指令
前端取指和后端发射等阶段的状态恢复可以同时进行

分支指令编号在解码阶段进行，每周期只对一条分支指令进行编号，每周期多条指令的超标量如果同时多条分支指令需要延迟到下一周期处理
主要对重命名映射表的恢复
6.正确性检查
使用PTAB将分支指令信息保存下来，表项由三部分组成：Valid表示表项是否被占用、Predict Address被预测的目标地址、Next PC预测为跳转的分支指令
的下一条指令PC值。分支指令在PTAB中的地址随着分支指令在流水线中流动，如果在PTAB表项中不存在说明预测为不发生跳转

7.超标量处理器的分支预测
超标量处理器每周期取指多条指令，此时一种预测方法是对一个周期内取出的所有指令都进行分支预测将第一个预测跳转的分支指令目标地址作为下个周期
取指令的地址
*/
#pragma once
#include "../common.h"
#include "../config.h"
#include "checkpoint_buffer.h"
#include "ras.h"

namespace component{
class branch_predictor{
    public:
        branch_predictor() : main_ras(RAS_SIZE){
            gshare_global_history = 0;
            memset(gshare_pht,0,sizeof(gshare_pht));
            memset(local_bht,0,sizeof(local_bht));
            memset(local_pht,0,sizeof(local_bht));
            memset(cpht,0,sizeof(cpht));
            call_global_history = 0;
            memset(call_target_cache,0,sizeof(call_target_cache));
            normal_global_history = 0;
            memset(normal_target_cache,0,sizeof(normal_target_cache));
        }
        void gshare_global_history_update(bool jump){
            gshare_global_history_retired = ((gshare_global_history_retired << 1) & GSHARE_GLOBAL_HISTORY_MASK) | (jump ? 1 : 0);
        }
        void gshare_update_prediction(uint64_t pc,bool jump,bool hit,uint32_t global_history_recover){
            uint64_t pc_p1 = (pc >> (2 + GSHARE_PC_P2_ADDR_WIDTH)) & GSHARE_PC_P1_ADDR_MASK;
            uint64_t pht_addr = gshare_global_history_retired ^ pc_p1;

            if(jump){
                if(gshare_pht[pht_addr] < 3)
                    gshare_pht[pht_addr]++;
            }else{
                if(gshare_pht[pht_addr] > 0)
                    gshare_pht[pht_addr]--;
            }
            if(!hit)
                gshare_global_history = global_history_recover;
            //gshare_global_history not update
        }
        void gshare_global_history_update_guess(bool jump){
            gshare_global_history = ((gshare_global_history << 1) & GSHARE_GLOBAL_HISTORY_MASK) | (jump ? 1 : 0);
        }

        bool gshare_get_prediction(uint64_t pc){
            uint64_t pc_p1 = (pc >> (2 + GSHARE_PC_P2_ADDR_WIDTH)) & GSHARE_PC_P1_ADDR_MASK;
            uint64_t pht_addr = gshare_global_history ^ pc_p1;

            return gshare_pht[pht_addr] >= 2;
        }

        void local_update_prediction(uint64_t pc,bool jump){
            uint64_t pc_p1 = (pc >> (2 + LOCAL_PC_P2_ADDR_WIDTH)) & LOCAL_PC_P1_ADDR_MASK;
            uint32_t bht_value = local_bht[pc_p1];
            uint32_t pht_addr  = bht_value ^ pc_p1;

            if(jump){
                if(local_pht[pht_addr] < 3)
                    local_pht[pht_addr]++;
                local_bht[pc_p1] = ((local_bht[pc_p1] << 1) & LOCAL_BHT_WIDTH_MASK) | 1;
            }else{
                if(local_pht[pht_addr] > 0)
                    local_pht[pht_addr]--;
                local_bht[pc_p1] = ((local_bht[pc_p1] << 1) & LOCAL_BHT_WIDTH_MASK) | 0;
            }
        }

        uint32_t local_get_bht_value(uint64_t pc){
            uint64_t pc_p1 = (pc >> (2 + LOCAL_PC_P2_ADDR_WIDTH)) & LOCAL_PC_P1_ADDR_MASK;
            return local_bht[pc_p1];
        }

        bool local_get_prediction(uint64_t pc){
            uint64_t pc_p1 = (pc >> (2 + LOCAL_PC_P2_ADDR_WIDTH)) & LOCAL_PC_P1_ADDR_MASK;
            uint32_t bht_value = local_bht[pc_p1];
            uint32_t pht_addr = bht_value ^ pc_p1;
            
            return local_bht[pht_addr] >= 2;
        }

        void call_global_history_update(bool jump){
            call_global_history = ((call_global_history << 1) & CALL_GLOBAL_HISTORY_MASK) | (jump ? 1 : 0);
        }
        void call_update_prediction(uint64_t pc,bool jump,uint64_t target){
            uint64_t pc_p1 = (pc >> (2 + CALL_PC_P2_ADDR_WIDTH)) & CALL_PC_P1_ADDR_MASK;
            uint64_t target_cache_addr = call_global_history ^ pc_p1;

            call_target_cache[target_cache_addr] = target;
            call_global_history_update(jump);
        }

        uint64_t call_get_prediction(uint64_t pc){
            uint64_t pc_p1 = (pc >> (2 + CALL_PC_P2_ADDR_WIDTH)) & CALL_PC_P1_ADDR_MASK;
            uint64_t target_cache_addr = call_global_history ^ pc_p1;

            return call_target_cache[target_cache_addr];
        }

        void normal_global_history_update(bool jump){
            normal_global_history = ((normal_global_history << 1) & NORMAL_GLOBAL_HISTORY_MASK) | (jump ? 1 : 0);
        }
        void normal_update_prediction(uint64_t pc,bool jump,uint64_t target){
            uint64_t pc_p1 = (pc >> (2 + NORMAL_PC_P2_ADDR_WIDTH)) & NORMAL_PC_P1_ADDR_MASK;
            uint64_t target_cache_addr = normal_global_history ^ pc_p1;

            normal_target_cache[target_cache_addr] = target;
            normal_global_history_update(jump);
        }
        uint64_t normal_get_prediction(uint64_t pc){
            uint64_t pc_p1 = (pc >> (2 + NORMAL_PC_P2_ADDR_WIDTH)) & NORMAL_PC_P1_ADDR_MASK;
            
            uint64_t target_cache_addr = normal_global_history ^ pc_p1;
            return normal_target_cache[target_cache_addr];
        }

        void cpht_update_prediction(uint64_t pc,bool hit){
            uint64_t pc_p1 = (pc >> (2 + GSHARE_PC_P2_ADDR_WIDTH)) & GSHARE_PC_P1_ADDR_MASK;
            uint64_t cpht_addr = gshare_global_history_retired ^ pc_p1;

            if(cpht[cpht_addr] <= 1){
                if(hit && (cpht[cpht_addr] > 0)){
                    cpht[cpht_addr]--;
                }else if(!hit){
                    cpht[cpht_addr]++;
                }
            }else{
                if(hit && (cpht[cpht_addr] < 3)){
                    cpht[cpht_addr]++;
                }else if(!hit){
                    cpht[cpht_addr]--;
                }
            }
        }

        bool cpht_get_prediction(uint64_t pc){
            uint64_t pc_p1 = (pc >> (2 + GSHARE_PC_P2_ADDR_WIDTH)) & GSHARE_PC_P1_ADDR_MASK;
            uint64_t cpht_addr = gshare_global_history ^ pc_p1;

            return cpht[cpht_addr] <= 1;
        }

        bool get_prediction(uint64_t pc,uint32_t inst,bool *jump,uint64_t *next_pc){
            auto op_data= inst;
            auto opcode = op_data & 0x7f;
            auto rd     = (op_data >> 7) & 0x1f;
            auto rs1    = (op_data >> 15) & 0x1f;
            auto imm_b  = (((op_data >> 8) & 0x0f) << 1) | (((op_data >> 25) & 0x3f) << 5) | (((op_data >> 7) & 0x01) << 11) |
                            (((op_data >> 31) & 0x01) << 12);
            auto imm_j  = (((op_data >> 12) & 0xff) << 12) | (((op_data >> 20) & 0x1) << 11) | (((op_data >> 21) & 0x3ff) << 1) |
                            (((op_data >> 31) & 0x01) << 20);

            auto need_jump_prediction = true;
            auto instruction_next_pc_valid = true;
            uint64_t instruction_next_pc = 0;

            /*
                rd         rs1         rs1 = rd         RAS action
              !link       !link            -               none
              !link        link            -               pop
               link       !link            -               push
               link        link            0               pop then push
               link        link            1               push
            */
            auto rd_is_link = (rd == 1) || (rd == 5);
            auto rs1_is_link = (rs1 == 1) || (rs1 == 5);

            switch(opcode){
                case 0x6f:{//jal
                    need_jump_prediction = false;
                    instruction_next_pc_valid = true;
                    instruction_next_pc = pc + SEXT(imm_j,21);
                    if(rd_is_link)
                        main_ras.push_addr(pc + 4);
                    break;
                }
                case 0x67:{//jalr
                    need_jump_prediction = false;
                    instruction_next_pc_valid = false;

                    if(rd_is_link){
                        if(rs1_is_link){
                            if(rs1 == rd){
                                //push
                                instruction_next_pc_valid = true;
                                instruction_next_pc       = call_get_prediction(pc);
                                main_ras.push_addr(pc + 4);
                            }else{
                                //pop,then push for coroutine context switch
                                instruction_next_pc_valid = true;
                                instruction_next_pc       = main_ras.pop_addr();
                                main_ras.push_addr(pc + 4);
                            }
                        }else{
                            //push
                            instruction_next_pc_valid = true;
                            instruction_next_pc       = call_get_prediction(pc);
                            main_ras.push_addr(pc + 4);
                        }
                    }else{
                        if(rs1_is_link){//pop
                            instruction_next_pc_valid = true;
                            instruction_next_pc = main_ras.pop_addr();
                        }else{
                            //none
                            instruction_next_pc_valid = true;
                            instruction_next_pc = normal_get_prediction(pc);
                        }
                    }
                    break;
                }
                case 0x63:{//beq bne blt bge bltu bgeu
                    need_jump_prediction = true;
                    instruction_next_pc_valid = !cpht_get_prediction(pc) ? local_get_prediction(pc) : gshare_get_prediction(pc);
                    instruction_next_pc       = instruction_next_pc_valid ? (pc + SEXT(imm_b,13)) : (pc + 4);
                    break;
                }
                default:    
                    return false;
            }

            if(!need_jump_prediction){
                *jump = true;
                if(!instruction_next_pc_valid)
                    return false;
                *next_pc = instruction_next_pc;
            }else{
                *jump = instruction_next_pc_valid;
                *next_pc = instruction_next_pc;
            }
            return true;
        }

        void update_prediction(uint64_t pc,uint32_t inst,bool jump,uint64_t next_pc,bool hit,uint32_t global_history_recover = 0){
            auto rd = (inst >> 7) & 0x1f;
            auto rs1 = (inst >> 15) & 0x1f;
            auto rd_is_link = (rd == 1) || (rd == 5);
            auto rs1_is_link = (rs1 == 1) || (rs1 == 5);

            if((inst & 0x7f) == 0x63){
                // gshare_update_prediction
                gshare_update_prediction(pc,jump,hit,global_history_recover);
                // local_update_prediction
                local_update_prediction(pc,jump);
                // cpht_update_prediction
                cpht_update_prediction(pc,hit);
                gshare_global_history_update(jump);
            }else if((inst & 0x7f) == 0x67){
                if(rd_is_link){
                    if(rs1_is_link){
                        if(rs1 == rd){
                            call_update_prediction(pc,jump,next_pc);
                        }
                        else{
                            //pop,then push for coroutine context switch
                        }
                    }
                    else{
                        //push
                        call_update_prediction(pc,jump,next_pc);
                    }
                }else{
                    if(rs1_is_link){
                        //pop
                    }else{
                        //none
                        normal_update_prediction(pc,jump,next_pc);
                    }
                }
            }
        }
        void save(checkpoint_t &cp,uint64_t pc){
            cp.global_history = gshare_global_history;
            cp.local_history  = local_get_bht_value(pc);
        }

        void update_prediction_guess(uint64_t pc,uint32_t inst,bool jump){
            if((inst & 0x7f) == 0x63){
                gshare_global_history_update_guess(jump);
            }
        }


        uint32_t local_bht[LOCAL_BHT_SIZE];
        uint32_t local_pht[LOCAL_PHT_SIZE];
        uint32_t gshare_global_history;
        uint32_t gshare_global_history_retired;//用于保存之前的GHR值，更新pht（gshare_global_history已经变为推测的值无法使用）
        uint32_t gshare_pht[GSHARE_PHT_SIZE];
        uint32_t cpht[GSHARE_PHT_SIZE];



        // normal/call target cache use to pridict target as BTB(branch target buffer)
        uint32_t call_global_history = 0;
        uint64_t call_target_cache[CALL_TARGET_CACHE_SIZE];
        uint32_t normal_global_history = 0;
        uint64_t normal_target_cache[NORMAL_TARGET_CACHE_SIZE];
        component::ras main_ras;
    
        
};
}