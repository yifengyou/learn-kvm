/*
 *  include/linux/anon_inodes.h
 *
 *  Copyright (C) 2007  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#ifndef _LINUX_ANON_INODES_H
#define _LINUX_ANON_INODES_H

struct file_operations;

int anon_inode_getfd(const char *name, const struct file_operations *fops,
		     void *priv);

#endif /* _LINUX_ANON_INODES_H */
