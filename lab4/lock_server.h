// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
#include <map>
#include <pthread.h>

class lock_server {
  
   protected:
    int nacquire;
  

  //stat to show that whether this lock has been used by other threads 
  //cond is used for scheduling to avoid starvation
  private:
    struct virtuLock {
        bool stat;
        pthread_cond_t cond;

        //to init the virtual lock and set the lock to be used by the thread that has created the lock
        virtuLock() {
            stat = true;
            pthread_cond_init(&cond, NULL);
        }
    };
  
    //lock to maintain the atomicity
    pthread_mutex_t global_lock;

    //used to store the lock states
    std::map<lock_protocol::lockid_t, virtuLock> locks;

 public:
  lock_server();
  ~lock_server() {};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
};

#endif 







