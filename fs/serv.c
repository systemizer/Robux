/*
 * File system server main loop -
 * serves IPC requests from other environments.
 */
#include <inc/lib.h>
#include <inc/x86.h>
#include <inc/string.h>

#include "fs.h"


#define debug 0

// The file system server maintains three structures
// for each open file.
//
// 1. The on-disk 'struct File' is mapped into the part of memory
//    that maps the disk.  This memory is kept private to the file
//    server.
// 2. Each open file has a 'struct Fd' as well, which sort of
//    corresponds to a Unix file descriptor.  This 'struct Fd' is kept
//    on *its own page* in memory, and it is shared with any
//    environments that have the file open.
// 3. 'struct OpenFile' links these other two structures, and is kept
//    private to the file server.  The server maintains an array of
//    all open files, indexed by "file ID".  (There can be at most
//    MAXOPEN files open concurrently.)  The client uses file IDs to
//    communicate with the server.  File IDs are a lot like
//    environment IDs in the kernel.  Use openfile_lookup to translate
//    file IDs to struct OpenFile.

struct OpenFile {
	uint32_t o_fileid;	// file id
	struct File *o_file;	// mapped descriptor for open file
	int o_mode;		// open mode
	struct Fd *o_fd;	// Fd page
};

// Max number of open files in the file system at once
#define MAXOPEN		1024
#define FILEVA		0xD0000000

// initialize to force into data section
struct OpenFile opentab[MAXOPEN] = {
	{ 0, 0, 1, 0 }
};

// Virtual address at which to receive page mappings containing client requests.
union Fsipc *fsreq = (union Fsipc *)0x0ffff000;

void
serve_init(void)
{
	int i;
	uintptr_t va = FILEVA;
	for (i = 0; i < MAXOPEN; i++) {
		opentab[i].o_fileid = i;
		opentab[i].o_fd = (struct Fd*) va;
		va += PGSIZE;
	}
}

// Allocate an open file.
int
openfile_alloc(struct OpenFile **o)
{
	int i, r;

	// Find an available open-file table entry
	for (i = 0; i < MAXOPEN; i++) {
		switch (pageref(opentab[i].o_fd)) {
		case 0:
			if ((r = sys_page_alloc(0, opentab[i].o_fd, PTE_P|PTE_U|PTE_W)) < 0)
				return r;
			/* fall through */
		case 1:
			opentab[i].o_fileid += MAXOPEN;
			*o = &opentab[i];
			memset(opentab[i].o_fd, 0, PGSIZE);
			return (*o)->o_fileid;
		}
	}
	return -E_MAX_OPEN;
}

// Look up an open file for envid.
int
openfile_lookup(envid_t envid, uint32_t fileid, struct OpenFile **po)
{
	struct OpenFile *o;

	o = &opentab[fileid % MAXOPEN];
	if (pageref(o->o_fd) == 1 || o->o_fileid != fileid)
		return -E_INVAL;
	*po = o;
	return 0;
}

// Open req->req_path in mode req->req_omode, storing the Fd page and
// permissions to return to the calling environment in *pg_store and
// *perm_store respectively.
int
serve_open(envid_t envid, struct Fsreq_open *req,
	   void **pg_store, int *perm_store)
{
	char path[MAXPATHLEN];
	struct File *f;
	int fileid;
	int r;
	struct OpenFile *o;
	struct Env env = envs[ENVX(envid)];

	if (debug)
		cprintf("serve_open %08x %s 0x%x\n", envid, req->req_path, req->req_omode);

	// Copy in the path, making sure it's null-terminated
	memmove(path, req->req_path, MAXPATHLEN);
	path[MAXPATHLEN-1] = 0;

	// Find an open file ID
	if ((r = openfile_alloc(&o)) < 0) {
		if (debug)
			cprintf("openfile_alloc failed: %e", r);
		return r;
	}
	fileid = r;

	// Open the file
	if (req->req_omode & O_CREAT) {
		if ((r = file_create(path, &f,env.env_gid,env.env_uid,DEFAULT_FILE_CREATE_PERM)) < 0) {
			if (!(req->req_omode & O_EXCL) && r == -E_FILE_EXISTS)
				goto try_open;
			if (debug)
				cprintf("file_create failed: %e", r);
			return r;
		}
	} else {
try_open:
		if ((r = file_open(path, &f)) < 0) {
			if (debug)
				cprintf("file_open failed: %e", r);
			return r;
		}
	}

	// Truncate
	if (req->req_omode & O_TRUNC) {
		if ((r = file_set_size(f, 0)) < 0) {
			if (debug)
				cprintf("file_set_size failed: %e", r);
			return r;
		}
	}

	// Save the file pointer
	o->o_file = f;

	// Fill out the Fd structure
	o->o_fd->fd_file.id = o->o_fileid;
	o->o_fd->fd_omode = req->req_omode & O_ACCMODE;
	o->o_fd->fd_dev_id = devfile.dev_id;
	
	o->o_mode = req->req_omode;
	
	// fill in permission bits
	o->o_fd->perm = f->f_perm;
	o->o_fd->gid = f->f_gid; 
	o->o_fd->uid = f->f_uid;

	if (debug)
		cprintf("sending success, page %08x\n", (uintptr_t) o->o_fd);

	// Share the FD page with the caller
	*pg_store = o->o_fd;
	*perm_store = PTE_P|PTE_U|PTE_W|PTE_SHARE;
	return 0;
}

// Set the size of req->req_fileid to req->req_size bytes, truncating
// or extending the file as necessary.
int
serve_set_size(envid_t envid, struct Fsreq_set_size *req)
{
	struct OpenFile *o;
	int r;

	if (debug)
		cprintf("serve_set_size %08x %08x %08x\n", envid, req->req_fileid, req->req_size);

	// Every file system IPC call has the same general structure.
	// Here's how it goes.

	// First, use openfile_lookup to find the relevant open file.
	// On failure, return the error code to the client with ipc_send.
	if ((r = openfile_lookup(envid, req->req_fileid, &o)) < 0)
		return r;

	// Second, call the relevant file system function (from fs/fs.c).
	// On failure, return the error code to the client.
	return file_set_size(o->o_file, req->req_size);
}

// Read at most ipc->read.req_n bytes from the current seek position
// in ipc->read.req_fileid.  Return the bytes read from the file to
// the caller in ipc->readRet, then update the seek position.  Returns
// the number of bytes successfully read, or < 0 on error.
int
serve_read(envid_t envid, union Fsipc *ipc)
{
	struct Fsreq_read *req = &ipc->read;
	struct Fsret_read *ret = &ipc->readRet;

	if (debug)
		cprintf("serve_read %08x %08x %08x\n", envid, req->req_fileid, req->req_n);

	// Look up the file id, read the bytes into 'ret', and update
	// the seek position.  Be careful if req->req_n > PGSIZE
	// (remember that read is always allowed to return fewer bytes
	// than requested).  Also, be careful because ipc is a union,
	// so filling in ret will overwrite req.
	//
	// Hint: Use file_read.
	// Hint: The seek position is stored in the struct Fd.
	// LAB 5: Your code here
	struct OpenFile *po;
	int r;

	if ((r = openfile_lookup(envid, req->req_fileid, &po)) < 0)
		return r;
	
	struct File *file = po->o_file;
	struct Fd *fd = po->o_fd;
	if ((r = file_read(file, 
					 ret->ret_buf, 
					 MIN(req->req_n, PGSIZE), 
					 fd->fd_offset)) < 0)
	{
			return r;
	}
	else
	{
		fd->fd_offset += r;
		return r;
	}
	
}

// Write req->req_n bytes from req->req_buf to req_fileid, starting at
// the current seek position, and update the seek position
// accordingly.  Extend the file if necessary.  Returns the number of
// bytes written, or < 0 on error.
int
serve_write(envid_t envid, struct Fsreq_write *req)
{
	if (debug)
		cprintf("serve_write %08x %08x %08x\n", envid, req->req_fileid, req->req_n);

	// LAB 5: Your code here.
	struct OpenFile *po;
	int r;

	if ((r = openfile_lookup(envid, req->req_fileid, &po)) < 0)
		return r;
	
	struct File *file = po->o_file;
	struct Fd *fd = po->o_fd;
	if((r = file_write(file, req->req_buf, req->req_n, fd->fd_offset)) < 0)
		return r;

	fd->fd_offset += r;
	return r;

}

// Stat ipc->stat.req_fileid.  Return the file's struct Stat to the
// caller in ipc->statRet.
int
serve_stat(envid_t envid, union Fsipc *ipc)
{
	struct Fsreq_stat *req = &ipc->stat;
	struct Fsret_stat *ret = &ipc->statRet;
	struct OpenFile *o;
	int r;

	if (debug)
		cprintf("serve_stat %08x %08x\n", envid, req->req_fileid);

	if ((r = openfile_lookup(envid, req->req_fileid, &o)) < 0)
		return r;

	strcpy(ret->ret_name, o->o_file->f_name);
	ret->ret_size = o->o_file->f_size;
	ret->ret_isdir = (o->o_file->f_type == FTYPE_DIR);
	ret->ret_uid = o->o_file->f_uid;
	ret->ret_gid = o->o_file->f_gid;
	ret->ret_perm = o->o_file->f_perm;

	return 0;
}

// Flush all data and metadata of req->req_fileid to disk.
int
serve_flush(envid_t envid, struct Fsreq_flush *req)
{
	struct OpenFile *o;
	int r;

	if (debug)
		cprintf("serve_flush %08x %08x\n", envid, req->req_fileid);

	if ((r = openfile_lookup(envid, req->req_fileid, &o)) < 0)
		return r;
	file_flush(o->o_file);
	return 0;
}

// Remove the file req->req_path.
int
serve_remove(envid_t envid, struct Fsreq_remove *req)
{
	char path[MAXPATHLEN];
	int r;

	if (debug)
		cprintf("serve_remove %08x %s\n", envid, req->req_path);

	// Delete the named file.
	// Note: This request doesn't refer to an open file.

	// Copy in the path, making sure it's null-terminated
	memmove(path, req->req_path, MAXPATHLEN);
	path[MAXPATHLEN-1] = 0;

	// Delete the specified file
	return file_remove(path);
}

// Sync the file system.
int
serve_sync(envid_t envid, union Fsipc *req)
{
	fs_sync();
	return 0;
}

//Change permission of a file
int
serve_chmod(envid_t envid, union Fsipc *req)
{
	struct OpenFile* po;
	int r;
	if ((r=openfile_lookup(envid, req->chmod.req_fileid,&po))<0)
		return r;
	if ((r=file_set_perm(po->o_file,req->chmod.f_perm))<0)
		return r;
	po->o_fd->perm = req->chmod.f_perm;
	return 0;		
}

//Change permission of a file
int
serve_chown(envid_t envid, union Fsipc *req)
{
	struct OpenFile* po;
	int r;
	if ((r=openfile_lookup(envid, req->chown.req_fileid,&po))<0)
		return r;
	if ((r=file_set_uid(po->o_file,req->chown.uid))<0)
		return r;
	po->o_fd->uid = req->chown.uid;
	return 0;		
}

//Change permission of a file
int
serve_chgrp(envid_t envid, union Fsipc *req)
{
	struct OpenFile* po;
	int r;
	if ((r=openfile_lookup(envid, req->chgrp.req_fileid,&po))<0)
		return r;
	if ((r=file_set_gid(po->o_file,req->chgrp.gid))<0)
		return r;
	po->o_fd->gid = req->chgrp.gid;
	return 0;		
}


typedef int (*fshandler)(envid_t envid, union Fsipc *req);

fshandler handlers[] = {
	// Open is handled specially because it passes pages
	/* [FSREQ_OPEN] =	(fshandler)serve_open, */
	[FSREQ_SET_SIZE] =	(fshandler)serve_set_size,
	[FSREQ_READ] =		serve_read,
	[FSREQ_WRITE] =		(fshandler)serve_write,
	[FSREQ_STAT] =		serve_stat,
	[FSREQ_FLUSH] =		(fshandler)serve_flush,
	[FSREQ_REMOVE] =	(fshandler)serve_remove,
	[FSREQ_SYNC] =		serve_sync,
	[FSREQ_CHMOD] =         serve_chmod,
	[FSREQ_CHOWN] =         serve_chown,
	[FSREQ_CHGRP] =         serve_chgrp
	
};
#define NHANDLERS (sizeof(handlers)/sizeof(handlers[0]))

static int
has_perm(envid_t envid,union Fsipc *fsreq, uint32_t req) {
	int fileid;
	struct Env env;
	struct Fd *fd;
	int r;

	env = envs[ENVX(envid)];

	//root bypasses all permission checks
	if (env.env_uid==0)
		return 0;

	//only root can use chown
	if (req==FSREQ_CHOWN)
		return -E_BAD_PERM;

	//exceptional cases: doesn't require permissions
	//file sync
	if (req==FSREQ_SYNC)
		return 0;

	//handle FSREQ_OPEN and FSREQ_REMOVE differently
	//because they dont refer to openfiles
	if (req==FSREQ_OPEN || req==FSREQ_REMOVE)
	{
		char path[MAXPATHLEN];
		struct File *pf;
		

		switch (req)
		{
		case FSREQ_OPEN:

			// Copy in the path, making sure it's null-terminated
			memmove(path, fsreq->open.req_path, MAXPATHLEN);
			path[MAXPATHLEN-1] = 0;


			//populate pf with struct File
			if ((r=file_open(path,&pf))<0 && r!=-E_NOT_FOUND)
				return r;
			
			//if creating a new file, return 0
			if ((fsreq->open.req_omode&~O_ACCMODE)==(O_CREAT|O_TRUNC) && r==-E_NOT_FOUND) 				
				return 0;

			// Get out of here if there is no file to open
			if(r == -E_NOT_FOUND)
					return r;
				
			
			switch(fsreq->open.req_omode&O_ACCMODE)
			{
			case O_RDONLY:
				if ((pf->f_uid==env.env_uid && pf->f_perm&FSP_O_R) ||
				    (pf->f_gid==env.env_gid && pf->f_perm&FSP_G_R) ||
				    pf->f_perm&FSP_A_R)
					return 0;
				else
					return -E_BAD_PERM;
				break;
			case O_WRONLY:
			case O_RDWR:
				if ((pf->f_uid==env.env_uid && pf->f_perm&FSP_O_W) ||
				    (pf->f_gid==env.env_gid && pf->f_perm&FSP_G_W) ||
				    pf->f_perm&FSP_A_W)
					return 0;
				else
					return -E_BAD_PERM;
				break;
			default:
				return -E_INVAL;

			}
			
		case FSREQ_REMOVE:
			// Copy in the path, making sure it's null-terminated
			memmove(path, fsreq->remove.req_path, MAXPATHLEN);
			path[MAXPATHLEN-1] = 0;
			
			//populate pf with struct File
			if ((r=file_open(path,&pf))<0)
				return r;

			cprintf("0x%x %d\n", pf, r);

			//permissions logic
			if ((pf->f_uid==env.env_uid && pf->f_perm&FSP_O_W) ||
			    (pf->f_gid==env.env_gid && pf->f_perm&FSP_G_W) ||
			    pf->f_perm&FSP_A_W)
				return 0;
			else
				return -E_BAD_PERM;
			break;
		default:
			return -E_INVAL;
			
		}
	} 
	else 
	{	       
		switch (req)
		{
		case FSREQ_CHGRP:
			fileid = fsreq->chgrp.req_fileid;
			break;
		case FSREQ_CHMOD:
			fileid = fsreq->chmod.req_fileid;
			break;
		case FSREQ_READ:
			fileid = fsreq->read.req_fileid;
			break;
		case FSREQ_STAT:
			fileid = fsreq->stat.req_fileid;
			break;
		case FSREQ_SET_SIZE:
			fileid = fsreq->set_size.req_fileid;
			break;
		case FSREQ_WRITE:
			fileid = fsreq->write.req_fileid;
			break;
		case FSREQ_FLUSH:
			fileid = fsreq->flush.req_fileid;
			break;
		default:
			return -E_INVAL;
		}
		
		

		struct OpenFile *op;
		if ((r=openfile_lookup(envid,fileid,&op))<0)
			return r;

		//grab file descriptor from open file
		fd = op->o_fd;
		
		//compare permissions of fd with env
		switch (req)
		{
		case FSREQ_CHGRP:
			if (fd->uid==env.env_uid && fd->gid==env.env_gid)
				r = 0;
			else
				r = -E_BAD_PERM;
			break;			
	        case FSREQ_CHMOD:
			if (fd->uid==env.env_uid)
				r = 0;
			else
				r = -E_BAD_PERM;
			break;
	        //These require the READ bit
		case FSREQ_READ:
		case FSREQ_STAT:
			if ((fd->uid==env.env_uid && fd->perm&FSP_O_R) ||
			    (fd->gid==env.env_gid && fd->perm&FSP_G_R) ||
			    fd->perm&FSP_A_R)
				r=0;
			else
				r=-E_BAD_PERM;
			break;
		// These require the WRITE bit
		case FSREQ_SET_SIZE:
		case FSREQ_WRITE:
		case FSREQ_FLUSH:
			if ((fd->uid==env.env_uid && fd->perm&FSP_O_W) ||
			    (fd->gid==env.env_gid && fd->perm&FSP_G_W) ||
			    fd->perm&FSP_A_W)
				r = 0;
			else
				r = -E_BAD_PERM;
			break;
		default:
			return -E_INVAL;
		}
	}
	return r;
}


void
serve(void)
{
	uint32_t req, whom;
	int perm, r;
	void *pg;

	while (1) {
		perm = 0;
		req = ipc_recv((int32_t *) &whom, fsreq, &perm);
		if (debug)
			cprintf("fs req %d from %08x [page %08x: %s]\n",
				req, whom, vpt[PGNUM(fsreq)], fsreq);

		// All requests must contain an argument page
		if (!(perm & PTE_P)) {
			cprintf("Invalid request from %08x: no argument page\n",
				whom);
			continue; // just leave it hanging...
		}


		if ((r=has_perm(whom,fsreq,req))==0)
		{
			pg = NULL;
			if (req == FSREQ_OPEN) {
				r = serve_open(whom, (struct Fsreq_open*)fsreq, &pg, &perm);
			} else if (req < NHANDLERS && handlers[req]) {
				r = handlers[req](whom, fsreq);
			} else {
			cprintf("Invalid request code %d from %08x\n", whom, req);
			r = -E_INVAL;
			}
		}
		// Return, either the result of has_perm or the op
		ipc_send(whom, r, pg, perm);
		sys_page_unmap(0, fsreq);
	}
}


void
umain(int argc, char **argv)
{
	//static_assert(sizeof(struct File) == 256);
	binaryname = "fs";
	cprintf("FS is running\n");

	// Check that we are able to do I/O
	outw(0x8A00, 0x8A00);
	cprintf("FS can do I/O\n");

	serve_init();
	fs_init();
	serve();
}

