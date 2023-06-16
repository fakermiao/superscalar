/*
 * @Author: 苗金标
 * @Date: 2023-05-26 09:32:39
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-06 15:50:42
 * @Description: 
 */
#pragma once
#include "../common.h"
#include "fifo.h"
namespace component{
    typedef struct rob_item{
        bool        complete;
        uint32_t    Areg;
        uint32_t    Preg;
        bool        OPreg_v;
        uint32_t    OPreg;
        uint64_t    pc;
        uint32_t    inst;
        bool        inst_v;

        bool        rd_valid;
        uint32_t    rd_id;
        int64_t    rd_value;

        bool        has_execp;
        rv_exc_code execp_id;
        uint64_t    execp_value;
        bool        predicted;
        bool        predicted_jump;
        uint64_t    predicted_next_pc;
        bool        checkpoint_id_valid;
        uint32_t    checkpoint_id;
        bool        bru_jump;
        uint64_t    bru_next_pc;
        FuType      fu_type;
        union{
            ALUOpType aluOp;
            BRUOpType bruOp;
            CSROpType csrOp;
            LSUOpType lsuOp;
            MDUOpType mduOp;
            MOUOpType mouOp;
        }fuOpType;
    }rob_item;

    class rob : public fifo<rob_item>{
        public:
            rob(uint32_t size) : fifo<rob_item>(size){}

            bool check_id_valid(uint32_t id){
                if(this->is_empty())
                    return false;
                else if(this->wstage == this->rstage)
                    return (id >= this->rptr) && (id < this->wptr);
                else 
                    return ((id >= this->rptr) && (id < this->size)) || (id < this->wptr);
            }

            uint32_t get_free_space(){
                return this->size - get_size();
            }

            bool push(rob_item element,uint32_t *item_id){
                *item_id = this->wptr;
                return fifo<rob_item>::push(element);
            }

            rob_item get_item(uint32_t item_id){
                return this->buffer[item_id];
            }

            void set_item(uint32_t item_id,rob_item item){
                this->buffer[item_id] = item;
            }

            bool get_front_id(uint32_t *front_id){
                if(this->is_empty())
                    return false;
                *front_id = this->rptr;
                return true;
            }

            bool get_tail_id(uint32_t *tail_id){
                if(this->is_empty())
                    return false;
                *tail_id = (wptr + this->size - 1) % this->size;
                return true;
            }

            bool get_prev_id(uint32_t id,uint32_t *prev_id){
                assert(check_id_valid(id));
                *prev_id = (id + this->size - 1) % this->size;
                return check_id_valid(*prev_id);
            }

            bool get_next_id(uint32_t id,uint32_t *next_id){
                assert(check_id_valid(id));
                *next_id = (id + 1) % this->size;
                return check_id_valid(*next_id);
            }
    };
}