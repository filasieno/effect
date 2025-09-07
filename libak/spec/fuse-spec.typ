= FUSE operations

#table(
  columns: (1fr, 2fr, 3fr),
  align: (left, left, left),
  [*Operation*], [*Description*], [*Key Parameters*],

  [init],
  [Initialize the filesystem connection.],
  stack(dir: ttb, spacing: 5pt, raw("struct fuse_conn_info *conn", lang: "c", block: false)),

  [destroy], [Clean up the filesystem.], stack(dir: ttb, spacing: 5pt, raw("void *userdata", lang: "c", block: false)),
  [getattr],
  [Get file attributes.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "struct stat *stbuf",
    lang: "c",
    block: false,
  )),

  [readlink],
  [Read the target of a symbolic link.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("char *buf", lang: "c", block: false),
    raw("size_t size", lang: "c", block: false),
  ),

  [mknod],
  [Create a file node.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("mode_t mode", lang: "c", block: false),
    raw("dev_t rdev", lang: "c", block: false),
  ),

  [mkdir],
  [Create a directory.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "mode_t mode",
    lang: "c",
    block: false,
  )),

  [unlink], [Delete a file.], stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false)),
  [rmdir], [Delete a directory.], stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false)),
  [symlink],
  [Create a symbolic link.],
  stack(dir: ttb, spacing: 5pt, raw("const char *from", lang: "c", block: false), raw(
    "const char *to",
    lang: "c",
    block: false,
  )),

  [rename],
  [Rename a file.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *from", lang: "c", block: false),
    raw("const char *to", lang: "c", block: false),
    raw("unsigned int flags", lang: "c", block: false),
  ),

  [link],
  [Create a hard link.],
  stack(dir: ttb, spacing: 5pt, raw("const char *from", lang: "c", block: false), raw(
    "const char *to",
    lang: "c",
    block: false,
  )),

  [chmod],
  [Change permissions.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "mode_t mode",
    lang: "c",
    block: false,
  )),

  [chown],
  [Change owner.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("uid_t uid", lang: "c", block: false),
    raw("gid_t gid", lang: "c", block: false),
  ),

  [truncate],
  [Change file size.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "off_t size",
    lang: "c",
    block: false,
  )),

  [open],
  [Open a file.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "struct fuse_file_info *fi",
    lang: "c",
    block: false,
  )),

  [read],
  [Read data from file.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("char *buf", lang: "c", block: false),
    raw("size_t size", lang: "c", block: false),
    raw("off_t offset", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
  ),

  [write],
  [Write data to file.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("const char *buf", lang: "c", block: false),
    raw("size_t size", lang: "c", block: false),
    raw("off_t offset", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
  ),

  [statfs],
  [Get filesystem statistics.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "struct statvfs *stbuf",
    lang: "c",
    block: false,
  )),

  [flush],
  [Flush cached data.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "struct fuse_file_info *fi",
    lang: "c",
    block: false,
  )),

  [release],
  [Release an open file.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "struct fuse_file_info *fi",
    lang: "c",
    block: false,
  )),

  [fsync],
  [Synchronize file contents.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("int isdatasync", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
  ),

  [setxattr],
  [Set extended attribute.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("const char *name", lang: "c", block: false),
    raw("const char *value", lang: "c", block: false),
    raw("size_t size", lang: "c", block: false),
    raw("int flags", lang: "c", block: false),
  ),

  [getxattr],
  [Get extended attribute.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("const char *name", lang: "c", block: false),
    raw("char *value", lang: "c", block: false),
    raw("size_t size", lang: "c", block: false),
  ),

  [listxattr],
  [List extended attributes.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("char *list", lang: "c", block: false),
    raw("size_t size", lang: "c", block: false),
  ),

  [removexattr],
  [Remove extended attribute.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "const char *name",
    lang: "c",
    block: false,
  )),

  [opendir],
  [Open a directory.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "struct fuse_file_info *fi",
    lang: "c",
    block: false,
  )),

  [readdir],
  [Read directory contents.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("void *buf", lang: "c", block: false),
    raw("fuse_fill_dir_t filler", lang: "c", block: false),
    raw("off_t offset", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
  ),

  [releasedir],
  [Release an open directory.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "struct fuse_file_info *fi",
    lang: "c",
    block: false,
  )),

  [fsyncdir],
  [Synchronize directory contents.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("int isdatasync", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
  ),

  [access],
  [Check file access permissions.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "int mask",
    lang: "c",
    block: false,
  )),

  [create],
  [Create and open a file.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("mode_t mode", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
  ),

  [ftruncate],
  [Truncate an open file.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("off_t size", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
  ),

  [fgetattr],
  [Get attributes of open file.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("struct stat *stbuf", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
  ),

  [lock],
  [Perform file locking.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
    raw("int cmd", lang: "c", block: false),
    raw("struct flock *lock", lang: "c", block: false),
  ),

  [utimens],
  [Update file timestamps.],
  stack(dir: ttb, spacing: 5pt, raw("const char *path", lang: "c", block: false), raw(
    "const struct timespec tv[2]",
    lang: "c",
    block: false,
  )),

  [bmap],
  [Map block index to device.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("size_t blocksize", lang: "c", block: false),
    raw("uint64_t *idx", lang: "c", block: false),
  ),

  [ioctl],
  [IO control operations.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("int cmd", lang: "c", block: false),
    raw("void *arg", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
    raw("unsigned int flags", lang: "c", block: false),
    raw("void *data", lang: "c", block: false),
  ),

  [poll],
  [Poll for I/O readiness.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
    raw("struct fuse_pollhandle *ph", lang: "c", block: false),
    raw("unsigned *reventsp", lang: "c", block: false),
  ),

  [write_buf],
  [Write using buffer.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("struct fuse_bufvec *buf", lang: "c", block: false),
    raw("off_t offset", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
  ),

  [read_buf],
  [Read using buffer.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("struct fuse_bufvec **bufp", lang: "c", block: false),
    raw("size_t size", lang: "c", block: false),
    raw("off_t offset", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
  ),

  [flock],
  [Advisory file locking.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
    raw("int op", lang: "c", block: false),
  ),

  [fallocate],
  [Preallocate space.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("int mode", lang: "c", block: false),
    raw("off_t offset", lang: "c", block: false),
    raw("off_t length", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
  ),

  [copy_file_range],
  [Copy range between files.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path_in", lang: "c", block: false),
    raw("struct fuse_file_info *fi_in", lang: "c", block: false),
    raw("off_t off_in", lang: "c", block: false),
    raw("const char *path_out", lang: "c", block: false),
    raw("struct fuse_file_info *fi_out", lang: "c", block: false),
    raw("off_t off_out", lang: "c", block: false),
    raw("size_t len", lang: "c", block: false),
    raw("int flags", lang: "c", block: false),
  ),

  [lseek],
  [Seek in file.],
  stack(
    dir: ttb,
    spacing: 5pt,
    raw("const char *path", lang: "c", block: false),
    raw("off_t off", lang: "c", block: false),
    raw("int whence", lang: "c", block: false),
    raw("struct fuse_file_info *fi", lang: "c", block: false),
  ),
)
