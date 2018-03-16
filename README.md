# YFS

This is a file system called Yet Another File System (yfs), which supporta basic file operation, fault tolerant, version control and support concurrent request from different clients.

### Lab1
- This part implements an inode manager to support the basic file system with operations like CREATE/GETATTR, PUT/GET, REMOVE.

### Lab2
- This part implements file system implementation with FUSE operations like CREATE/MKNOD, LOOKUP, READDIR, SETATTR, WRITE, READ, MKDIR, UNLINK and SIMBOLIC LINK.

### Lab3
- This part uses RPC to implement a single client file server.

### Lab4
- This part implememnt a lock server.
- Add locking to ensure that concurrent operations to the same file/directory from different clients occur one at a time.

### Lab5
- This part adds support for version control operations based on log.

### Lab6
- This part implement Paxos and use it to agree to a sequence of membership changes (i.e., view changes).

### Lab7
- This part provides the yfs with the ability of fault tolerance using erasure code.
