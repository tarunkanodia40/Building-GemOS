#include<pipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>


// Per process info for the pipe.
struct pipe_info_per_process {

    int is_ropen;
    int is_wopen;
    int pid;

};

// Global information for the pipe.
struct pipe_info_global {

    char *pipe_buff;    // Pipe buffer: DO NOT MODIFY THIS.
    int read_pos;
    int write_pos;
    int buff_offset;
    // TODO:: Add members as per your need...

};

// Pipe information structure.
// NOTE: DO NOT MODIFY THIS STRUCTURE.
struct pipe_info {

    struct pipe_info_per_process pipe_per_proc [MAX_PIPE_PROC];
    struct pipe_info_global pipe_global;

};


// Function to allocate space for the pipe and initialize its members.
struct pipe_info* alloc_pipe_info () {
	
    // Allocate space for pipe structure and pipe buffer.
    struct pipe_info *pipe = (struct pipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);

    // Assign pipe buffer.
    pipe->pipe_global.pipe_buff = buffer;

    /**
     *  TODO:: Initializing pipe fields
     *  
     *  Initialize per process fields for this pipe.
     *  Initialize global fields for this pipe.
     *
     */
    pipe->pipe_global.read_pos=0;
    pipe->pipe_global.write_pos=0;
    pipe->pipe_global.buff_offset=0;

    for (int i = 0; i < MAX_PIPE_PROC; ++i)
    {
    	pipe->pipe_per_proc[i].pid=-1;
    	pipe->pipe_per_proc[i].is_ropen=0;
    	pipe->pipe_per_proc[i].is_wopen=0;
    }
    // Return the pipe.
    return pipe;

}

// Function to free pipe buffer and pipe info object.
// NOTE: DO NOT MODIFY THIS FUNCTION.
void free_pipe (struct file *filep) {

    os_page_free(OS_DS_REG, filep->pipe->pipe_global.pipe_buff);
    os_page_free(OS_DS_REG, filep->pipe);

}

// Fork handler for the pipe.
int do_pipe_fork (struct exec_context *child, struct file *filep) {

    /**
     *  TODO:: Implementation for fork handler
     *
     *  You may need to update some per process or global info for the pipe.
     *  This handler will be called twice since pipe has 2 file objects.
     *  Also consider the limit on no of processes a pipe can have.
     *  Return 0 on success.
     *  Incase of any error return -EOTHERS.
     *
     */
	// fill at first index
	if(!filep || filep->type!=PIPE)
		return -EOTHERS;
	int proc_cnt=0,r_open=0,w_open=0;
	int idx=-1;
    int par=-1;

	for (int i = 0; i < MAX_PIPE_PROC; ++i)
	{
        if(child->pid == filep->pipe->pipe_per_proc[i].pid)
            return 0;   // Pipe info for this process already updated.
        if(child->ppid == filep->pipe->pipe_per_proc[i].pid)
            par=i;
		proc_cnt += (filep->pipe->pipe_per_proc[i].pid!=-1);
        if(filep->pipe->pipe_per_proc[i].pid!=-1){
        	r_open += (filep->pipe->pipe_per_proc[i].is_ropen==1);
    		w_open += (filep->pipe->pipe_per_proc[i].is_wopen==1);
        }
	}

    for (int i = 0; i < MAX_PIPE_PROC ; ++i)
    {
        if(filep->pipe->pipe_per_proc[i].pid==-1){
            idx=i;
            break;
        }
    }

	if(par==-1 || proc_cnt==MAX_PIPE_PROC || idx==-1)
		return -EOTHERS;

	filep->pipe->pipe_per_proc[idx].pid=child->pid;
	filep->pipe->pipe_per_proc[idx].is_ropen=filep->pipe->pipe_per_proc[par].is_ropen;
	filep->pipe->pipe_per_proc[idx].is_wopen=filep->pipe->pipe_per_proc[par].is_wopen;
    // Return successfully.
    return 0;

}

// Function to close the pipe ends and free the pipe when necessary.
long pipe_close (struct file *filep) {

    /**
     *  TODO:: Implementation of Pipe Close
     *
     *  Close the read or write end of the pipe depending upon the file
     *      object's mode.
     *  You may need to update some per process or global info for the pipe.
     *  Use free_pipe() function to free pipe buffer and pipe object,
     *      whenever applicable.
     *  After successful close, it return 0.
     *  Incase of any error return -EOTHERS.
     *
     */

    int ret_value;

    if(!filep || filep->type!=PIPE || filep->ref_count==0)
        return -EOTHERS;
    

    // Close the file and return.
    struct exec_context *current = get_current_ctx();
    int curr = -1;

    int proc_cnt=0,r_open=0,w_open=0;
	for (int i = 0; i < MAX_PIPE_PROC; ++i)
	{
		if(filep->pipe->pipe_per_proc[i].pid == current->pid){
            curr=i;
        }
		proc_cnt += (filep->pipe->pipe_per_proc[i].pid!=-1);
        if(filep->pipe->pipe_per_proc[i].pid!=-1){
        	r_open += (filep->pipe->pipe_per_proc[i].is_ropen==1);
    		w_open += (filep->pipe->pipe_per_proc[i].is_wopen==1);
        }

	}
    if(curr==-1)
        return -EOTHERS;
    if((filep->mode & O_READ)){
    	if(filep->pipe->pipe_per_proc[curr].is_ropen){
	       	filep->pipe->pipe_per_proc[curr].is_ropen=0;
	       	r_open--;
    	}
    	else return -EOTHERS;
    }
    else if((filep->mode & O_WRITE)){
    	if(filep->pipe->pipe_per_proc[curr].is_wopen){
	       	filep->pipe->pipe_per_proc[curr].is_wopen=0;
	       	w_open--;
    	}
    	else return -EOTHERS;
    }
    
    if (!filep->pipe->pipe_per_proc[curr].is_ropen && !filep->pipe->pipe_per_proc[curr].is_wopen)
    {
    	filep->pipe->pipe_per_proc[curr].pid=-1;
    }
    if(r_open==0 && w_open==0){
        free_pipe(filep);
    }
    ret_value = file_close(filep);         // DO NOT MODIFY THIS LINE.

    return ret_value;

}

// Check whether passed buffer is valid memory location for read or write.
int is_valid_mem_range (unsigned long buff, u32 count, int access_bit) {

    /**
     *  TODO:: Implementation for buffer memory range checking
     *
     *  Check whether passed memory range is suitable for read or write.
     *  If access_bit == 1, then it is asking to check read permission.
     *  If access_bit == 2, then it is asking to check write permission.
     *  If range is valid then return 1.
     *  Incase range is not valid or have some permission issue return -EBADMEM.
     *
     */

    int ret_value = -EBADMEM;

    struct exec_context *curr = get_current_ctx();
    int val=-1;
    for (int i = 0; i < MAX_MM_SEGS; ++i)
    {
        if(curr->mms[i].start>=STACK_START - MAX_STACK_SIZE){
            if(buff>= curr->mms[i].start && buff+count-1 <= curr->mms[i].end){
                val=i;
                break;
            }
        }
        else{
            if(buff>= curr->mms[i].start && buff+count-1 < curr->mms[i].next_free){
                val=i;
                break;
            }
        }
    }

    if(val!=-1){
        if((access_bit & curr->mms[val].access_flags))
            ret_value=1;
    }
    else{
        struct vm_area *v = curr->vm_area;
        while(v){
            if(buff>=v->vm_start && buff+count-1<=v->vm_end){
                if(access_bit & v->access_flags)
                    ret_value = 1;
            }
            v = v->vm_next;
        }
    }


    // Return the finding.
    
    return ret_value;

}

// Function to read given no of bytes from the pipe.
int pipe_read (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of Pipe Read
     *
     *  Read the data from pipe buffer and write to the provided buffer.
     *  If count is greater than the present data size in the pipe then just read
     *       that much data.
     *  Validate file object's access right.
     *  On successful read, return no of bytes read.
     *  Incase of Error return valid error code.
     *       -EACCES: In case access is not valid.
     *       -EINVAL: If read end is already closed.
     *       -EOTHERS: For any other errors.
     *
     */

    if(!filep || !buff)
        return -EOTHERS;
    if(!(filep->mode & O_READ))
        return -EACCES;
    
    int ret = is_valid_mem_range((unsigned long)buff,count,O_WRITE);
    if(ret<0)
        return -EACCES;

    int bytes_read = 0;

    int pid = get_current_ctx()->pid;
    int curr=-1;
    int proc_cnt=0,r_open=0,w_open=0;
	for (int i = 0; i < MAX_PIPE_PROC; ++i)
	{
		if(filep->pipe->pipe_per_proc[i].pid == pid){
            curr=i;
        }
		proc_cnt += (filep->pipe->pipe_per_proc[i].pid!=-1);
        if(filep->pipe->pipe_per_proc[i].pid!=-1){
        	r_open += (filep->pipe->pipe_per_proc[i].is_ropen==1);
    		w_open += (filep->pipe->pipe_per_proc[i].is_wopen==1);
        }

	}
    

    if(curr==-1 || !(filep->pipe->pipe_per_proc[curr].is_ropen) )
        return -EINVAL;
    while(bytes_read < count){
        if(filep->pipe->pipe_global.buff_offset==0)
            return bytes_read;
        buff[bytes_read] = filep->pipe->pipe_global.pipe_buff[filep->pipe->pipe_global.read_pos];
        filep->pipe->pipe_global.read_pos = (filep->pipe->pipe_global.read_pos+1)%MAX_PIPE_SIZE;
        bytes_read++;
        filep->pipe->pipe_global.buff_offset--;
    }
    // Return no of bytes read.
    return bytes_read;

}

// Function to write given no of bytes to the pipe.
int pipe_write (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of Pipe Write
     *
     *  Write the data from the provided buffer to the pipe buffer.
     *  If count is greater than available space in the pipe then just write data
     *       that fits in that space.
     *  Validate file object's access right.
     *  On successful write, return no of written bytes.
     *  Incase of Error return valid error code.
     *       -EACCES: In case access is not valid.
     *       -EINVAL: If write end is already closed.
     *       -EOTHERS: For any other errors.
     *
     */
    if(!filep || !buff)
        return -EOTHERS;
    if(!(filep->mode & O_WRITE))
    	return -EACCES;
    int pid = get_current_ctx()->pid;
    int curr=-1;
    int ret = is_valid_mem_range((unsigned long)buff,count,O_READ);
    if(ret<0)
        return -EACCES;
    for (int i = 0; i < MAX_PIPE_PROC; ++i)
    {
        if(filep->pipe->pipe_per_proc[i].pid==pid){
            curr=i;
            break;
        }
    }
    if(curr==-1 || !(filep->pipe->pipe_per_proc[curr].is_wopen))
        return -EINVAL;

    int bytes_written = 0;

    while(bytes_written < count){
        if(filep->pipe->pipe_global.buff_offset==MAX_PIPE_SIZE)
            return bytes_written;
        filep->pipe->pipe_global.pipe_buff[filep->pipe->pipe_global.write_pos]=buff[bytes_written];
        bytes_written++;
        filep->pipe->pipe_global.buff_offset++;
        filep->pipe->pipe_global.write_pos = (filep->pipe->pipe_global.write_pos + 1)%MAX_PIPE_SIZE; 
    }
    // Return no of bytes written.
    return bytes_written;

}

// Function to create pipe.
int create_pipe (struct exec_context *current, int *fd) {

    /**
     *  TODO:: Implementation of Pipe Create
     *
     *  Find two free file descriptors.
     *  Create two file objects for both ends by invoking the alloc_file() function. 
     *  Create pipe_info object by invoking the alloc_pipe_info() function and
     *       fill per process and global info fields.
     *  Fill the fields for those file objects like type, fops, etc.
     *  Fill the valid file descriptor in *fd param.
     *  On success, return 0.
     *  Incase of Error return valid Error code.
     *       -ENOMEM: If memory is not enough.
     *       -EOTHERS: Some other errors.
     *
     */
    int k=0;
    if(!current)
    	return -EOTHERS;
    while(current->files[k]){
        k++;
        if(k==MAX_OPEN_FILES)
            return -EOTHERS;
    }
    fd[0]=k;
    if(k+1==MAX_OPEN_FILES)
        return -EOTHERS;
    k++;
    while(current->files[k]){
        k++;
        if(k==MAX_OPEN_FILES)
            return -EOTHERS;
    }
    fd[1]=k;
    struct file* f1 = alloc_file();
    if(!f1)
        return -ENOMEM;
    struct file* f2 = alloc_file();
    if(!f2)
        return -ENOMEM;
    struct pipe_info* p = alloc_pipe_info();
    if(!p)
        return -ENOMEM;

    p->pipe_per_proc[0].is_ropen=1;
    p->pipe_per_proc[0].is_wopen=1;
    p->pipe_per_proc[0].pid=get_current_ctx()->pid;


    f1->pipe = p;
    f1->type = PIPE;
    f1->inode = NULL;
    f1->mode = O_READ;
    f1->ref_count = 1;
 
    f1->fops->read = pipe_read;
    f1->fops->write = pipe_write;
    f1->fops->close = pipe_close;

    f2->pipe = p;
    f2->type = PIPE;
    f2->inode = NULL;
    f2->mode = O_WRITE;
    f2->ref_count = 1;
    f2->fops->read = pipe_read;
    f2->fops->write = pipe_write;
    f2->fops->close = pipe_close;

    current->files[fd[0]]=f1;
    current->files[fd[1]]=f2;
    // Simple return.
    return 0;

}
