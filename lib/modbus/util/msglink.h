/**
 * @file msglink.h
 * @brief 消息,邮箱,队列API库
 * @author mo
 * @version v0.1.0
 * @date 2018-12-01
 *
 * @pre mem.h
 * @attention 消息链表有两个主要API, MsgAlloc, MsgDealloc 所有消息的处理必须 ::MsgAlloc 创建, ::MsgDealloc 释放 \n
 * MsgLen MsgSpare MsgSetSpare 在消息未释放前,可以使用, Spare是消息里一个预留空间,8位,可供用户特殊使用 \n
 * 消息邮箱 只可使用 ::MsgAlloc 创建的消息 \n
 * 消息队列 只可使用 ::MsgAlloc 创建的消息, @b 信息队列的API未检测指针的有效性由用户处理
 */

#ifndef __MSG_LINK_H__
#define __MSG_LINK_H__

#include <stdint.h>
#include <stdbool.h>
#include "mem.h"

#ifdef __cplusplus
extern "C" {
#endif

//! 表于创建消息邮箱时,不限信息消箱的存储数量
#define MSGBOX_UNLIMITED_CAP    0xffff


#define MSG_SUCCESS             (0)   //!< 成功
#define MSG_FAILED              (-1)  //!< 表明其它失败
#define MSG_INVALID_POINTER     (-2)  //!< 表明指针无效
#define MSG_BUFFER_NOT_AVAIL    (-3)  //!< 表明释放时,信息处于队列上,不可释放
#define MSG_QBOX_FULL           (-4)  //!< 表明消息邮箱满

//! 消息邮箱结构体
typedef struct {
    uint16_t dumy0;
    uint16_t dumy1;
    void *pdumy2;
} MsgBox_t;

//！ 队列指针
typedef void *MsgQ_t;

//! 静态初始化一个信息邮箱句柄缓存
#define MSGBOX_STATIC_INIT(MaxCap) { 0, (MaxCap), NULL}


/**
 * @brief   分配一条指定len长度的消息
 * @param   len 消息长度
 * @return  指向消息的指针
 *      - nil 表示分配失败
 */
void *MsgAlloc(const uint16_t len);

/**
 * @brief   释放一条消息
 * @param   msg_ptr 指向消息指针
 * @return  0 成功, < 0 错误码
 * @see     错误码相关 ::MSG_SUCCESS
 * @pre     msg_ptr 变量必须 ::MsgAlloc 的返回值
 * @see     ::MsgAlloc
 */
int MsgDealloc(void *const msg_ptr);

/**
 * @brief   获得消息的长度
 * @param   msg_ptr 指向消息指针
 * @return  返回消息长度,如果指针无效时返回0
 * @pre     msg_ptr 变量必须 ::MsgAlloc 的返回值
 */
uint16_t MsgLen(void *const msg_ptr);

/**
 * @brief   获得消息的预留空间的值 (供用户自定义使用)
 * @param   msg_ptr 指向消息的指针
 * @retval  预留空间的值,如果指针无效时返回0
 * @pre msg_ptr 变量必须 ::MsgAlloc 的返回值
 */
uint8_t MsgSpare(void *const msg_ptr);

/**
 * @brief   设置消息的预留空间的值 (供用户自定义使用)
 * @param   msg_ptr 指向消息的指针
 * @return  0 成功, < 0 错误码
 * @see     错误码相关 ::MSG_SUCCESS
 * @pre     msg_ptr 变量必须 ::MsgAlloc 的返回值
 */
int MsgSetSpare(void *const msg_ptr, const uint8_t val);

/**
 * @brief   动态创建一个消息邮箱,获得句柄
 * @param   MaxCap 邮箱最大存储能力,不限制设置为 ::MSGBOX_UNLIMITED_CAP
 * @return  返回句柄
 *      - nil 表示分配失败
 */
MsgBox_t *MsgBoxNew(const uint16_t MaxCap);

/**
 * @brief 使用静态邮箱句柄缓存 初始化一个消息邮箱句柄
 * @param pMsgBoxBuffer 静态结构体,或静态初始化 ::MSGBOX_STATIC_INIT
 * @param MaxCap 邮箱最大存储能力,不限制设置为 ::MSGBOX_UNLIMITED_CAP
 * @return None
 * @par 示例
 * @code
 * static MsgBox_t msgBox
 * MsgBoxAssign(&msgBox,100)
 * // static MsgBox_t msgBox = MSGBOX_STATIC_INIT(100)
 * @endcode
 */
void MsgBoxAssign(MsgBox_t *const pMsgBoxBuffer, const uint16_t MaxCap);

/**
 * @brief   获得消息邮箱中消息数量
 * @param   msgBox 消息邮箱句柄
 * @return  消息数量,无效指针返回0
 * @pre     msgBox 变量必须 ::MsgBoxNew 的返回值
 */
uint16_t MsgBoxCnt(MsgBox_t *const msgBox);

/**
 * @brief   获得消息邮箱中空闲数量
 * @param   msgBox 消息邮箱句柄
 * @return  空闲消息数量,无效指针返回0
 * @pre msgBox 变量必须 ::MsgBoxNew 的返回值
 */
uint16_t MsgBoxIdle(MsgBox_t *const msgBox);

/**
 * @brief   获取消息邮箱一条信息
 * @param   msgBox 消息邮箱句柄
 * @return  消息指针
 *      - NULL 无消息
 * @pre msgBox 变量必须 ::MsgBoxNew 的返回值
 */
void *MsgBoxAccept(MsgBox_t *const msgBox);

/**
 * @brief   查看消息邮箱第一条信息,但并不取出
 * @param   msgBox 消息邮箱句柄
 * @return 消息指针
 *      - NULL 无消息
 * @pre msgBox 变量必须 ::MsgBoxNew 的返回值
 */
void *MsgBoxPeek(MsgBox_t *const msgBox);

/**
 * @brief   向消息邮箱发送一条消息
 * @param   msgBox 消息邮箱句柄
 * @param   msg_ptr 消息指针
 * @return  0 成功, < 0 错误码
 * @see     错误码相关 ::MSG_SUCCESS
 * @pre     msgBox变量必须 ::MsgBoxNew 分配,或 ::MsgBoxAssign 静态分配 \n
 *          msg_ptr变量必须 ::MsgAlloc 的返回值
 * @see     ::MsgBoxNew,::MsgBoxAssign,::MsgAlloc,::MsgBoxGenericPost
 */
#define MsgBoxPost(msgBox, msg_ptr) MsgBoxGenericPost(msgBox, msg_ptr, false)

/**
 * @brief   向消息邮箱头 发送一条信息
 * @param   msgBox 消息邮箱句柄
 * @param   msg_ptr 消息指针
 * @return  0 成功, < 0 错误码
 * @see     错误码相关 ::MSG_SUCCESS
 * @pre     msgBox变量必须 ::MsgBoxNew 分配,或 ::MsgBoxAssign 静态分配 \n
 *          msg_ptr变量必须 ::MsgAlloc 的返回值
 * @see     ::MsgBoxNew,::MsgBoxAssign,::MsgAlloc,::MsgBoxGenericPost
 */
#define MsgBoxPostFront(msgBox, msg_ptr) MsgBoxGenericPost(msgBox, msg_ptr, true)

/*********************** 信息队列 可用于扩展信息功能时使用 **********************************************/
/**
 * @brief   取出消息队列第一条信息
 * @param   q_ptr 信息队列头
 * @return  返回信息指针
 *      - NULL 无消息
 */
void *MsgQPop(MsgQ_t *const q_ptr);

/**
 * @brief   查看消息队列第一条信息,但不取出
 * @param   q_ptr 消息队列头
 * @return  返回信息指针
 *      - NULL 无消息
 */
void *MsgQPeek(MsgQ_t *const q_ptr);
/**
 * @brief   向消息队列 发送一条信息
 * @param   q_ptr 消息队列头
 * @param   msg_ptr 消息指针
 * @return  0 成功, < 0 错误码
 * @see     错误码相关 ::MSG_SUCCESS
 * @pre     msg_ptr 变量必须 ::MsgAlloc 的返回值
 */
#define MsgQPut(q_ptr, msg_ptr)  MsgQGenericPut( q_ptr, msg_ptr, false )
/**
 * @brief   向信息队列头 发送一条信息
 * @param   q_ptr 消息队列头
 * @param   msg_ptr 消息指针
 * @return  0 成功, < 0 错误码
 * @see     错误码相关 ::MSG_SUCCESS
 * @pre     msg_ptr 变量必须 ::MsgAlloc 的返回值
 */
#define MsgQPutFront(q_ptr, msg_ptr)  MsgQGenericPut( q_ptr, msg_ptr, true )

/**
 * @brief   将消息从队列中踢出 Take out of the link list
 * @param   q_ptr 消息队列头
 * @param   msg_ptr 消息指针
 * @param   preMsg_ptr - 前一条信息指针
 * @return  0 成功, < 0 错误码
 * @see     错误码相关 ::MSG_SUCCESS
 * @pre     msg_ptr,preMsg_ptr 变量必须 ::MsgAlloc 的返回值
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

/*********************** 消息队列**********************************************/
/*********************** 内部API,不可独立调用**********************************************/
/**
 * @brief   向消息邮箱发送一条消息
 * @param   msgBox 消息邮箱句柄
 * @param   msg_ptr 消息指针
 * @param   isFront 消息位置
 *      - false: 放入消息邮箱尾
 *      - true: 放入消息邮箱头
 * @return  0 成功, < 0 错误码
 * @pre     msgBox变量必须 ::MsgBoxNew 分配,或 ::MsgBoxAssign 静态分配 \n
 *          msg_ptr变量必须 ::MsgAlloc 的返回值
 * @see     ::MsgBoxNew,::MsgBoxAssign,::MsgAlloc
 */
int MsgBoxGenericPost(MsgBox_t *const msgBox, void *const msg_ptr, const uint8_t isFront);

/**
 * @brief   向信息队列 发送一条信息
 * @param   q_ptr 消息队列头
 * @param   msg_ptr 消息指针
 * @param   isFront 消息位置
 *      - false: 放入消息邮箱尾
 *      - true: 放入消息邮箱头
 * @return  0 成功, < 0 错误码
 * @see     错误码相关 ::MSG_SUCCESS
 * @pre     msg_ptr 变量必须 ::MsgAlloc 的返回值
 */
void MsgQGenericPut(MsgQ_t *const q_ptr, void *const msg_ptr, const uint8_t isFront);

/**
 * @brief   从消息链表中获得下一条消息
 * @param   msg_ptr 消息指针
 * @return  下一条消息指针
 *      - NULL 无消息
 * @pre     msg_ptr 变量必须 ::MsgAlloc 的返回值
 * @note    消息必需在队列上,这属于内部API,不建议调用
 */
void *MsgQNext(void *const msg_ptr);
/*********************** 内部API,不可独立调用**********************************************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif


