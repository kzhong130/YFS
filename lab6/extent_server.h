// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"
<<<<<<< HEAD

class extent_server {

 public:
  extent_server();

=======
#include "inode_manager.h"
#include <vector>

class extent_server {
 protected:
#if 0
  typedef struct extent {
    std::string data;
    struct extent_protocol::attr attr;
  } extent_t;
  std::map <extent_protocol::extentid_t, extent_t> extents;
#endif
  inode_manager *im;

 public:

  //to record current version
  int version = 0;

  int ncommit = -1;

  std::vector<std::string> log;

  //to record the commit index in the log vector
  std::vector<int> commit_point;

  extent_server();

  int create(uint32_t type, extent_protocol::extentid_t &id);
>>>>>>> lab5
  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
<<<<<<< HEAD
=======
  int commit(int, int &);
  int undo(int, int &);
  int redo(int, int &);

  
>>>>>>> lab5
};

#endif 







