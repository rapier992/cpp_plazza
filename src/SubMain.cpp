//
// SubMain.cpp for  in /home/querat_g/tmp/cpp_plazza/querat_g/src
//
// Made by querat_g
// Login   <querat_g@epitech.net>
//
// Started on  Tue Apr 19 09:58:24 2016 querat_g
// Last update Sun Apr 24 14:49:07 2016 querat_g
//

#include "SubMain.hh"
#include "SafeDeque.hpp"

Plazza::SubMain::SubMain(pid_t pid, NamedPipe *pipe1, NamedPipe *pipe2, int nbThreads)
  : _pid(pid)
  , _pipe1(pipe1)
  , _pipe2(pipe2)
  , _clock(time(NULL))
  , _timeSinceLastEvent(0)
  , _nbThreads(nbThreads)
  , _threads(nbThreads, _actionsToDo, _answers)
{
  // std::cerr << "SubMain " << _pid << "created" << std::endl;
}

Plazza::SubMain::~SubMain()
{
  (void)_nbThreads;
  delete (_pipe1);
  delete (_pipe2);
  DEBUG(_pid << " Plazza::SubMain::~SubMain(): no tasks for 5 seconds, exiting");
}

bool
Plazza::SubMain::receiveAction()
{
  Plazza::Packet::Raw::Action   action;

  DEBUG("receiving action...");
  (*_pipe1) >> action;
  DEBUG("... Ok");

  if (action.magic != Plazza::Packet::MAGIC) {
    std::cerr << _pid << " could not get action from _pipe1" << std::endl;
    return (false);
  }

  this->_actionsToDo.push_back(std::make_pair(action.fileName, action.type));

  return (true);
}

bool
Plazza::SubMain::sendSolvedAction(std::string const & str)
{
  Plazza::Packet::Header        head;

  head.magic = Plazza::Packet::MAGIC;
  head.size = str.size() + 1;

  DEBUG("sending answer back to Main process ...");
  _pipe2->writeTo(&head, sizeof(head));
  _pipe2->writeTo(str.c_str(), str.size() + 1);
  DEBUG("Answer sent !");

  return (true);
}

void
Plazza::SubMain::_incrementTimeSinceLastEvent() {
  time_t now = time(NULL);

  _timeSinceLastEvent += (now - _clock);
  _clock = now;
}

bool
Plazza::SubMain::_shouldExit() const {
  return ((_timeSinceLastEvent > 5.0f) &&
          (_actionsToDo.empty())  &&
          (_answers.empty()));
}

bool
Plazza::SubMain::mainLoop()
{
  _pipe1->openReadingEnd();

  while (!this->_shouldExit())
    {
      if (_pipe1->isReadyToRead())
        {
          DEBUG("shit's ready to read yo !");
          receiveAction();
        }

      if (!_answers.empty()) {
        sendSolvedAction(_answers.front());
        _answers.pop_front();
      }

      _incrementTimeSinceLastEvent();
    }

  return (true);
}
