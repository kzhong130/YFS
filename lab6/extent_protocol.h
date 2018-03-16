// extent wire protocol

#ifndef extent_protocol_h
#define extent_protocol_h

#include "rpc.h"

class extent_protocol {
 public:
  typedef int status;
  typedef unsigned long long extentid_t;
  enum xxstatus { OK, RPCERR, NOENT, IOERR };
  enum rpc_numbers {
    put = 0x6001,
    get,
    getattr,
<<<<<<< HEAD
    remove
  };

  struct attr {
=======
    remove,
    create,
    commit,
    redo,
    undo
  };

  enum types {
    T_DIR = 1,
    T_FILE,
    T_SLNK
    //T_SYM
  };

  struct attr {
    uint32_t type;
>>>>>>> lab5
    unsigned int atime;
    unsigned int mtime;
    unsigned int ctime;
    unsigned int size;
  };
};

inline unmarshall &
operator>>(unmarshall &u, extent_protocol::attr &a)
{
<<<<<<< HEAD
=======
  u >> a.type;
>>>>>>> lab5
  u >> a.atime;
  u >> a.mtime;
  u >> a.ctime;
  u >> a.size;
  return u;
}

inline marshall &
operator<<(marshall &m, extent_protocol::attr a)
{
<<<<<<< HEAD
=======
  m << a.type;
>>>>>>> lab5
  m << a.atime;
  m << a.mtime;
  m << a.ctime;
  m << a.size;
  return m;
}

#endif 
