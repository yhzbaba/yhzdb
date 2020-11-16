/*
 * ==================================================================
 * 
 * Filename: pmd.cpp
 * 
 * Created: 11/03/2020 21:35 PM
 * 
 * Author: Yuan Haizhuo
 * 
 * ==================================================================
 */
#include "pmd.hpp"
#include "pmdOptions.hpp"
#include "pd.hpp"

EDB_KRCB pmd_krcb;
extern char _pdDiagLogPath[OSS_MAX_PATHSIZE + 1];

int EDB_KRCB::init(pmdOptions *options)
{
  setDBStatus(EDB_DB_NORMAL);
  setDataFilePath(options->getDBPath());
  setLogFilePath(options->getLogPath());
  strncpy(_pdDiagLogPath, getLogFilePath(), sizeof(_pdDiagLogPath));
  setSvcName(options->getServiceName());
  setMaxPool(options->getMaxPool());
  PD_LOG(PDEVENT, "%d", options->getMaxPool());
  return EDB_OK;
}