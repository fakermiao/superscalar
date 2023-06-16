/*
 * @Author: 苗金标
 * @Date: 2023-06-01 14:44:05
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 14:29:34
 * @Description: 
 */
#include "lsu.h"

#define DATABYTE 0xff
#define DATAHALF ((DATABYTE << 8) | DATABYTE)
#define DATAWORD ((DATAHALF << 16) | DATAHALF)

namespace Supercore{
    execute_channel lsu::evaluate(wb_feedback_pack wb_feedback){
        execute_channel exe_channel;
        memset(&exe_channel,0,sizeof(exe_channel));

        if(!(wb_feedback.enable && wb_feedback.flush)){
            if(!issue_lsu_fifo->is_empty() && !lsu_wb_fifo->is_full()){
                instStr instInfo;
                issue_lsu_fifo->get_front(&instInfo);

                bool store_buffer_full = false;
                if(instInfo.valid){
                    uint64_t lsu_value = 0;
                    auto lsu_addr = instInfo.rs1_value + instInfo.imm;
                    switch(instInfo.fuOpType.lsuOp){
                        case LSUOpType::lb:{
                            uint8_t buf;
                            rv_exc_code exp_id = mem_read(lsu_addr,1,(unsigned char*)&buf);
                            if(exp_id == exc_custom_ok){
                                lsu_value = store_buffer->get_value(lsu_addr,1,buf);
                                instInfo.rd_enable = true;
                                instInfo.rd_value = sign_extend(lsu_value,8);
                            }else{
                                instInfo.has_execp = true;
                                instInfo.execp_id  = exp_id;
                            }
                            break;
                        }
                        case LSUOpType::lh:{
                            int16_t buf;
                            rv_exc_code exp_id = mem_read(lsu_addr,2,(unsigned char*)&buf);
                            if(exp_id == exc_custom_ok){
                                lsu_value = store_buffer->get_value(lsu_addr,2,buf);
                                instInfo.rd_enable = true;
                                instInfo.rd_value  = sign_extend(lsu_value,16);
                            }else{
                                instInfo.has_execp = true;
                                instInfo.execp_id  = exp_id;
                            }
                            break;
                        }
                        case LSUOpType::lw:{
                            int32_t buf;
                            rv_exc_code exp_id = mem_read(lsu_addr,4,(unsigned char*)&buf);
                            if(exp_id == exc_custom_ok){
                                lsu_value = store_buffer->get_value(lsu_addr,4,buf);
                                instInfo.rd_enable = true;
                                printf("lw store addr:%lx, data:%lx\n",lsu_addr,lsu_value);
                                instInfo.rd_value  = sign_extend(lsu_value,32);
                            }else{
                                instInfo.has_execp = true;
                                instInfo.execp_id  = exp_id;
                            }
                            break;
                        }
                        case LSUOpType::ld:{
                            int64_t buf;
                            rv_exc_code exp_id = mem_read(lsu_addr,8,(unsigned char*)&buf);
                            if(exp_id == exc_custom_ok){
                                lsu_value = store_buffer->get_value(lsu_addr,8,buf);
                                instInfo.rd_enable = true;
                                instInfo.rd_value = lsu_value;
                            }else{
                                instInfo.has_execp = true;
                                instInfo.execp_id = exp_id;
                            }
                            break;
                        }
                        case LSUOpType::lbu:{
                            uint8_t buf;
                            rv_exc_code exp_id = mem_read(lsu_addr,1,(unsigned char*)&buf);
                            if(exp_id == exc_custom_ok){
                                lsu_value = store_buffer->get_value(lsu_addr,1,buf);
                                instInfo.rd_enable = true;
                                instInfo.rd_value  = lsu_value;
                            }else{
                                instInfo.has_execp = true;
                                instInfo.execp_id = exp_id;
                            }
                            break;
                        }
                        case LSUOpType::lhu:{
                            uint16_t buf;
                            rv_exc_code exp_id = mem_read(lsu_addr,2,(unsigned char*)&buf);
                            if(exp_id == exc_custom_ok){
                                lsu_value = store_buffer->get_value(lsu_addr,2,buf);
                                instInfo.rd_enable = true;
                                instInfo.rd_value = lsu_value;
                            }else{
                                instInfo.has_execp = true;
                                instInfo.execp_id = exp_id;
                            }
                            break;
                        }
                        case LSUOpType::lwu:{
                            uint32_t buf;
                            rv_exc_code exp_id = mem_read(lsu_addr,4,(unsigned char*)&buf);
                            if(exp_id == exc_custom_ok){
                                lsu_value = store_buffer->get_value(lsu_addr,4,buf);
                                instInfo.rd_enable = true;
                                instInfo.rd_value = lsu_value;
                            }else{
                                instInfo.has_execp = true;
                                instInfo.execp_id = exp_id;
                            }
                            break;
                        }
                        case LSUOpType::sb:{
                            if(store_buffer->is_full()){
                                store_buffer_full = true;
                            }else{
                                component::store_buffer_item item;
                                item.enable = true;
                                item.addr   = lsu_addr;
                                item.size   = 1;
                                item.data   = instInfo.rs2_value & DATABYTE;
                                item.committed = false;
                                item.pc     = instInfo.pc;
                                item.rob_id = instInfo.rob_id;
                                store_buffer->push_sync(item);
                            }
                            break;
                        }
                        case LSUOpType::sh:{
                            if(store_buffer->is_full()){
                                store_buffer_full = true;
                            }else{
                                component::store_buffer_item item;
                                item.enable = true;
                                item.addr   = lsu_addr;
                                item.size   = 2;
                                item.data   = instInfo.rs2_value & DATAHALF;
                                item.committed = false;
                                item.pc     = instInfo.pc;
                                item.rob_id = instInfo.rob_id;
                                store_buffer->push_sync(item);
                                // printf("store pc:%lx,addr:%lx,data:%lx\n",item.pc,item.addr,item.data);
                            }
                            break;
                        }
                        case LSUOpType::sw:{
                            if(store_buffer->is_full()){
                                store_buffer_full = true;
                            }else{
                                component::store_buffer_item item;
                                item.enable = true;
                                item.addr   = lsu_addr;
                                item.size   = 4;
                                item.data   = instInfo.rs2_value & DATAWORD;    
                                item.committed = false;
                                item.pc     = instInfo.pc;
                                item.rob_id = instInfo.rob_id;
                                store_buffer->push_sync(item);
                                printf("store pc:%lx,addr:%lx,data:%lx\n",item.pc,item.addr,item.data);
                            }
                            break;
                        }
                        case LSUOpType::sd:{
                            if(store_buffer->is_full()){
                                store_buffer_full = true;
                            }else{
                                component::store_buffer_item item;
                                item.enable = true;
                                item.addr   = lsu_addr;
                                item.size   = 8;
                                item.data   = instInfo.rs2_value;
                                item.committed = false;
                                item.pc     = instInfo.pc;
                                item.rob_id = instInfo.rob_id;
                                store_buffer->push_sync(item);
                            }
                            break;
                        }
                        case LSUOpType::lr:{
                            if(instInfo.rs2_id != 0){
                                instInfo.has_execp = true;
                                instInfo.execp_id  = exc_illegal_instr;
                            }else{
                                int64_t lr_result;
                                rv_exc_code execp_id = priv.va_lr(instInfo.rs1_value,8,(uint8_t*)&lr_result);
                                if(execp_id == exc_custom_ok){  
                                    lsu_value = store_buffer->get_value(instInfo.rs1_value,8,lr_result);
                                    instInfo.rd_enable = true;
                                    instInfo.rd_value  = lsu_value;
                                }else{
                                    instInfo.has_execp = true;
                                    instInfo.execp_id  = execp_id;
                                } 
                            }
                            break;
                        }
                        case LSUOpType::lrw:{
                            if(instInfo.rs2_id != 0){
                                instInfo.has_execp = true;
                                instInfo.execp_id  = exc_illegal_instr;
                            }else{
                                int32_t lrw_result;
                                rv_exc_code execp_id = priv.va_lr(instInfo.rs1_value,4,(uint8_t*)&lrw_result);
                                if(execp_id == exc_custom_ok){
                                    lsu_value = store_buffer->get_value(instInfo.rs1_value,4,lrw_result);
                                    instInfo.rd_enable = true;
                                    instInfo.rd_value  = lsu_value;
                                }else{
                                    instInfo.has_execp = true;
                                    instInfo.execp_id  = execp_id;
                                }
                            }
                            break;
                        }
                        case LSUOpType::sc:{
                            if(store_buffer->is_full()){
                                store_buffer_full = true;
                            }else{
                                bool sc_result;
                                rv_exc_code exc = priv.super_va_sc(instInfo.rs1_value,8,(uint8_t*)&instInfo.rs2_value,sc_result);
                                if(exc == exc_custom_ok){
                                    component::store_buffer_item item;
                                    item.enable = true;
                                    item.addr   = instInfo.rs1_value;
                                    item.size   = sc_result ? 0 : 8;
                                    item.data   = instInfo.rs2_value;
                                    item.committed = false;
                                    item.pc     = instInfo.pc;
                                    item.rob_id = instInfo.rob_id;
                                    store_buffer->push_sync(item);
                                    instInfo.rd_enable = true;
                                    instInfo.rd_value = sc_result;
                                }else{
                                    instInfo.has_execp = true;
                                    instInfo.execp_id = exc;
                                }
                            }
                            break;
                        }
                        case LSUOpType::scw:{
                            if(store_buffer->is_full()){
                                store_buffer_full = true;
                            }else{
                                bool sc_result;
                                rv_exc_code exc = priv.super_va_sc(instInfo.rs1_value,4,(uint8_t*)&instInfo.rs2_value,sc_result);
                                if(exc == exc_custom_ok){
                                    component::store_buffer_item item;
                                    item.enable = true;
                                    item.addr   = instInfo.rs1_value;
                                    item.size   = sc_result ? 0 : 4;
                                    item.data   = instInfo.rs2_value & DATAWORD;
                                    item.committed = false;
                                    item.pc     = instInfo.pc;
                                    item.rob_id = instInfo.rob_id;
                                    store_buffer->push_sync(item);
                                    instInfo.rd_enable = true;
                                    instInfo.rd_value  = sc_result;
                                }else{
                                    instInfo.has_execp = true;
                                    instInfo.execp_id  = exc;
                                }
                            }
                            break;
                        }
                        case LSUOpType::amoswap: case LSUOpType::amoadd: case LSUOpType::amoxor: case LSUOpType::amoand: case LSUOpType::amoor:
                        case LSUOpType::amomin:  case LSUOpType::amomax: case LSUOpType::amominu: case LSUOpType::amomaxu:{
                            int64_t amo_result;
                            int64_t store_result;
                            rv_exc_code execp_id = priv.super_va_amo(instInfo.rs1_value,8,instInfo.fuOpType.lsuOp,instInfo.rs2_value,amo_result,store_result);
                            // printf("hello lsu amoadd_d\n");
                            if(execp_id == exc_custom_ok){
                                component::store_buffer_item item;
                                item.enable = true;
                                item.addr   = instInfo.rs1_value;
                                item.size   = 8;
                                item.data   = store_result;
                                item.committed = false;
                                item.pc     = instInfo.pc;
                                item.rob_id = instInfo.rob_id;
                                if(store_buffer->is_full())
                                    store_buffer_full = true;
                                else{     
                                    store_buffer->push_sync(item);
                                    instInfo.rd_enable = true;
                                    instInfo.rd_value  = amo_result;
                                }
                            }else{
                                instInfo.has_execp = true;
                                instInfo.execp_id  = execp_id;
                            }
                            break;
                        }
                        case LSUOpType::amoswapw: case LSUOpType::amoaddw: case LSUOpType::amoxorw: case LSUOpType::amoandw: case LSUOpType::amoorw:
                        case LSUOpType::amominw:  case LSUOpType::amomaxw: case LSUOpType::amominuw: case LSUOpType::amomaxuw:{
                            int64_t amo_result;
                            int64_t store_result;
                            rv_exc_code execp_id = priv.super_va_amo(instInfo.rs1_value,4,instInfo.fuOpType.lsuOp,instInfo.rs2_value,amo_result,store_result);
                            if(execp_id == exc_custom_ok){
                                component::store_buffer_item item;
                                item.enable = true;
                                item.addr   = instInfo.rs1_value;
                                item.size   = 4;
                                item.data   = store_result;
                                item.committed = false;
                                item.pc     = instInfo.pc;
                                item.rob_id = instInfo.rob_id;
                                if(store_buffer->is_full())
                                    store_buffer_full = true;
                                else{
                                    store_buffer->push_sync(item);
                                    instInfo.rd_enable = true;
                                    instInfo.rd_value  = sign_extend(amo_result,32);
                                }
                            }else{
                                instInfo.has_execp = true;
                                instInfo.execp_id  = execp_id;
                            }
                            break;
                        }
                    }
                    exe_channel.rd_enable = instInfo.rd_enable;
                    exe_channel.rd_id     = instInfo.rd_phy;
                    exe_channel.rd_value  = instInfo.rd_value;

                    this->lsu_p(instInfo);
                }
                if(!store_buffer_full){
                    // printf("pc:%lx,data:%lx\n",instInfo.pc,instInfo.rd_value);
                    lsu_wb_fifo->push(instInfo);
                    issue_lsu_fifo->pop(&instInfo);
                }
            }
        }else{
            lsu_wb_fifo->flush();
        }
        return exe_channel;
    }
}