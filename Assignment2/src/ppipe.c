#include<ppipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>


int max(int a,int b){
    if(a>b)
        return a;
    return b;
}
// Per process information for the ppipe.
struct ppipe_info_per_process {
    // TODO:: Add members as per your need...

    int pid;
    int loc_offset;
    int is_ropen;
    int is_wopen;
    int read_pos;

};

// Global information for the ppipe.
struct ppipe_info_global {

    char *ppipe_buff;       // Persistent pipe buffer: DO NOT MODIFY THIS.
    int buff_offset;
    int write_pos;

    // TODO:: Add members as per your need...

};

// Persistent pipe structure.
// NOTE: DO NOT MODIFY THIS STRUCTURE.
struct ppipe_info {

    struct ppipe_info_per_process ppipe_per_proc [MAX_PPIPE_PROC];
    struct ppipe_info_global ppipe_global;

};


// Function to allocate space for the ppipe and initialize its members.
struct ppipe_info* alloc_ppipe_info() {

    // Allocate space for ppipe structure and ppipe buffer.
    struct ppipe_info *ppipe = (struct ppipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);

    // Assign ppipe buffer.
    ppipe->ppipe_global.ppipe_buff = buffer;

    /**
     *  TODO:: Initializing pipe fields
     *
     *  Initialize per process fields for this ppipe.
     *  Initialize global fields for this ppipe.
     *
     */
    ppipe->ppipe_global.write_pos=0;
    ppipe->ppipe_global.buff_offset=0;

    for (int i = 0; i < MAX_PPIPE_PROC; ++i)
    {
        ppipe->ppipe_per_proc[i].pid=-1;
        ppipe->ppipe_per_proc[i].is_ropen=0;
        ppipe->ppipe_per_proc[i].is_wopen=0;
        ppipe->ppipe_per_proc[i].read_pos=0;
        ppipe->ppipe_per_proc[i].loc_offset=0;

    }

    // Return the ppipe.

    return ppipe;

}

// Function to free ppipe buffer and ppipe info object.
// NOTE: DO NOT MODIFY THIS FUNCTION.
void free_ppipe (struct file *filep) {

    os_page_free(OS_DS_REG, filep->ppipe->ppipe_global.ppipe_buff);
    os_page_free(OS_DS_REG, filep->ppipe);

} 

// Fork handler for ppipe.
int do_ppipe_fork (struct exec_context *child, struct file *filep) {
    
    /**
     *  TODO:: Implementation for fork handler
     *
     *  You may need to update some per process or global info for the ppipe.
     *  This handler will be called twice since ppipe has 2 file objects.
     *  Also consider the limit on no of processes a ppipe can have.
     *  Return 0 on success.
     *  Incase of any error return -EOTHERS.
     *
     */

    // Return successfully.
    if(!filep || filep->type!=PPIPE)
        return -EOTHERS;
    int ppid = child->ppid;
    int proc_cnt=0,r_open=0,w_open=0;
    int idx=-1;
    int curr_parent=-1;
    for (int i = 0; i < MAX_PPIPE_PROC; ++i)
    {
        if(child->pid == filep->ppipe->ppipe_per_proc[i].pid)
            return 0; //PPIPE info updated already
        if(filep->ppipe->ppipe_per_proc[i].pid==ppid)
            curr_parent=i;
        proc_cnt += (filep->ppipe->ppipe_per_proc[i].pid!=-1);
        if(filep->ppipe->ppipe_per_proc[i].pid!=-1){
            r_open += (filep->ppipe->ppipe_per_proc[i].is_ropen==1);
            w_open += (filep->ppipe->ppipe_per_proc[i].is_wopen==1);
        }
    }
    for (int i = 0; i < MAX_PPIPE_PROC; ++i)
    {
        if(filep->ppipe->ppipe_per_proc[i].pid==-1){
            idx=i;
            break;
        }
    }

    if(proc_cnt==MAX_PPIPE_PROC || idx==-1 || curr_parent==-1)
        return -EOTHERS;
    

    filep->ppipe->ppipe_per_proc[idx].pid=child->pid;
    filep->ppipe->ppipe_per_proc[idx].is_ropen=filep->ppipe->ppipe_per_proc[curr_parent].is_ropen;
    filep->ppipe->ppipe_per_proc[idx].is_wopen=filep->ppipe->ppipe_per_proc[curr_parent].is_wopen;
    filep->ppipe->ppipe_per_proc[idx].read_pos=filep->ppipe->ppipe_per_proc[curr_parent].read_pos;
    filep->ppipe->ppipe_per_proc[idx].loc_offset=filep->ppipe->ppipe_per_proc[curr_parent].loc_offset;
    // Return successfully.

    return 0;

}


// Function to close the ppipe ends and free the ppipe when necessary.
long ppipe_close (struct file *filep) {

    /**
     *  TODO:: Implementation of Pipe Close
     *
     *  Close the read or write end of the ppipe depending upon the file
     *      object's mode.
     *  You may need to update some per process or global info for the ppipe.
     *  Use free_pipe() function to free ppipe buffer and ppipe object,
     *      whenever applicable.
     *  After successful close, it return 0.
     *  Incase of any error return -EOTHERS.
     *                                                                          
     */

    int ret_value;

    if(!filep || filep->type!=PPIPE || filep->ref_count==0)
        return -EOTHERS;
    int curr=-1;
    int proc_cnt=0,r_open=0,w_open=0;
    struct exec_context *current = get_current_ctx();
    for (int i = 0; i < MAX_PPIPE_PROC; ++i)
    {
        if(filep->ppipe->ppipe_per_proc[i].pid == current->pid){
            curr=i;
        }
        proc_cnt += (filep->ppipe->ppipe_per_proc[i].pid!=-1);
        if(filep->ppipe->ppipe_per_proc[i].pid!=-1){
            r_open += (filep->ppipe->ppipe_per_proc[i].is_ropen==1);
            w_open += (filep->ppipe->ppipe_per_proc[i].is_wopen==1);
        }
    }

    if(curr==-1)
        return -EOTHERS;
    if(filep->mode & O_READ){
            if(filep->ppipe->ppipe_per_proc[curr].is_ropen){
                filep->ppipe->ppipe_per_proc[curr].is_ropen=0;
                r_open--;
            }
            else return -EOTHERS;
    }
    else if(filep->mode & O_WRITE){
        if(filep->ppipe->ppipe_per_proc[curr].is_wopen){
            filep->ppipe->ppipe_per_proc[curr].is_wopen=0;
            w_open--;
        }
        else return -EOTHERS;
    }

    if (!filep->ppipe->ppipe_per_proc[curr].is_wopen && !filep->ppipe->ppipe_per_proc[curr].is_ropen)
    {
        filep->ppipe->ppipe_per_proc[curr].pid=-1;
        filep->ppipe->ppipe_per_proc[curr].read_pos=0;
        filep->ppipe->ppipe_per_proc[curr].loc_offset=0;
    }

    if(r_open==0 && w_open==0){
        free_ppipe(filep);
    }

    // Close the file.
    ret_value = file_close (filep);         // DO NOT MODIFY THIS LINE.

    // And return.
    return ret_value;

}

// Function to perform flush operation on pipe.
int do_flush_ppipe (struct file *filep) {

    /**
     *  TODO:: Implementation of Flush system call
     *
     *  Reclaim the region of the persistent pipe which has been read by 
     *      all the processes.
     *  Return no of reclaimed bytes.
     *  In case of any error return -EOTHERS.
     *
     */
    if(!filep || filep->type!=PPIPE)
        return -EOTHERS;
    int reclaimed_bytes = 0;
    int mx_off = 0;
    int write_pos = filep->ppipe->ppipe_global.write_pos;
    int r = 0;
    for (int i = 0; i < MAX_PPIPE_PROC; ++i)
    {   
        if(filep->ppipe->ppipe_per_proc[i].pid!=-1 && filep->ppipe->ppipe_per_proc[i].is_ropen){
            r=1;
            mx_off = max(mx_off,filep->ppipe->ppipe_per_proc[i].loc_offset);
        }
    }
    if(!r)
        return 0;
    int new_buff_offset = mx_off;
    
    reclaimed_bytes = filep->ppipe->ppipe_global.buff_offset-new_buff_offset;
    filep->ppipe->ppipe_global.buff_offset = new_buff_offset;
    return reclaimed_bytes;

}

// Read handler for the ppipe.
int ppipe_read (struct file *filep, char *buff, u32 count) {
    
    /**
     *  TODO:: Implementation of PPipe Read
     *
     *  Read the data from ppipe buffer and write to the provided buffer.
     *  If count is greater than the present data size in the ppipe then just read
     *      that much data.
     *  Validate file object's access right.
     *  On successful read, return no of bytes read.
     *  Incase of Error return valid error code.
     *      -EACCES: In case access is not valid.
     *      -EINVAL: If read end is already closed.
     *      -EOTHERS: For any other errors.
     *
     */

    if(!filep || !buff)
        return -EOTHERS;
    if(!(filep->mode & O_READ))
        return -EACCES;
    
    int bytes_read = 0;

    int pid = get_current_ctx()->pid;
    int curr=-1;
    for (int i = 0; i < MAX_PPIPE_PROC; ++i)
    {
        if(filep->ppipe->ppipe_per_proc[i].pid==pid){
            curr=i;
            break;
        }
    }

    if(curr==-1 || !(filep->ppipe->ppipe_per_proc[curr].is_ropen))
        return -EINVAL;
    while(bytes_read < count){
        if(filep->ppipe->ppipe_per_proc[curr].loc_offset==0)
            return bytes_read;
        buff[bytes_read] = filep->ppipe->ppipe_global.ppipe_buff[filep->ppipe->ppipe_per_proc[curr].read_pos];
        filep->ppipe->ppipe_per_proc[curr].read_pos = (filep->ppipe->ppipe_per_proc[curr].read_pos+1)%MAX_PPIPE_SIZE;
        filep->ppipe->ppipe_per_proc[curr].loc_offset--;
        bytes_read++;
    }
    // Return no of bytes read.
    return bytes_read;
	
}

// Write handler for ppipe.
int ppipe_write (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of PPipe Write
     *
     *  Write the data from the provided buffer to the ppipe buffer.
     *  If count is greater than available space in the ppipe then just write
     *      data that fits in that space.
     *  Validate file object's access right.
     *  On successful write, return no of written bytes.
     *  Incase of Error return valid error code.
     *      -EACCES: In case access is not valid.
     *      -EINVAL: If write end is already closed.
     *      -EOTHERS: For any other errors.
     *
     */
    if(!filep || !buff || filep->type!=PPIPE)
        return -EOTHERS;
    if( !(filep->mode & O_WRITE))
        return -EACCES;
    int pid = get_current_ctx()->pid;
    int curr=-1;
    for (int i = 0; i < MAX_PPIPE_PROC; ++i)
    {
        if(filep->ppipe->ppipe_per_proc[i].pid==pid){
            curr=i;
            break;
        }
    }
    if(curr==-1 || !(filep->ppipe->ppipe_per_proc[curr].is_wopen))
        return -EINVAL;

    int bytes_written = 0;

    while(bytes_written < count){
        if(filep->ppipe->ppipe_global.buff_offset==MAX_PPIPE_SIZE){
            break;
        }
        filep->ppipe->ppipe_global.ppipe_buff[filep->ppipe->ppipe_global.write_pos]=buff[bytes_written];
        bytes_written++;
        filep->ppipe->ppipe_global.buff_offset++;
        filep->ppipe->ppipe_global.write_pos = (filep->ppipe->ppipe_global.write_pos + 1)%MAX_PPIPE_SIZE; 
    }
    for (int i = 0; i < MAX_PPIPE_PROC; ++i)
    {
        if(filep->ppipe->ppipe_per_proc[i].pid!=-1){
            filep->ppipe->ppipe_per_proc[i].loc_offset = (filep->ppipe->ppipe_per_proc[i].loc_offset + bytes_written);
        }
    }
    // Return no of bytes written.
    return bytes_written;

}

// Function to create persistent pipe.
int create_persistent_pipe (struct exec_context *current, int *fd) {

    /**
     *  TODO:: Implementation of PPipe Create
     *
     *  Find two free file descriptors.
     *  Create two file objects for both ends by invoking the alloc_file() function.
     *  Create ppipe_info object by invoking the alloc_ppipe_info() function and
     *      fill per process and global info fields.
     *  Fill the fields for those file objects like type, fops, etc.
     *  Fill the valid file descriptor in *fd param.
     *  On success, return 0.
     *  Incase of Error return valid Error code.
     *      -ENOMEM: If memory is not enough.
     *      -EOTHERS: Some other errors.
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
    struct ppipe_info* p = alloc_ppipe_info();
    if(!p)
        return -ENOMEM;

    p->ppipe_per_proc[0].is_ropen=1;
    p->ppipe_per_proc[0].is_wopen=1;
    p->ppipe_per_proc[0].loc_offset=0;
    p->ppipe_per_proc[0].pid=get_current_ctx()->pid;


    f1->ppipe = p;
    f1->type = PPIPE;
    f1->inode = NULL;
    f1->mode = O_READ;
    f1->ref_count = 1;
 
    f1->fops->read = ppipe_read;
    f1->fops->write = ppipe_write;
    f1->fops->close = ppipe_close;  

    f2->ppipe = p;
    f2->type = PPIPE;
    f2->inode = NULL;
    f2->mode = O_WRITE;
    f2->ref_count = 1;
    f2->fops->read = ppipe_read;
    f2->fops->write = ppipe_write;
    f2->fops->close = ppipe_close;

    current->files[fd[0]]=f1;
    current->files[fd[1]]=f2;

    // Simple return.
    return 0;

}
