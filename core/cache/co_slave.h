/*
 * @Author: 苗金标
 * @Date: 2023-04-14 09:52:34
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-04-17 14:48:48
 * @Description: 
 */
#pragma once
#include <cstdint>
class co_slave{
    public:
        virtual void invalidate_exclusive(uint64_t start_addr) = 0;
        virtual void invalidate_shared(uint64_t start_addr) = 0;
};
enum l1_line_status{
    L1_INVALID, L1_SHARED, L1_EXCLUSIVE, L1_MODIFIED
};
