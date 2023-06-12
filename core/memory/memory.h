/*
 * @Author: 苗金标
 * @Date: 2023-04-13 20:16:43
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-04-13 20:19:11
 * @Description: 
 */
#pragma once
class memory{
    public:
        virtual bool do_read(unsigned long start_addr,unsigned long size,unsigned char* buffer) = 0;
        virtual bool do_write(unsigned long start_addr,unsigned long size,const unsigned char* buffer) = 0;
};
