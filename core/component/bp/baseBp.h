/*
 * @Author: 苗金标
 * @Date: 2023-05-23 19:55:32
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-24 15:42:50
 * @Description: 
 */
#pragma once
#include "../../common.h"
#include "../checkpoint_buffer.h"
namespace component{
    class baseBp{
        // bool get_prediction(uint64_t pc,uint32_t inst,bool *jump,uint64_t *next_pc)
        public:
            virtual bool get_prediction(uint64_t pc,uint32_t inst,bool *jump,uint64_t *next_pc) = 0;
            virtual void save(checkpoint_t &cp,uint64_t cur_pc) = 0;
            virtual void update_prediction_guess(uint64_t pc,uint32_t inst,bool jump) = 0;
            virtual void update_prediction(uint64_t pc,uint32_t inst,bool jump,uint64_t next_pc,bool hit,uint32_t global_history_recover) = 0;
    };
}
