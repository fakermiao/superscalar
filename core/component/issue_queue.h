/*
 * @Author: 苗金标
 * @Date: 2023-05-29 10:31:19
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-07 17:20:29
 * @Description: 
 */
#pragma once
#include "../common.h"
#include "fifo.h"
#include <queue>

namespace component{
    template<typename T>
    //默认为私有继承，加public改变继承
    class issue_queue : public fifo<T>{
        private:
            enum class sync_request_type{
                set_item,compress
            };

            typedef struct sync_request{
                sync_request_type req;
                uint32_t item_id;
                T item;
                std::vector<uint32_t> issued_list;
            }sync_request;
            
            std::queue<sync_request> sync_request_q;
            bool check_id_valid(uint32_t id){
                if(this->is_empty()){
                    return false;
                }else if(this->wstage == this->rstage){
                    return (id >= this->rptr) && (id < this->wptr);
                }else{
                    return ((id >= this->rptr) && (id < this->size)) || (id < this->wptr);
                }
            }

            bool get_next_id_stage(uint32_t id,bool stage,uint32_t *next_id,bool *next_stage){
                assert(check_id_valid(id));
                *next_id = (id + 1) % this->size;
                *next_stage = ((id + 1) >= this->size) != stage;
                return check_id_valid(*next_id);
            }

        public:
            issue_queue(uint32_t size) : fifo<T>(size){}

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

            uint32_t get_space(){
                return this->size - this->get_size();
            }

            T get_item(uint32_t id){
                assert(check_id_valid(id));
                return this->buffer[id];
            }

            void set_item(uint32_t item_id,T item){
                assert(check_id_valid(item));
                this->buffer[item_id] = item;
            }

            void set_item_sync(uint32_t id,T item){
                sync_request req_t;
                req_t.req = sync_request_type::set_item;
                req_t.item_id = id;
                req_t.item = item;
                sync_request_q.push(req_t);
            }

            void compress(std::vector<uint32_t> issued_list){
                uint32_t first_id = 0;
                if((issued_list.size() > 0) && get_front_id(&first_id)){
                    auto cur_id     = first_id;
                    bool cur_stage  = this->rstage;
                    auto i          = 0;
                    auto item_issued_found= false;
                    auto next_id    = first_id;
                    bool new_wstage = this->rstage;
                    auto new_wptr   = first_id;

                    //从读指针处开始向后压缩发射队列,cur_id是遍历到的位置、next_id是压缩后的遍历位置
                    do{
                        if(!item_issued_found){
                            if((i < issued_list.size()) && (cur_id == issued_list[i])){
                                item_issued_found = true;
                                next_id = cur_id;
                                new_wstage = cur_stage;
                                new_wptr = cur_id;
                            }
                        }else{
                            if((i < issued_list.size()) && (cur_id == issued_list[i])){
                                i++;
                            }else{
                                set_item(next_id,get_item(cur_id));
                                get_next_id_stage(next_id,new_wstage,&new_wptr,&new_wstage);
                                next_id = new_wptr;
                            }
                        }
                    }while(get_next_id_stage(cur_id,cur_stage,&cur_id,&cur_stage) && (cur_id != first_id));

                    this->wptr = new_wptr;
                    this->wstage = new_wstage;
                }
            }

            void compress_sync(std::vector<uint32_t> &issued_list){
                sync_request req_t;
                req_t.req = sync_request_type::compress;
                req_t.issued_list = issued_list;
                sync_request_q.push(req_t);
            }

            void sync(){
                while(!sync_request_q.empty()){
                    sync_request req_t = sync_request_q.front();
                    sync_request_q.pop();

                    switch(req_t.req){
                        case sync_request_type::set_item:
                            this->set_item(req_t.item_id,req_t.item);
                            break;
                        case sync_request_type::compress:
                            this->compress(req_t.issued_list);
                    }
                }
            }
    };
}