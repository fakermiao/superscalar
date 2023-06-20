/*
 * @Author: 苗金标
 * @Date: 2023-04-06 15:07:01
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-06 15:02:29
 * @Description: https://zhuanlan.zhihu.com/p/486528142
 * https://blog.csdn.net/Pandacooker/article/details/116423306
 */
#pragma once
#include "../../common.h"
namespace Supercore{
enum priv_mode{
    U_MODE = 0,
    S_MODE = 1,
    H_MODE = 2,
    M_MODE = 3
};

enum rv_csr_addr{
    //unprivileged counter/times
    csr_cycle   = 0xc00,
    csr_time    = 0xc01,
    csr_instret = 0xc02,        //instructions retired counter
    //supervisor trap setup
    csr_sstatus = 0x100,        //status register
    csr_sie     = 0x104,        //interrupt-enable register
    csr_stvec   = 0x105,        //trap handler base address
    csr_scounteren = 0x106,     //counter enable
    //supervisor trap handling
    csr_sscratch= 0x140,        //scratch register for supervisor trap handlers
    csr_sepc    = 0x141,        //execption program counter
    csr_scause  = 0x142,        //trap cause
    csr_stval   = 0x143,        //bad address or instruction
    csr_sip     = 0x144,        //interrupt pending
    //supervisor protection and translation
    csr_satp    = 0x180,
    //machine information registers
    csr_mvendorid = 0xf11,      //vendor ID
    csr_marchid = 0xf12,        //architecture ID
    csr_mimpid  = 0xf13,        //implementation ID
    csr_mhartid = 0xf14,        //hardware thread ID
    csr_mconfigptr = 0xf15,     //pointer to configuration data structure
    //machine trap setup
    /*
    +-63-+-62------38-+--37--+--36--+-35----34-+-33----32-+-31----23-+--22--+--21--+--20--+--19--+--18--+--
   | SD  |    WPRI    | MBE  | SBE  | SXL[1:0] | UXL[1:0] |   WPRI   | TSR  |  TW  |  TVW |  MXR | SUM  | 
   +-----+------------+------+------+----------+----------+----------+------+------+------+------+------+--

   +--17--+-16---15-+-14---13-+-12----11-+-10----9-+--8--+---7---+---6---+---5---+---4---+--3--+---2---+--1--+--0---+
  | MPRV  | XS[1:0] | FS[1:0] | MPP[1:0] | VS[1:0] | SPP | MPIE  |  UBE  | SPIE  | WPRI  | MIE | WPRI  | SIE | WPRI |
  +-------+---------+---------+----------+---------+-----+-------+-------+-------+-------+-----+-------+-----+------+
  https://blog.csdn.net/dai_xiangjun/article/details/123373456
  进入中断时设置mie为0屏蔽中断，mpie设置为当前mie，mpp设置为当前特权形式
  返回时，从mpie、mpp恢复状态
    */
    csr_mstatus = 0x300,   
    /*
    LEN-1-----LEN-2---LEN-3--------26--25---------------------0
   |   MXL[1:0]     |        0        |    extensions[25:0]    |
   +----------------+-----------------+------------------------+
   MXL为1表示32位，为2表示64位，为3表示128位
   extensions对应26个英文字母及表示相应的指令集
    */     
    csr_misa    = 0x301,
    csr_medeleg = 0x302,        //machine execption delegation register，每一位对应一种异常
    csr_mideleg = 0x303,        //machine interrupt delegation register，每一位对应一种中断
    csr_mie     = 0x304,        //每一位对应一种中断是否使能
    /*
    +-LEN-1-----------------2--+-1-------0-+
    |     BASE[LEN-1:2]        |   MODE    |
    +--------------------------+-----------+ 

    MODE为0时直接寻址，跳转地址为BASE; MODE为1时间接寻址，跳转地址为BASE+4*cause
    */
    csr_mtvec   = 0x305,
    csr_mcounteren = 0x306,
    //machine trap handling
    csr_mscratch = 0x340,
    csr_mepc    = 0x341,
    csr_mcause  = 0x342,
    csr_mtval   = 0x343,
    csr_mip     = 0x344,        //每一位对应一种中断是否挂起
    //machine memory protection
    csr_pmpcfg0 = 0x3a0,
    csr_pmpcfg1 = 0x3a1,
    csr_pmpcfg2 = 0x3a2,
    csr_pmpcfg3 = 0x3a3,
    csr_pmpaddr0= 0x3b0,
    csr_pmpaddr1= 0x3b1,
    csr_pmpaddr2= 0x3b2,
    csr_pmpaddr3= 0x3b3,
    csr_pmpaddr4= 0x3b4,
    csr_pmpaddr5= 0x3b5,
    csr_pmpaddr6= 0x3b6,
    csr_pmpaddr7= 0x3b7,
    csr_pmpaddr8= 0x3b8,
    csr_pmpaddr9= 0x3b9,
    csr_pmpaddr10   = 0x3ba,
    csr_pmpaddr11   = 0x3bb,
    csr_pmpaddr12   = 0x3bc,
    csr_pmpaddr13   = 0x3bd,
    csr_pmpaddr14   = 0x3be,
    csr_pmpaddr15   = 0x3bf,
    //machine counter/timers
    csr_mcycle  = 0xb00,
    csr_minstret  = 0xb02,
    //debug/trace registers
    csr_tselect = 0x7a0,
    csr_tdata1  = 0x7a1
    
};

struct csr_mstatus_def {
    uint64_t blank0 : 1;
    uint64_t sie    : 1;// supervisor interrupt enable
    uint64_t blank1 : 1;
    uint64_t mie    : 1;// machine interrupt enable
    uint64_t blank2 : 1;
    uint64_t spie   : 1;// sie prior to trapping
    uint64_t ube    : 1;// u big-endian, zero
    uint64_t mpie   : 1;// mie prior to trapping
    uint64_t spp    : 1;// supervisor previous privilege mode.
    uint64_t vs     : 2;// without vector, zero
    uint64_t mpp    : 2;// machine previous privilege mode.
    uint64_t fs     : 2;// without float, zero
    uint64_t xs     : 2;// without user ext, zero
    uint64_t mprv   : 1;// Modify PRiVilege (Turn on virtual memory and protection for load/store in M-Mode) when mpp is not M-Mode
    // mprv will be used by OpenSBI.
    uint64_t sum    : 1;// permit Supervisor User Memory access
    uint64_t mxr    : 1;// Make eXecutable Readable
    uint64_t tvm    : 1;// Trap Virtual Memory (raise trap when sfence.vma and sinval.vma executing in S-Mode)
    uint64_t tw     : 1;// Timeout Wait for WFI
    uint64_t tsr    : 1;// Trap SRET
    uint64_t blank3 : 9;
    uint64_t uxl    : 2;// user xlen
    uint64_t sxl    : 2;// supervisor xlen
    uint64_t sbe    : 1;// s big-endian
    uint64_t mbe    : 1;// m big-endian
    uint64_t blank4 : 25;
    uint64_t sd     : 1;// no vs,fs,xs, zero
};

#define rv_ext(x) (1<<(x-'a'))
struct csr_misa_def {
    uint64_t ext    : 26;
    uint64_t blank  : 36;
    uint64_t mxl    : 2;
};

struct csr_sstatus_def {
    uint64_t blank0 : 1;
    uint64_t sie    : 1;// supervisor interrupt enable
    uint64_t blank1 : 3;
    uint64_t spie   : 1;// sie prior to trapping
    uint64_t ube    : 1;// u big-endian, zero
    uint64_t blank2 : 1;// mie prior to trapping
    uint64_t spp    : 1;// supervisor previous privilege mode.
    uint64_t vs     : 2;// without vector, zero
    uint64_t blank3 : 2;// machine previous privilege mode.
    uint64_t fs     : 2;// without float, zero
    uint64_t xs     : 2;// without user ext, zero
    uint64_t blank4 : 1;
    uint64_t sum    : 1;// permit Supervisor User Memory access
    uint64_t mxr    : 1;// Make eXecutable Readable
    uint64_t blank5 : 12;
    uint64_t uxl    : 2;// user xlen
    uint64_t blank6 : 29;
    uint64_t sd     : 1;// no vs,fs,xs, zero
};

enum rv_init_code{
    int_s_sw    = 1,
    int_m_sw    = 3,
    int_s_timer = 5,
    int_m_timer = 7,
    int_s_ext   = 9,
    int_m_ext   = 11
};


const uint64_t s_int_mask   = (1ull<<int_s_ext) | (1ull<<int_s_sw) | (1ull<<int_s_timer);
const uint64_t m_int_mask   = s_int_mask | (1ull<<int_m_ext) | (1ull<<int_m_sw) | (1ull<<int_m_timer);
const uint64_t s_exc_mask   = (1<<16)  - 1 - (1<<exc_ecall_from_m);

struct csr_tvec_def{
    uint64_t mode : 2;  //0:redirect, 1:vectored
    uint64_t base : 62;
};

struct int_def{
    uint64_t blank0 : 1;
    uint64_t s_s_ip : 1; // 1
    uint64_t blank1 : 1;
    uint64_t m_s_ip : 1; // 3
    uint64_t blank2 : 1;
    uint64_t s_t_ip : 1; // 5
    uint64_t blank3 : 1;
    uint64_t m_t_ip : 1; // 7
    uint64_t blank4 : 1;
    uint64_t s_e_ip : 1; // 9
    uint64_t blank5 : 1;
    uint64_t m_e_ip : 1; // 11
};
const uint64_t counter_mask = (1<<0) | (1<<2);

struct csr_cause_def{
    uint64_t cause : 63;
    uint64_t interrupt : 1;
    csr_cause_def(uint64_t cause,uint64_t interrupt=0):cause(cause),interrupt(interrupt){}
    csr_cause_def():cause(0),interrupt(0){}
};

struct csr_counteren_def{
    uint64_t cycle : 1;
    uint64_t time  : 1;
    uint64_t instr_retire : 1;
    uint64_t blank : 61;
};
}


















