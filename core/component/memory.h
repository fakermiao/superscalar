/*
 * @Author: 苗金标
 * @Date: 2023-04-03 20:09:26
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-04-17 14:58:39
 * @Description: 
 */

#ifndef MEMORY_H
#define MEMORY_H
#include "../common.h"
namespace component{
    class memory{
        public:
            uint8_t *mem;
            uint64_t base;
            uint64_t size;
            bool test_mode;
            bool has_error;

            public:
            memory(uint32_t base,uint64_t size){
                mem = new uint8_t[size]();
                this->base = base;
                this->size = size;
                test_mode  = false;
                has_error  = false;
            }
            ~memory(){ delete[] mem;}

            void write(uint64_t addr,void *buffer,uint8_t len){
                switch(len){
                    case 1:
                        mem[addr - base] = *(uint8_t *)buffer;
                        break;
                    case 2:
                        *(uint16_t *)(mem + addr - base) = *(uint16_t *)buffer;
                        break;
                    case 4:
                        *(uint32_t *)(mem + addr - base) = *(uint32_t *)buffer;
                        break;
                    case 8:
                        *(uint64_t *)(mem + addr - base) = *(uint64_t *)buffer;
                        break;
                    default:break;
                }
            }

            void read(uint64_t addr,void *buffer,uint8_t len){
                switch(len){
                    case 1:
                        *(uint8_t *)buffer = mem[addr - base];
                        break;
                    case 2:
                        *(uint16_t *)buffer = *(uint16_t *)(mem + addr - base);
                        break;
                    case 4:
                        *(uint32_t *)buffer = *(uint32_t *)(mem + addr - base);
                        break;
                    case 8:
                        *(uint64_t *)buffer = *(uint64_t *)(mem + addr - base);
                        break;
                    default:
                        break;
                }
            }
            void write8(uint32_t addr, uint8_t value)
            {

                    mem[addr - this->base] = value;
            }


            void write16(uint32_t addr, uint16_t value)
            {
                    *(uint16_t *)(mem + (addr - this->base)) = value;
            }


            void write32(uint32_t addr, uint32_t value)
            {
                    *(uint32_t *)(mem + (addr - this->base)) = value;
            }


            uint8_t read8(uint32_t addr)
            {
                    return mem[addr - this->base];
            }

            uint16_t read16(uint32_t addr)
            {
                    return *(uint16_t *)(mem + (addr - this->base));
            }

            uint32_t read32(uint32_t addr)
            {
                    return *(uint32_t *)(mem + (addr - this->base));
            }

    };
}
#endif