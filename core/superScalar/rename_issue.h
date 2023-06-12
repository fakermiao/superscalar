/*
 * @Author: 苗金标
 * @Date: 2023-05-26 10:41:39
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-26 10:41:41
 * @Description: 
 */
/*
 * @Author: 苗金标
 * @Date: 2023-05-25 15:42:17
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-25 15:46:41
 * @Description: 
 */
#pragma once
#include "../common.h"
#include "../config.h"
#include "../component/fifo.h"

typedef struct rename_issue_pack{
    instStr rename_issue[RENAME_WIDTH];
}rename_issue_pack;