/*
 * @Author: 苗金标
 * @Date: 2023-05-24 19:47:59
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 17:16:19
 * @Description: 
 * rat考虑到一个周期内多条指令的读写使用sync同步处理一个周期内所有指令的请求
 */
#pragma once
#include "../common.h"
#include "fifo.h"
#include "checkpoint_buffer.h"
#include <queue>
namespace component{
    class rat{
        uint32_t phy_reg_num;
        uint32_t arch_reg_num;
        uint32_t *phy_map_table;
        uint32_t *phy_map_table_valid;
        uint32_t *phy_map_table_visible;//表示最近的映射关系（同一arch可能存在多个映射表项）
        uint32_t bitmap_size;

        fifo<uint32_t> freelist = fifo<uint32_t>(PHY_REG_NUM);

        enum class sync_request_type{
            set_map,restore_map,restore,release_map
        };

        typedef struct sync_request{
            sync_request_type req;
            uint32_t    arg1;
            uint32_t    arg2;
            checkpoint_t cp;
        }sync_request;

        std::queue<sync_request> sync_request_q;

        void set_valid(uint32_t phy_id,bool v){
            assert(phy_id < phy_reg_num);
            if(v){
                phy_map_table_valid[phy_id / (sizeof(phy_map_table_valid[0]) * 8)] |= 1ULL << (phy_id % (sizeof(phy_map_table_valid[0]) * 8));
            }else{
                phy_map_table_valid[phy_id / (sizeof(phy_map_table_valid[0]) * 8)] &= ~(1ULL << (phy_id % (sizeof(phy_map_table_valid[0]) * 8)));
            }
        }
        bool get_valid(uint32_t phy_id){
            assert(phy_id < phy_reg_num);
            return phy_map_table_valid[phy_id / (sizeof(phy_map_table_valid[0]) * 8)] & (1ULL << (phy_id % (sizeof(phy_map_table_valid[0]) * 8)));
        }
        void set_visible(uint32_t phy_id,bool v){
            assert(phy_id < phy_reg_num);
            if(v){
                phy_map_table_visible[phy_id / (sizeof(phy_map_table_visible[0]) * 8)] |= 1ULL << (phy_id % (sizeof(phy_map_table_visible[0]) * 8));
            }else{
                phy_map_table_visible[phy_id / (sizeof(phy_map_table_visible[0]) * 8)] &= ~(1ULL << (phy_id % (sizeof(phy_map_table_visible[0]) * 8)));
            }
        }
        bool get_visible(uint32_t phy_id){
            assert(phy_id < phy_reg_num);
            return phy_map_table_visible[phy_id / (sizeof(phy_map_table_visible[0]) * 8)] & (1ULL << (phy_id % (sizeof(phy_map_table_visible[0]) * 8)));
        }
        public:
            rat(uint32_t phy_reg_num,uint32_t arch_reg_num) : phy_reg_num(phy_reg_num),arch_reg_num(arch_reg_num){
                bitmap_size = (phy_reg_num + sizeof(phy_map_table_valid[0]) * 8 - 1) / (sizeof(phy_map_table_valid[0]) * 8);
                phy_map_table = new uint32_t[phy_reg_num];
                phy_map_table_valid = new uint32_t[bitmap_size];
                phy_map_table_visible = new uint32_t[bitmap_size];
                for(uint32_t i = 0;i < PHY_REG_NUM;i++){
                    freelist.push(i);
                }
            }
            void reset(){
                memset(phy_map_table,0,sizeof(uint32_t) * phy_reg_num);
                memset(phy_map_table_valid,0,sizeof(uint32_t) * bitmap_size);
                memset(phy_map_table_visible,0,sizeof(uint32_t) * bitmap_size);
                freelist.flush();

                /*第一次测试用，后面可删去*/
                for(uint32_t i = 0;i < ARCH_REG_NUM;i++){
                    set_valid(i,true);
                    set_visible(i,true);
                    phy_map_table[i] = i;
                }
                for(uint32_t i = 0;i < PHY_REG_NUM;i++){
                    freelist.push(i);
                }
            }
            
            void save(checkpoint_t &cp){
                memcpy(cp.rat_phy_map_table_valid,phy_map_table_valid,sizeof(cp.rat_phy_map_table_valid));
                memcpy(cp.rat_phy_map_table_visible,phy_map_table_visible,sizeof(cp.rat_phy_map_table_visible));
                memcpy(cp.rat_phy_map_table,phy_map_table,sizeof(cp.rat_phy_map_table));
            }
            void restore(checkpoint_t &cp){
                memcpy(phy_map_table_valid,cp.rat_phy_map_table_valid,sizeof(cp.rat_phy_map_table_valid));
                memcpy(phy_map_table_visible,cp.rat_phy_map_table_visible,sizeof(cp.rat_phy_map_table_visible));
            }            

            void restore_map(uint32_t new_phy_id,uint32_t old_phy_id){
                assert(new_phy_id < phy_reg_num);
                assert(old_phy_id < phy_reg_num);
                assert(get_valid(new_phy_id));
                assert(get_valid(old_phy_id));
                assert(get_visible(new_phy_id));
                assert(!get_visible(old_phy_id));
                phy_map_table[new_phy_id] = 0;
                set_valid(new_phy_id,false);
                set_visible(new_phy_id,false);
                set_valid(old_phy_id,true);
                set_visible(old_phy_id,true);
            }

            void release_map(uint32_t phy_id){
                assert(phy_id < phy_reg_num);
                assert(get_valid(phy_id));
                assert(!get_visible(phy_id));
                phy_map_table[phy_id] = 0;
                set_valid(phy_id,false);
            }

            uint32_t get_free_phy_id(uint32_t num,uint32_t *ret){
                uint32_t ret_cnt = 0;
                for(uint32_t i = 1;i < phy_reg_num;i++){
                    if(!get_valid(i)){
                        ret[ret_cnt++] = i;
                        if(ret_cnt >= num)
                            break; 
                    }
                }
                return ret_cnt;
            }

            bool get_phy_id(uint32_t arch_id,uint32_t *phy_id){
                int cnt = 0;
                assert((arch_id >0) && (arch_id < arch_reg_num));
                for(uint32_t i = 0;i < phy_reg_num;i++){
                    if(get_valid(i) && get_visible(i) && (phy_map_table[i] == arch_id)){
                        *phy_id = i;
                        cnt++;
                    }
                }
                assert(cnt <= 1);
                return cnt == 1;
            }
            
            uint32_t get_arch_id(uint32_t phy_id){
                return phy_map_table[phy_id];
            }
            
            uint32_t set_map(uint32_t arch_id,uint32_t phy_id){
                uint32_t old_phy_id;
                assert(phy_id < phy_reg_num);
                assert((arch_id > 0) && (arch_id < arch_reg_num));
                assert(!get_valid(phy_id));

                bool ret = get_phy_id(arch_id,&old_phy_id);

                phy_map_table[phy_id] = arch_id;
                set_valid(phy_id,true);
                set_visible(phy_id,true);

                if(ret){
                    set_visible(old_phy_id,false);
                }
                return old_phy_id;
            }

            void restore_map_sync(uint32_t new_phy_id,uint32_t old_phy_id){
                sync_request req_t;
                req_t.req = sync_request_type::restore_map;
                req_t.arg1 = new_phy_id;
                req_t.arg2 = old_phy_id;
                sync_request_q.push(req_t);
            }
            void set_map_sync(uint32_t arch_id,uint32_t phy_id){
                sync_request sync_request_t;
                sync_request_t.req = sync_request_type::set_map;
                sync_request_t.arg1 = arch_id;
                sync_request_t.arg2  = phy_id;
                sync_request_q.push(sync_request_t);
            }

            void restore_sync(checkpoint_t &cp){
                sync_request req_t;
                req_t.req = sync_request_type::restore;
                req_t.cp = cp;
                sync_request_q.push(req_t);
            }
            
            void release_map_sync(uint32_t phy_id){
                sync_request req_t;
                req_t.req = sync_request_type::release_map;
                req_t.arg1 = phy_id;

                sync_request_q.push(req_t);
            }

            void sync(){
                sync_request req_t;
                while(!sync_request_q.empty()){
                    req_t = sync_request_q.front();
                    sync_request_q.pop();
                    switch(req_t.req){
                        case sync_request_type::set_map:{
                            set_map(req_t.arg1,req_t.arg2);
                            break;
                        }
                        case sync_request_type::restore_map:{
                            restore_map(req_t.arg1,req_t.arg2);
                            break;
                        }
                        case sync_request_type::restore:{
                            restore(req_t.cp);
                            break;
                        }
                        case sync_request_type::release_map:{
                            release_map(req_t.arg1);
                            break;
                        }
                    }
                }
            }

            /*以下cp相关函数是为了rename阶段除第一条指令外的分支指令checkpoint使用，由于同一周期rename的指令set_map下一周期才写入rat
            *因此需要一个旁路将同周期其它指令的映射关系写入cp中
            */
            void cp_set_valid(checkpoint_t &cp,uint32_t phy_id,bool v){
                assert(phy_id < phy_reg_num);

                if(v){
                    cp.rat_phy_map_table_valid[phy_id / BITSIZE(cp.rat_phy_map_table_valid[0])] |= 1ULL << (phy_id % BITSIZE(cp.rat_phy_map_table_valid[0]));
                }else{
                    cp.rat_phy_map_table_valid[phy_id / BITSIZE(cp.rat_phy_map_table_valid[0])] &= ~(1ULL << (phy_id % BITSIZE(cp.rat_phy_map_table_valid[0])));
                }
            }
            bool cp_get_valid(checkpoint_t &cp,uint32_t phy_id){
                assert(phy_id < phy_reg_num);
                return cp.rat_phy_map_table_valid[phy_id / BITSIZE(cp.rat_phy_map_table_valid)] & (1ULL << (phy_id % BITSIZE(cp.rat_phy_map_table_valid)));
            }
            void cp_set_visible(checkpoint_t &cp,uint32_t phy_id,bool v){
                assert(phy_id < phy_reg_num);

                if(v){
                    cp.rat_phy_map_table_visible[phy_id / BITSIZE(cp.rat_phy_map_table_visible[0])] |= 1ULL << (phy_id % BITSIZE(cp.rat_phy_map_table_visible[0]));
                }else{
                    cp.rat_phy_map_table_visible[phy_id / BITSIZE(cp.rat_phy_map_table_visible[0])] &= ~(1ULL << (phy_id % BITSIZE(cp.rat_phy_map_table_visible[0])));
                }
            }
            bool cp_get_visible(checkpoint_t &cp,uint32_t phy_id){
                assert(phy_id < phy_reg_num);
                return cp.rat_phy_map_table_visible[phy_id / BITSIZE(cp.rat_phy_map_table_visible[0])] & (1ULL << (phy_id % BITSIZE(cp.rat_phy_map_table_visible[0])));
            }
            bool cp_get_phy_id(checkpoint_t &cp,uint32_t arch_id,uint32_t *phy_id){
                int cnt = 0;
                assert((arch_id > 0) && (arch_id < arch_reg_num));

                for(uint32_t i = 0;i < phy_reg_num;i++){
                    if(cp_get_valid(cp,i) && cp_get_visible(cp,i) && (cp.rat_phy_map_table[i] == arch_id)){
                        *phy_id = i;
                        cnt++;
                    }
                }
                assert(cnt <= 1);
                return cnt == 1;
            }
            void cp_set_map(checkpoint_t &cp,uint32_t arch_id,uint32_t phy_id){
                uint32_t old_phy_id;
                assert(phy_id < phy_reg_num);
                assert((arch_id > 0) && (arch_id < arch_reg_num));
                assert(!cp_get_valid(cp,phy_id));
                bool ret = cp_get_phy_id(cp,arch_id,&old_phy_id);
                cp.rat_phy_map_table[phy_id] = arch_id;
                cp_set_valid(cp,phy_id,true);
                cp_set_visible(cp,phy_id,true);
                if(ret){
                    cp_set_visible(cp,old_phy_id,false);
                }
            }
            void cp_release_map(checkpoint_t &cp,uint32_t phy_id){
                assert(phy_id < phy_reg_num);
                assert(cp_get_valid(cp,phy_id));
                assert(!cp_get_visible(cp,phy_id));
                cp_set_valid(cp,phy_id,false);
            }
    };
}