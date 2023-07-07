#include "simplefs-ops.h"
extern struct filehandle_t file_handle_array[MAX_OPEN_FILES]; // Array for storing opened files

int simplefs_create(char *filename){
    /*
	    Create file with name `filename` from disk
	*/
	
	//check if a file with the sam efilename exists
	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	for (int i = 0; i < NUM_INODES; i++)
	{
		simplefs_readInode(i, inode);
		if (inode->status == INODE_IN_USE && strcmp(inode->name, filename) == 0)
		{
			free(inode);
			return -1;
		}
	}
	// struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	// allocate new inode
	int inode_number = simplefs_allocInode();
	if (inode_number == -1) //if -1 then no freeinode is avail
	{
		free(inode);
		return -1;
	}

	
	// mark inode inuse
	inode->status = INODE_IN_USE;
	strcpy(inode->name, filename);
	//make size 0 as new file
	inode->file_size = 0;
	// as no content -1
	for(int i=0; i<MAX_FILE_SIZE; i++){
		inode->direct_blocks[i] = -1;
	}

	simplefs_writeInode(inode_number, inode);
	return inode_number;

	free(inode);	
    return -1;
}


void simplefs_delete(char *filename){
    /*
	    delete file with name `filename` from disk
	*/

	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	//iterate through inodes array
	for (int i = 0; i < NUM_INODES; i++)
	{

		simplefs_readInode(i, inode);

		if (inode->status == INODE_IN_USE) //if in use
		{
			// check filename
			if (strcmp(inode->name, filename) == 0) {
				//iterate thriugh all blocks it ponts to and free them
				for (int j = 0; j < NUM_INODE_BLOCKS; j++){
					if (inode->direct_blocks[j] != -1){
						//free them
						simplefs_freeDataBlock(inode->direct_blocks[j]);
					}
				}			
				memset(inode->name, 0, MAX_NAME_STRLEN);
				// free inode 
				simplefs_freeInode(i);
				// free ptr
				free(inode);
				return;
			}
		}
	}

	free(inode);
	return;

}

int simplefs_open(char *filename){
    /*
	    open file with name `filename`
	*/
// filehandle array stores openfiles's inode number and offset
	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	int is_node = -1;
	//iter through all inoeds
	for (int i = 0; i < NUM_INODES; i++)
	{
		// read inode
		simplefs_readInode(i, inode);
		//compare filename
		if (inode->status == INODE_IN_USE && strcmp(inode->name, filename) == 0)
		{
			is_node = i;
			// break;
			for (int i = 0; i < MAX_OPEN_FILES; i++) {
				// filehandle array has space
				if (file_handle_array[i].inode_number == -1) {
					//set inode number
					file_handle_array[i].inode_number = is_node;
					//new file so 0 offset
					file_handle_array[i].offset = 0;
					//free ptr
					free(inode);
					return i;
				}
			}
		}
	}

	if (is_node == -1){
		free(inode);
    	return -1;
	}

	free(inode);
    return -1;
}

void simplefs_close(int file_handle){
    /*
	    close file pointed by `file_handle`
	*/
// update file handle array
	if (file_handle < MAX_OPEN_FILES)
	{
		file_handle_array[file_handle].inode_number = -1;
		file_handle_array[file_handle].offset = 0;
	}
	return; 

}

int simplefs_read(int file_handle, char *buf, int nbytes){
    /*
	    read `nbytes` of data into `buf` from file pointed by `file_handle` starting at current offset
	*/
	struct filehandle_t file_desc = file_handle_array[file_handle];
	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	simplefs_readInode(file_desc.inode_number, inode);

	int offset = file_desc.offset;
	int id_block = offset/BLOCKSIZE;
	int residual = offset% BLOCKSIZE;

	// if not enough datat to read, return -1
	if(inode->file_size < (offset + nbytes)){
		free(inode);
        return -1;
	}
	
	memset(buf, 0, nbytes);
	// array of blocksize
	char *alloc_data = malloc(BLOCKSIZE);

	while (nbytes > 0)
	{
		//read one block into alloc_Data
		simplefs_readDataBlock(inode->direct_blocks[id_block], alloc_data);

		// current block offset + nbytes
		int newsize = offset % BLOCKSIZE + nbytes;

		// if datat to be read goes beyond current block
		if (newsize > BLOCKSIZE){
			newsize = BLOCKSIZE;
		}
		// data to read from current block
		int tocopy = newsize - offset % BLOCKSIZE;

		if (newsize == BLOCKSIZE || newsize <= BLOCKSIZE) {
			// copy datat to be read from current block into buf from alloc_data
			strncat(buf, &alloc_data[offset % BLOCKSIZE], tocopy);

			// update offset
			offset = offset + tocopy;
			// update bytes to be read
			nbytes = nbytes - tocopy;
			// go to next block
			id_block++;
		}		
	}

	free(alloc_data);
	free(inode);
	return 0;
    // return -1;
}


int simplefs_write(int file_handle, char *buf, int nbytes){
    /*
	    write `nbytes` of data from `buf` to file pointed by `file_handle` starting at current offset
	*/


	struct filehandle_t file_desc = file_handle_array[file_handle];
	int offset = file_desc.offset;
	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
    
	// index of block till which it is filled
	int id_block = (file_desc.offset)/BLOCKSIZE;
	// offset of last block if it is partially filled
	int residual = (file_desc.offset)% BLOCKSIZE;

	simplefs_readInode(file_desc.inode_number, inode);
	// how many new data blocks have been allocated, so, that if fails wrote I can deallocate
    int alloc_data[MAX_FILE_SIZE];

	int total_size = 0;   // points to buf size
	int i = 0; 

	if(MAX_FILE_SIZE*BLOCKSIZE < (file_desc.offset + nbytes)){
		free(inode);
        return -1;
	}
    
	while(nbytes >= 0){

		if(residual!=0){

			char new_temp_buffer[BLOCKSIZE];
			char write_buffer[BLOCKSIZE];

			int difference = BLOCKSIZE - residual;
			// store prev data in new_temp_buffer
			simplefs_readDataBlock(inode->direct_blocks[id_block], new_temp_buffer);

			// if data cannot be filled within the same block, fill that block completely
			if (difference < nbytes){
				total_size += difference ;
				nbytes -= difference;
				//copy og data from block 
				memcpy(write_buffer, new_temp_buffer, residual);
				//copy new data from buff
				memcpy(write_buffer + (offset%BLOCKSIZE), buf, difference);

				simplefs_writeDataBlock(inode->direct_blocks[id_block], write_buffer);

				id_block++;
				
			}
			else{
				// if nbytes < diff, then only that block can fit all data to be written
				total_size += nbytes;

				// copy block's og data from new_temp_buffer to write_buffer
				memcpy(write_buffer, new_temp_buffer, residual);
				// new datat to be written is added after the og data
				memcpy(write_buffer + (offset%BLOCKSIZE), buf, nbytes);

				// write all the data to the inode's block number given
				simplefs_writeDataBlock(inode->direct_blocks[id_block],write_buffer);

				nbytes -= nbytes;
				id_block++;
				//update file size
				inode->file_size += total_size;

				simplefs_writeInode(file_desc.inode_number,inode);
				free(inode);
				return 0;
			}
		}

		if (nbytes <= BLOCKSIZE){
			// get new data block to be written on 
			int checker_ = simplefs_allocDataBlock();
			if (checker_ != -1){
				// store only remaining data in new_temp_buf if less than an entire block
				char new_temp_buffer[nbytes];
				alloc_data[i] = checker_;
				
				// read data from buf, total_size gives point frdom where to star reading
				memcpy(new_temp_buffer,buf + total_size,nbytes);
				total_size += nbytes ;

				// map the allocated block to the inode ptr
				inode->direct_blocks[id_block] = checker_;
				// write data
				simplefs_writeDataBlock(inode->direct_blocks[id_block], new_temp_buffer);

				
			    // id_block++;
			    nbytes = 0; // difference;
				// i++;
                inode->file_size += total_size;
				// update to inode 
				simplefs_writeInode(file_desc.inode_number,inode);
				free(inode);
			    return 0;
			}
			else{
				for (int j=0; j<i; j++){
					// if fails deallocate all by iter through loop
					simplefs_freeDataBlock(alloc_data[j]);

					for(int m_it=0;m_it<MAX_FILE_SIZE;m_it++){

						// allocated blocks set to -1
						if(inode->direct_blocks[m_it] == alloc_data[j] ){
							inode->direct_blocks[m_it] = -1;
						}
					}
					
				}
				// write to inode
				simplefs_writeInode(file_desc.inode_number,inode);
				break;
			}
		}

		// allocate a block to fill it completely
		
		int checker_ = simplefs_allocDataBlock();

		if (checker_ != -1){
			// write data size = blocksize
			inode->direct_blocks[id_block] = checker_;
			alloc_data[i] = checker_;

			char new_temp_buffer[BLOCKSIZE];
			memcpy(new_temp_buffer,buf + total_size,BLOCKSIZE);
			total_size += BLOCKSIZE;
			nbytes -= BLOCKSIZE ;
			
			simplefs_writeDataBlock(inode->direct_blocks[id_block],new_temp_buffer);
			
			id_block++;
			i++;
		}
		else{

			// if fails then dealloc all

			for (int j=0;j<=i;j++){

				simplefs_freeDataBlock(alloc_data[j]);

				for(int m_it=0; m_it<MAX_FILE_SIZE; m_it++){

					if(inode->direct_blocks[m_it] == alloc_data[j] ){
						inode->direct_blocks[m_it] = -1;
					}
				}
				
			}
			// write inode
			simplefs_writeInode(file_desc.inode_number,inode);
			break;
		}
    }
	free(inode);
	return -1;
}

// int simplefs_write(int file_handle, char *buf, int nbytes){
//     /*
// 	    write `nbytes` of data from `buf` to file pointed by `file_handle` starting at current offset
// 	*/
// 	struct filehandle_t file_desc = file_handle_array[file_handle];
// 	if (file_desc.offset + nbytes > MAX_FILE_SIZE * BLOCKSIZE) {
// 		return -1;
// 	}


int simplefs_seek(int file_handle, int nseek){
    /*
	   increase `file_handle` offset by `nseek`
	*/
	struct filehandle_t *filehandle = &file_handle_array[file_handle];

    if (nseek < 0){
		if ( filehandle->offset + nseek < 0){
			return -1;
		}
		else{
			filehandle->offset = filehandle->offset + nseek;
			return 0;
		}
	}
	else{
		struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
        simplefs_readInode(filehandle->inode_number, inode);
		if (inode->file_size < filehandle->offset + nseek){
			free(inode);
			return -1;
		}
		else{
			filehandle->offset = filehandle->offset + nseek;
			free(inode);
			return 0;
		}
	}
    return -1;
}