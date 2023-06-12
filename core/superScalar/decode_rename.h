/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:39:28
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-26 10:39:29
 * @Description: 
 */
/*
 * @Author: 苗金标
 * @Date: 2023-05-25 15:28:04
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-25 15:30:39
 * @Description: 
 */
#pragma once

// #pragma once
// #include "../common.h"
// #include "../config.h"
// #include "../component/fifo.h"

// typedef struct decode_issue_pack{
//     instStr decode_issue[DECODE_WIDTH];
// }decode_issue_pack;

#include "../common.h"
#include "../config.h"
#include "../component/fifo.h"

typedef struct decode_rename_pack{
    instStr decode_issue[DECODE_WIDTH];
}decode_rename_pack;
