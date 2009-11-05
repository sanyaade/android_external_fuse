/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB.
*/

#include "fuse_i.h"
#include "fuse_misc.h"
#include "fuse_lowlevel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

struct procdata {
	struct fuse *f;
	struct fuse_chan *prevch;
	struct fuse_session *prevse;
	fuse_processor_t proc;
	void *data;
};

static void mt_session_proc(void *data, const char *buf, size_t len,
			    struct fuse_chan *ch)
{
	struct procdata *pd = (struct procdata *) data;
	struct fuse_cmd *cmd = *(struct fuse_cmd **) buf;

	(void) len;
	(void) ch;
	pd->proc(pd->f, cmd, pd->data);
}

static void mt_session_exit(void *data, int val)
{
	struct procdata *pd = (struct procdata *) data;
	if (val)
		fuse_session_exit(pd->prevse);
	else
		fuse_session_reset(pd->prevse);
}

static int mt_session_exited(void *data)
{
	struct procdata *pd = (struct procdata *) data;
	return fuse_session_exited(pd->prevse);
}

static int mt_chan_receive(struct fuse_chan **chp, char *buf, size_t size)
{
	struct fuse_cmd *cmd;
	struct procdata *pd = (struct procdata *) fuse_chan_data(*chp);

	assert(size >= sizeof(cmd));

	cmd = fuse_read_cmd(pd->f);
	if (cmd == NULL)
		return 0;

	*(struct fuse_cmd **) buf = cmd;

	return sizeof(cmd);
}

