/*
 * ==================================================================
 * 
 * Filename: edb.hpp
 * 
 * Created: 10/15/2020 13:51 PM
 * 
 * Author: Yuan Haizhuo
 * 
 * ==================================================================
 */
#ifndef _EDB_HPP_
#define _EDB_HPP_

#include "core.hpp"
#include "ossSocket.hpp"
#include "commandFactory.hpp"

const int CMD_BUFFER_SIZE = 512;
class Edb
{
public:
  Edb() {}
  ~Edb() {}

  void start(void);

protected:
  void prompt(void);

private:
  void split(const std::string &text, char delim, std::vector<std::string> &result);
  char *readLine(char *p, int length);
  int readInput(const char *pPrompt, int numIndent);

  ossSocket _sock;
  CommandFactory _cmdFactory;
  char _cmdBuffer[512];
};

#endif