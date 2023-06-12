/*
 * @Author: 苗金标
 * @Date: 2023-05-23 16:46:38
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-23 16:54:47
 * @Description: 
 */
#include "core/common.h"
#include <dlfcn.h>

void (*ref_difftest_memcpy)(uint64_t addr,void *buf,size_t n,bool direction) = NULL;
void (*ref_difftest_regcpy)(void *dut,bool direction) = NULL;
void (*ref_difftest_exec)(uint64_t n,bool meip,bool msip,bool mtip,bool seip,uint64_t mtime,bool setFlag) = NULL;
void (*ref_difftest_settick)(uint64_t tick) = NULL;
enum {DIFFTEST_TO_DUT,DIFFTEST_TO_REF};

void init_difftest(const char *ref_so_file,const char *load_path,bool is_rvtest){
  void *handle;
  handle = dlopen(ref_so_file,RTLD_LAZY);//打开一个动态库文件
  assert(handle);

  ref_difftest_memcpy = reinterpret_cast<void (*)(uint64_t,void *,size_t,bool)>(dlsym(handle,"difftest_memcpy"));//查找符号
  assert(ref_difftest_memcpy);

  ref_difftest_regcpy = reinterpret_cast<void (*)(void *,bool)>(dlsym(handle,"difftest_regcpy"));
  assert(ref_difftest_regcpy);

  ref_difftest_exec = reinterpret_cast<void (*)(uint64_t,bool,bool,bool,bool,uint64_t,bool)>(dlsym(handle,"difftest_exec"));
  assert(ref_difftest_exec);

  ref_difftest_settick = reinterpret_cast<void (*)(uint64_t)>(dlsym(handle,"difftest_settick"));
  assert(ref_difftest_settick);

  void (*ref_difftest_init)(const char *,bool) = reinterpret_cast<void (*)(const char *,bool)>(dlsym(handle,"difftest_init"));
  assert(ref_difftest_init);

  ref_difftest_init(load_path,is_rvtest);
}

struct diff_context_t{
  uint64_t gpr[32];
  uint64_t pc;
};
static int regNum = 0;
static inline bool difftest_check_reg(const char *name,uint64_t pc,uint64_t ref,uint64_t dut){
  if(ref != dut){
    printf("%s/%d is different after executing instruction at pc = %lx, right = %lx, wrong =%lx, diff = %lx,cycle = %ld\n",
           name,regNum,pc, ref, dut, ref ^ dut,cpu.cycle);
    return false;
  }
  return true;
}
static inline int check_reg_idx(int idx){
  assert(idx >= 0 && idx < 32);
  return idx;
}
const char *regs[] = {
    "$0", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
    "a1", "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
    "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

static inline const char *reg_name(int idx,int width){
  extern const char *regs[];
  return regs[check_reg_idx(idx)];
}

void diff_checkregs(void *diff_context,uint64_t cur_pc){
  struct diff_context_t *ctx = (struct diff_context_t *)diff_context;
  if(difftest_check_reg("pc",cpu.pc,ctx->pc,cur_pc)){
    for(int i = 0; i < 32; i++){
      regNum = i;
      if(difftest_check_reg(reg_name(i,64),cpu.pc,ctx->gpr[i],cpu.gpr[i]) == false){
        exit(1);
      }
    }
    return;
  }
  exit(1);
}

//获取DEF cpu当前状态
void *get_cpu_state(uint64_t pc){
  uint64_t cur_pc = pc;
  void *ptr = malloc(sizeof(uint64_t) + sizeof(cpu.gpr));
  memcpy(ptr,cpu.gpr,sizeof(cpu.gpr));
  memcpy((void *)((intptr_t)ptr+sizeof(cpu.gpr)),&cur_pc,sizeof(uint64_t));
  return ptr;
}

