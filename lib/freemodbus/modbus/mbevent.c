
/* ----------------------- Modbus includes ----------------------------------*/
#include "mbevent.h"

/**
  * @brief  �¼���ʼ��
  * @param  None
  * @retval None
  */
bool xMBEventInit(mb_device_t *dev)
{
    dev->xEventInFlag = false;
    
    return true;
}

/**
  * @brief  �¼�����
  * @param  None
  * @retval None
  */
bool xMBEventPost(mb_device_t *dev)
{
  dev->xEventInFlag = true;
  return true;
}

/**
  * @brief  �¼�����
  * @param  None
  * @retval None
  */
bool xMBEventGet(mb_device_t *dev)
{  
  //�����¼�����
    if( dev->xEventInFlag ){
        //����¼�
        dev->xEventInFlag = false;
        return true;
    }
  
  return false;
}
