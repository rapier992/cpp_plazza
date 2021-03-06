//
// NamedPipe.cpp for  in /home/querat_g/tmp/cpp_plazza/querat_g/src
//
// Made by querat_g
// Login   <querat_g@epitech.net>
//
// Started on  Tue Apr 12 17:46:41 2016 querat_g
// Last update Fri Apr 22 15:35:30 2016 querat_g
//

#include "NamedPipe.hh"

NamedPipe::NamedPipe(std::string const & name)
  : _name(name)
  , _C_name(_name.c_str())
  , _fdin(-1)
  , _fdout(-1)
{
  this->_tryCreatePipe();
}

NamedPipe::~NamedPipe()
{
  this->_close();
  unlink(this->_C_name);
}

bool
NamedPipe::_tryCreatePipe() const
{
  // If the named pipe exists we can use it directly
  if (!(access(this->_C_name, F_OK)))
    return (true);

  // Else we have to create it
  if ((mknod(this->_C_name, S_IFIFO | 0666, 0)) == -1)
    {
      CERR("named pipe creation failed");
      return (false);
    }
  return (true);
}

bool
NamedPipe::openWritingEnd()
{
  this->_tryCreatePipe();
  if (!(IS_VALID_FD(this->_fdout))) // Input side of the pipe
  {
    this->_fdout = open(this->_C_name, O_WRONLY); //  | O_NONBLOCK
    if (this->_fdout == -1)
      std::cerr << "NamedPipe::open(): _fdout: "
                << strerror(errno) << std::endl;
    }
  return (this->_fdout != (-1));
}

bool
NamedPipe::openReadingEnd()
{
  this->_tryCreatePipe();
  if (!(IS_VALID_FD(this->_fdin))) // Input side of the pipe
    {
      this->_fdin = open(this->_C_name, O_RDWR); // | O_NONBLOCK
      if (this->_fdin == -1)
        std::cerr << "NamedPipe::open(): _fdin: "
                  << strerror(errno) << std::endl;
    }
  return (this->_fdin != (-1));
}

bool
NamedPipe::_close()
{
  if (IS_VALID_FD(this->_fdin))
    close(this->_fdin);
  if (IS_VALID_FD(this->_fdout))
    close(this->_fdout);
  this->_fdin = (-1);
  this->_fdout = (-1);
  return (true);
}

void
NamedPipe::writeTo(void const *data, size_t size)
{
  this->openWritingEnd();
  write(this->_fdout, data, size);
}

bool
NamedPipe::readFrom(void *buffer, size_t requestedReadSize)
{
  if (!this->openReadingEnd())
    return (false);

  size_t actualReadSize = read(this->_fdin, buffer, requestedReadSize);
  return (actualReadSize == requestedReadSize);
}


NamedPipe &
operator<<(NamedPipe &dis, t_FileActionPair const & fileActionPair)
{
  Plazza::Packet::Raw::Action   act;

  // Fill the Header first ...
  act.magic = Plazza::Packet::MAGIC;
  act.size = sizeof(act);
  // ... Then the raw data
  act.type = fileActionPair.second;
  std::strncpy(act.fileName, fileActionPair.first.c_str(), FILENAME_SIZE);
  act.fileName[FILENAME_SIZE - 1] = '\0'; // better safe than sorry

  // Finally, write it into the named pipe
  dis.writeTo(&act, sizeof(act));

  return (dis);
}

NamedPipe &
operator>>(NamedPipe &dis, t_FileActionPair & pair)
{
  Plazza::Packet::Raw::Action action;

  memset(&action, 0, sizeof(action));

  if (!dis.readFrom(&action, sizeof(action))) {
    return (dis);
  }

  if (action.magic != Plazza::Packet::MAGIC) {
    return (dis);
  }
  pair.first = action.fileName;
  pair.second = action.type;

  DEBUG("op>> " << pair.first << " "<< pair.second);

  return (dis);
}


NamedPipe &
operator>>(NamedPipe &dis, Plazza::Packet::Raw::Action & action)
{
  std::memset(&action, 0, sizeof(action));

  if (!dis.readFrom(&action, sizeof(action))) {
    std::memset(&action, 0, sizeof(action));
    return (dis);
  }

  if (action.magic != Plazza::Packet::MAGIC) {
    std::memset(&action, 0, sizeof(action));
    return (dis);
  }

  return (dis);
}

bool
NamedPipe::isReadyToRead()
{
#define POLLFLAGS (POLLIN)

  this->openReadingEnd();
  int   ret = 0;
  struct pollfd poll_ = {
    .fd      = this->_fdin,
    .events  = POLLFLAGS,
    .revents = 0
  };

  if ((ret = poll(&poll_, 1, Plazza::POLL_TIMEOUT)) < 0) {
    CERR(getpid() << " could not poll() !");
    return (false);
  }

  return ((ret && (poll_.revents & POLLFLAGS)) ? true : false);
}
