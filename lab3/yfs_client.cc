// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



std::string itoa(size_t n){
    std::string result;
    for (unsigned i=0; i<sizeof(size_t); i++){
        //printf("2333\n");
        //printf("itoa times %d: %c\n", i ,(unsigned char)(n >> (8 * i) & 0xFF));
        result += (unsigned char)((n >> (8 * i) & 0xFF));
    }
    return result;
}

size_t stoi(std::string s){
    size_t result=0;
    for (unsigned i=0; i<sizeof(size_t); i++){
        result += ((size_t)s[i] << (8 * i));
    }
    return result;
}

//transform inum into string with certain length (sizeof(inum))
std::string InumToLenStr(yfs_client::inum Inum){
    std::string result;
    for (unsigned i=0; i<sizeof(yfs_client::inum); i++){
        result += (unsigned char)((Inum >> (8 * i) & 0xFF));
    }
    return result;
}

yfs_client::inum str2inum(std::string n)
{
    yfs_client::inum result = 0;
    for (unsigned i=0; i<sizeof(yfs_client::inum); i++){
        result += ((yfs_client::inum)(unsigned char)n[i] << (8 * i));
    }
    return result;
}

/*
yfs_client::yfs_client()
{
    ec = new extent_client();

}
*/

//yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
yfs_client::yfs_client(std::string extent_dst)
{
    ec = new extent_client(extent_dst);
    if (ec->put(1, "") != extent_protocol::OK)
        printf("error init root dir\n"); // XYB: init root dir
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    } 
    printf("isfile: %lld is a dir or a symbol link\n", inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

bool
yfs_client::isdir(inum inum)
{
    // Oops! is this still correct when you implement symlink?
    //return (!isfile(inum)) && (!issymlink(inum));
        extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        printf("isdir: %lld is a dir\n", inum);
        return true;
    }
    printf("isdir: %lld is not a dir\n", inum);
    return false;
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
    std::string buf;
    r = ec->get(ino, buf);
    if (r != OK) {
      printf("error with get while setattr\n");
      return r;
    }

    buf.resize(size);

    r = ec->put(ino, buf);
    if (r != OK) {
      printf("error with put while setattr\n");
      return r;
    }

    return r;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    std::string buf;
    r = ec->get(parent, buf);
    if(r != OK){
        printf("The parent %lld doesn't exist\n", parent);
        return EXIST;
    }

    bool found;
    printf("create lookup\n");
    lookup(parent, name, found, ino_out);
    if(found){
        printf("The file %s already exist\n", name);
        return found;
    }

    r = ec->create(extent_protocol::T_FILE, ino_out);
    if(r != OK){
        printf("Create %s encountered an error\n", name);
        return IOERR;
    }

    ec->get(parent, buf);

    //update the directory(parent) information
    int n = strlen(name);
    printf("create file name %s\n", name);
    //printf("I create the file %s lenth is %d\n transform lenth is %s ori length is %d\n", std::string(name).c_str(), n, itoa(n).c_str(), stoi(itoa(n)));
    //printf("len is %s, string len is %d\n", itoa(n).c_str(), itoa(n).size());
    //printf("Inum prev:%lld inum:%lld\n", ino_out, str2inum(InumToLenStr(ino_out)));
    buf = buf + itoa(n) + std::string(name) + InumToLenStr(ino_out);
    if (ec->put(parent, buf) != OK) {
        printf("error with put while create\n");
        return IOERR;
    }

    return r;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
     printf("mkdir\n");
   if (!isdir(parent)) {
        printf("The parent %lld is not a dir\n", parent);
        return IOERR;
    }
    bool found;
    lookup(parent, name, found, ino_out);
    if (found) {
        printf("The dir %s already exists\n", name);
        return EXIST;
    }
    if (ec->create(extent_protocol::T_DIR, ino_out) != extent_protocol::OK) {
        printf("error whike creating dir\n");
        return IOERR;
    }
    std::string buf;
    if (ec->get(parent, buf) != extent_protocol::OK) {
        printf("error with get\n");
        return IOERR;
    }
    size_t n = strlen(name);
    //printf("sbsbsb\n");
    //printf("Inum prev:%lld inum:%lld\n", ino_out, str2inum(InumToLenStr(ino_out)));
    buf = buf + itoa(n) + std::string(name) + InumToLenStr(ino_out);
    if (ec->put(parent, buf) != extent_protocol::OK) {
        printf("error with put while mkdir\n");
        return IOERR;
    }

    return r;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    printf("lookup file name %s\n",name);
    if(!isdir(parent)){
        printf("The parent %lld is not a dir while lookup\n", parent);
        return IOERR;
    }
    found = false;
    std::string buf;
    if(ec->get(parent, buf) != OK){
        printf("Error with get dir %lld\n", parent);
        return IOERR;
    }

    int length = buf.size();
    int pos = 0;
    while(pos < length){
        std::string temp = buf.substr(pos, sizeof(size_t));
        int len = stoi(temp);
        std::string Name = buf.substr(pos + sizeof(size_t), len);
        //printf("Lookup: len is %d name is %s, passed name is %s\n", len, Name.c_str(), name);
        if(Name == (std::string)name){
            
            found = true;
            ino_out = str2inum(buf.substr(pos + sizeof(size_t) + len, sizeof(inum)));
            return r;
        }
        pos += sizeof(size_t) + len + sizeof(inum);
    }

    return NOENT;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
    if (!isdir(dir)) {
        return IOERR;
    }
    std::string buf;
    if (ec->get(dir, buf) != OK) {
        printf("error with get\n");
        return IOERR;
    }
    int pos = 0; 
    int length = buf.size();
    dirent ent;
    while (pos < length) {
        std::string tmp = buf.substr(pos, sizeof(size_t));
        size_t len = stoi(tmp);
        
        ent.name = buf.substr(pos + sizeof(size_t), len);
        ent.inum = str2inum(buf.substr(pos + sizeof(size_t) + len, sizeof(inum)));
        list.push_back(ent);
        pos = pos + sizeof(size_t) + len + sizeof(inum);
    }

    return r;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: read using ec->get().
     */
    std::string buf;
    if((r=ec->get(ino, buf)) != OK){
        printf("error with get while read\n");
        return r;
    }
    if(off < buf.size()){
        data = buf.substr(off, size);
    }
    else data = "";
    return r;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    std::string buf;
    r = ec->get(ino, buf);
    if(r != OK){
        printf("error with get while write\n");
        return r;
    }
    int length = buf.size();


    //buf = buf.substr(0, off) + std::string(data).substr(0, size);


    //if off is larger than buf's length fill buf with resize
    if(length < off) {
        buf.resize(off);   
        buf.append(data, size);
    } 
    else {
        //change the size to off
        if(length < off + (int)size) {
            buf.resize(off);
            buf.append(data, size);
        } 
        else buf.replace(off, size, std::string(data,size));     
    }
  
    bytes_written = size;  
    r = ec->put(ino, buf);
    if(r != OK){
        printf("error with put while write\n");
        return r;
    }
    return r;
}

int yfs_client::unlink(inum parent,const char *name)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
    if (!isdir(parent)) {
        return IOERR;
    }
    std::string buf;
    if (ec->get(parent, buf) != OK) {
        printf("error with get\n");
        return IOERR;
    }
    int pos = 0;
    int length = buf.size();
    std::string tmp;
    while (pos < length) {
        tmp = buf.substr(pos, sizeof(size_t));
        size_t len = stoi(tmp);
        std::string Name = buf.substr(pos + sizeof(size_t), len);

        if (Name == (std::string)name) {
            inum ino_out = str2inum(buf.substr(pos + sizeof(size_t) + len, sizeof(inum)));
            if (isdir(ino_out)) {
                return IOERR;
            }
            ec->remove(ino_out);
            buf.erase(pos, sizeof(size_t) + len + sizeof(inum));
            if (ec->put(parent, buf) != OK) {
                printf("error with put with unlink\n");
                return IOERR;
            }
            return OK;
        }
        pos = pos + sizeof(size_t) + len + sizeof(inum);
    }
    
    return r;
}

int yfs_client::symlink(inum parent, const char* name,inum& ino_out, const char* link){
    if(!isdir(parent)){
        printf("parent %lld is not a dir\n", parent);
        return IOERR;
    }
    printf("I have reached here\n");
    bool found;
    lookup(parent, name, found, ino_out);
    if(found){
        printf("file name %s already exists\n", name);
        return EXIST;
    }
    
    if (ec->create(extent_protocol::T_SLNK, ino_out) != OK) {
        printf("error while create symbol link %s\n", name);
        return IOERR;
    }
    
    if (ec->put(ino_out, std::string(link)) != OK) {
        printf("error with put while create symbol link %s\n", name);
        return IOERR;
    }
    std::string buf;
    if (ec->get(parent, buf) != OK) {
        printf("error with get while create symbol link %s\n", name);
        return IOERR;
    }
    
    buf = buf + itoa(strlen(name)) + std::string(name) + InumToLenStr(ino_out);
    if (ec->put(parent, buf) != OK) {
        printf("error with put while put parent %lld\n", parent);
        return IOERR;
    }
    printf("bye\n");
    return OK;
}

int yfs_client::readlink(inum ino, std::string& buf){  

    printf("readlink\n");
    if (!issymlink(ino)) {
        printf("the inode %lld is not a symbol link\n", ino);
        return IOERR;
    }

    if (ec->get(ino, buf) != OK) {
        printf("error with get while read link %lld\n", ino);
        return IOERR;
    }

    return OK;


}

bool yfs_client::issymlink(inum inum){
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_SLNK) {
        printf("issymlink: %lld is a symbol link\n", inum);
        return true;
    } 
    printf("issymlink: %lld is a dir or a file\n", inum);
    return false;
}

int
yfs_client::getsymlink(inum inum, symlinkinfo &fin)
{
    int r = OK;

    printf("getsymlink %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getsymlink %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}