/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:39:19
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-26 10:39:20
 * @Description: 
 */
/*
 * @Author: 苗金标
 * @Date: 2023-04-03 20:42:44
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-04-03 21:07:29
 * @Description: 
 */
#pragma once
#include "../common.h"
#include "../config.h"
#include "../component/fifo.h"

typedef struct decode_issue_pack{
    instStr decode_issue[DECODE_WIDTH];
}decode_issue_pack;