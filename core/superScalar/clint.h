/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:39:09
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-26 10:39:10
 * @Description: 
 */
#pragma once
#include "../memory/memory.h"
#include <cstdint>

template <unsigned int nr_hart=1>
class Clint : public memory{
    public:
        Clint(){
            mtime = 0;
            for(unsigned int i = 0;i < nr_hart;i++){
                mtimecmp[i] = 0;
                msip[i] = 0;
            }
        }

        bool do_read(unsigned long start_addr,unsigned long size,unsigned char*buffer){
            if(start_addr >= 0x4000){//mtimecmp、mtime
                if(start_addr >= 0xbff8 && start_addr + size <= 0xc000){//mtime
                    memcpy(buffer,((char*)(&mtime))+start_addr-0xbff8,size);
                }else if(start_addr >= 0x4000 && start_addr + size <= 0x4000 + 8 * nr_hart){//mtimecmp
                    memcpy(buffer,((char*)(&mtimecmp))+start_addr-0x4000,size);
                }else return false;
            }else{//msip
                if(start_addr + size <= 4 * nr_hart){
                    memcpy(buffer,((char*)(&msip))+start_addr,size);
                }else return false;
            }
            return true;
        }

        bool do_write(unsigned long start_addr,unsigned long size,const unsigned char* buffer){
            if(start_addr >= 0x4000){//mtimecmp、mtime
                if(start_addr >= 0xbff8 && start_addr + size <= 0xc000){//mtime
                    memcpy(((char*)(&mtime))+start_addr-0xbff8,buffer,size);
                }else if(start_addr >= 0x4000 && start_addr+size <= 0x4000+8*nr_hart){
                    memcpy(((char*)(&mtimecmp))+start_addr-0x4000,buffer,size);
                }
                else return false;
            }else{//msip
                if(start_addr + size <= 4*nr_hart){
                    memcpy(((char*)(&msip))+start_addr,buffer,size);
                    for(unsigned int i=0;i<nr_hart;i++) msip[i] &= 1;
                }
                else return false;
            }
            return true;
        }

        void tick(){
            mtime += 10;
        }
        void set_time(uint64_t x){
            mtime = x;
        }

        bool m_s_irq(unsigned int hart_id){//machine software irq
            if(hart_id >= nr_hart) assert(false);
            else return (msip[hart_id] & 1);
        }
        bool m_t_irq(unsigned int hart_id){//machine timer irq
            if(hart_id >= nr_hart) assert(false);
            else return (mtime > mtimecmp[hart_id]);
        }
        void set_cmp(uint64_t new_val){
            mtimecmp = new_val;
        }
    private:
        uint64_t mtime;
        uint64_t mtimecmp[nr_hart];
        uint32_t msip[nr_hart];
};