/*
 * @Author: 苗金标
 * @Date: 2023-04-14 15:48:29
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-04-14 16:03:37
 * @Description: https://zhuanlan.zhihu.com/p/516582856
 */
#pragma once
#include <bitset>
#include <bit>
template <int nr_ways = 4>
class tree_plru{
    public:
        tree_plru(){
            depth = 0;
            while(1 << depth != nr_ways) depth++;
        }
        void mark_used(int idx){
            int pos = 1;
            for(int i = depth - 1;i >= 0;i--){
                path[pos] = (idx >> i) & 1;
                pos = (pos << 1) | ((idx >> 1) & 1);
            }
        }
        int get_replace(){
            int res = 0;
            int pos = 1;
            for(int i = depth - 1;i >= 0;i--){
                res = (res << 1) | (path[pos] ^ 1);
                pos = (pos << 1) | (path[pos] ^ 1);
            }
            mark_used(res);
            return res;
        }

    private:
        static_assert(__builtin_popcount(nr_ways) == 1);
        std::bitset <nr_ways-1> path;
        int depth;
};