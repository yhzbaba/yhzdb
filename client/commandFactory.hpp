/*
 * ==================================================================
 * 
 * Filename: commandFactory.hpp
 * 
 * Created: 10/15/2020 14:00 PM
 * 
 * Author: Yuan Haizhuo
 * 
 * ==================================================================
 */
#ifndef _COMMAND_FACTORY_HPP_
#define _COMMAND_FACTORY_HPP_

#include "command.hpp"

#define COMMAND_BEGIN               \
  void CommandFactory::addCommand() \
  {
#define COMMAND_END }
#define COMMAND_ADD(cmdName, cmdClass)                  \
  {                                                     \
    ICommand *pObj = new cmdClass();                    \
    std::string str = cmdName;                          \
    _cmdMap.insert(COMMAND_MAP::value_type(str, pObj)); \
  }

class CommandFactory
{
  typedef std::map<std::string, ICommand *> COMMAND_MAP;

public:
  CommandFactory();
  ~CommandFactory() {}
  void addCommand();
  ICommand *getCommandProcesser(const char *pCmd);

private:
  COMMAND_MAP _cmdMap;
};

#endif