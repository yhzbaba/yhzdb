/*
 * ==================================================================
 * 
 * Filename: pmdAgent.cpp
 * 
 * Created: 11/23/2020 18:49 PM
 * 
 * Author: Yuan Haizhuo
 * 
 * ==================================================================
 */
#include "pd.hpp"
#include "pmdEDUMgr.hpp"
#include "pmdEDU.hpp"
#include "ossSocket.hpp"
#include "../bson/src/bson.h"
#include "pmd.hpp"
using namespace bson;
using namespace std;

// y的倍数向上对齐
#define ossRoundUpToMultipleX(x, y) (((x) + ((y)-1)) - (((x) + ((y)-1)) % (y)))
#define PMD_AGENT_RECIEVE_BUFFER_SZ 4096
#define EDB_PAGE_SIZE 4096

static int pmdProcessAgentRequest(char *pReceiveBuffer, int packetSize,
                                  char **ppResultBuffer, int *pResultBuffersize,
                                  bool *disconnect, pmdEDUCB *cb)
{
  EDB_ASSERT(disconnect, "disconnect can't be NULL!");
  EDB_ASSERT(pReceiveBuffer, "pReceivedBuffer is NULL!");
  PD_LOG(PDEVENT, "Process Agent Request");
  int rc = EDB_OK;
  unsigned int probe = 0;
  **(int **)(ppResultBuffer) = 4;
  *pResultBuffersize = 4;
  return rc;
}

struct MsgReply
{
  int a;
};

int pmdAgentEntryPoint(pmdEDUCB *cb, void *arg)
{
  int rc = EDB_OK;
  unsigned int probe = 0;
  bool disconnect = false;
  char *pReceiveBuffer = NULL;
  char *pResultBuffer = NULL;
  int receiveBufferSize = ossRoundUpToMultipleX(PMD_AGENT_RECIEVE_BUFFER_SZ, EDB_PAGE_SIZE);

  int resultBufferSize = sizeof(MsgReply);
  int packetLength = 0;
  EDUID myEDUID = cb->getID();
  pmdEDUMgr *eduMgr = cb->getEDUMgr();

  // 接收socket
  int s = *((int *)&arg);
  ossSocket sock(&s);
  sock.disableNagle();

  // 分配接收内存
  pReceiveBuffer = (char *)malloc(sizeof(char) * receiveBufferSize);
  if (!pReceiveBuffer)
  {
    rc = EDB_OOM;
    probe = 10;
    goto error;
  }

  // 分配返回内存
  pResultBuffer = (char *)malloc(sizeof(char) * resultBufferSize);
  if (!pResultBuffer)
  {
    rc = EDB_OOM;
    probe = 10;
    goto error;
  }

  while (!disconnect)
  {
    // receive next packet
    // 首先接收大小
    rc = pmdRecv(pReceiveBuffer, sizeof(int), &sock, cb);
    if (rc)
    {
      if (rc == EDB_APP_FORCED)
      {
        disconnect = true;
        continue;
      }
      probe = 30;
      goto error;
    }
    packetLength = *(int *)(pReceiveBuffer);
    PD_LOG(PDDEBUG, "Received packet size = %d", packetLength);
    if (packetLength < (int)sizeof(int))
    {
      probe = 40;
      rc = EDB_INVALIDARG;
      goto error;
    }
    if (receiveBufferSize < packetLength + 1)
    {
      PD_LOG(PDDEBUG, "Receive buffer size is too small: %d , but packetLength is: %d", receiveBufferSize, packetLength);
      int newSize = ossRoundUpToMultipleX(packetLength + 1, EDB_PAGE_SIZE);
      if (newSize < 0)
      {
        probe = 50;
        rc = EDB_INVALIDARG;
        goto error;
      }
      free(pReceiveBuffer);
      pReceiveBuffer = (char *)malloc(sizeof(char) * (newSize));
      if (!pReceiveBuffer)
      {
        rc = EDB_OOM;
        probe = 60;
        goto error;
      }
      *(int *)(pReceiveBuffer) = packetLength;
      receiveBufferSize = newSize;
    }
    // 接收正式数据
    rc = pmdRecv(&pReceiveBuffer[sizeof(int)], packetLength - sizeof(int), &sock, cb);
    if (rc)
    {
      if (rc == EDB_APP_FORCED)
      {
        disconnect = true;
        continue;
      }
      probe = 70;
      goto error;
    }
    // 保证包以0结尾
    pReceiveBuffer[packetLength] = 0;
    if (EDB_OK != (rc = eduMgr->activateEDU(myEDUID)))
    {
      goto error;
    }
    if (resultBufferSize > (int)sizeof(MsgReply))
    {
      resultBufferSize = (int)sizeof(MsgReply);
      free(pResultBuffer);
      pResultBuffer = (char *)malloc(sizeof(char) * resultBufferSize);
      if (!pResultBuffer)
      {
        rc = EDB_OOM;
        probe = 80;
        goto error;
      }
    }
    rc = pmdProcessAgentRequest(pReceiveBuffer, packetLength, &pResultBuffer, &resultBufferSize, &disconnect, cb);
    if (rc)
    {
      PD_LOG(PDERROR, "Error processing Agent request, rc = %d", rc);
    }
    // 如果不是断开，那么我们需要发送返回值
    if (!disconnect)
    {
      rc = pmdSend(pResultBuffer, *(int *)pResultBuffer, &sock, cb);
      if (rc)
      {
        if (rc == EDB_APP_FORCED)
          ;
        probe = 90;
        goto error;
      }
    }
    if (EDB_OK != (rc = eduMgr->waitEDU(myEDUID)))
    {
      goto error;
    }
  }
done:
  if (pReceiveBuffer)
    free(pReceiveBuffer);
  if (pResultBuffer)
    free(pResultBuffer);
  sock.close();
  return rc;
error:
  switch (rc)
  {
  case EDB_SYS:
    PD_LOG(PDSEVERE, "EDU id %d cannot be found, probe %d", myEDUID, probe);
    break;
  case EDB_EDU_INVAL_STATUS:
    PD_LOG(PDSEVERE, "EDU status is not valid, probe %d", probe);
    break;
  case EDB_INVALIDARG:
    PD_LOG(PDSEVERE, "Invalid atgument recieved by agent, probe %d", probe);
    break;
  case EDB_OOM:
    PD_LOG(PDSEVERE, "Failed to allocate memory by agent, probe %d", probe);
    break;
  case EDB_NETWORK:
    PD_LOG(PDSEVERE, "Network error occured, probe %d", probe);
    break;
  case EDB_NETWORK_CLOSE:
    PD_LOG(PDDEBUG, "Remote connection closed");
    rc = EDB_OK;
    break;
  default:
    PD_LOG(PDSEVERE, "Internal error, probe %d", probe);
  }
  goto done;
}
