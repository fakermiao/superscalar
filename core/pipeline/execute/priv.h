/*
 * @Author: 苗金标
 * @Date: 2023-04-06 10:40:16
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-24 10:09:47
 * @Description: 
 */
/*
处理特权和访存指令
访存：
    lb,lh,lw,ld,lbu,lhu,lwu,sb,sh,sw,sd,
    lr ,sc ,amoswap ,amoadd ,amoxor ,amoand ,amoxor ,amoand ,amoor ,amomin ,amomax ,amominu ,amomaxu,
    lrw,scw,amoswapw,amoaddw,amoxorw,amoandw,amoxorw,amoandw,amoorw,amominw,amomaxw,amominuw,amomaxuw
csr：
    ecall,ebreak,sret,wfi,mret,sfence_vma,csrrw,csrrs,csrrc,csrrwi,
    csrrsi,csrrci
mou：
    fence,fencei,sfence_vma

暂时设置为正常读写csr不刷新流水线，ecall、ebreak等情况要刷新流水线

ip & ie register:
    bits meip & meie are the interrupt-pending and interrupt-enable bits for machine-level external interrupts
    bits mtip & mtie are the intertupt-pending and interrupt-enable bits for machine timer interrupts
    bits msip & msie are the interrupt-pending and interrupt-enable bits for machine-level software interrupts.
    bits seip & seie are the interrupt-pending and interrupt-enable bits for supervisor-level external interrupts.
*/
#pragma once

#include "../../common.h"
#include "../../component/memory.h"
#include "priv_common.h"
#include "../rv_sv39.h"
#include "../../cache/l1_i_cache.h"
#include "../../cache/l1_d_cache.h"

static bool riscv_test = true;

class Priv{
    public:
        Priv(component::memory *mem,uint64_t hart_id,l2_cache<L2_WAYS,L2_NR_SETS,L2_SZLINE,32> &l2):mem(mem),hart_id(hart_id),l2(l2),i_tlb(l2,hart_id),
        d_tlb(l2,hart_id),l1i(&l2,hart_id),l1d(&l2,hart_id)
        {
            reset();
        }
        void reset(){
            trap_pc = 0;
            cur_need_trap = false;
            cur_priv = M_MODE;
            next_priv = M_MODE;
            status = 0;
            csr_mstatus_def *mstatus = (csr_mstatus_def *)&status;
            mstatus->sxl = 2;
            mstatus->uxl = 2;

            csr_misa_def *isa = (csr_misa_def *)&misa;
            isa->ext = rv_ext('i') | rv_ext('m') | rv_ext('a') | rv_ext('s') | rv_ext('u');
            isa->blank = 0;
            isa->mxl = 2;

            medeleg = 0;
            mideleg = 0;
            ie = 0;
            mtvec = 0;
            mcounteren = 0;
            ip = 0;
            mcycle = 0;
            stvec = 0;
            sscratch = 0;
            sepc = 0;
            scause = 0;
            stval = 0;
            satp = 0;
            scounteren = 0;

            pmpaddr0 = 0;
            pmpcfg0 = 0;
        }

        void inter_exec(bool meip,bool msip,bool mtip,bool seip){
            mcycle++;
            int_def *ip_bits = (int_def*)&ip;

            ip_bits->m_e_ip = meip;
            ip_bits->m_s_ip = msip;
            ip_bits->m_t_ip = mtip;
            ip_bits->s_e_ip = seip;
            // cur_need_trap = false;
            // return check_and_raise_int();
        }
        /*tval存放pc或指针，是异常/中断相关信息*/
        void raise_trap(uint64_t cur_pc,csr_cause_def cause,uint64_t tval = 0){
            assert(!cur_need_trap);
            cur_need_trap = true;
            bool trap_to_s = false;
            // printf("PRIV raise trap\tmtvec:%lx\n",mtvec);
            /*中断/异常默认自陷到m态执行，通过mideleg、medeleg设置来委托到s态处理*/
            if(cur_priv != M_MODE){
                // printf("TRAP_PC\t\n");
                if(cause.interrupt){
                    // printf("interrupt--mideleg:%lx,cause:%lx\n",mideleg,cause.cause);
                    if(mideleg & (1 << cause.cause)) trap_to_s = true;
                }else{
                    // printf("execption--medeleg:%lx,cause:%lx\n",medeleg,cause.cause);
                    if(medeleg & (1 << cause.cause)) trap_to_s = true;
                }
            }

            /*进入中断操作*/
            if(trap_to_s){
                stval = tval;
                scause = *((uint64_t *)&cause);
                sepc   = cur_pc;
                csr_sstatus_def *sstatus = (csr_sstatus_def*)&status;
                sstatus->spie = sstatus->sie;
                sstatus->sie  = 0;
                sstatus->spp  = cur_priv;
                csr_tvec_def *tvec = (csr_tvec_def*)&stvec;
                trap_pc = (tvec->base << 2) + ((tvec->mode == 1) ? (cause.cause) * 4 : 0);
                cur_priv = S_MODE;
            }else{
                mtval = tval;
                mcause = *((uint64_t*)&cause);
                mepc = cur_pc;
                csr_mstatus_def *mstatus = (csr_mstatus_def*)&status;
                mstatus->mpie = mstatus->mie;
                mstatus->mie  = 0;
                mstatus->mpp  = cur_priv;
                csr_tvec_def *tvec = (csr_tvec_def*)&mtvec;
                trap_pc = (tvec->base << 2) + (tvec->mode ? (cause.cause) * 4 : 0);
                cur_priv = M_MODE;
            }
            if(cause.cause == exc_instr_pgfault && tval == trap_pc) {printf("trap_pc:%lx,tval:%lx,pc:%lx\n",trap_pc,tval,cur_pc); assert(false);}
        }
        
        void post_exc(){
            if(!cur_need_trap){
                minstret ++;
            }
            cur_need_trap = false;
            csr_mstatus_def *mstatus = (csr_mstatus_def *)&status;
            assert(mstatus->blank0 == 0);
            assert(mstatus->blank1 == 0);
            assert(mstatus->blank2 == 0);
            assert(mstatus->blank4 == 0);
            // cur_priv = next_priv;
        }
        uint64_t get_trap_pc(){
            return trap_pc;
        }
        uint64_t get_mtvec(){
            return mtvec;
        }
        bool need_trap(){
            return cur_need_trap;
        }

        /*
        csr寄存器高4为是权限信息，8、9两位是特权级相关，10、11读写权限相关
        */
        bool check_op_permission_check(uint16_t csr_index,bool write){
            if( ((csr_index >> 8) & 3) > cur_priv) return false;
            if( (((csr_index >> 10) & 3) == 3) && write) return false;
            return true;
        }
        //寄存器不存在时返回false，否则返回true
        bool csr_read(uint16_t csr_index,uint64_t &csr_result){
            switch(static_cast<rv_csr_addr>(csr_index & 0xfff)){
                case csr_mvendorid:
                    csr_result = 0;
                    break;
                case csr_marchid:
                    csr_result = 0;
                    break;
                case csr_mimpid:
                    csr_result = 0;
                    break;
                case csr_mhartid:
                    csr_result = 0;
                    break;
                case csr_mconfigptr:
                    csr_result = 0;
                    break;
                case csr_mstatus:
                    csr_result = status;
                    break;
                case csr_misa:
                    csr_result = misa;
                    break;
                case csr_medeleg:
                    csr_result = medeleg;
                    break;
                case csr_mideleg:
                    csr_result = mideleg;
                    break;
                case csr_mie:
                    csr_result = ie;
                    break;
                case csr_mtvec:
                    csr_result = mtvec;
                    break;
                case csr_mcounteren:
                    csr_result = mcounteren;
                    break;
                case csr_mscratch:
                    csr_result = mscratch;
                    break;
                case csr_mepc:
                    csr_result = mepc;
                    break;
                case csr_mcause:
                    csr_result = mcause;
                    break;
                case csr_mtval:
                    csr_result = mtval;
                    break;
                case csr_mip:
                    csr_result = ip;
                    break;
                case csr_mcycle:
                    csr_result = mcycle;
                    break;
                case csr_minstret:
                    csr_result = minstret;
                    break;
                /************
                 cemu not implement
                ***********/
                // case csr_instret:
                //     csr_result = minstret;
                //     break;
                case csr_sstatus:{
                    csr_result = 0;
                    csr_sstatus_def *sstatus = (csr_sstatus_def *)&csr_result;
                    csr_mstatus_def *mstatus = (csr_mstatus_def *)&status;
                    sstatus->sie    = mstatus->sie;
                    sstatus->spie   = mstatus->spie;
                    sstatus->ube    = mstatus->ube;
                    sstatus->spp    = mstatus->spp;
                    sstatus->vs     = mstatus->vs;
                    sstatus->fs     = mstatus->fs;
                    sstatus->xs     = mstatus->xs;
                    sstatus->sum    = mstatus->sum;
                    sstatus->mxr    = mstatus->mxr;
                    sstatus->uxl    = mstatus->uxl;
                    sstatus->sd     = mstatus->sd;
                    break;
                }
                case csr_sie:{
                    csr_result = ie & s_int_mask;
                    break;
                }
                case csr_stvec:{
                    csr_result = stvec;
                    break;
                }
                case csr_scounteren:{
                    csr_result = scounteren;
                    break;
                }
                case csr_sscratch:{
                    csr_result = sscratch;
                    break;
                }
                case csr_sepc:{
                    csr_result = sepc;
                    break;
                }
                case csr_scause:{
                    csr_result = scause;
                    break;
                }
                case csr_stval:{
                    csr_result = stval;
                    break;
                }
                case csr_sip:{
                    csr_result = ip & s_int_mask;
                    break;
                }
                case csr_satp:{
                    const csr_mstatus_def *mstatus = (csr_mstatus_def *)&status;
                    if(cur_priv == S_MODE && mstatus->tvm) return false;
                    csr_result = satp;
                    break;
                }
                case csr_cycle : {
                    csr_counteren_def *mcen = (csr_counteren_def*)&mcounteren;
                    csr_counteren_def *scen = (csr_counteren_def*)&scounteren;
                    if(cur_priv <= S_MODE && (!mcen->cycle || !scen->cycle)) return false;
                    csr_result = mcycle;
                    break;
                }
                case csr_tselect:{
                    csr_result = 1;
                    break;
                }
                case csr_tdata1:
                    csr_result = 0;
                    break;
                /*pmpaddr、pmpcfg寄存器的实现是可选的，完成riscv-tests测试后可删除*/
                case csr_pmpaddr0:
                    csr_result = pmpaddr0;
                    break;
                case csr_pmpcfg0:
                    csr_result = pmpcfg0;
                    break;
                default:
                    return false;
            }
            return true;
        }

        bool csr_write(uint16_t csr_index,uint64_t csr_data){
            switch(static_cast<rv_csr_addr>(csr_index & 0xfff)){
                
                case csr_mstatus:{
                    csr_mstatus_def *nstatus = (csr_mstatus_def *)&csr_data;
                    csr_mstatus_def *mstatus = (csr_mstatus_def *)&status;
                    mstatus->sie    = nstatus->sie;
                    mstatus->mie    = nstatus->mie;
                    mstatus->spie   = nstatus->spie;
                    mstatus->mpie   = nstatus->mpie;
                    assert(mstatus->spie != 2);
                    assert(mstatus->mpie != 2);
                    mstatus->spp    = nstatus->spp;
                    mstatus->mpp    = nstatus->mpp;
                    mstatus->mprv   = nstatus->mprv;
                    mstatus->sum    = nstatus->sum;
                    mstatus->mxr    = nstatus->mxr;
                    mstatus->tvm    = nstatus->tvm;
                    mstatus->tvm    = nstatus->tvm;
                    mstatus->tw     = nstatus->tw;
                    mstatus->tsr    = nstatus->tsr;
                    break;
                }
                case csr_misa:{
                    break;
                }
                case csr_medeleg:
                    medeleg = csr_data & s_exc_mask;
                    break;
                case csr_mideleg:
                    mideleg = csr_data & s_int_mask;
                    break;
                case csr_mie:
                    ie      = csr_data & m_int_mask;
                    break;
                case csr_mtvec:{
                    csr_tvec_def *tvec = (csr_tvec_def *)&csr_data;
                    assert(tvec->mode <= 1);
                    mtvec = csr_data;
                    break;
                }
                case csr_mcounteren:{
                    mcounteren = csr_data & counter_mask;
                    break;
                }
                case csr_mscratch:{
                    mscratch = csr_data;
                    break;
                }
                case csr_mepc:
                    mepc = csr_data;
                    break;
                case csr_mcause:
                    mcause = csr_data;
                    break;
                case csr_mtval:
                    mtval = csr_data;
                    break;
                case csr_mip:
                    ip = csr_data & m_int_mask;
                    break;
                case csr_mcycle:
                    mcycle = csr_data;
                    break;
                case csr_sstatus:{
                    csr_sstatus_def *nstatus = (csr_sstatus_def *)&csr_data;
                    csr_sstatus_def *sstatus = (csr_sstatus_def *)&status;
                    sstatus->sie = nstatus->sie;
                    sstatus->spie = nstatus->spie;
                    assert(sstatus->spie != 2);
                    sstatus->spp = nstatus->spp;
                    sstatus->sum = nstatus->sum;
                    sstatus->mxr = nstatus->mxr;
                    break;
                }
                case csr_sie:
                    ie = (ie & (~s_int_mask)) | (csr_data & s_int_mask);
                    break;
                case csr_stvec:{
                    csr_tvec_def *tvec = (csr_tvec_def *)&csr_data;
                    assert(tvec->mode <= 1);
                    stvec = csr_data;
                    // printf("csr_data:%lx\n",csr_data);
                    // assert(csr_data != 0x8020014c);
                    // assert(0);
                    break;
                }
                case csr_scounteren:
                    scounteren = csr_data & counter_mask;
                    break;
                case csr_sscratch:
                    sscratch = csr_data;
                    break;
                case csr_sepc:
                    sepc = csr_data;
                    break;
                case csr_scause:
                    scause = csr_data;
                    break;
                case csr_stval:
                    stval = csr_data;
                    break;
                case csr_sip:
                    ip = (ip & (~s_int_mask)) | (csr_data & s_int_mask);
                    break;
                case csr_satp:{
                    const csr_mstatus_def *mstatus = (csr_mstatus_def *)&status;
                    if(cur_priv == S_MODE && mstatus->tvm) return false;
                    satp_def *satp_reg = (satp_def *)&csr_data;
                    if(satp_reg->mode != 0 && satp_reg->mode != 8) satp_reg->mode = 0;
                    satp = csr_data;
                    break;
                }
                case csr_tselect:
                    break;
                case csr_tdata1:
                    break;
                /*pmpaddr、pmpcfg寄存器实现是可选的，完成riscv-tests测试后可删除*/
                case csr_pmpaddr0:
                    pmpaddr0 = csr_data;
                    break;
                case csr_pmpcfg0:
                    pmpcfg0 = csr_data;
                    break;
                default:
                    return false;
            }
            return true;
        }
        
        bool csr_setbit(uint16_t csr_index,uint64_t csr_mask){
            uint64_t tmp;
            bool ret = csr_read(csr_index,tmp);
            if(!ret) return false;
            tmp |= csr_mask;
            ret &= csr_write(csr_index,tmp);
            return ret;
        }

        bool csr_clearbit(uint16_t csr_index,uint64_t csr_mask){
            uint64_t tmp;
            bool ret = csr_read(csr_index,tmp);
            if(!ret) return false;
            tmp &= ~csr_mask;
            ret &= csr_write(csr_index,tmp);
            return ret;
        }
        //note:the core shoule raise exception when return value is not exc_custom ok.
        //fetch instruction
        rv_exc_code va_if(uint64_t start_addr,uint64_t size,uint8_t *buffer){
            const satp_def *satp_reg = (satp_def *)&satp;
            if(cur_priv == M_MODE || satp_reg->mode == 0){
                bool pstatus = l1i.pa_if(start_addr,size,buffer);
                if(!pstatus) return exc_instr_acc_fault;
                else return exc_custom_ok;
            }else{
                //note:if the pc misalign but didn't beyond page range,the execption should be raise by core
                if((start_addr >> 12) != ((start_addr + size - 1) >> 12)) return exc_instr_misalign;
                sv39_tlb_entry *tlb_e = i_tlb.local_tlbe_get(*satp_reg,start_addr);
                if(!tlb_e || !tlb_e->A || !tlb_e->X) return exc_instr_pgfault;
                if((cur_priv == U_MODE && !tlb_e->U) || (cur_priv == S_MODE && tlb_e->U)) return exc_instr_pgfault;
                uint64_t pa = tlb_e->ppa + (start_addr % ((tlb_e->pagesize == 1)?(1<<12):((tlb_e->pagesize==2)?(1<<21):(1<<30))));
                bool pstatus = l1i.pa_if(pa,size,buffer);
                if(!pstatus) return exc_instr_acc_fault;
                else return exc_custom_ok;

            }

        }

        rv_exc_code va_read(uint64_t start_addr,uint64_t size,uint8_t *buffer){
            const satp_def *satp_reg = (satp_def *)&satp;
            const csr_mstatus_def *mstatus = (csr_mstatus_def*)&status;
            if((cur_priv == M_MODE && (!mstatus->mprv || mstatus->mpp == M_MODE)) || satp_reg->mode == 0){
                bool pstatus = l1d.pa_read(start_addr,size,buffer);
                if(!pstatus) return exc_load_acc_fault;
                else return exc_custom_ok;
            }else{
                if((start_addr >> 12) != ((start_addr + size - 1) >> 12)) return exc_load_misalign;
                sv39_tlb_entry *tlb_e = d_tlb.local_tlbe_get(*satp_reg,start_addr);
                if(!tlb_e || !tlb_e->A || (!tlb_e->R && !(mstatus->mxr && !tlb_e->X))) return exc_load_pgfault;
                priv_mode priv = (mstatus->mprv && cur_priv == M_MODE) ? static_cast<priv_mode>(mstatus->mpp) : cur_priv;
                if(priv == U_MODE && !tlb_e->U) return exc_load_acc_fault;
                uint64_t pa = tlb_e->ppa + (start_addr % ((tlb_e->pagesize == 1)?(1<<12):((tlb_e->pagesize==2)?(1<<21):(1<<30))));
                bool pstatus = l1d.pa_read(pa,size,buffer);
                if(!pstatus) return exc_load_acc_fault;
                else return exc_custom_ok;
            }
        }

        rv_exc_code va_write(uint64_t start_addr,uint64_t size,const uint8_t *buffer){
            const satp_def *satp_reg = (satp_def *)&satp;
            const csr_mstatus_def *mstatus = (csr_mstatus_def*)&status;
            if((cur_priv == M_MODE && (!mstatus->mprv || mstatus->mpp == M_MODE)) || satp_reg->mode == 0){
                if(riscv_test){
                    if(start_addr == 0x80001000){
                        uint64_t tohost = *(uint64_t*)buffer;
                        if(tohost == 1){
                            if(tohost == 1){
                                printf("***Test Pass!\n");
                                exit(0);
                            }else{
                                printf("---Failed with value 0x%lx\n",tohost);
                                exit(1);
                            }
                        }
                    }
                }
                bool pstatus = l1d.pa_write(start_addr,size,buffer);
                if(!pstatus) return exc_store_acc_fault;
                else return exc_custom_ok;
            }else{
                if((start_addr >> 12) != ((start_addr + size - 1) >> 12)) return exc_store_misalign;
                sv39_tlb_entry *tlb_e = d_tlb.local_tlbe_get(*satp_reg,start_addr);

                if(!tlb_e || !tlb_e->A || !tlb_e->D || !tlb_e->W) return exc_store_pgfault;

                priv_mode priv = (mstatus->mprv && cur_priv == M_MODE) ? static_cast<priv_mode>(mstatus->mpp) : cur_priv;
                if(priv == U_MODE && !tlb_e->U) return exc_store_pgfault;

                if(!mstatus->sum && priv == S_MODE && tlb_e->U) return exc_store_pgfault;
                uint64_t pa = tlb_e->ppa + (start_addr % ((tlb_e->pagesize == 1)?(1<<12):((tlb_e->pagesize == 2)?(1<<21):(1<<30))));

                if(riscv_test){
                    if(pa == 0x80001000){
                        uint64_t tohost = *(uint64_t*)buffer;
                        if(tohost == 1){
                            if(tohost == 1){
                                printf("***Test Pass***\n");
                                exit(0);
                            }else{
                                printf("---Failed with value 0x---%lx\n",tohost);
                                exit(1);
                            }
                        }
                    }
                }
                bool pstatus = l1d.pa_write(pa,size,buffer);
                if(!pstatus) return exc_store_pgfault;
                else return exc_custom_ok;
            }
        }
        // note: core should check whether amoop and start_addr is valid
        rv_exc_code va_lr(uint64_t start_addr,uint64_t size,uint8_t *buffer){
            assert(size == 4 || size == 8);
            const satp_def *satp_reg = (satp_def *)&satp;
            const csr_mstatus_def *mstatus = (csr_mstatus_def*)&status;
            if((cur_priv == M_MODE && (!mstatus->mprv || mstatus->mpp == M_MODE)) || satp_reg->mode == 0){
                bool pstatus = l1d.pa_lr(start_addr,size,buffer);
                if(!pstatus) return exc_store_acc_fault;
                else return exc_custom_ok;
            }else{
                if((start_addr >> 12) != ((start_addr + size - 1) >> 12)) return exc_store_misalign;
                sv39_tlb_entry *tlb_e = d_tlb.local_tlbe_get(*satp_reg,start_addr);
                if(!tlb_e || !tlb_e->A || (!tlb_e->R && !(mstatus->mxr && !tlb_e->X))) return exc_store_pgfault;
                priv_mode priv = (mstatus->mprv && cur_priv ==M_MODE) ? static_cast<priv_mode>(mstatus->mpp) : cur_priv;
                if(priv == U_MODE && !tlb_e->U) return exc_store_pgfault;
                if(!mstatus->sum && priv == S_MODE && tlb_e->U) return exc_store_acc_fault;
                uint64_t pa = tlb_e->ppa + (start_addr % ((tlb_e->pagesize == 1)?(1<<12):((tlb_e->pagesize == 2)?(1<<21):(1<<30))));
                bool pstatus = l1d.pa_lr(pa,size,buffer);
                if(!pstatus) return exc_store_acc_fault;
                else return exc_custom_ok;
            }
        }

        //note:if va_sc return != exc_custom_ok,sc_fail shouldn't commit.
        rv_exc_code va_sc(uint64_t start_addr,uint64_t size,const uint8_t *buffer,bool &sc_fail){
            assert(size == 4 || size == 8);
            const satp_def *satp_reg = (satp_def *)&satp;
            const csr_mstatus_def *mstatus = (csr_mstatus_def*)&status;
            if((cur_priv == M_MODE && (!mstatus->mprv || mstatus->mpp == M_MODE)) || satp_reg->mode == 0){
                bool pstatus = l1d.pa_sc(start_addr,size,buffer,sc_fail);
                if(!pstatus) return exc_store_acc_fault;
                else return exc_custom_ok;
            }else{
                if((start_addr >> 12) != ((start_addr + size - 1) >> 12)) return exc_store_misalign;
                sv39_tlb_entry *tlb_e = d_tlb.local_tlbe_get(*satp_reg,start_addr);
                if(!tlb_e || !tlb_e->A || !tlb_e->D || !tlb_e->W) return exc_store_pgfault;
                priv_mode priv = (mstatus->mprv && cur_priv == M_MODE) ? static_cast<priv_mode>(mstatus->mpp) : cur_priv;
                if(priv == U_MODE && !tlb_e->U) return exc_store_pgfault;
                if(!mstatus->sum && priv == S_MODE && tlb_e->U) return exc_store_pgfault;
                uint64_t pa = tlb_e->ppa + (start_addr % ((tlb_e->pagesize == 1)?(1<<12):((tlb_e->pagesize==2)?(1<<21):(1<<30))));
                bool pstatus = l1d.pa_sc(pa,size,buffer,sc_fail);
                if(!pstatus) return exc_store_pgfault;
                else return exc_custom_ok;
            }
        }

        rv_exc_code va_amo(uint64_t start_addr,uint64_t size,LSUOpType op,int64_t src,int64_t &dst){
            assert(size == 4 || size == 8);
            const satp_def *satp_reg = (satp_def *)&satp;
            const csr_mstatus_def *mstatus = (csr_mstatus_def*)&status;
            if((cur_priv == M_MODE && (!mstatus->mprv || mstatus->mpp == M_MODE)) || satp_reg->mode == 0){
                bool pstatus = l1d.pa_amo_op(start_addr,size,op,src,dst);
                if(!pstatus) return exc_store_acc_fault;
                else return exc_custom_ok;
            }else{
                if((start_addr >> 12) != ((start_addr + size - 1) >> 12)) return exc_store_misalign;
                sv39_tlb_entry *tlb_e = d_tlb.local_tlbe_get(*satp_reg,start_addr);
                if(!tlb_e || !tlb_e->A || !tlb_e->D || !tlb_e->W) return exc_store_pgfault;
                priv_mode priv = (mstatus->mprv && cur_priv == M_MODE) ? static_cast<priv_mode>(mstatus->mpp) : cur_priv;
                if(priv == U_MODE && !tlb_e->U) return exc_store_pgfault;
                if(!mstatus->sum && priv == S_MODE && tlb_e->U) return exc_store_pgfault;
                uint64_t pa = tlb_e->ppa + (start_addr % ((tlb_e->pagesize == 1)?(1<<12):((tlb_e->pagesize==2)?(1<<21):(1<<30))));
                bool pstatus = l1d.pa_amo_op(pa,size,op,src,dst);
                if(!pstatus) return exc_store_pgfault;
                else return exc_custom_ok;
            }
        }

        void ecall(uint64_t cur_pc){
            csr_cause_def cause;
            cause.cause = cur_priv + 8;
            cause.interrupt = 0;
            raise_trap(cur_pc,cause);
        }
        void ebreak(){
            assert(false);
        }
        bool sret(){
            const csr_mstatus_def *mstatus = (csr_mstatus_def*)&status;
            if(cur_priv < S_MODE || mstatus->tsr) return false;
            csr_mstatus_def *sstatus = (csr_mstatus_def *)&status;
            sstatus->sie = sstatus->spie;
            cur_priv = static_cast<priv_mode>(sstatus->spp);
            // next_priv = static_cast<priv_mode>(sstatus->spp);
            sstatus->spie = 1;
            if(sstatus->spp != M_MODE) sstatus->mprv = 0;
            sstatus->spp = U_MODE;
            cur_need_trap = true;
            trap_pc = sepc;
            return true;
        }
        bool mret(){
            if(cur_priv != M_MODE) return false;
            csr_mstatus_def *mstatus = (csr_mstatus_def *)&status;
            mstatus->mie = mstatus->mpie;
            cur_priv = static_cast<priv_mode>(mstatus->mpp);
            // next_priv = static_cast<priv_mode>(mstatus->spp);
            mstatus->mpie = 1;
            if(mstatus->mpp != M_MODE) mstatus->mprv = 0;
            mstatus->mpp = U_MODE;
            cur_need_trap = true;
            trap_pc = mepc;
            return true;
        }
        bool sfence_vma(uint64_t vaddr,uint64_t asid){
            const csr_mstatus_def *mstatus = (csr_mstatus_def*)&status;
            if(cur_priv < S_MODE || (cur_priv == S_MODE && mstatus->tvm)) return false;
            i_tlb.sfence_vma(vaddr,asid);
            d_tlb.sfence_vma(vaddr,asid);
            return true;
        }
        void fence_i(){
            l1i.fence_i();
        }
        
        uint64_t int2index(uint64_t int_mask){//with priority
            /*
                According to spec, multiple simultaneous interrupts destined for M-mode are
                handled in the following decreasing: MEI, MSI, MTI, SEI, SSI, STI.
            */
            if(int_mask & (1ull<<int_m_ext)){
                return int_m_ext; 
            }else if(int_mask & (1ull<<int_m_sw)){
                return int_m_sw;
            }else if(int_mask & (1ull<<int_m_timer)){
                return int_m_timer;
            }else if(int_mask & (1ull<<int_s_ext)){
                return int_s_ext;
            }else if(int_mask & (1ull<<int_s_sw)){
                return int_s_sw;
            }else if(int_mask & (1ull<<int_s_timer)){
                return int_s_timer;
            }else return exc_custom_ok;
        }

        rv_exc_code check_and_raise_int(){ // TODO: Find interrupts trap priority compare to exceptions. Now interrupts are prior to exceptions.
            /*
             An interrupt i will trap to M-mode (causing the privilege mode to change to M-mode) if all of the following are true:
             (a) either the current privilege mode is M and the MIE bit in the mstatus register is set, or the current privilege
             mode has less privilege than M-mode;
             (b) bit i is set both mip and mie;
             (c) if register mideleg exists,bit i is not set in mideleg
            */
           /*
            An interrupt i will trap to S-mode if both of the following are true:
             (a) either the current privilege mode is S and the SIE bit in the sstatus register is set, or the current privilege mode has
             less privilege than S-mode;
             (b) bit i is set in both sip and sie.
           */
            /*
                out of spec:mideleg layout as sip rather than mip.
                M[EST]I bits in mideleg is hardwired 0.
                OpenSBI will not delegate these ints to S-Mode. So we don't need to implement.
            */
            //Note:WFI is not affacted by mstatus.mie and mstatus.sie and mideleg,but we implement WFI as nop currently.
            csr_mstatus_def *mstatus = (csr_mstatus_def*)&status;
            uint64_t int_bits = ip & ie;
            uint64_t final_int_index = exc_custom_ok;
            if(cur_priv == M_MODE){
                //Note: we should use mideleg as interrupt disable in M-mode as interrupts didn't transfer to lower levels.
                uint64_t mmode_int_bits = int_bits & (~mideleg);
                if(mmode_int_bits && mstatus->mie) final_int_index = int2index(mmode_int_bits);
            }else{
                //check traps to smode or mmode
                uint64_t int_index = int2index(int_bits);
                if((1ul<<int_index) & mideleg){ // delegate to s
                    //Note:if delegate to S but sie is 0, we should not raise trap to M-mode
                    if(mstatus->sie || cur_priv < S_MODE) final_int_index = int_index;
                }else{
                    if(mstatus->mie || cur_priv < M_MODE) final_int_index = int_index;
                }
            }
            return (rv_exc_code)final_int_index;
        }
        component::memory *mem;
        uint64_t hart_id;

        //访存:tlb、l2_cache（bus）、cache
        l2_cache<L2_WAYS,L2_NR_SETS,L2_SZLINE,32> &l2;

        rv_sv39<8,false> i_tlb;
        rv_sv39<8,true> d_tlb;

        l1_i_cache <L1I_WAYS,L1I_SZLINE,L1I_NR_SETS> l1i;
        l1_d_cache <L1D_WAYS,L1D_SZLINE,L1D_NR_SETS> l1d;
        

        //const uint64_t &cur_pc,处理器pc的引用，流水线中并不适用
        priv_mode   cur_priv;
        uint64_t    trap_pc;
        bool        cur_need_trap;

        priv_mode   next_priv;

        //CSRS
        uint64_t    status; //mstatus and sstatus
        uint64_t    misa;
        uint64_t    medeleg;
        uint64_t    mideleg;
        uint64_t    ie;
        uint64_t    mtvec;
        uint64_t    mscratch;
        uint64_t    mepc;
        uint64_t    mcause;
        uint64_t    mtval;
        uint64_t    mcounteren;
        uint64_t    ip;
        uint64_t    mcycle;
        uint64_t    minstret;

        uint64_t    stvec;
        uint64_t    sscratch;
        uint64_t    sepc;
        uint64_t    scause;
        uint64_t    stval;
        uint64_t    satp;

        uint64_t    scounteren;

        uint64_t    pmpaddr0;
        uint64_t    pmpcfg0;

};
