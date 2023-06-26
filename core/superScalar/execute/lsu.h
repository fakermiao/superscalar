/*
 * @Author: 苗金标
 * @Date: 2023-05-31 14:27:32
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 14:28:32
 * @Description: 
 */
#pragma once
#include "../../common.h"
#include "../execute.h"
#include "../wb.h"
#include "priv.h"
#include "../../component/store_buffer.h"
#include "../../color.h"

/*
原子指令的写回寄存器如sc要在提交阶段写回内存时才能够确定
*/
namespace Supercore{
    class lsu{
        public:
            uint32_t num;
            component::fifo<instStr> *issue_lsu_fifo;
            component::fifo<instStr> *lsu_wb_fifo;
            Priv& priv;
            component::store_buffer *store_buffer;

            lsu(uint32_t num,component::fifo<instStr> *issue_lsu_fifo,component::fifo<instStr> *lsu_wb_fifo,Priv& priv,
                component::store_buffer *store_buffer) : num(num),issue_lsu_fifo(issue_lsu_fifo),lsu_wb_fifo(lsu_wb_fifo),
                priv(priv),store_buffer(store_buffer){}
            execute_channel evaluate(wb_feedback_pack wb_feedback);

            rv_exc_code mem_read(uint64_t start_addr,uint64_t size,uint8_t *buffer){
                if(start_addr % size != 0)
                return exc_load_misalign;
                return priv.va_read(start_addr,size,buffer);
            }
            rv_exc_code mem_write(uint64_t start_addr,uint64_t size,uint8_t *buffer){
                if(start_addr % size != 0)
                    return exc_store_misalign;
                return priv.va_write(start_addr,size,buffer);
            }
    
            void lsu_p(instStr& instInfo){
                if(cpu.cycle >= DIFFTEST_CYC)
                    printf("%slsu%s%d/%ld%s:\t\tpc:%lx,inst:%x\n",FORMATFETCH,FORMATCLC,num,cpu.cycle,FORMATEND,instInfo.pc,instInfo.inst);
            }
    };


}