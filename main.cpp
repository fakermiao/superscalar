/*
 * @Author: 苗金标
 * @Date: 2023-06-05 19:42:09
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-09 15:38:01
 * @Description: 
 * 
 * 
 * ****************
 * 由于issue_q先输出再处理输入因此重命名到发射存在一个周期延迟
 * ****************
 */
#include <stdio.h>
#include <filesystem>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "core/device/uartlite.h"
#include "core/memory/ram.h"
#include "core/config.h"
#include "core/superScalar/plic.h"
#include "core/superScalar/clint.h"
#include "core/superScalar/fetch.h"
#include "core/superScalar/decode.h"
#include "core/superScalar/rename.h"
#include "core/superScalar/issue.h"
#include "core/superScalar/execute/alu.h"
#include "core/superScalar/execute/bru.h"
#include "core/superScalar/execute/csr.h"
#include "core/superScalar/execute/lsu.h"
#include "core/superScalar/execute/mdu.h"
#include "core/superScalar/execute/mou.h"
#include "core/superScalar/wb.h"

#include "core/component/bp/branch_predict.h"
#include "core/component/checkpoint_buffer.h"
#include "core/component/rat.h"
#include "core/component/store_buffer.h"

#define ARRLEN(arr) (int)(sizeof(arr) / sizeof(arr[0]))
CPU_state cpu;


component::memory mem(0,0x80000000);
// const char *load_path = "test/rv64mi/rv64mi-p-illegal.bin";
//step1. test phy address,ma_data、simple do not test
//0615: fix lsu bugs, all I instructions test execpt ma_data、simple，all M instructions test
//0616: 修改原子指令逻辑，成功跑通所有原子指令，之前原子指令逻辑错误，没有写回寄存器的值
//step2. test virtual address
const char *load_path = "test/rv64ui/rv64ui-v-add.bin";
// const char *load_path = "test/rv64um/rv64um-v-div.bin";
// const char *load_path = "test/rv64ua/rv64ua-v-lrsc.bin";
// const char *load_path = "test/fw_payload.bin";

ram dram(4096ul*1024l*1024l,(unsigned long)0x0000,load_path);
l2_cache <L2_WAYS,L2_NR_SETS,L2_SZLINE,32> l2;
Clint<2> clint;
Plic<4,4> plic;
uartlite uart;
Supercore::Priv priv(&mem,0,l2);

component::branch_predictor bp;
component::checkpoint_buffer cp(16);
component::rat rat(PHY_REG_NUM,ARCH_REG_NUM);
component::rob rob(ROB_SIZE);
component::store_buffer storeBuffer(STORE_BUFFER_SIZE);
component::issue_queue<instStr> issue_q(ISSUE_QUEUE_SIZE);
component::regfile<uint64_t> phy_regfile(PHY_REG_NUM);
component::fifo<instStr> *issue_alu_fifo[ALU_UNIT_NUM];
component::fifo<instStr> *issue_bru_fifo[BRU_UNIT_NUM];
component::fifo<instStr> *issue_csr_fifo[CSR_UNIT_NUM];
component::fifo<instStr> *issue_lsu_fifo[LSU_UNIT_NUM];
component::fifo<instStr> *issue_mdu_fifo[MDU_UNIT_NUM];
component::fifo<instStr> *issue_mou_fifo[MOU_UNIT_NUM];

component::fifo<instStr> *alu_wb_fifo[ALU_UNIT_NUM];
component::fifo<instStr> *bru_wb_fifo[BRU_UNIT_NUM];
component::fifo<instStr> *csr_wb_fifo[CSR_UNIT_NUM];
component::fifo<instStr> *lsu_wb_fifo[LSU_UNIT_NUM];
component::fifo<instStr> *mdu_wb_fifo[MDU_UNIT_NUM];
component::fifo<instStr> *mou_wb_fifo[MOU_UNIT_NUM];

component::fifo<fetch_decode_pack> fetch_decode_fifo(FETCH_DECODE_FIFO_SIZE);
// component::fifo<decode_issue_pack> decode_issue_fifo(DECODE_RENAME_FIFO_SIZE);
component::fifo<decode_rename_pack> decode_rename_fifo(DECODE_RENAME_FIFO_SIZE);
component::fifo<rename_issue_pack> rename_issue_fifo(RENAME_ISSUE_FIFO_SIZE);
            // issue(component::fifo<rename_issue_pack> *rename_issue_fifo,component::fifo<instStr> **issue_alu_fifo,component::fifo<instStr> **issue_bru_fifo,
            //     component::fifo<instStr> **issue_csr_fifo,component::fifo<instStr> **issue_lsu_fifo,component::fifo<instStr> **issue_mdu_fifo,
            //     component::fifo<instStr> **issue_mou_fifo,component::regfile<uint64_t> *phy_regfile,component::checkpoint_buffer *cp)

static Supercore::bru_feedback_pack bru_feedback;
static Supercore::wb_feedback_pack  wb_feedback;
static Supercore::exe_feedback_t exe_feedback;

Supercore::fetch  fetch_stage(0x80000000,priv,&mem,&fetch_decode_fifo,&bp,&cp);
Supercore::decode decode_stage(&fetch_decode_fifo,&decode_rename_fifo);
Supercore::rename rename_stage(&decode_rename_fifo,&rename_issue_fifo,&rat,&rob,&cp);
Supercore::issue  issue_stage(issue_q,&rename_issue_fifo,issue_alu_fifo,issue_bru_fifo,issue_csr_fifo,issue_lsu_fifo,issue_mdu_fifo,
                          issue_mou_fifo,&phy_regfile,&cp);
Supercore::alu    *alu_stage[ALU_UNIT_NUM];
Supercore::bru    *bru_stage[BRU_UNIT_NUM];
Supercore::csr    *csr_stage[CSR_UNIT_NUM];
Supercore::lsu    *lsu_stage[LSU_UNIT_NUM];
Supercore::mdu    *mdu_stage[MDU_UNIT_NUM];
Supercore::mou    *mou_stage[MOU_UNIT_NUM];
Supercore::wb     wb_stage(priv,true,&bp,&cp,alu_wb_fifo,bru_wb_fifo,csr_wb_fifo,lsu_wb_fifo,mdu_wb_fifo,mou_wb_fifo,
                            &rat,&rob,&phy_regfile,&storeBuffer);

void init(){
    assert(l2.add_dev(0x2000000,0x10000,&clint));
    assert(l2.add_dev(0xc000000,0x4000000,&plic));
    assert(l2.add_dev(0x60100000,1024*1024,&uart));
    assert(l2.add_dev(0x80000000,2048l*1024l*1024l,&dram));
    std::ifstream file(load_path,std::ios::in | std::ios::binary);
    file.read((char *)mem.mem,std::filesystem::file_size(load_path));
  
    memset(&bru_feedback,0,sizeof(bru_feedback));
    memset(&wb_feedback,0,sizeof(wb_feedback));
    memset(&exe_feedback,0,sizeof(exe_feedback));

    //初始化执行单元以及输入输出
    for(uint32_t i = 0;i < ALU_UNIT_NUM;i++){
        issue_alu_fifo[i]   = new component::fifo<instStr>(1);
        alu_wb_fifo[i]      = new component::fifo<instStr>(1);
        alu_stage[i]        = new Supercore::alu(i,issue_alu_fifo[i],alu_wb_fifo[i]);
    }
    for(uint32_t i = 0;i < BRU_UNIT_NUM;i++){
        issue_bru_fifo[i]   = new component::fifo<instStr>(1);
        bru_wb_fifo[i]      = new component::fifo<instStr>(1);
        bru_stage[i]        = new Supercore::bru(i,issue_bru_fifo[i],bru_wb_fifo[i]);
    }
    for(uint32_t i = 0;i < CSR_UNIT_NUM;i++){
        issue_csr_fifo[i]   = new component::fifo<instStr>(1);
        csr_wb_fifo[i]      = new component::fifo<instStr>(1);
        csr_stage[i]        = new Supercore::csr(i,issue_csr_fifo[i],csr_wb_fifo[i],priv);
    }
    for(uint32_t i = 0;i < LSU_UNIT_NUM;i++){
        issue_lsu_fifo[i]   = new component::fifo<instStr>(1);
        lsu_wb_fifo[i]      = new component::fifo<instStr>(1);
        lsu_stage[i]        = new Supercore::lsu(i,issue_lsu_fifo[i],lsu_wb_fifo[i],priv,&storeBuffer);
    }
    for(uint32_t i = 0;i < MDU_UNIT_NUM;i++){
        issue_mdu_fifo[i]   = new component::fifo<instStr>(1);
        mdu_wb_fifo[i]      = new component::fifo<instStr>(1);
        mdu_stage[i]        = new Supercore::mdu(i,issue_mdu_fifo[i],mdu_wb_fifo[i]);
    }
    for(uint32_t i = 0;i < MOU_UNIT_NUM;i++){
        issue_mou_fifo[i]   = new component::fifo<instStr>(1);
        mou_wb_fifo[i]      = new component::fifo<instStr>(1);
        mou_stage[i]        = new Supercore::mou(i,issue_mou_fifo[i],mou_wb_fifo[i],priv);
    }

    rat.reset();
    phy_regfile.reset();

    //初始化寄存器
    for(int i = 0;i < 32;i++){
        cpu.gpr[i] = 0;
        cpu.gpr_v[i] = true;
    }
    cpu.pc = 0x80000000;
    cpu.cycle = 0;
}

bool delay_cr = false;
void exec_once(){
    cpu.cycle++;
    clint.set_time(cpu.cycle + 1);
    plic.update_ext(1,uart.irq());
    // meip、 msip、 mtip、 seip
    bool meip = plic.get_int(0);
    bool msip = clint.m_s_irq(0);
    bool mtip = clint.m_t_irq(0);
    bool seip = plic.get_int(1);

    priv.inter_exec(meip,msip,mtip,seip);
    wb_feedback = wb_stage.evaluate();
    //功能单元的执行
    {
        uint32_t exe_num = 0;
        for(uint32_t i = 0;i < ALU_UNIT_NUM;i++){
            exe_feedback.exe_channel[exe_num++] = alu_stage[i]->evaluate(wb_feedback);
        }
        for(uint32_t i = 0;i < BRU_UNIT_NUM;i++){
            exe_feedback.exe_channel[exe_num++] = bru_stage[i]->evaluate(wb_feedback);
        }
        for(uint32_t i = 0;i < CSR_UNIT_NUM;i++){
            exe_feedback.exe_channel[exe_num++] = csr_stage[i]->evaluate(wb_feedback);
        }
        for(uint32_t i = 0;i < LSU_UNIT_NUM;i++){
            exe_feedback.exe_channel[exe_num++] = lsu_stage[i]->evaluate(wb_feedback);
        }
        for(uint32_t i = 0;i < MDU_UNIT_NUM;i++){
            exe_feedback.exe_channel[exe_num++] = mdu_stage[i]->evaluate(wb_feedback);
        }
        for(uint32_t i = 0;i < MOU_UNIT_NUM;i++){
            exe_feedback.exe_channel[exe_num++] = mou_stage[i]->evaluate(wb_feedback);
        }
    }
    issue_stage.evaluate(wb_feedback,exe_feedback);
    rename_stage.evaluate(wb_feedback);
    decode_stage.evaluate(wb_feedback);
    fetch_stage.evaluate(bru_feedback,wb_feedback);

    rat.sync();
    storeBuffer.sync();
    cp.sync();
    phy_regfile.sync();
    issue_q.sync();
    printf("\n");
}

void exec(uint64_t n){
    for(uint64_t i = 0;i < n;i++)
        exec_once();
}

static int cmd_help(char *args);
static int cmd_q(char *args) {
    return -1;
}
static int cmd_c(char *args){
    exec(-1);
    return 0;
}
static int cmd_si(char *args) {
    int steps;
    if(args == NULL){ steps = 1; }
    else sscanf(args,"%d",&steps);
    exec(steps);
    return 0;
}
static int cmd_info(char *args){
    for(int i = 0;i < 32;i++){
        printf("gpr[%02d]\t:%08lx\n",i,cpu.gpr[i]);
    }
    return 0;
}
static struct {
    const char *name;
    const char *description;
    int (*handler) (char *);
}cmd_table [] = {
    { "help", "Display informations about all supported commands", cmd_help },
    { "c", "Continue the execution of the program", cmd_c },
    { "q", "Exit NEMU", cmd_q },
    { "si", "Single step execution 单步执行", cmd_si },
    { "info", "Print program status 打印程序状态", cmd_info },
    // { "x", "Scan memory 扫描内存", cmd_x},
    // { "p", "表达式求值", cmd_p },
    // { "w", "set watchpoint", cmd_w},
    // { "d", "delte watchpoint", cmd_d},
  /* TODO: Add more commands */
};

#define NR_CMD ARRLEN(cmd_table)
static int cmd_help(char *args) {
    /* extract the first argument */
    char *arg = strtok(NULL, " ");
    int i;

    if (arg == NULL) {
        /* no argument given */
        for (i = 0; i < NR_CMD; i ++) {
            printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        }
    }
    else {
        for (i = 0; i < NR_CMD; i ++) {
            if (strcmp(arg, cmd_table[i].name) == 0) {
                printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
                return 0;
            }
        }
        printf("Unknown command '%s'\n", arg);
    }
    return 0;
}

static char* rl_gets() {
    static char *line_read = NULL;

    if (line_read) {
        free(line_read);
        line_read = NULL;
    }

    line_read = readline("(miao) ");

    if (line_read && *line_read) {
        add_history(line_read);
    }

    return line_read;
}
void sdb_mainloop() {
    for (char *str; (str = rl_gets()) != NULL; ) {
        char *str_end = str + strlen(str);

        /* extract the first token as the command */
        char *cmd = strtok(str, " ");
        if (cmd == NULL) { continue; }

        /* treat the remaining string as the arguments,
        * which may need further parsing
        */
        char *args = cmd + strlen(cmd) + 1;
        if (args >= str_end) {
            args = NULL;
        }

        int i;
        for (i = 0; i < NR_CMD; i ++) {
            if (strcmp(cmd, cmd_table[i].name) == 0) {
            if (cmd_table[i].handler(args) < 0) { return; }
                break;
            }
        }

        if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
    }
}


int main(int argc,char *argv[]){
    
    init();
    sdb_mainloop();
    return 0;
}
