/*
 * ==================================================================
 * 
 * Filename: pmdTcpListener.hpp
 * 
 * Created: 10/15/2020 14:00 PM
 * 
 * Author: Yuan Haizhuo
 * 
 * ==================================================================
 */
#include "core.hpp"
#include "ossSocket.hpp"

#define PMD_TCPLISTENER_RETRY 5
#define OSS_MAX_SERVICENAME NI_MAXSERV

int pmdTcpListenerEntryPoint()
{
  int rc = EDB_OK;
  int port = 58127;
  ossSocket sock(port);
  rc = sock.initSocket();
  if (rc)
  {
    printf("Failed to initalize socket, rc = %d", rc);
    goto error;
  }
  rc = sock.bind_listen();
  if (rc)
  {
    printf("Failed to bind/listen socket, rc = %d", rc);
    goto error;
  }
  // master loop for tcp listener
  while (true)
  {
    int s;
    rc = sock.accept(&s, NULL, NULL);
    if (EDB_TIMEOUT == rc)
    {
      rc = EDB_OK;
      continue;
    }
    char buffer[1024];
    int size;
    ossSocket sock1(&s);
    sock1.disableNagle();
    do
    {
      rc = sock1.recv((char *)&size, 4);
      if (rc && rc != EDB_TIMEOUT)
      {
        printf("Failed to receive size, rc = %d", rc);
        goto error;
      }
    } while (rc == EDB_TIMEOUT);
    do
    {
      rc = sock1.recv(&buffer[0], size - sizeof(int));
      if (rc && rc != EDB_TIMEOUT)
      {
        printf("Failed to receive buffer, rc = %d", rc);
        goto error;
      }
    } while (rc == EDB_TIMEOUT);
    printf("%s\n", buffer);
    sock1.close();
  }

done:
  return rc;
error:
  switch (rc)
  {
  case EDB_SYS:
    printf("System error occurred!\n");
    break;
  default:
    printf("Internal error!\n");
  }

  goto done;
}