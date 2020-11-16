/*
 * ==================================================================
 * 
 * Filename: pmd.hpp
 * 
 * Created: 11/09/2020 20:10 PM
 * 
 * Author: Yuan Haizhuo
 * 
 * event communicate between dispatch units
 * 
 * ==================================================================
 */
#ifndef PMDEDUEVENT_HPP__
#define PMDEDUEVENT_HPP__

#include "core.hpp"
enum pmdEDUEventTypes
{
  PMD_EDU_EVENT_NONE = 0,
  PMD_EDU_EVENT_TERM,      // terminate
  PMD_EDU_EVENT_RESUME,    // resume a EDU, the data is startEDU's argv
  PMD_EDU_EVENT_ACTIVE,    // active
  PMD_EDU_EVENT_DEACTIVE,  // deactive
  PMD_EDU_EVENT_TIMEOUT,   // timeout
  PMD_EDU_EVENT_LOCKWAKEUP // transaction lock wake up
};

class pmdEDUEvent
{
public:
  pmdEDUEvent() : _eventType(PMD_EDU_EVENT_NONE),
                  _release(false),
                  _Data(NULL)
  {
  }

  pmdEDUEvent(pmdEDUEventTypes type) : _eventType(type),
                                       _release(false),
                                       _Data(NULL)
  {
  }

  pmdEDUEvent(pmdEDUEventTypes type, bool release, void *data) : _eventType(type),
                                                                 _release(release),
                                                                 _Data(data)
  {
  }

  void reset()
  {
    _eventType = PMD_EDU_EVENT_NONE;
    _release = false;
    _Data = NULL;
  }

  pmdEDUEventTypes _eventType;
  bool _release;
  void *_Data;
};

#endif