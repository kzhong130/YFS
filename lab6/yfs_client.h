#ifndef yfs_client_h
#define yfs_client_h

#include <string>
<<<<<<< HEAD
=======

#include "lock_protocol.h"
#include "lock_client.h"

>>>>>>> lab5
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>

<<<<<<< HEAD
#include "lock_protocol.h"
#include "lock_client.h"

class yfs_client {
  extent_client *ec;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

=======

#define CA_FILE "./cert/ca.pem:
#define USERFILE	"./etc/passwd"
#define GROUPFILE	"./etc/group"


class yfs_client {
  extent_client *ec;
  lock_client *lc;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST,
  			NOPEM, ERRPEM, EINVA, ECTIM, ENUSE };

  typedef int status;


  struct filestat {
  	unsigned long long size;
	unsigned long mode;
	unsigned short uid;
	unsigned short gid;
  };

>>>>>>> lab5
  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
<<<<<<< HEAD
=======
	unsigned long mode;
	unsigned short uid;
	unsigned short gid;
>>>>>>> lab5
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
<<<<<<< HEAD
  };
=======
	unsigned long mode;
	unsigned short uid;
	unsigned short gid;
  };

>>>>>>> lab5
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  static std::string filename(inum);
  static inum n2i(std::string);
<<<<<<< HEAD
 public:

  yfs_client(std::string, std::string);
=======

 public:
  yfs_client();
  yfs_client(std::string, std::string, const char*);
>>>>>>> lab5

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);
<<<<<<< HEAD
=======

  int setattr(inum, filestat, unsigned long);
  int lookup(inum, const char *, bool &, inum &);
  int create(inum, const char *, mode_t, inum &);
  int readdir(inum, std::list<dirent> &);
  int write(inum, size_t, off_t, const char *, size_t &);
  int read(inum, size_t, off_t, std::string &);
  int unlink(inum,const char *);
  int mkdir(inum , const char *, mode_t , inum &);

  int verify(const char* cert_file, unsigned short*);
  
  /** you may need to add symbolic link related methods here.*/
  int symlink(inum, const char *, inum &,const char*);
  int readlink(inum, std::string &);
  bool issymlink(inum);

  struct symlinkinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };

  int getsymlink(inum, symlinkinfo &);

  int commit();

  int undo();

  int redo();

>>>>>>> lab5
};

#endif 
