/*
 * @Author: 苗金标
 * @Date: 2023-04-14 15:44:22
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-01 20:19:17
 * @Description: 
 */
#pragma once
#include "co_slave.h"
#include "l2_cache.h"
#include <bitset>
#include "cache_config.h"
#include "replacement/tree_plru.h"
#include "../common.h"

template <int nr_ways = L1D_WAYS,int sz_cache_line = L1D_SZLINE,int nr_sets = L1D_NR_SETS>
struct l1_d_cache_set{
    uint64_t tag[nr_ways];
    uint8_t  data[nr_ways][sz_cache_line];
    l1_line_status status[nr_ways];
    tree_plru <nr_ways> replace;
    l1_d_cache_set(){
        for(int i = 0;i < nr_ways;i++)
            status[i] = L1_INVALID;
    }

    bool match(uint64_t addr,int &hit_way_id){
        uint64_t search_tag = addr / sz_cache_line / nr_sets;
        for(int i = 0;i < nr_ways;i++){
            if(tag[i] == search_tag && status[i] != L1_INVALID){
                hit_way_id = i;
                return true;
            }
        }
        return false;
    }
}; 

template <int nr_ways = L1D_WAYS,int sz_cache_line = L1D_SZLINE, int nr_sets = L1D_NR_SETS>
class l1_d_cache : public co_slave{
    static_assert(__builtin_popcount(nr_ways) == 1);
    static_assert(__builtin_popcount(nr_sets) == 1);
    static_assert(__builtin_popcount(sz_cache_line) == 1);
    public:
        l1_d_cache(l2_cache <L2_WAYS,L2_NR_SETS,L2_SZLINE,32> *l2_cache,uint64_t hart_id):hart_id(hart_id){
            slave_id = l2_cache->register_slave(this);
            l2 = l2_cache;
            lr_valid = false;
        }

        void invalidate_exclusive(uint64_t start_addr){//for l2 req
            lr_valid = false;
            l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> *select_set = &set_data[get_index(start_addr)];
            int way_id;
            assert(select_set->match(start_addr,way_id));
            assert(select_set->status[way_id] == L1_EXCLUSIVE || select_set->status[way_id] == L1_MODIFIED);
            if(select_set->status[way_id] == L1_MODIFIED){
                l2->cache_line_writeback(start_addr,select_set->data[way_id],slave_id);
            }
            select_set->status[way_id] = L1_SHARED;
        }

        void invalidate_shared(uint64_t start_addr){// for l2 req
            lr_valid = false;
            l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> *select_set = &set_data[get_index(start_addr)];
            int way_id;
            assert(select_set->match(start_addr,way_id));
            if(select_set->status[way_id] == L1_MODIFIED){
                l2->cache_line_writeback(start_addr,select_set->data[way_id],slave_id);
            }
            select_set->status[way_id] = L1_INVALID;
        }

        bool pa_read(uint64_t start_addr,uint64_t size,uint8_t *buffer){
            if(start_addr + size <= 0x80000000){//MMIO形式，外设编址为0x80000000一下的地址
                bool res = l2->pa_read_uncached(start_addr,size,buffer);
                return res;
            }else{
                assert(sz_cache_line - (start_addr % sz_cache_line) >= size);
                bool res = l1_include(start_addr);
                if(!res) return false;
                l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> *select_set = &set_data[get_index(start_addr)];
                int way_id;
                assert(select_set->match(start_addr,way_id));
                select_set->replace.mark_used(way_id);
                memcpy(buffer,&(select_set->data[way_id][start_addr % sz_cache_line]),size);
                return true;
            }
        }

        bool pa_write(uint64_t start_addr,uint64_t size,const uint8_t *buffer){
            if(start_addr + size <= 0x80000000){
                bool res = l2->pa_write_uncached(start_addr,size,buffer);
                return res;
            }else{
                assert(sz_cache_line - (start_addr % sz_cache_line) >= size);
                lr_valid = false;
                bool res = l1_include_exclusive(start_addr);
                if(!res) return false;
                l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> *select_set = &set_data[get_index(start_addr)];
                int way_id;
                assert(select_set->match(start_addr,way_id));
                select_set->replace.mark_used(way_id);
                assert(select_set->status[way_id] == L1_EXCLUSIVE || select_set->status[way_id] == L1_MODIFIED);
                memcpy(&(select_set->data[way_id][start_addr % sz_cache_line]),buffer,size);
                select_set->status[way_id] = L1_MODIFIED;
                return true;
            }
        }

        //note:check adddress alignment in the core and raise address misalign execption
        bool pa_lr(uint64_t pa,uint64_t size,uint8_t *dst){
            if(pa + size <= 0x80000000){
                return false;
            }
            // assert(pa % size == 0);
            lr_pa = pa;
            lr_size = size;
            lr_valid = true;
            bool res = l1_include_exclusive(pa);
            if(!res){
                return false;
            }
            l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> *select_set = &set_data[get_index(pa)];
            int way_id;
            assert(select_set->match(pa,way_id));
            assert(select_set->status[way_id] == L1_EXCLUSIVE || select_set->status[way_id] == L1_MODIFIED);
            select_set->replace.mark_used(way_id);
            memcpy(dst,&(select_set->data[way_id][pa % sz_cache_line]),size);
            return true;
        }

        //note:if pa_write return false,sc_fail shouldn't commit
        bool pa_sc(uint64_t pa,uint64_t size,const uint8_t *src,bool &sc_fail){
            if(pa + size <= 0x80000000){
                return false;
            }
            assert(pa % size == 0);
            l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> *select_set = &set_data[get_index(pa)];
            int way_id;
            if(!lr_valid || lr_pa != pa || lr_size != size || !(select_set->match(pa,way_id)) || select_set->status[way_id] == L1_SHARED){
                sc_fail = true;
                return true;
            }
            select_set->replace.mark_used(way_id);
            sc_fail = false;
            lr_valid = false;
            memcpy(&(select_set->data[way_id][pa % sz_cache_line]),src,size);
            select_set->status[way_id] = L1_MODIFIED;
            return true;
        }

        //note:core should check whether amoop is valid
        bool pa_amo_op(uint64_t pa,uint64_t size,LSUOpType op,int64_t src,int64_t &dst){
            if(pa + size <= 0x80000000) return false;
            assert(pa % size == 0);
            lr_valid = false;
            bool fetch_res = l1_include_exclusive(pa);
            if(!fetch_res) return false;
            l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> *select_set = &set_data[get_index(pa)];
            int way_id;
            assert(select_set->match(pa,way_id));
            assert(select_set->status[way_id] == L1_EXCLUSIVE || select_set->status[way_id] == L1_MODIFIED);
            select_set->replace.mark_used(way_id);
            int64_t res = 0;
            if(size == 4){
                int32_t res32;
                memcpy(&res32,&(select_set->data[way_id][pa % sz_cache_line]),4);
                res = res32;
            }else{
                assert(size == 8);
                memcpy(&res,&(select_set->data[way_id][pa % sz_cache_line]),8);
            }
            int64_t to_write;
            switch(op){
                case LSUOpType::amoswap: case LSUOpType::amoswapw:
                    to_write = src;
                    break;
                case LSUOpType::amoadd: case LSUOpType::amoaddw:
                    to_write = src + res;
                    break;
                case LSUOpType::amoand: case LSUOpType::amoandw:
                    to_write = src & res;
                    break;
                case LSUOpType::amoor: case LSUOpType::amoorw:
                    to_write = src | res;
                    break;
                case LSUOpType::amoxor: case LSUOpType::amoxorw:
                    to_write = src ^ res;
                    break;
                case LSUOpType::amomax: case LSUOpType::amomaxw:
                    to_write = std::max((int64_t)src,(int64_t)res);
                    break;
                case LSUOpType::amomin: case LSUOpType::amominw:
                    to_write = std::min((int64_t)src,(int64_t)res);
                    break;
                case LSUOpType::amomaxu: case LSUOpType::amomaxuw:
                    to_write = std::max((uint64_t)src,(uint64_t)res);
                    break;
                case LSUOpType::amominu: case LSUOpType::amominuw:
                    to_write = std::min((uint64_t)src,(uint64_t)res);
                    break;
                default:
                    assert(false);
            }
            dst = res;
            memcpy(&(select_set->data[way_id][pa % sz_cache_line]),&to_write,size);
            select_set->status[way_id] = L1_MODIFIED;
            return true;
        }

        bool super_pa_sc(uint64_t pa,uint64_t size,const uint8_t *src,bool &sc_fail){
            if(pa + size <= 0x80000000){
                return false;
            }
            // assert(pa % size == 0);
            l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> *select_set = &set_data[get_index(pa)];
            int way_id;
            if(!lr_valid || lr_pa != pa || lr_size != size || !(select_set->match(pa,way_id)) || select_set->status[way_id] == L1_SHARED){
                sc_fail = true;
                return true;
            }
            select_set->replace.mark_used(way_id);
            sc_fail = false;
            lr_valid = false;
            return true;
        }

        bool super_pa_amo_op(uint64_t pa,uint64_t size,LSUOpType op,int64_t src,int64_t &dst,int64_t &store){
            if(pa + size <= 0x80000000) return false;
            // assert(pa % size == 0);
            lr_valid = false;
            bool fetch_res = l1_include_exclusive(pa);
            if(!fetch_res) return false;
            l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> *select_set = &set_data[get_index(pa)];
            int way_id;
            assert(select_set->match(pa,way_id));
            assert(select_set->status[way_id] == L1_EXCLUSIVE || select_set->status[way_id] == L1_MODIFIED);
            select_set->replace.mark_used(way_id);
            int64_t res = 0;
            if(size == 4){
                int32_t res32;
                memcpy(&res32,&(select_set->data[way_id][pa % sz_cache_line]),4);
                res = res32;
            }else{
                assert(size == 8);
                memcpy(&res,&(select_set->data[way_id][pa % sz_cache_line]),8);
            }
            int64_t to_write;
            switch(op){
                case LSUOpType::amoswap: case LSUOpType::amoswapw:
                    to_write = src;
                    break;
                case LSUOpType::amoadd: case LSUOpType::amoaddw:
                    to_write = src + res;
                    break;
                case LSUOpType::amoand: case LSUOpType::amoandw:
                    to_write = src & res;
                    break;
                case LSUOpType::amoor: case LSUOpType::amoorw:
                    to_write = src | res;
                    break;
                case LSUOpType::amoxor: case LSUOpType::amoxorw:
                    to_write = src ^ res;
                    break;
                case LSUOpType::amomax: case LSUOpType::amomaxw:
                    to_write = std::max((int64_t)src,(int64_t)res);
                    break;
                case LSUOpType::amomin: case LSUOpType::amominw:
                    to_write = std::min((int64_t)src,(int64_t)res);
                    break;
                case LSUOpType::amomaxu: case LSUOpType::amomaxuw:
                    to_write = std::max((uint64_t)src,(uint64_t)res);
                    break;
                case LSUOpType::amominu: case LSUOpType::amominuw:
                    to_write = std::min((uint64_t)src,(uint64_t)res);
                    break;
                default:
                    assert(false);
            }
            dst = res;
            store = to_write;
            return true;
        }

    private:
        uint64_t hart_id;
        void l1_invalidate(uint64_t addr){
            l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> *select_set = &set_data[get_index(addr)];
            int way_id;
            if(!(select_set->match(addr,way_id))){
                return;
            }
            if(select_set->status[way_id] == L1_MODIFIED){
                l2->cache_line_writeback(addr,select_set->data[way_id],slave_id);
            }
            l2->release_shared(addr,slave_id);
            select_set->status[way_id] = L1_INVALID;
        }

        bool l1_include(uint64_t addr){
            l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> *select_set = &set_data[get_index(addr)];
            int way_id;
            if(!(select_set->match(addr,way_id))){
                way_id = select_set->replace.get_replace();
                if(select_set->status[way_id] != L1_INVALID){
                    l1_invalidate((select_set->tag[way_id] * nr_sets * sz_cache_line) | (get_index(addr) * sz_cache_line));
                    assert(select_set->status[way_id] == L1_INVALID);
                }
                bool res = l2->cache_line_fetch(addr,select_set->data[way_id],slave_id);
                if(!res) return false;
                select_set->tag[way_id] = get_tag(addr);
                select_set->status[way_id] = L1_SHARED;
                return true;
            }
            else return true;
        }

        bool l1_include_exclusive(uint64_t addr){
            bool res = l1_include(addr);
            if(!res) return false;
            l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> *select_set = &set_data[get_index(addr)];
            int way_id;
            assert(select_set->match(addr,way_id));
            if(select_set->status[way_id] != L1_EXCLUSIVE && select_set->status[way_id] != L1_MODIFIED){
                l2->acquire_exclusive(addr,slave_id);
                select_set->status[way_id] = L1_EXCLUSIVE;
            }
            return true;
        }
        uint64_t get_index(uint64_t addr){
            return (addr / sz_cache_line) % nr_sets;
        }
        uint64_t get_tag(uint64_t addr){
            return addr / sz_cache_line / nr_sets;
        }
        l1_d_cache_set <nr_ways,sz_cache_line,nr_sets> set_data[nr_sets];
        l2_cache <L2_WAYS,L2_NR_SETS,L2_SZLINE,32> *l2;
        int slave_id;
        uint64_t lr_pa;
        uint64_t lr_size;
        bool lr_valid;
};
