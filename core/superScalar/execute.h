/*
 * @Author: 苗金标
 * @Date: 2023-05-30 19:53:16
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-30 19:59:24
 * @Description: 
 */
#pragma once
#include "../common.h"

namespace Supercore{
    typedef struct execute_channel{
        bool     rd_enable;
        uint32_t rd_id;
        uint32_t rd_value;
    }execute_channel;
}
