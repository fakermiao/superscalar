/*
 * @Author: 苗金标
 * @Date: 2023-05-29 20:13:13
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-05-29 20:15:42
 * @Description: 
 */
#pragma once
#include <cstdint>

const uint32_t ALU_UNIT_NUM = 2;
const uint32_t BRU_UNIT_NUM = 1;
const uint32_t CSR_UNIT_NUM = 1;
const uint32_t LSU_UNIT_NUM = 1;
const uint32_t MDU_UNIT_NUM = 2;
const uint32_t MOU_UNIT_NUM = 1;

const uint32_t EXECUTE_UNIT_NUM = ALU_UNIT_NUM + BRU_UNIT_NUM + CSR_UNIT_NUM + LSU_UNIT_NUM + MDU_UNIT_NUM + MOU_UNIT_NUM;