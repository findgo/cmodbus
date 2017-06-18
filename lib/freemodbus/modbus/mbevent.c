
/* ----------------------- Modbus includes ----------------------------------*/
#include "mbevent.h"

/**
  * @brief  事件初始化
  * @param  None
  * @retval None
  */
bool xMBSemBinaryInit(mb_Device_t *dev)
{
    dev->xEventInFlag = false;
    
    return true;
}

/**
  * @brief  事件发送
  * @param  None
  * @retval None
  */
bool xMBSemGive(mb_Device_t *dev)
{
  dev->xEventInFlag = true;
  return true;
}

/**
  * @brief  事件接收
  * @param  None
  * @retval None
  */
bool xMBSemTake(mb_Device_t *dev)
{  
  //若有事件更新
    if( dev->xEventInFlag ){
        //获得事件
        dev->xEventInFlag = false;
        return true;
    }
  
  return false;
}
