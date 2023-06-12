/*
 * @Author: 苗金标
 * @Date: 2023-03-31 20:26:27
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-30 17:12:01
 * @Description: 
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <queue>
#include <iomanip>
#include <type_traits>
#include <unordered_map>
#include <memory>
#include <utility>
#include <algorithm>
#include <map>
#include <vector>
#include <cctype>
typedef uint64_t REG_WIDTH;

enum class FuType{
    alu,bru,csr,lsu,mdu,mou
};
enum class ALUOpType{
    add,sll,slt,sltu,xor_,srl,or_,and_,sub,sra,
    addw,subw,sllw,srlw,sraw,lui,auipc
};
enum class BRUOpType{
    jal,jalr,beq,bne,blt,bge,bltu,bgeu
};
/*csrrs读并置位，csrrc读并清除某位*/
enum class CSROpType{
    ecall,ebreak,sret,wfi,mret,csrrw,csrrs,csrrc,csrrwi,
    csrrsi,csrrci
};
enum class LSUOpType{
    lb,lh,lw,ld,lbu,lhu,lwu,sb,sh,sw,sd,
    lr ,sc ,amoswap ,amoadd ,amoxor ,amoand ,amoor ,amomin ,amomax ,amominu ,amomaxu,
    lrw,scw,amoswapw,amoaddw,amoxorw,amoandw,amoorw,amominw,amomaxw,amominuw,amomaxuw
};
enum class  MDUOpType{
    mul,mulh,mulhsu,mulhu,div,divu,rem,remu,
    mulw,divw,divuw,remw,remuw
};
enum class MOUOpType{
    fence,fencei,sfence_vma
};

enum amo_funct {
    AMOLR   = 0b00010,
    AMOSC   = 0b00011,
    AMOSWAP = 0b00001,
    AMOADD  = 0b00000,
    AMOXOR  = 0b00100,
    AMOAND  = 0b01100,
    AMOOR   = 0b01000,
    AMOMIN  = 0b10000,
    AMOMAX  = 0b10100,
    AMOMINU = 0b11000,
    AMOMAXU = 0b11100
};


//手册53页
enum rv_exc_code{
    exc_instr_misalign  = 0,
    exc_instr_acc_fault = 1,
    exc_illegal_instr   = 2,
    exc_breakpoint      = 3,
    exc_load_misalign   = 4,
    exc_load_acc_fault  = 5,
    exc_store_misalign  = 6, //including amo
    exc_store_acc_fault = 7,
    exc_ecall_from_u    = 8,
    exc_ecall_from_s    = 9,
    exc_ecall_from_m    = 11,
    exc_instr_pgfault   = 12,
    exc_load_pgfault    = 13,
    exc_store_pgfault   = 15, //including amo
    exc_custom_ok       = 24
};
typedef struct instStr{
    bool     enable;
    bool     valid;
    uint64_t pc;
    uint32_t inst;
    uint32_t rob_id;
    uint64_t imm;
    bool     has_execp;
    rv_exc_code execp_id;
    uint64_t execp_value;
    bool     checkpoint_id_valid;
    uint32_t checkpoint_id;
    bool     issued;


    bool     predicted;
    bool     predicted_jump;
    uint64_t predicted_next_pc;
    bool     bru_jump;
    uint64_t bru_next_pc;

    bool     jump;
    uint64_t next_pc;

    uint64_t debug_pc;

    uint32_t rs1_valid;//表示rs1信号是否有效
    uint32_t rs1_enable;
    uint32_t rs1_id;
    uint32_t rs1_phy;
    int64_t  rs1_value;

    uint32_t rs2_valid;//表示rs2信号是否有效
    uint32_t rs2_enable;
    uint32_t rs2_id;
    uint32_t rs2_phy;
    int64_t  rs2_value;
    
    uint32_t rd_enable;
    uint32_t rd_valid;//表示rd信号是否有效
    int64_t  rd_value;
    uint32_t rd_id;
    uint32_t rd_phy;

    FuType   fuType;
    union
    {
        ALUOpType aluOp;
        BRUOpType bruOp;
        CSROpType csrOp;
        LSUOpType lsuOp;
        MDUOpType mduOp;
        MOUOpType mouOp;
    }fuOpType;
    

}instStr;

union rv_instr {
    struct rv_r {
        unsigned int opcode : 7;
        unsigned int rd     : 5;
        unsigned int funct3 : 3;
        unsigned int rs1    : 5;
        unsigned int rs2    : 5;
        unsigned int funct7 : 7;
    } r_type;

    struct rv_i {
        unsigned int opcode : 7;
        unsigned int rd     : 5;
        unsigned int funct3 : 3;
        unsigned int rs1    : 5;
        int          imm12  : 12;
    } i_type;

    struct rv_s {
        unsigned int opcode : 7;
        unsigned int imm_4_0: 5;
        unsigned int funct3 : 3;
        unsigned int rs1    : 5;
        unsigned int rs2    : 5;
        int     imm_11_5    : 7;
    } s_type;

    struct rv_b {
        unsigned int opcode : 7;
        unsigned int imm_11 : 1;
        unsigned int imm_4_1: 4;
        unsigned int funct3 : 3;
        unsigned int rs1    : 5;
        unsigned int rs2    : 5;
        unsigned int imm_10_5: 6;
        int     imm_12      : 1;
    } b_type;

    struct rv_u {
        unsigned int opcode : 7;
        unsigned int rd     : 5;
        int     imm_31_12   : 20;
    } u_type;

    struct rv_j {
        unsigned int opcode : 7;
        unsigned int rd     : 5;
        unsigned int imm_19_12: 8;
        unsigned int imm_11 : 1;
        unsigned int imm_10_1: 10;
        int     imm_20      : 1;
    } j_type;
};
#define SEXT(x, len) ({ struct { int64_t n : len; } __x = { .n = x }; (uint64_t)__x.n; })
#define BITMASK(bits) ((1ull << (bits)) - 1)
#define BITS(x, hi, lo) (((x) >> (lo)) & BITMASK((hi) - (lo) + 1)) // similar to x[hi:lo] in verilog

inline uint64_t sign_extend(uint64_t imm, uint64_t imm_length)
{
    assert((imm_length > 0) && (imm_length < 64));
    auto sign_bit = (imm >> (imm_length - 1));
    auto extended_imm = ((sign_bit == 0) ? 0 : (((sign_bit << (64 - imm_length)) - 1) << imm_length)) | imm;
    return extended_imm;
}

//初始时，gpr要设置为0但gpr_v要设置为true;
struct CPU_state
{
    uint64_t gpr[32];
    bool     gpr_v[32];
    uint64_t pc;
    uint64_t cycle;
};
extern CPU_state cpu;

// static uint64_t diff_cycle = 94378880;
struct satp_def{
    uint64_t ppn      : 44;
    uint64_t asid     : 16;
    uint64_t mode     : 4;
};
struct sv39_va{
    uint64_t page_off   : 12;
    uint64_t vpn_0      : 9;
    uint64_t vpn_1      : 9;
    uint64_t vpn_2      : 9;
    uint64_t blank      : 25;
};
struct sv39_pte{
    uint64_t V : 1; //valid
    uint64_t R : 1; //read
    uint64_t W : 1; //write
    uint64_t X : 1; //execute
    uint64_t U : 1; //user
    uint64_t G : 1; //global
    uint64_t A : 1; //access
    uint64_t D : 1; //dirty
    uint64_t RSW : 2; //reserved for user by supervisor softwar
    uint64_t PPN0 : 9;
    uint64_t PPN1 : 9;
    uint64_t PPN2 : 26;
    uint64_t reserved : 7;
    uint64_t PBMT : 2; //Svpbmt is not implemented, return 0
    uint64_t N : 1;
};
static_assert(sizeof(sv39_pte) == 8,"sv39_pte should be 8 bytes.");
// note: A and D not implement. so use them as permission bit and raise page fault.

#define bitsizeof(x) (sizeof(x) * 8)