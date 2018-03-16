// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <map>

lock_server::lock_server():
  nacquire (0)
{
  pthread_mutex_init(&global_lock, NULL);  
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  // Your lab4 code goes here
  pthread_mutex_lock(&global_lock);
  //printf("pid %d try to acquire %d\n",pthread_self(),lid);
  if(locks.find(lid) == locks.end()){
    locks[lid].stat = true;
    pthread_mutex_unlock(&global_lock);
    if(ret != lock_protocol::OK ) printf("aaa\n");
    printf("acquire %d\n",lid);
    return ret;
  }
  else{
    printf("acquire lock %d has been used\n",lid);
    while(locks[lid].stat==true){
      pthread_cond_wait(&locks[lid].cond, &global_lock);
    }
    locks[lid].stat=true;
    pthread_mutex_unlock(&global_lock);
    if(ret != lock_protocol::OK ) printf("bbb\n"); 
    printf("acquire %d\n",lid);    
    return ret;
  }
  if(ret != lock_protocol::OK ) printf("ccc\n");  
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  // Your lab4 code goes here
  pthread_mutex_lock(&global_lock);
  
  if(locks.find(lid)==locks.end()){
    return lock_protocol::IOERR;
  }
  
  locks[lid].stat=false;
  pthread_mutex_unlock(&global_lock);
  printf("release %d\n",lid);  
  pthread_cond_signal(&locks[lid].cond);
  return ret;
}
