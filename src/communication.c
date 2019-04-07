/**
 * @file    Communication.c
 * @author  Ferenc Nemeth
 * @date    22 Aug 2018
 * @brief   _write() function for printf() and tracer.
 *
 *          Copyright (c) 2018 Ferenc Nemeth - https://github.com/ferenc-nemeth
 *          Copyright (c) 2019 Vadym Mishchuk - https://github.com/vad32m
 */
#include <stdint.h>

#include <libopencm3/cm3/itm.h>
#include <libopencm3/cm3/scb.h>

#include "fault_handler.h"

/**
 * @brief   This is needed by printf().
 * @param   file
 * @param   *data
 * @param   len
 * @return  len
 */
int _write(int file, char *data, int len)
{
  for (uint16_t i = 0u; i < len; i++)
  {
      while (!(ITM_STIM8(0) & ITM_STIM_FIFOREADY)) {}

	    ITM_STIM8(0) = data[i];
  }

  return len;
}
