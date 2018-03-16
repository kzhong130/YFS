#include <ctime>
#include "inode_manager.h"

// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf)
{
  /*
   *your lab1 code goes here.
   *if id is smaller than 0 or larger than BLOCK_NUM 
   *or buf is null, just return.
   *put the content of target block into buf.
   *hint: use memcpy
  */
  if(!(id < 0 || id >= BLOCK_NUM || buf == NULL)){
  
  	memcpy(buf, blocks[id], BLOCK_SIZE);  
  }

}

void
disk::write_block(blockid_t id, const char *buf)
{
  /*
   *your lab1 code goes here.
   *hint: just like read_block
  */


  if(!(id < 0 || id >= BLOCK_NUM || buf == NULL)){

        memcpy(blocks[id], buf, BLOCK_SIZE);
  }
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
  /*
   * your lab1 code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.

   *hint: use macro IBLOCK and BBLOCK.
          use bit operation.
          remind yourself of the layout of disk.
   */

  
  //blockid_t num = IBLOCK(INODE_NUM, sb.nblocks) + 1;
  blockid_t num = 2 + INODE_NUM / IPB + BLOCK_NUM / BPB;
  char buf[BLOCK_SIZE];
  while(num < sb.nblocks){
    read_block(BBLOCK(num),buf);
    for(int i = 0; i < BLOCK_SIZE && num < sb.nblocks; i++){
      char mask = 0x80;
      char temp = buf[i];
      for(int j = 0; j < 8 && num <sb.nblocks;j++){
        if((temp & mask) == 0){
          buf[i] = buf[i] | mask;
          write_block(BBLOCK(num),buf);
          return num;

        }
        num++;
        mask = mask>>1;

      }
    }
  }
  
  


  
}

void
block_manager::free_block(uint32_t id)
{
  /* 
   * your lab1 code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  if(id < 2 + INODE_NUM / IPB + BLOCK_NUM / BPB || id > BLOCK_NUM-1) return;
  char buf[BLOCK_SIZE];
  read_block(BBLOCK(id), buf);
  uint32_t num = id % (BLOCK_SIZE * 8);
  uint32_t bit_num = num % 8;
  num = num/8;
  buf[num] = buf[num] & (~(0x80 >> bit_num));
  write_block(BBLOCK(id), buf);
  
  
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;

}

void
block_manager::read_block(uint32_t id, char *buf)
{
  d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
  /* 
   * your lab1 code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
    
   * if you get some heap memory, do not forget to free it.
   */
   uint32_t num = 1; 
   char buf[BLOCK_SIZE];
   while(num <= bm->sb.ninodes){
      bm->read_block(IBLOCK(num, bm->sb.nblocks), buf);
      for (int i = 0; i < IPB ; i++) {
        inode_t * iNode = (inode_t *)buf + i;
        if (iNode->type == 0) {
          iNode->type = type;
          iNode->size = 0;
          iNode->atime = time(0);
          iNode->mtime = time(0);
          iNode->ctime = time(0);
          bm->write_block(IBLOCK(num, bm->sb.nblocks), buf);
          return num;
        }
        num++;
      }
   }
  
}

void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your lab1 code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   * do not forget to free memory if necessary.
   */

  
   inode_t* iNode = get_inode(inum);
   if(iNode == NULL) return;
   iNode->type = 0;
   iNode->size = 0;
   iNode->atime = time(0);
   iNode->mtime = time(0);
   put_inode(inum, iNode);
   char buf[BLOCK_SIZE];
   bm->read_block(BBLOCK(inum),buf);
   int num = inum % (BLOCK_SIZE * 8);
   int bit_num = num % 8;
   num = num/8;
   buf[num] = buf[num] & (~(0x80 >> bit_num));
   bm->write_block(BBLOCK(inum), buf);



}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino, *ino_disk;
  char buf[BLOCK_SIZE];

  printf("\tim: get_inode %d\n", inum);

  if (inum < 0 || inum >= INODE_NUM) {
    printf("\tim: inum out of range\n");
    return NULL;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  // printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  if (ino_disk->type == 0) {
    printf("\tim: inode not exist\n");
    return NULL;
  }

  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;

  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
  /*
   * your lab1 code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_out
   */
   inode_t* iNode = get_inode(inum);
   int temp = iNode->size % BLOCK_SIZE;
   int block_num = iNode->size / BLOCK_SIZE;
   if(temp != 0) block_num++;

   *buf_out = (char*) malloc(block_num * BLOCK_SIZE * sizeof(char));

   // check if block number is larger than NDIRECT
   if(block_num < NDIRECT){
     for(int i = 0; i < block_num; i++){
       bm->read_block(iNode->blocks[i], *buf_out + i * BLOCK_SIZE);
     }
   }

   else{
     int i;
     for(i = 0; i < NDIRECT; i++){
       bm->read_block(iNode->blocks[i], *buf_out + i * BLOCK_SIZE);
     }

     //To get the indirect block of the inode
     blockid_t inDirect[NINDIRECT];
     bm->read_block(iNode->blocks[NDIRECT], (char*)(inDirect));
     for(; i < block_num; i++){
       bm->read_block(inDirect[i - NDIRECT], *buf_out + i * BLOCK_SIZE);
     }

   }
   iNode->atime = time(0);
   put_inode(inum, iNode);
   *size = iNode->size;
   free(iNode);

}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your lab1 code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode.
   * you should free some blocks if necessary.
   */

   
   inode_t* iNode = get_inode(inum);
   int old_size = iNode->size;
   int old_num = 0;
   int new_num = 0;
   if(old_size != 0) old_num = (old_size-1) / BLOCK_SIZE + 1;
   if(size != 0) new_num = (size-1) / BLOCK_SIZE + 1;
   //old_num = (old_size-1) / BLOCK_SIZE + 1;
   
   //new_num = (size-1) / BLOCK_SIZE + 1;
   iNode->size = size;
   iNode->atime = time(0);
	 iNode->mtime = time(0);
   iNode->ctime = time(0);
   

   if(new_num > MAXFILE) return;
   //new_num is larger than old_num
   if(new_num >= old_num){

     //new_num is still smaller than NDIRECT
     if(new_num <= NDIRECT){
       for(int i = old_num; i < new_num; i++){
         iNode->blocks[i] = bm->alloc_block();
       }
       for(int i = 0; i < new_num; i++){
         bm->write_block(iNode->blocks[i], buf + i * BLOCK_SIZE);
       }
     }

     //new_num is larger than NDIRECT
     else{
       //old_num is larger than NDIRECT
       if(old_num > NDIRECT){
         blockid_t inDirect[NINDIRECT];
         bm->read_block(iNode->blocks[NDIRECT],(char *)inDirect);
         for(int i = old_num; i < new_num; i++){
           inDirect[i - NDIRECT] = bm->alloc_block();
         }
         bm->write_block(iNode->blocks[NDIRECT],(char *)inDirect);


         for(int i = 0; i < NDIRECT; i++){
           bm->write_block(iNode->blocks[i], buf + i *BLOCK_SIZE);
         }
         for(int i = NDIRECT; i < new_num; i++){
           bm->write_block(inDirect[i - NDIRECT], buf + i * BLOCK_SIZE);
         }

       }
       //new_num is larger than NDIRECT but old_num is smaller than NDIRECT
       else{
         for(int i = old_num; i < NDIRECT; i++){
           iNode->blocks[i] = bm->alloc_block();
         }
         iNode->blocks[NDIRECT] = bm->alloc_block();
         blockid_t inDirect[NINDIRECT];
         //bm->read_block(iNode->blocks[NDIRECT],(char *)inDirect);
         for(int i = NDIRECT; i < new_num; i++){
           inDirect[i - NDIRECT] = bm->alloc_block();
         }
         bm->write_block(iNode->blocks[NDIRECT],(char *)inDirect);

         for(int i = 0; i < NDIRECT; i++){
           bm->write_block(iNode->blocks[i], buf + i *BLOCK_SIZE);
         }
         for(int i = NDIRECT; i < new_num; i++){
           bm->write_block(inDirect[i - NDIRECT], buf + i * BLOCK_SIZE);
         }

       }       
     }
   }

   //old_num is larger than new_num
   else{
     if(old_num <= NDIRECT){
       for(int i = new_num; i< old_num; i++){
         bm->free_block(iNode->blocks[i]);
       }
       for(int i = 0; i < new_num; i++){
         bm->write_block(iNode->blocks[i], buf + i * BLOCK_SIZE);
       }
     }


     if(old_num > NDIRECT && new_num > NDIRECT){
       blockid_t inDirect[NINDIRECT];
       bm->read_block(iNode->blocks[NDIRECT],(char *)inDirect);
       for(int i = new_num; i < old_num; i++){
         bm->free_block(inDirect[i-NDIRECT]);
       }
       bm->write_block(iNode->blocks[NDIRECT],(char*)inDirect);


       for(int i = 0; i < NDIRECT; i++){
         bm->write_block(iNode->blocks[i], buf + i *BLOCK_SIZE);
       }
       for(int i = NDIRECT; i < new_num; i++){
         bm->write_block(inDirect[i - NDIRECT], buf + i * BLOCK_SIZE);
       }
       
     }

     if(new_num <= NDIRECT && old_num > NDIRECT){
 
       //bm->write_block(iNode->blocks[NDIRECT],(char*)inDirect);
  
       for(int i = new_num; i < NDIRECT; i++){
         bm->free_block(iNode->blocks[i]);
       }

       blockid_t inDirect[NINDIRECT];
       bm->read_block(iNode->blocks[NDIRECT],(char *)inDirect);
       for(int i = NDIRECT; i < old_num; i++){
         bm->free_block(inDirect[i-NDIRECT]);
       }
       bm->free_block(iNode->blocks[NDIRECT]);


       for(int i = 0; i < new_num ;i++){
         bm->write_block(iNode->blocks[i], buf + i * BLOCK_SIZE);
       }
     }
   }



   put_inode(inum, iNode);
   free(iNode);


}

void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your lab1 code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
  inode_t * iNode = get_inode(inum);

  if (iNode != NULL) {
    a.type = iNode->type;
    a.atime = iNode->atime;
    a.mtime = iNode->mtime;
    a.ctime = iNode->ctime;
    a.size = iNode->size;

    free(iNode);
  }


}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your lab1 code goes here
   * note: you need to consider about both the data block and inode of the file
   * do not forget to free memory if necessary.
   */
   
   inode_t* iNode = get_inode(inum);
   if(iNode == NULL) return;
   int size = 0;
   if(iNode->size != 0) size = (iNode->size-1) / BLOCK_SIZE + 1;
   if(size <= NDIRECT){
     for(int i = 0; i< size; i++){
       bm->free_block(iNode->blocks[i]);
     }
   }
   else{
     for(int i = 0; i < NDIRECT; i++){
       bm->free_block(iNode->blocks[i]);
     }
     char buf[BLOCK_SIZE];
     bm->read_block(iNode->blocks[NDIRECT], buf);
     for(int i = NDIRECT; i < size; i++){
       bm->free_block(buf[i-NDIRECT]);
     }
     //bm->write_block(iNode->blocks[NDIRECT], buf);

     bm->free_block(iNode->blocks[NDIRECT]);
     
   }
   free_inode(inum);
   free(iNode);
   
   
}
