/*
 * ==================================================================
 * 
 * Filename: pmdEDUMgr.hpp
 * 
 * Created: 11/09/2020 20:09 PM
 * 
 * Author: Yuan Haizhuo
 * 
 * thread pool
 * 
 * ==================================================================
 */
#ifndef PMDEDUMGR_HPP__
#define PMDEDUMGR_HPP__

#include "core.hpp"
#include "pmdEDU.hpp"
#include "ossLatch.hpp"
#include "ossUtil.hpp"

#define EDU_SYSTEM 0x01
#define EDU_USER 0x02
#define EDU_ALL (EDU_SYSTEM | EDU_USER)

class pmdEDUMgr
{
private:
  std::map<EDUID, pmdEDUCB *> _runQueue;
  std::map<EDUID, pmdEDUCB *> _idleQueue;

  std::map<unsigned int, EDUID> _tid_eduid_map;

  ossSLatch _mutex;

  EDUID _EDUID;
  // list of system EDUS
  std::map<unsigned int, EDUID> _mapSystemEDUS;
  // is suspended
  bool _isQuiesced;
  bool _isDestroyed;

public:
  pmdEDUMgr() : _EDUID(1), _isQuiesced(false), _isDestroyed(false) {}
  ~pmdEDUMgr() { reset(); }
  void reset()
  {
    _destroyAll();
  }
  unsigned int size()
  {
    unsigned int num = 0;
    _mutex.get_shared();
    num = (unsigned int)_runQueue.size() + (unsigned int)_idleQueue.size();
    _mutex.release_shared();
    return num;
  }
  unsigned int sizeIdle()
  {
    unsigned int num = 0;
    _mutex.get_shared();
    num = (unsigned int)_idleQueue.size();
    _mutex.release_shared();
    return num;
  }
  unsigned int sizeRun()
  {
    unsigned int num = 0;
    _mutex.get_shared();
    num = (unsigned int)_runQueue.size();
    _mutex.release_shared();
    return num;
  }
  // 拿到系统edu的数量
  unsigned int sizeSystem()
  {
    unsigned int num = 0;
    _mutex.get_shared();
    num = (unsigned int)_mapSystemEDUS.size();
    _mutex.release_shared();
    return num;
  }
  EDUID getSystemEDU(EDU_TYPES edu)
  {
    EDUID eduID = PMD_INVALID_EDUID;
    _mutex.get_shared();
    std::map<unsigned int, EDUID>::iterator it = _mapSystemEDUS.find(edu);
    if (it != _mapSystemEDUS.end())
    {
      eduID = it->second;
    }
    _mutex.release_shared();
    return eduID;
  }
  // 判断一个eduID是否为系统EDU
  bool isSystemEDU(EDUID eduID)
  {
    bool isSys = false;
    _mutex.get_shared();
    isSys = _isSystemEDU(eduID);
    _mutex.release_shared();
    return isSys;
  }
  // 注册系统edu
  void regSystemEDU(EDU_TYPES edu, EDUID eduid)
  {
    _mutex.get();
    _mapSystemEDUS[edu] = eduid;
    _mutex.release();
  }
  bool isQuiesced()
  {
    return _isQuiesced;
  }
  void setQuiesced(bool b)
  {
    _isQuiesced = b;
  }
  bool isDestroyed()
  {
    return _isDestroyed;
  }
  // 是否可以回线程池
  static bool isPoolable(EDU_TYPES type)
  {
    return (EDU_TYPE_AGENT == type);
  }

private:
  int _createNewEDU(EDU_TYPES type, void *arg, EDUID *eduID);
  int _destroyAll();                                 // √
  int _forceEDUs(int property = EDU_ALL);            // √
  unsigned int _getEDUCount(int property = EDU_ALL); // √
  void _setDestroyed(bool b)                         // √
  {
    _isDestroyed = b;
  }
  bool _isSystemEDU(EDUID eduID) // √
  {
    std::map<unsigned int, EDUID>::iterator it = _mapSystemEDUS.begin();
    while (it != _mapSystemEDUS.end())
    {
      if (eduID == it->second)
      {
        return true;
      }
      it++;
    }
    return false;
  }

  /**
   * @brief PMD_EDU_WAITING | PMD_EDU_IDLE | PMD_EDU_CREATING 状态下调用
   * 设置为PMD_EDU_DESTROY， 把控制块移出线程池
   */
  int _destroyEDU(EDUID eduID);

  /**
   * @brief creating | waiting 状态下可调用，并且只能 agent 线程使用
   * 让这个 edu 返回线程池，设置状态为 idle ，放入 idle 队列
   * 如果不是 agent 类型，会被定向到销毁。
   */
  int _deactivateEDU(EDUID eduID);

public:
  /*
    * EDU Status Transition Table
    * C: CREATING
    * R: RUNNING
    * W: WAITING
    * I: IDLE
    * D: DESTROY
    * c: createNewEDU
    * a: activateEDU
    * d: destroyEDU
    * w: waitEDU
    * t: deactivateEDU
    *   C   R   W   I   D  <--- from
    * C c
    * R a   -   a   a   -  <--- Creating / Idle / Waiting status can move to Running status
    * W -   w   -   -   -  <--- Running status move to Waiting
    * I t   -   t   -   -  <--- Creating / Waiting status move to Idle
    * D d   -   d   d   -  <--- Creating / Waiting / Idle can be destroyed
    * ^ To
    */
  /*
    * This function must be called against a thread that either in
    * creating/idle or waiting status
    * Threads in creating/waiting status should be sit in runqueue
    * Threads in idle status should be sit in idlequeue
    * This function set the status to PMD_END_RUNNING and bring
    * the control block to runqueue if it was idle status
    * Parameter:
    *   EDU ID (UINt64)
    * Return:
    *   SDB_OK (success)
    *   SDB_SYS (the given eduid can't be found)
    *   SDB_EDU_INVAL_STATUS (EDU is found but not with expected status)
    */
  int activateEDU(EDUID eduID);

  /*
    * This function must be called against a thread that in running
    * status
    * Threads in running status will be put in PMD_EDU_WAITING and
    * remain in runqueue
    * Parameter:
    *   EDU ID (UINt64)
    * Return:
    *   SDB_OK (success)
    *   SDB_SYS (the given eduid can't be found)
    *   SDB_EDU_INVAL_STATUS (EDU is found but not with expected status)
    */
  int waitEDU(EDUID eduID);

  /*
    * This function is called to get an EDU run the given function
    * Depends on if there's any idle EDU, manager may choose an existing
    * idle thread or creating a new threads to serve the request
    * Parmaeter:
    *   pmdEntryPoint ( void (*entryfunc) (pmdEDUCB*, void*) )
    *   type (EDU type, PMD_TYPE_AGENT for example )
    *   arg ( void*, pass to pmdEntryPoint )
    * Output:
    *   eduid ( UINt64, the edu id for the assigned EDU )
    * Return:
    *   SDB_OK (success)
    *   SDB_SYS (internal error (edu id is reused) or creating thread fail)
    *   SDB_OOM (failed to allocate memory)
    *   SDB_INVALIDARG (the type is not valid )
    */
  int startEDU(EDU_TYPES type, void *arg, EDUID *eduid);

  /*
    * 根据 eduID 找到对应的控制块，然后 postEvent 就行了
    */
  int postEDUPost(EDUID eduID, pmdEDUEventTypes type,
                  bool release = false, void *pData = NULL); // √

  /*
    * This function should wait an event for EDU
    * If there are more than one event sitting in the queue
    * waitEDU function should pick up the earliest event
    * This function will wait forever if the input is less than 0
    * Parameter:
    *    EDU ID ( EDUID )
    *    millisecond for the period of waiting ( -1 by default )
    * Output:
    *    Reference for event
    * Return:
    *   SDB_OK ( success )
    *   SDB_SYS ( given EDU ID can't be found )
    *   SDB_TIMEOUT ( timeout )
    */
  int waitEDUPost(EDUID eduID, pmdEDUEvent &event,
                  long long millsecond);

  /*
    * This function should return an waiting/creating EDU to pool
    * (cannot be running)
    * Pool will decide whether to destroy it or pool the EDU
    * Any thread main function should detect the destroyed output
    * deactivateEDU supposed only happened to AGENT EDUs
    * Parameter:
    *   EDU ID ( EDUID )
    * Output:
    *   Pointer for whether the EDU is destroyed
    * Return:
    *   SDB_OK ( success )
    *   SDB_SYS ( given EDU ID can't be found )
    *   SDB_EDU_INVAL_STATUS (EDU is found but not with expected status)
    */
  int returnEDU(EDUID eduID, bool force, bool *destroyed);

  int forceUserEDU(EDUID eduID);

  pmdEDUCB *getEDU(unsigned int tid);
  pmdEDUCB *getEDU();
  pmdEDUCB *getEDUByID(EDUID eduID);
  void setEDU(unsigned int tid, EDUID eduid);
};

#endif