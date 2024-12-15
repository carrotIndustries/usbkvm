#include "../Inc/i2c_comm.h"
#include "tusb.h"
#include "common/tusb_fifo.h"
#include "stm32f0xx_hal.h"

extern I2C_HandleTypeDef hi2c1;


static void *ptxbuf ;
static void *prxbuf;

static i2c_req_all_t i2c_req_rxbuf;
static i2c_resp_all_t i2c_resp_txbuf;
i2c_resp_all_t i2c_resp_buf;
static uint8_t is_rx;

static void copy_resp(i2c_resp_all_t *dest, const i2c_resp_all_t *src)
{
  const uint8_t *ps = (const void*)src;
  uint8_t *pd = (void*)dest;
  ps+=sizeof(i2c_resp_all_t);
  pd+=sizeof(i2c_resp_all_t);
  do {
    ps--;
    pd--;
    *pd = *ps;
  } while(ps != (void*)src);
}


TU_FIFO_DEF(i2c_req_fifo,  4, i2c_req_all_t, 0);

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
  if( TransferDirection==I2C_DIRECTION_TRANSMIT ) {
    is_rx = 1;
    prxbuf = &i2c_req_rxbuf;
    HAL_I2C_Slave_Seq_Receive_IT(hi2c, prxbuf, 1, I2C_NEXT_FRAME);
    prxbuf++;
  }
  else {
    is_rx = 0;
    copy_resp(&i2c_resp_txbuf, &i2c_resp_buf);
    ptxbuf = &i2c_resp_txbuf;
    ptxbuf++; // skip _head
    HAL_I2C_Slave_Seq_Transmit_IT(hi2c, ptxbuf, 1, I2C_NEXT_FRAME);
    ptxbuf++;
  }
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  HAL_I2C_Slave_Seq_Transmit_IT(hi2c, ptxbuf, 1, I2C_NEXT_FRAME);
  ptxbuf++;
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  HAL_I2C_Slave_Seq_Receive_IT(hi2c, prxbuf, 1, I2C_NEXT_FRAME);
  if((prxbuf-((void*)&i2c_req_rxbuf)) < sizeof(i2c_req_rxbuf))
    prxbuf++;
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if(is_rx) {
    tu_fifo_write(&i2c_req_fifo, &i2c_req_rxbuf);
  }
  hi2c1.Instance->TXDR = 0x83;
  HAL_I2C_EnableListen_IT(hi2c); // slave is ready again
}

uint8_t i2c_read_req(i2c_req_all_t *req)
{
    return tu_fifo_read(&i2c_req_fifo, req);
}
