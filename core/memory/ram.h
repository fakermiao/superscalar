/*
 * @Author: 苗金标
 * @Date: 2023-04-17 15:00:21
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-04-17 15:22:58
 * @Description: 
 */
#pragma once
#include "memory.h"
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <assert.h>
#include <string.h>
class ram: public memory{
    public:
        ram(unsigned long size_bytes,unsigned long mbase){
            mem = new unsigned char[size_bytes];
            mem_size = size_bytes;
            base = mbase;
        }
        ram(unsigned long size_bytes,unsigned long mbase,const unsigned char *init_binary,unsigned long init_binary_len):ram(size_bytes,mbase){
            assert(init_binary_len <= size_bytes);
            memcpy(mem,init_binary,init_binary_len);
        }
        ram(unsigned long size_bytes,unsigned long mbase,const char *init_file):ram(size_bytes,mbase){
            unsigned long file_size = std::filesystem::file_size(init_file);
            if(file_size > mem_size){
                std::cerr << "ram size is not big enough for init file." << std::endl;
                file_size = size_bytes;
            }
            std::ifstream file(init_file,std::ios::in | std::ios::binary);
            file.read((char*)mem,file_size);
        }
        bool do_read(unsigned long start_addr,unsigned long size,unsigned char* buffer){
            if(start_addr + size <= mem_size){
                memcpy(buffer,&mem[start_addr + base],size);
                return true;
            }
            else return false;
        }
        bool do_write(unsigned long start_addr,unsigned long size,const unsigned char* buffer){
            if(start_addr <= mem_size){
                memcpy(&mem[start_addr + base],buffer,size);
                return true;
            }
            else return false;
        }
    private:
        unsigned char *mem;
        unsigned long mem_size;
        unsigned long base;
};