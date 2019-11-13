#include "msglink.h"

#define MSG_BOX_CNT(msgBox_ptr)      (((msgBoxInner_t *) (msgBox_ptr))->count)
#define MSG_BOX_CAP(msgBox_ptr)      (((msgBoxInner_t *) (msgBox_ptr))->capacity)
#define MSG_BOX_QHEAD(msgBox_ptr)      (((msgBoxInner_t *) (msgBox_ptr))->qHead)

#define MSG_HDR_MARK(msg_ptr)     (((msgHdr_t *) (msg_ptr) - 1)->mark)
#define MSG_HDR_SPARE(msg_ptr)    (((msgHdr_t *) (msg_ptr) - 1)->spare)
#define MSG_HDR_LEN(msg_ptr)      (((msgHdr_t *) (msg_ptr) - 1)->len)
#define MSG_HDR_NEXT(msg_ptr)     (((msgHdr_t *) (msg_ptr) - 1)->next)

// 信息头部
typedef struct {
    uint8_t mark; // 标记是否在链表中
    uint8_t spare;
    uint16_t len;
    void *next;
} msgHdr_t;

// 消息队列结构体
typedef struct {
    uint16_t count;
    uint16_t capacity;
    MsgQ_t qHead;
} msgBoxInner_t;

void *MsgAlloc(const uint16_t msgLen) {
    msgHdr_t *hdr;

    if (msgLen == 0)
        return (void *) NULL;

    hdr = (msgHdr_t *) KMalloc((size_t) (sizeof(msgHdr_t) + msgLen));
    if (hdr) {
        hdr->next = NULL;
        hdr->len = msgLen;
        hdr->mark = 0; // not on qbox list
        hdr->spare = 0;

        return ((void *) (hdr + 1)); // pass head, point to the data
    }

    return (void *) (NULL);
}

int MsgFree(void *const msg_ptr) {
    if (msg_ptr == NULL)
        return (MSG_INVALID_POINTER);

    // don't deallocate msg buffer when it on the list
    if (MSG_HDR_MARK(msg_ptr) == 1)
        return (MSG_BUFFER_NOT_AVAIL);

    KFree((void *) ((uint8_t *) msg_ptr - sizeof(msgHdr_t)));

    return (MSG_SUCCESS);
}

uint16_t MsgLen(void *const msg_ptr) {
    if (msg_ptr == NULL)
        return 0;

    return MSG_HDR_LEN(msg_ptr);
}

int MsgSetSpare(void *const msg_ptr, const uint8_t val) {
    if (msg_ptr == NULL)
        return (MSG_INVALID_POINTER);

    MSG_HDR_SPARE(msg_ptr) = val;

    return MSG_SUCCESS;
}

uint8_t MsgSpare(void *const msg_ptr) {
    if (msg_ptr == NULL)
        return 0;

    return MSG_HDR_SPARE(msg_ptr);
}

MsgBox_t *MsgBoxNew(const uint16_t MaxCap) {
    msgBoxInner_t *pNewmsgbox;

    pNewmsgbox = (msgBoxInner_t *) KMalloc(sizeof(msgBoxInner_t));
    if (pNewmsgbox) {
        pNewmsgbox->capacity = MaxCap;
        pNewmsgbox->count = 0;
        pNewmsgbox->qHead = NULL;
    }

    return (MsgBox_t *) pNewmsgbox;
}

void MsgBoxAssign(MsgBox_t *const pMsgBoxBuffer, const uint16_t MaxCap) {
    msgBoxInner_t *pNewmsgbox = (msgBoxInner_t *) pMsgBoxBuffer;
    if (pNewmsgbox) {
        pNewmsgbox->capacity = MaxCap;
        pNewmsgbox->count = 0;
        pNewmsgbox->qHead = NULL;
    }
}

uint16_t MsgBoxCnt(MsgBox_t *const msgBox) {
    if (msgBox == NULL)
        return 0;

    return MSG_BOX_CNT(msgBox);
}

uint16_t MsgBoxIdle(MsgBox_t *const msgBox) {
    if (msgBox == NULL)
        return 0;

    return (MSG_BOX_CAP(msgBox) - MSG_BOX_CNT(msgBox));
}

void *MsgBoxAccept(MsgBox_t *const msgBox) {
    // no message on the list
    if (MSG_BOX_CNT(msgBox) == 0)
        return NULL;

    MSG_BOX_CNT(msgBox)--;

    return MsgQPop(&MSG_BOX_QHEAD(msgBox));
}

void *MsgBoxPeek(MsgBox_t *const msgBox) {
    // no message on the list
    if (MSG_BOX_CNT(msgBox) == 0)
        return NULL;

    return MsgQPeek(&MSG_BOX_QHEAD(msgBox));
}

int MsgBoxGenericPost(MsgBox_t *const msgBox, void *const msg_ptr, const uint8_t isFront) {
    if (msg_ptr == NULL || msgBox == NULL) {
        return (MSG_INVALID_POINTER);
    }

    if (MSG_BOX_CAP(msgBox) != MSGBOX_UNLIMITED_CAP && ((MSG_BOX_CAP(msgBox) - MSG_BOX_CNT(msgBox)) < 1))
        return MSG_QBOX_FULL;

    // Check the message header ,not init it success, or message on the list
    if (MSG_HDR_NEXT(msg_ptr) != NULL || MSG_HDR_MARK(msg_ptr) != 0) {
        return (MSG_INVALID_POINTER);
    }

    MSG_BOX_CNT(msgBox)++;
    MsgQGenericPut(&MSG_BOX_QHEAD(msgBox), msg_ptr, isFront);

    return (MSG_SUCCESS);
}

void MsgQGenericPut(MsgQ_t *const q_ptr, void *const msg_ptr, const uint8_t isFront) {
    void *list;

    MSG_HDR_MARK(msg_ptr) = 1; // mark on the list
    if (isFront == 1) { // put to front
        // Push message to head of queue
        MSG_HDR_NEXT(msg_ptr) = *q_ptr;
        *q_ptr = msg_ptr;
    } else { // put to back
        // set nex to null
        MSG_HDR_NEXT(msg_ptr) = NULL;
        // If first message in queue
        if (*q_ptr == NULL) {
            *q_ptr = msg_ptr;
        } else {
            // Find end of queue
            for (list = *q_ptr; MSG_HDR_NEXT(list) != NULL; list = MSG_HDR_NEXT(list));

            // Add message to end of queue
            MSG_HDR_NEXT(list) = msg_ptr;
        }
    }
}

void *MsgQPop(MsgQ_t *const q_ptr) {
    void *msg_ptr = NULL;

    if (*q_ptr != NULL) {
        // Dequeue message
        msg_ptr = *q_ptr;
        *q_ptr = MSG_HDR_NEXT(msg_ptr);
        MSG_HDR_NEXT(msg_ptr) = NULL;
        MSG_HDR_MARK(msg_ptr) = 0;
    }

    return msg_ptr;
}

void *MsgQPeek(MsgQ_t *const q_ptr) {
    return (void *) (*q_ptr);
}

// Take out of the link list
void MsgQExtract(MsgQ_t *const q_ptr, void *const msg_ptr, void *const preMsg_ptr) {
    if (msg_ptr == *q_ptr) {
        // remove from first
        *q_ptr = MSG_HDR_NEXT(msg_ptr);
    } else {
        // remove from middle
        MSG_HDR_NEXT(preMsg_ptr) = MSG_HDR_NEXT(msg_ptr);
    }
    MSG_HDR_NEXT(msg_ptr) = NULL;
    MSG_HDR_MARK(msg_ptr) = 0;
}

// get next message
void *MsgQNext(void *const msg_ptr) {
    return MSG_HDR_NEXT(msg_ptr);
}




