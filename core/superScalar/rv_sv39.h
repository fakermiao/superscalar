/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:42:00
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-06 15:23:03
 * @Description: 
 */
/*
 * @Author: 苗金标
 * @Date: 2023-04-16 21:03:02
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-09 16:24:36
 * @Description: 
 */
#pragma once

#include "../common.h"
#include <cstdint>
#include <utility>
#include "../cache/l2_cache.h"
#include "../cache/cache_config.h"
namespace Supercore{
struct sv39_tlb_entry{
    uint64_t ppa;
    uint64_t vpa;
    uint16_t asid;
    uint8_t  pagesize; //0:invalid, 1:4KB, 2: 2M, 3:1G
    bool     R; //read
    bool     W; //write
    bool     X; //execute
    bool     U; //user
    bool     G; //global
    bool     A; //access
    bool     D; //dirty    
};

// uint64_t pa_pc;

template <unsigned int nr_tlb_entry = 32, bool is_data = true>//is_data用来区分itlb、dtlb
class rv_sv39{
    public:
        rv_sv39(l2_cache<L2_WAYS,L2_NR_SETS,L2_SZLINE,32> &bus,uint64_t slave_id):bus(bus){
            random = 0;
            for(u_int32_t i = 0;i < nr_tlb_entry;i++) tlb[i].pagesize = 0;
        }
        void sfence_vma(uint64_t vaddr,uint64_t asid){
            for(uint32_t i = 0;i < nr_tlb_entry;i++){
                if(tlb[i].asid == asid || asid == 0){
                    if(vaddr == 0) tlb[i].pagesize = 0;
                    else{
                        switch (tlb[i].pagesize){
                            case 1: //4KB
                                if((tlb[i].vpa & (-(1ll<<12))) == (vaddr & (-(1ll<<12)))) tlb[i].pagesize = 0;
                                break;
                            case 2: //2MB
                                if((tlb[i].vpa & (-(1ll<<21))) == (vaddr & (-(1ll<<21)))) tlb[i].pagesize = 0;
                                break;
                            case 3: //1G
                                if((tlb[i].vpa & (-(1ll<<30))) == (vaddr & (-(1ll<<30)))) tlb[i].pagesize = 0;
                            default:
                                break;
                        }
                    }
                }
            }
        }
        sv39_tlb_entry* local_tlbe_get(satp_def satp,uint64_t va){
            sv39_va *va_struct = (sv39_va*)&va;
            assert((va_struct->blank == 0b1111111111111111111111111 && (va_struct->vpn_2 >> 8)) || (va_struct->blank == 0 && ((va_struct->vpn_2 >> 8) == 0)));
            // we should raise access fault before call sv39
            sv39_tlb_entry *res = local_tlb_get(satp,va);
            if (res) {
                return res;
            }
            // slow path, ptw
            sv39_pte pte;
            uint64_t page_size;
            bool ptw_result = ptw(satp,va,pte,page_size);
            if (!ptw_result) return NULL; // return null when page fault.
            // write back to tlb
            res = &tlb[random];
            random = (random + 1) % nr_tlb_entry;
            res->ppa = (((((uint64_t)pte.PPN2 << 9) | (uint64_t)pte.PPN1) << 9) | (uint64_t)pte.PPN0) << 12;
            res->vpa = (page_size == (1<<12)) ? (va - (va % (1<<12))) : (page_size == (1<<21)) ? (va - (va % (1<<21))) : (va - (va % (1<<30)));
            res->asid = satp.asid;
            res->pagesize = (page_size == (1<<12)) ? 1 : (page_size == (1<<21)) ? 2 : 3;
            res->R = pte.R;
            res->W = pte.W;
            res->X = pte.X;
            res->U = pte.U;
            res->G = pte.G;
            res->A = pte.A;
            res->D = pte.D;
            if (local_tlb_get(satp,va) != res) assert(false);
            return res;
        }
    
    private:
        l2_cache <L2_WAYS,L2_NR_SETS,L2_SZLINE,32> &bus;
        unsigned int random;
        sv39_tlb_entry tlb[nr_tlb_entry];
        uint64_t slave_id;
        bool ptw(satp_def satp,uint64_t va_in,sv39_pte &pte_out,uint64_t &pagesize){
            sv39_va *va = (sv39_va*)&va_in;
            if(satp.mode != 8) return false; //mode is not sv39
            uint64_t pt_addr = ((satp.ppn) << 12); //satp低20位为PPN域，保存了根页表的物理地址
            sv39_pte pte;
            for(int i = 2;i >= 0;i--){
                uint64_t search_addr = pt_addr+((i==2?va->vpn_2:(i==1?va->vpn_1:va->vpn_0))*sizeof(sv39_pte));
                bool res = bus.pa_read_cached(search_addr,sizeof(sv39_pte),(uint8_t*)&pte);
                if(!res){
                    // printf("pt_addr=%lx, vpn=%lx\n",pt_addr,((i==2?va->vpn_2:(i==1?va->vpn_1:va->vpn_0))));
                    // printf("\nerror ptw pa=%lx, level=%d, satp=%lx\n",pt_addr+((i==2?va->vpn_2:(i==1?va->vpn_1:va->vpn_0))*sizeof(sv39_pte)),i,*(uint64_t *)&satp);
                    return false;
                }
                if(!pte.V || (!pte.R && pte.W) || pte.reserved || pte.PBMT){
                    
                    for (int j=0;j<(1<<9);j++) {
                        uint64_t tmp_pte = 0;
                        bus.pa_read_uncached(pt_addr+(j*8),sizeof(sv39_pte),(uint8_t*)&tmp_pte);
                        if (tmp_pte != 0) {
                        }else{
                            // printf("we found\n");
                        }
                    }
                    
                    return false;
                }
                if(pte.R || pte.X){//leaf
                    if(i == 2 && (pte.PPN1 || pte.PPN0)) return false;//make sure that superpage entries trap when PPN LSBs are set.
                    if(i == 1 && pte.PPN0) return false; //make sure that superpage entries trap when PPN LSBs are set.
                    pte_out = pte;
                    pagesize = (1 << 12) << (9*i);
                    // uint64_t pa = (((((uint64_t)pte.PPN2 << 9) | (uint64_t)pte.PPN1) << 9) | (uint64_t)pte.PPN0) << 12;
                    return true;
                }else{//valid non-leaf
                    pt_addr = (((((uint64_t)pte.PPN2 << 9) | (uint64_t)pte.PPN1) << 9) | (uint64_t)pte.PPN0) << 12;
                }
            }
            return false;
        }
        sv39_tlb_entry* local_tlb_get(satp_def satp,uint64_t va){
            sv39_tlb_entry* res = NULL;
            for(uint32_t i = 0;i < nr_tlb_entry;i++){
                if(tlb[i].asid == satp.asid || tlb[i].G){
                    switch (tlb[i].pagesize){
                        case 1: //4KB
                            if((va & (-(1ll<<12))) == tlb[i].vpa){
                                assert(res == NULL);
                                res = &tlb[i];
                            }
                            break;
                        case 2: //2MB
                            if((va & (-(1ll<<21))) == tlb[i].vpa){
                                assert(res == NULL);
                                res = &tlb[i];
                            }
                            break;
                        case 3: // 1G
                            if((va & (-(1ll<<30))) == tlb[i].vpa){
                                assert(res == NULL);
                                res = &tlb[i];
                            }
                        default:
                            break;
                    }
                }
            }
            return res;
        }
};
}