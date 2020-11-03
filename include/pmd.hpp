/*
 * ==================================================================
 * 
 * Filename: pmd.hpp
 * 
 * Created: 11/02/2020 20:48 PM
 * 
 * Author: Yuan Haizhuo
 * 
 * ==================================================================
 */
#ifndef PMD_HPP__
#define PMD_HPP__

#include "core.hpp"

// 正常，关闭，非正常退出状态
enum EDB_DB_STATUS
{
  EDB_DB_NORMAL = 0,
  EDB_DB_SHUTDOWN,
  EDB_DB_PANIC
};

#define EDB_IS_DB_NORMAL (EDB_DB_NORMAL == pmdGetKRCB()->getDBStatus())
#define EDB_IS_DB_DOWN (EDB_DB_SHUTDOWN == pmdGetKRCB()->getDBStatus() || EDB_DB_PANIC == pmdGetKRCB()->getDBStatus())
#define EDB_IS_DB_UP (!EDB_IS_DB_DOWN)
#define EDB_SHUTDOWN_DB (pmdGetKRCB()->setDBStatus(EDB_DB_SHUTDOWN));

class pmdOptions;

// kernel control block
class EDB_KRCB
{
private:
  // configured options
  char _dataFilePath[OSS_MAX_PATHSIZE + 1];
  char _logFilePath[OSS_MAX_PATHSIZE + 1];
  int _maxPool;
  char _svcName[NI_MAXSERV + 1];
  EDB_DB_STATUS _dbStatus;

public:
  EDB_KRCB()
  {
    _dbStatus = EDB_DB_NORMAL;
    memset(_dataFilePath, 0, sizeof(_dataFilePath));
    memset(_dataFilePath, 0, sizeof(_logFilePath));
    memset(_svcName, 0, sizeof(_svcName));
    _maxPool = 0;
  }
  ~EDB_KRCB() {}

  // get database status
  inline EDB_DB_STATUS getDBStatus()
  {
    return _dbStatus;
  }
  // get data file path
  inline const char *getDataFilePath()
  {
    return _dataFilePath;
  }
  // get log file path
  inline const char *getLogFilePath()
  {
    return _logFilePath;
  }
  // get service name
  inline const char *getSvcName()
  {
    return _svcName;
  }
  // get max thread pool
  inline int getMaxPool()
  {
    return _maxPool;
  }
  // setup database status
  inline void setDBStatus(EDB_DB_STATUS status)
  {
    _dbStatus = status;
  }

  // set data file path
  void setDataFilePath(const char *pPath)
  {
    strncpy(_dataFilePath, pPath, sizeof(_dataFilePath));
  }

  // set log file path
  void setLogFilePath(const char *pPath)
  {
    strncpy(_logFilePath, pPath, sizeof(_logFilePath));
  }
  // set service name
  void setSvcName(const char *pName)
  {
    strncpy(_svcName, pName, sizeof(_svcName));
  }
  // set max pool
  void setMaxPool(int maxPool)
  {
    _maxPool = maxPool;
  }
  // set up from pmdOptions
  int init(pmdOptions *options);
};

extern EDB_KRCB pmd_krcb;

inline EDB_KRCB *pmdGetKRCB()
{
  return &pmd_krcb;
}

#endif