/*
 * ==================================================================
 * 
 * Filename: pmdTcpListener.cpp
 * 
 * Created: 10/15/2020 14:00 PM
 * 
 * Author: Yuan Haizhuo
 * 
 * ==================================================================
 */
#include "core.hpp"
#include "ossSocket.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "pmdEDUMgr.hpp"
#include "pd.hpp"

#define PMD_TCPLISTENER_RETRY 5
#define OSS_MAX_SERVICENAME NI_MAXSERV

int pmdTcpListenerEntryPoint(pmdEDUCB *cb, void *arg)
{
  int rc = EDB_OK;
  pmdEDUMgr *eduMgr = cb->getEDUMgr();
  EDUID myEDUID = cb->getID();
  unsigned int retry = 0;
  EDUID agentEDU = PMD_INVALID_EDUID;
  char svcName[OSS_MAX_SERVICENAME + 1];
  // 重试循环，如果因为网络问题进不去里层循环太多次那么我们就直接结束吧
  while (retry <= PMD_TCPLISTENER_RETRY && !EDB_IS_DB_DOWN)
  {
    retry++;

    strcpy(svcName, pmdGetKRCB()->getSvcName());

    int port = 0;
    int len = strlen(svcName);
    for (int i = 0; i < len; i++)
    {
      // 字符串转成数字，即端口号
      if (svcName[i] >= '0' && svcName[i] <= '9')
      {
        port = port * 10;
        port += int(svcName[i] - '0');
      }
      else
      {
        PD_LOG(PDERROR, "service name error!\n");
      }
    }

    ossSocket sock(port);
    rc = sock.initSocket();
    EDB_VALIDATE_GOTOERROR(EDB_OK == rc, rc, "Failed initialize socket")

    rc = sock.bind_listen();
    EDB_VALIDATE_GOTOERROR(EDB_OK == rc, rc, "Failed to bind/listen socket");
    // once bind is successful, let's set the state of EDU to RUNNING
    if (EDB_OK != (rc = eduMgr->activateEDU(myEDUID)))
    {
      goto error;
    }
    // 监听循环
    while (!EDB_IS_DB_DOWN)
    {
      int s;
      rc = sock.accept(&s, NULL, NULL);
      // 因为是监听超时，所以不妨继续
      if (EDB_TIMEOUT == rc)
      {
        rc = EDB_OK;
        continue;
      }
      // 裂开了就直接结束
      if (rc && EDB_IS_DB_DOWN)
      {
        rc = EDB_OK;
        goto done;
      }
      else if (rc)
      {
        // 重启本地socket即可
        PD_LOG(PDERROR, "Failed to accept socket in TcpListener");
        PD_LOG(PDEVENT, "Restarting socket to listen");
        break;
      }

      // assign the socket to the arg
      void *pData = NULL;
      *((int *)&pData) = s;

      rc = eduMgr->startEDU(EDU_TYPE_AGENT, pData, &agentEDU);
      if (rc)
      {
        if (rc == EDB_QUIESCED)
        {
          PD_LOG(PDWARNING, "Reject new connection due to quiesced database");
        }
        else
        {
          PD_LOG(PDERROR, "Failed to start EDU agent");
        }
        // fail, close socket
        ossSocket newsock(&s);
        newsock.close();
        continue;
      }
    }
    if (EDB_OK != (rc = eduMgr->waitEDU(myEDUID)))
    {
      goto error;
    }
  } // while(retry <= PMD_TCPLISTENER_RETRY)
done:
  return rc;
error:
  switch (rc)
  {
  case EDB_SYS:
    PD_LOG(PDSEVERE, "System error occured");
    break;
  default:
    PD_LOG(PDSEVERE, "Internal error");
  }
  goto done;
}