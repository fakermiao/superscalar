/*
 * @Author: 苗金标
 * @Date: 2023-05-31 14:29:13
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-05 20:12:41
 * @Description: 
 */
#pragma once

#include "../common.h"
#include "fifo.h"
#include <algorithm>
#include <queue>

namespace component{
    typedef struct store_buffer_item{
        bool        enable;
        bool        committed;
        uint32_t    rob_id;
        uint64_t    pc;
        uint64_t    addr;
        uint64_t    data;
        uint64_t    size;
    }store_buffer_item;
    typedef struct store_buffer_state_pack{
        uint32_t wptr;
        bool     wstage;
    }store_buffer_state_pack;

    class store_buffer : public fifo<store_buffer_item>{
        public:
            enum class sync_request_type{
                set_item,push,pop,restore
            };

            typedef struct sync_request{
                sync_request_type req;
                store_buffer_item store_buffer_item_t;
                store_buffer_state_pack store_buffer_state_pack_t;
            }sync_request;

            std::queue<sync_request> sync_request_q;
            store_buffer(uint32_t size) : fifo<store_buffer_item>(size){}

            bool check_id_valid(uint32_t id){
                if(this->is_empty())
                    return false;
                else if(this->wstage == this->rstage)
                    return (id >= this->rptr) && (id < this->wptr);
                else
                    return ((id >= this->rptr) && (id < this->size)) || (id < this->wptr);
            }

            bool get_front_id(uint32_t *front_id){
                if(this->is_empty())
                    return false;
                *front_id = this->rptr;
                return true;
            }

            bool get_next_id(uint32_t id,uint32_t *next_id){
                assert(check_id_valid(id));
                *next_id = (id + 1) % this->size;
                return check_id_valid(*next_id);
            }

            store_buffer_item get_item(uint32_t id){
                assert(check_id_valid(id));
                return this->buffer[id];
            }


            uint64_t get_value(uint64_t addr,uint64_t size,uint64_t mem_value){
                uint64_t result = mem_value;
                uint32_t cur_id;
                if(get_front_id(&cur_id)){
                    auto first_id = cur_id;
                    do{
                        auto cur_item = get_item(cur_id);
                        if(cur_item.enable){
                            /*
                                case1.
                                    addr                                                              addr+size
                                            cur_item.addr                      cur_item.addr+size
                                case2.
                                    addr                                                              addr+size
                                                    cur_item.addr                                                   cur_item.addr+size
                                case3.
                                                                addr                                  addr+size
                                    cur_item.addr                                                                   cur_item.addr+size
                                case4.
                                                        addr                                          addr+size
                                    cur_item.addr                               cur_item.addr+size
                            */
                            if((cur_item.addr >= addr) && (cur_item.addr < (addr + size))){
                                uint64_t bit_offset = (cur_item.addr - addr) * 8;
                                uint64_t bit_length = std::min(cur_item.size,addr + size - cur_item.addr) * 8;
                                uint64_t bit_mask   = (bit_length == 64) ? 0xffffffffffffffff : ((1ul << bit_length) - 1);
                                result &= ~(bit_mask << bit_offset);
                                result |= (cur_item.data & bit_mask) << bit_offset;

                            }else if((cur_item.addr < addr) && (cur_item.addr + cur_item.size) > addr){
                                uint64_t bit_offset = (addr - cur_item.addr) * 8;
                                uint64_t bit_length = std::min(size,cur_item.addr + cur_item.size - addr) * 8;
                                uint64_t bit_mask   = (bit_length == 64) ? 0xffffffffffffffff : ((1 << bit_length) - 1);
                                result &= ~bit_mask;
                                result |= (cur_item.data >> bit_offset) & bit_mask;
                            }
                        }
                    }while(get_next_id(cur_id,&cur_id) && (cur_id != first_id));
                }
                return result;
            }

            void push_sync(store_buffer_item item){
                sync_request req_t;
                req_t.req = sync_request_type::push;
                req_t.store_buffer_item_t = item;
                sync_request_q.push(req_t);
            }
            
            void print(){
                if(this->is_empty())
                    return;
                for(uint32_t i = this->rptr;i != this->wptr;i = (i + 1) % size)
                    printf("store print pc:%lx,addr:%lx,data:%lx\n",this->buffer[i].pc,this->buffer[i].addr,this->buffer[i].data);
            }

            void sync(){
                while(!sync_request_q.empty()){
                    auto req_t = sync_request_q.front();
                    sync_request_q.pop();
                    switch(req_t.req){
                        case sync_request_type::push:
                            this->push(req_t.store_buffer_item_t);
                            break;
                        default:
                            assert(0);
                    }
                }
            }
    };
}