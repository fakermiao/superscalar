/*
 * @Author: 苗金标
 * @Date: 2023-04-28 16:01:02
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 17:25:26
 * @Description:  
 * lzq的dreamcore乱序发射顺序执行而且是单周期执行，这于顺序流水线类似，不需要设置valid位，只
 * 需要判断exe、wb阶段前递数据即可，但实际应该像tomulaso算法一样考虑物理寄存器是否可用，
 * 使用空闲列表物理寄存器堆数量多于流水线中的指令数不用考虑冲突
 * 只要顺序判断读取和写入物理寄存器堆不存在太大逻辑问题，比如wb阶段释放物理寄存器，后续重新分配，这样放入
 * 发射队列之前的还会将该物理寄存器无效化
 * 后续有bug再处理
 * 
 * 涉及到多个流水线阶段使用可能需要sync处理
 */
#pragma once

#include "../common.h"
#include "checkpoint_buffer.h"

namespace component{
    template<typename T>
    class regfile{
        public:
            T *reg_data;
            uint64_t *reg_data_valid;
            uint32_t size;
            uint32_t bitmap_size;

            enum class sync_request_type{
                write,set_valid,restore
            };
            typedef struct sync_request{
                sync_request_type req;
                uint32_t arg1;
                T arg2;
                bool arg3;
                checkpoint_t cp;
            }sync_request;

            std::queue<sync_request> sync_request_q;
            regfile(uint32_t size) : size(size){
                reg_data = new T[size];
                bitmap_size = (size + (sizeof(reg_data_valid[0]) * 8) - 1) / (sizeof(reg_data_valid[0]) - 1);
                                // phy_map_table_valid = new uint32_t[bitmap_size];
                reg_data_valid = new uint64_t[bitmap_size];
            }
            void reset(){
                for(uint32_t i = 0;i < size;i++){
                    reg_data[i] = 0;
                    set_valid(i,true);
                }
            }
            ~regfile(){
                delete[] reg_data;
                delete[] reg_data_valid;
            }

            void save(checkpoint_t &cp){
                memcpy(cp.phy_regfile_data_valid,reg_data_valid,sizeof(cp.phy_regfile_data_valid));
            }
            void write(uint32_t addr,T value,bool valid){
                assert(addr < size);
                reg_data[addr] = value;
                if(valid){
                    reg_data_valid[addr / (sizeof(reg_data_valid[0]) * 8)] |= 1ull << (addr % (sizeof(reg_data_valid[0]) * 8));
                }else{
                    reg_data_valid[addr / (sizeof(reg_data_valid[0]) * 8)] &= ~(1ull << (addr % (sizeof(reg_data_valid[0]) * 8)));
                }
            }
            void set_valid(uint32_t addr,bool valid){
                assert(addr < size);
                if(valid){
                    reg_data_valid[addr / (sizeof(reg_data_valid[0]) * 8)] |= 1ull << (addr % (sizeof(reg_data_valid[0]) * 8));
                }else{
                    reg_data_valid[addr / (sizeof(reg_data_valid[0]) * 8)] &= ~(1ull << (addr % (sizeof(reg_data_valid[0]) * 8)));
                }
            }
            T read_data(uint32_t addr){
                assert(addr < size);
                return reg_data[addr];
            }

            bool read_data_valid(uint32_t addr){
                assert(addr < size);
                // printf("read_data_valid\taddr:%d,result:%lx\n",addr,reg_data_valid[addr / (sizeof(reg_data_valid[0]) * 8)]);
                return (reg_data_valid[addr / (sizeof(reg_data_valid[0]) * 8)] & (1ull << (addr % (sizeof(reg_data_valid[0]) * 8))));
            }

            void write_sync(uint32_t addr,T value,bool valid){
                sync_request req_t;
                req_t.req = sync_request_type::write;
                req_t.arg1 = addr;
                req_t.arg2 = value;
                req_t.arg3 = valid;
                sync_request_q.push(req_t);
            }
            void set_valid_sync(uint32_t addr,bool valid){
                sync_request req_t;
                req_t.req  = sync_request_type::set_valid;
                req_t.arg1 = addr;
                req_t.arg3 = valid;
                sync_request_q.push(req_t);
            }
            void restore(checkpoint_t &cp){
                memcpy(reg_data_valid,cp.phy_regfile_data_valid,sizeof(cp.phy_regfile_data_valid));
            }
            void restore_sync(checkpoint_t &cp){
                sync_request req_t;
                req_t.req = sync_request_type::restore;
                req_t.cp = cp;
                sync_request_q.push(req_t);
            }
            void sync(){
                sync_request req_t;
                while(!sync_request_q.empty()){
                    req_t = sync_request_q.front();
                    sync_request_q.pop();
                    switch(req_t.req){
                        case sync_request_type::write:{
                            this->write(req_t.arg1,req_t.arg2,req_t.arg3);
                            break;
                        }
                        case sync_request_type::set_valid:{
                            this->set_valid(req_t.arg1,req_t.arg3);
                            break;
                        }
                        case sync_request_type::restore:{
                            this->restore(req_t.cp);
                            break;
                        }
                    }
                }
            }

            //为了分支预测失败时的恢复，此时应该只保留真值映射
            bool cp_get_data_valid(checkpoint_t &cp,uint32_t addr){
                assert(addr < size);
                return (cp.phy_regfile_data_valid[addr / BITSIZE(cp.phy_regfile_data_valid[0])] & (1ULL << (addr % BITSIZE(cp.phy_regfile_data_valid[0])))) != 0;
            }

            void cp_set_data_valid(checkpoint_t &cp,uint32_t addr,bool valid){
                assert(addr < size);
                if(valid){
                    cp.phy_regfile_data_valid[addr / BITSIZE(cp.phy_regfile_data_valid[0])] |= 1ULL << (addr % BITSIZE(cp.phy_regfile_data_valid[0]));
                }else{
                    cp.phy_regfile_data_valid[addr / BITSIZE(cp.phy_regfile_data_valid[0])] &= ~(1ULL << (addr % BITSIZE(cp.phy_regfile_data_valid[0])));
                }
            }
    };
}
