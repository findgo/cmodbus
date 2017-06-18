
/* ----------------------- Modbus includes ----------------------------------*/
#include "mbevent.h"

/**
  * @brief  �¼���ʼ��
  * @param  None
  * @retval None
  */
bool xMBSemBinaryInit(mb_Device_t *dev)
{
    dev->xEventInFlag = false;
    
    return true;
}

/**
  * @brief  �¼�����
  * @param  None
  * @retval None
  */
bool xMBSemGive(mb_Device_t *dev)
{
  dev->xEventInFlag = true;
  return true;
}

/**
  * @brief  �¼�����
  * @param  None
  * @retval None
  */
bool xMBSemTake(mb_Device_t *dev)
{  
  //�����¼�����
    if( dev->xEventInFlag ){
        //����¼�
        dev->xEventInFlag = false;
        return true;
    }
  
  return false;
}
