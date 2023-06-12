/*
 * @Author: 苗金标
 * @Date: 2023-04-28 16:03:31
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-05 20:36:46
 * @Description: 
 * sync作用类似于上升沿同步逻辑，在sync这个点同步更新所有组件
 */
#pragma once
#include "../common.h"
#include "../config.h"
#include "fifo.h"
namespace component{
    static const uint32_t phy_reg_num_bitmap_size = (PHY_REG_NUM + bitsizeof(uint64_t) - 1) / (bitsizeof(uint64_t));

    typedef struct checkpoint_t{
        uint64_t rat_phy_map_table_valid[phy_reg_num_bitmap_size];
        uint64_t rat_phy_map_table_visible[phy_reg_num_bitmap_size];
        uint64_t phy_regfile_data_valid[phy_reg_num_bitmap_size];

        uint32_t rat_phy_map_table[PHY_REG_NUM];
        uint32_t global_history;
        uint32_t local_history;

        //clone the info into cp
        void clone(checkpoint_t &cp){
            memcpy(&cp.rat_phy_map_table_valid,&rat_phy_map_table_valid,sizeof(rat_phy_map_table_valid));
            memcpy(&cp.rat_phy_map_table_visible,&rat_phy_map_table_visible,sizeof(rat_phy_map_table_visible));
            memcpy(&cp.phy_regfile_data_valid,&phy_regfile_data_valid,sizeof(phy_regfile_data_valid));
            cp.global_history = global_history;
            cp.local_history  = local_history;
        }
    }checkpoint_t;

    class checkpoint_buffer : public fifo<checkpoint_t>{
        private:
            enum class sync_request_type_t{
                pop,set_item
            };

            typedef struct sync_request_t{
                sync_request_type_t req;
                uint32_t arg1;
                checkpoint_t arg2;
            }sync_request_t;

            std::queue<sync_request_t> sync_request_q;

            bool check_new_id_valid(uint32_t id){
                if(this->is_full()){
                    return false;
                }else if(this->wstage == this->rstage){
                    return !((id>=this->rptr) && (id<this->wptr));
                }else{
                    return !(((id>=this->rptr)&&(id<this->size))||(id<this->wptr));
                }
            }

            bool check_id_valid(uint32_t id){
                if(this->is_empty()){
                    return false;
                }else if(this->wstage == this->rstage){
                    return (id>=this->rptr) && (id<wptr);
                }else{
                    return ((id>=this->rptr) &&  (id<this->size)) || (id<this->wptr);
                }
            }

        public:
            checkpoint_buffer(uint32_t size) : fifo<checkpoint_t>(size){}

            bool push(checkpoint_t element,uint32_t *item_id){
                *item_id = this->wptr;
                return fifo<checkpoint_t>::push(element);
            }
            bool pop(){
                checkpoint_t tem;
                return fifo<checkpoint_t>::pop(&tem);
            }

            void set_item(uint32_t item_id,checkpoint_t &item){
                this->buffer[item_id] = item;
            }
            checkpoint_t get_item(uint32_t item_id){
                return this->buffer[item_id];
            }

            void set_item_sync(uint32_t item_id,checkpoint_t &item){
                sync_request_t t_req;
                t_req.req = sync_request_type_t::set_item;
                t_req.arg1 = item_id;
                t_req.arg2 = item;
                sync_request_q.push(t_req);
            }

            void sync(){
                sync_request_t t_req;
                while(!sync_request_q.empty()){
                    t_req = sync_request_q.front();
                    sync_request_q.pop();
                    switch(t_req.req){
                        case sync_request_type_t::set_item:{
                            set_item(t_req.arg1,t_req.arg2);
                            break;
                        }
                        case sync_request_type_t::pop:{
                            pop();
                            break;
                        }
                    }
                }
            }
    };
}
