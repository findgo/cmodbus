/**
  ******************************************************************************
  * @file
  * @author
  * @version
  * @date
  * @brief
  ******************************************************************************
  * @attention 	20181213     v2.1   	jgb		 功能完全版支持
  ******************************************************************************
  */

/*
 * 信息链表有两个主要API, MsgAlloc, MsgDealloc 所有信息的处理必须这里创建
 * MsgLen MsgSpare MsgSetSpare 在信息未释放前,可以使用spare 是信息里一个预留空间,8位,可供用户特殊使用
 *
 * 信息邮箱 只可使用前面创建的信息
 *
 * 信息队列 只可使用前面创建的信息, 注意,信息队列的API,未检测指针的有效性,仅供用户使用
 *
 * NOTE: MSG_BOX_UNLIMITED_CAP 表明信息条目不作限制
 */

#ifndef __MSG_LINK_H__
#define __MSG_LINK_H__

#include <stdint.h>
#include <stdbool.h>
#include "mem.h"

#ifdef __cplusplus
extern "C" {
#endif

// 表于创建信息邮箱,表示不限信息邮箱的存储数量
#define MSGBOX_UNLIMITED_CAP    0xffff

// 返回信息错误码
#define MSG_SUCCESS             (0)   // 成功
#define MSG_FAILED              (-1)  // 其它失败
#define MSG_INVALID_POINTER     (-2)  // 表明指针无效
#define MSG_BUFFER_NOT_AVAIL    (-3)  // 表明释放时,信息处于队列上,不可释放
#define MSG_QBOX_FULL           (-4)  // 信息邮箱满
#define MSG_INVALID_TASK        (-11)  // 无效任务

typedef struct {
    uint16_t dumy0;
    uint16_t dumy1;
    void *pdumy2;
} MsgBox_t;

// 队列
typedef void *MsgQ_t;

//静态初始化一个信息邮箱句柄缓存
#define MSGBOX_STATIC_INIT(MaxCap) { 0, (MaxCap), NULL}

// 信息
/**
 * @brief   分配一个信息
 * @param   len - 信息长度
 * @return  指向信息
 */
void *MsgAlloc(const uint16_t msgLen);

/**
 * @brief   释放一个信息
 * @param   msg_ptr - 指向信息
 * @return  返回错误码
 */
int MsgDealloc(void *const msg_ptr);

/**
 * @brief   获得信息的长度
 * @param   msg_ptr - 指向信息
 * @return  返回信息长度, 指针无效返回0
 */
uint16_t MsgLen(void *const msg_ptr);

/**
 * @brief   获得信息 预留空间的值 (供用户自定义使用)
 * @param   msg_ptr - 指向信息
 * @return  预留空间的值
 */
uint8_t MsgSpare(void *const msg_ptr);

/**
 * @brief   设置信息 预留空间的值 (供用户自定义使用)
 * @param   msg_ptr - 指向信息
 * @return 返回错误码
 */
int MsgSetSpare(void *const msg_ptr, const uint8_t val);

//信息邮箱
/**
 * @brief   创建一个信息邮箱句柄
 * @param   MaxCap - 邮箱最大存储能力,不限制设置为 MSGBOX_UNLIMITED_CAP
 * @return 返回句柄
 */
MsgBox_t *MsgBoxNew(const uint16_t MaxCap);

/**
 * @brief   用静态邮箱句柄缓存 创建 一个信息邮箱句柄
 * @param   MaxCap - 邮箱最大存储能力,不限制设置为 MSGBOX_UNLIMITED_CAP
 * @return 返回句柄
 */
void MsgBoxAssign(MsgBox_t *const pmsgboxBuffer, const uint16_t MaxCap);

/**
 * @brief   获得信息邮箱中信息数量
 * @param   msgbox_t * - 信息邮箱句柄
 * @return  信息数量
 */
uint16_t MsgBoxCnt(MsgBox_t *const msgbox);

/**
 * @brief   获得信息邮箱中空闲全置数量
 * @param   msgbox_t * - 信息邮箱句柄
 * @return  空闲位置数量
 */
uint16_t MsgBoxIdle(MsgBox_t *const msgbox);

/**
 * @brief   取出信息邮箱一条信息
 * @param   msgbox_t * - 信息邮箱句柄
 * @return  有信息返回信息指针,否则为NULL
 */
void *MsgBoxAccept(MsgBox_t *const msgbox);

/**
 * @brief   查看信息邮箱第一条信息,但并不取出
 * @param   msgbox_t * - 信息邮箱句柄
 * @return  有信息返回信息指针,否则为NULL
 */
void *MsgBoxPeek(MsgBox_t *const msgbox);
/**
 * @brief   向信息邮箱发送一条信息
 * @param   msgbox_t * - 信息邮箱句柄
 * @param   msg_ptr - 信息指针
 * @return  返回错误码
 */
#define MsgBoxPost(msgbox, msg_ptr)        msgBoxGenericpost(msgbox, msg_ptr, false)
/**
 * @brief   向信息邮箱头 发送一条信息
 * @param   msgbox_t * - 信息邮箱句柄
 * @param   msg_ptr - 信息指针
 * @return  返回错误码
 */
#define MsgBoxPostFront(msgbox, msg_ptr)   msgBoxGenericpost(msgbox, msg_ptr, true)

/*********************** 信息队列**********************************************/
// 可用于扩展信息功能时使用
/**
 * @brief   取出信息队列第一条信息
 * @param   q_ptr - 信息队列头
 * @return  有信息返回信息指针,否则为NULL
 */
void *MsgQPop(MsgQ_t *const q_ptr);

/**
 * @brief   查看信息队列第一条信息,但不取出
 * @param   q_ptr - 信息队列头
 * @return  有信息返回信息指针,否则为NULL
 */
void *MsgQPeek(MsgQ_t *const q_ptr);
/**
 * @brief   向信息队列 发送一条信息
 * @param   q_ptr - 信息队列头
 * @param   msg_ptr - 信息指针
 * @return  返回错误码
 */
#define MsgQPut(q_ptr, msg_ptr)  MsgQGenericPut( q_ptr, msg_ptr, false )
/**
 * @brief   向信息队列头 发送一条信息
 * @param   q_ptr - 信息队列头
 * @param   msg_ptr - 信息指针
 * @return  返回错误码
 */
#define MsgQPutFront(q_ptr, msg_ptr)  MsgQGenericPut( q_ptr, msg_ptr, true )

/**
 * @brief   将信息从队列中踢出 Take out of the link list
 * @param   q_ptr - 信息队列头
 * @param   msg_ptr - 信息指针
 * @param   preMsg_ptr - 前一条信息指针
 * @return  返回错误码
 */
void MsgQExtract(MsgQ_t *const q_ptr, void *const msg_ptr, void *const preMsg_ptr);
// scan msgQ each message
#define MsgQ_for_each_msg(q_ptr, listmsg) for(listmsg = *(q_ptr); listmsg != NULL;listmsg = MsgQNext(listmsg))
// how to take a messge from the list
/*
{
    void *prev = NULL;
    void *srch;

    msgQ_for_each_msg(q_ptr, srch){
        if(message find){
            //take out the list
            msgQextract(q_ptr, srch, prev);
           // do you job
        }
        else{
            prev = srch; // save previous message
        }
    }
}
//or
{
    void *prev = NULL;
    void *srch;

    msgQ_for_each_msg(q_ptr, srch){
        if(message find)
            break;
        prev = srch; // save previous message
    }

    if(srh){
        //take out the list
        MsgQExtract(q_ptr, srch, prev);
    }

}
*/
/*********************** 信息队列**********************************************/

/*********************** 内部API,不可独立调用**********************************************/
/**
 * @brief   向信息邮箱 发送一条信息
 * @param   msgbox_t * - 信息邮箱句柄
 * @param   msg_ptr - 信息指针
 * @param   isFront - false: 放入信息邮箱尾 : true: 放入信息邮箱头
 * @return  返回错误码
 */
int MsgBoxGenericPost(MsgBox_t *const msgbox, void *const msg_ptr, const uint8_t isFront);

/**
 * @brief   向信息队列头 发送一条信息
 * @param   q_ptr - 信息队列头
 * @param   msg_ptr - 信息指针
 * @param   isFront - false: 放入信息队列尾 : true: 放入信息队列头
 * @return  返回错误码
 */
void MsgQGenericPut(MsgQ_t *const q_ptr, void *const msg_ptr, const uint8_t isFront);

/**
 * @brief   从信息获得下一条信息        ( 信息必需在队列上,这属于内部API,不得调用 )
 * @param   msg_ptr - 信息指针
 * @return  下一条消息指针
 */
void *MsgQNext(void *const msg_ptr);
/*********************** 内部API,不可独立调用**********************************************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif


