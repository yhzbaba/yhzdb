/*
 * ==================================================================
 * 
 * Filename: commandFactory.cpp
 * 
 * Created: 10/16/2020 15:24 PM
 * 
 * Author: Yuan Haizhuo
 * 
 * ==================================================================
 */
#include <ctype.h>

#include "commandFactory.hpp"

void _lowercase(string &str)
{
  for (auto &c : str)
  {
    c = tolower(c);
  }
}

CommandFactory::CommandFactory()
{
  addCommand();
}

ICommand *CommandFactory::getCommandProcesser(const char *pCmd)
{
  ICommand *pProcessor = NULL;
  COMMAND_MAP::iterator iter;
  iter = _cmdMap.find(pCmd);
  if (iter != _cmdMap.end())
  {
    pProcessor = iter->second;
  }
  return pProcessor;
}