// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server() 
{
  im = new inode_manager();

}

int extent_server::create(uint32_t type, extent_protocol::extentid_t &id)
{
  // alloc a new inode and return inum
  printf("extent_server: create inode\n");

  id = im->alloc_inode(type);

  //create log

  std::stringstream ss;
  ss  << " create " << id <<" "<<type;
  std::string new_log;
  ss >> new_log;
  new_log = ss.str();
  log.push_back(new_log);
  printf("create log add %s\n",new_log.c_str());
  
  ncommit = -1;

  return extent_protocol::OK;
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  id &= 0x7fffffff;
  
  const char * cbuf = buf.c_str();
  int size = buf.size();
  printf("es put\n");

  //create log before the action is done
  int nsize = 0;
  char *oldbuf = NULL;
  im->read_file(id,&oldbuf,&nsize);

  std::string res;
  if (nsize == 0)
    res = "";
  else {
    res.assign(oldbuf, nsize);
    free(oldbuf);
  }
  std::stringstream ss;
  ss << " put " << id << " " << nsize << " " << res <<" "<<size<<" "<<buf;
  std::string new_log;
  new_log = ss.str();
  log.push_back(new_log);
  printf("put log add %s\n",new_log.c_str());
  
  im->write_file(id, cbuf, size);
  
  ncommit = -1;
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  printf("extent_server: get %lld\n", id);

  id &= 0x7fffffff;

  int size = 0;
  char *cbuf = NULL;

  im->read_file(id, &cbuf, &size);
  if (size == 0)
    buf = "";
  else {
    buf.assign(cbuf, size);
    free(cbuf);
  }

  return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  printf("extent_server: getattr %lld\n", id);

  id &= 0x7fffffff;
  
  extent_protocol::attr attr;
  memset(&attr, 0, sizeof(attr));
  im->getattr(id, attr);
  a = attr;

  return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  printf("extent_server: write %lld\n", id);

  id &= 0x7fffffff;

  int size = 0;
  char *buf_out = NULL;

  std::string buf;

  im->read_file(id, &buf_out, &size);
  if (size == 0)
    buf = "";
  else {
    buf.assign(buf_out, size);
    free(buf_out);
  }

  uint32_t type = im->get_inode_type(id);
  std::stringstream ss;
  ss << " remove " << id << " " << size << " " << buf<<" " <<type;
  std::string new_log;
  new_log = ss.str();

  printf("remove log add %s\n",new_log.c_str());
  log.push_back(new_log);

  im->remove_file(id);
 
  ncommit = -1;
  return extent_protocol::OK;
}

int extent_server::commit(int, int &){

  printf("es commit\n");
  version++;
  ncommit = version;
  std::stringstream ss;
  ss << " commit "<< version;
  log.push_back(ss.str());
  printf("commit log add %s\n",ss.str().c_str());
  commit_point.push_back(log.size()-1);
  printf("commit_point index %d\n",log.size()-1);
  return extent_protocol::OK;
}


int extent_server::undo(int, int &){

  /*
  for(int i = 0; i<log.size();i++){
    printf("log %d %s\n",i,log[i].c_str());
  }
``*/
  printf("es undo\n");

  int pre_version = version - 1;

  /*
  if(pre_version <= 1){
    printf("[warning]version less than 0, illegal!\n");
    assert(0);
  }
  */

  int new_index,old_index;

  if(ncommit == -1) {
    new_index = log.size();
    old_index = commit_point[version-1];
  }

  else{
  //version's index before undo in the log vector 
    new_index = commit_point[version];

  
  //version's index after undo in the log vector
    old_index = commit_point[version-1];
  }

  printf("ncommit %d version %d\n",ncommit,version);
  printf("old_index %d\n",old_index);  

  
  printf("new_index %d\n",new_index);

  for(int i = new_index-1 ; i > old_index; i--){
    std::stringstream ss;
    uint32_t vnum;
    std::string action;
    ss << log[i];
    ss >> action;
    if(action == "commit"){
        ss >> vnum;
        printf("[warning] should not exists such situation\n");
    }
    else if(action == "create"){
        extent_protocol::extentid_t id;
        ss >> id;
        printf("undo create %d\n",id);
        im->remove_file(id);
        printf("undo create finish\n");
    }
    
    else if(action == "put"){
        extent_protocol::extentid_t id;
        uint32_t size;
        ss >> id >> size;

        printf("undo put id:%d size:%d\n",id,size);

        /*
        for(uint32_t j=0; j<size+2; j++)
            ss.get();
        ss >> size;
        */
        ss.get();
        char cbuf[size];
        for(uint32_t j=0; j<size; j++)
            ss.get(cbuf[j]);

        printf("undo put buf %s\n",cbuf);
        im->write_file(id, cbuf, size);

        printf("undo put finish\n");
    }
    else if(action == "remove"){
        extent_protocol::extentid_t id;
        uint32_t size;
        ss >> id >> size;
        printf("undo remove id:%d size:%d\n",id,size);
        
        ss.get();
        char cbuf[size];
        for(uint32_t j=0; j<size; j++)
            ss.get(cbuf[j]);
        
        ss.get();
        uint32_t type;
        ss>>type;

        im->set_inode(id,type);
        printf("undo remove buf %s\n",cbuf);
        im->write_file(id, cbuf, size);
        printf("undo remove finish\n");
    }
    else{
      printf("[warning] invalid action %s",action.c_str());
    }

    ncommit = version;
  }

  
  version --;

  printf("finish undo to version %d",version);
  return extent_protocol::OK;
}
int extent_server::redo(int, int &){
  
  int new_index=commit_point[version+1];
  int old_index=commit_point[version];
  
  printf("redo:new_index %d old_index %d\n",new_index,old_index);

  for(int i=old_index+1;i<new_index;i++){
    std::stringstream ss;
    uint32_t vnum;
    std::string action;
    ss << log[i];
    ss >> action;
    if(action == "commit"){
        printf("[warning] should not exist commit\n");
    }
    else if(action == "create"){
        extent_protocol::extentid_t id;
        ss >> id;
        uint32_t type;
        ss>>type;
        im->set_inode(id,type);
        im->write_file(id, "", 0);
    }
    else if(action == "put"){
        extent_protocol::extentid_t id;
        uint32_t size;
        std::string buf;
        ss >> id >> size;
        ss.get();
        char nbuf[size];
        for(uint32_t j=0; j<size; j++)
            ss.get(nbuf[j]);
        ss.get();
        ss>>size;
        ss.get();
        char cbuf[size];
        for(uint32_t j=0; j<size; j++)
            ss.get(cbuf[j]);
        im->write_file(id, cbuf, size);
    }
    else if(action == "remove"){
        extent_protocol::extentid_t id;
        ss >> id;
        im->remove_file(id);
    }
  }

  version++;
  ncommit = version;
  return extent_protocol::OK;

}


