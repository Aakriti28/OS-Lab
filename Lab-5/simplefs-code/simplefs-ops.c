#include "simplefs-ops.h"
extern struct filehandle_t file_handle_array[MAX_OPEN_FILES]; // Array for storing opened files

int simplefs_create(char *filename){
    /*
	    Create file with name `filename` from disk
	*/
	
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
	int inode_number = simplefs_allocInode();
	if (inode_number == -1)
	{
		free(inode);
		return -1;
	}

	// simplefs_readInode(inode_number, inode);

	// if (condition != -1) {
	inode->status = INODE_IN_USE;
	strcpy(inode->name, filename);
	inode->file_size = 0;
	// memset(inode->direct_blocks, -1, sizeof(inode->direct_blocks));
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
	for (int i = 0; i < NUM_INODES; i++)
	{
		simplefs_readInode(i, inode);
		if (inode->status == INODE_IN_USE)
		{
			if (strcmp(inode->name, filename) == 0) {
				for (int j = 0; j < NUM_INODE_BLOCKS; j++){
					if (inode->direct_blocks[j] != -1){
						simplefs_freeDataBlock(inode->direct_blocks[j]);
					}
				}			
				memset(inode->name, 0, MAX_NAME_STRLEN);
				simplefs_freeInode(i);
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
	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	int is_node = -1;

	for (int i = 0; i < NUM_INODES; i++)
	{
		simplefs_readInode(i, inode);
		if (inode->status == INODE_IN_USE && strcmp(inode->name, filename) == 0)
		{
			is_node = i;
			// break;
			for (int i = 0; i < MAX_OPEN_FILES; i++) {
				if (file_handle_array[i].inode_number == -1) {
					file_handle_array[i].inode_number = is_node;
					file_handle_array[i].offset = 0;
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

	if(inode->file_size < (offset + nbytes)){
		free(inode);
        return -1;
	}
	
	memset(buf, 0, nbytes);
	char *alloc_data = malloc(BLOCKSIZE);

	while (nbytes > 0)
	{
		simplefs_readDataBlock(inode->direct_blocks[id_block], alloc_data);
		int newsize = offset % BLOCKSIZE + nbytes;

		if (newsize > BLOCKSIZE){
			newsize = BLOCKSIZE;
		}
		int tocopy = newsize - offset % BLOCKSIZE;

		if (newsize == BLOCKSIZE || newsize <= BLOCKSIZE) {
			strncat(buf, &alloc_data[offset % BLOCKSIZE], tocopy);

			offset = offset + tocopy;
			nbytes = nbytes - tocopy;
			// offset += tocopy;
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
    
	int id_block = (file_desc.offset)/BLOCKSIZE;
	int residual = (file_desc.offset)% BLOCKSIZE;

	simplefs_readInode(file_desc.inode_number, inode);
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
			simplefs_readDataBlock(inode->direct_blocks[id_block], new_temp_buffer);

			if (difference < nbytes){
				total_size += difference ;
				nbytes -= difference;

				memcpy(write_buffer, new_temp_buffer, residual);
				memcpy(write_buffer + (offset%BLOCKSIZE), buf, difference);

				simplefs_writeDataBlock(inode->direct_blocks[id_block], write_buffer);

				id_block++;
				
			}
			else{
				total_size += nbytes;

				memcpy(write_buffer,new_temp_buffer,residual);
				memcpy(write_buffer + (offset%BLOCKSIZE),buf,nbytes);

				simplefs_writeDataBlock(inode->direct_blocks[id_block],write_buffer);

				nbytes -= nbytes;
				id_block++;
				
				inode->file_size += total_size;

				simplefs_writeInode(file_desc.inode_number,inode);
				free(inode);
				return 0;
			}
		}

		if (nbytes <= BLOCKSIZE){
			int checker_ = simplefs_allocDataBlock();
			if (checker_ != -1){

				char new_temp_buffer[nbytes];
				alloc_data[i] = checker_;
				

				memcpy(new_temp_buffer,buf + total_size,nbytes);
				total_size += nbytes ;

				inode->direct_blocks[id_block] = checker_;
				simplefs_writeDataBlock(inode->direct_blocks[id_block], new_temp_buffer);

				
			    // id_block++;
			    nbytes = 0; // difference;
				// i++;
                inode->file_size += total_size;
				simplefs_writeInode(file_desc.inode_number,inode);
				free(inode);
			    return 0;
			}
			else{
				for (int j=0; j<i; j++){

					simplefs_freeDataBlock(alloc_data[j]);

					for(int m_it=0;m_it<MAX_FILE_SIZE;m_it++){

						if(inode->direct_blocks[m_it] == alloc_data[j] ){
							inode->direct_blocks[m_it] = -1;
						}
					}
					
				}
				simplefs_writeInode(file_desc.inode_number,inode);
				break;
			}
		}
		
		int checker_ = simplefs_allocDataBlock();

		if (checker_ != -1){
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

			for (int j=0;j<=i;j++){

				simplefs_freeDataBlock(alloc_data[j]);

				for(int m_it=0; m_it<MAX_FILE_SIZE; m_it++){

					if(inode->direct_blocks[m_it] == alloc_data[j] ){
						inode->direct_blocks[m_it] = -1;
					}
				}
				
			}
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