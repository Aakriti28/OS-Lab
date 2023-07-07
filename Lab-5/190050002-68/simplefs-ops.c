#include "simplefs-ops.h"
extern struct filehandle_t file_handle_array[MAX_OPEN_FILES]; // Array for storing opened files

int simplefs_create(char *filename){
    /*
	    Create file with name `filename` from disk
	*/
    for(int i=0;i<NUM_INODES;i++){
    struct inode_t *inodeptr = (struct inode_t *)malloc(sizeof(struct inode_t));
	simplefs_readInode(i, inodeptr);
	if(inodeptr->status == INODE_IN_USE){
		if (inodeptr->name == filename){
			free(inodeptr);
			return -1;
		}
	}
	free(inodeptr);
	}
	struct inode_t *inodeptr = (struct inode_t *)malloc(sizeof(struct inode_t));
    int res = simplefs_allocInode();
	if (res != -1){
	inodeptr->status = INODE_IN_USE;
	memcpy(inodeptr->name,filename, sizeof(filename)-1);
	inodeptr->file_size = 0;
	for(int i=0; i<MAX_FILE_SIZE; i++)
        inodeptr->direct_blocks[i] = -1;
	simplefs_writeInode(res,inodeptr);}
    return res;
}


void simplefs_delete(char *filename){
    /*
	    delete file with name `filename` from disk
	*/
	for(int i=0;i<NUM_INODES;i++){
    struct inode_t *inodeptr = (struct inode_t *)malloc(sizeof(struct inode_t));
	simplefs_readInode(i, inodeptr);
	if(inodeptr->status == INODE_IN_USE){
		if (strcmp(inodeptr->name,filename)==0){
            for (int j = 0; j < MAX_FILE_SIZE; j++){
                   if (inodeptr->direct_blocks[j] != -1){
					   simplefs_freeDataBlock(inodeptr->direct_blocks[j]);
				   }
			}
            simplefs_freeInode(i);
			free(inodeptr);
			return;
		}
	}
	free(inodeptr);
	}
	return;
}

int simplefs_open(char *filename){
    /*
	    open file with name `filename`
	*/
	for(int i=0; i< NUM_INODES; i++){
    struct inode_t *inodeptr = (struct inode_t *)malloc(sizeof(struct inode_t));
	simplefs_readInode(i, inodeptr);
	if(inodeptr->status == INODE_IN_USE){
		if (strcmp(inodeptr->name,filename)==0){
            for(int j=0;j<MAX_OPEN_FILES;j++){
				if(file_handle_array[j].inode_number == -1){
					file_handle_array[j].inode_number = i;
					file_handle_array[j].offset = 0;
					free(inodeptr);
					return j;
				}
			}
		}
	}
	free(inodeptr);
	}
    return -1;
}

void simplefs_close(int file_handle){
    /*
	    close file pointed by `file_handle`
	*/
	file_handle_array[file_handle].inode_number = -1;
	file_handle_array[file_handle].offset = -1;
    return;
}

int simplefs_read(int file_handle, char *buf, int nbytes){
    /*
	    read `nbytes` of data into `buf` from file pointed by `file_handle` starting at current offset
	*/
	int fd = file_handle_array[file_handle].inode_number;
	struct inode_t *inodeptr = (struct inode_t *)malloc(sizeof(struct inode_t));
    simplefs_readInode(fd, inodeptr);
	int num_block = (file_handle_array[file_handle].offset)/BLOCKSIZE;
	int isres = (file_handle_array[file_handle].offset)% BLOCKSIZE;
	int currpoint = 0;   // points to buf size
	int bytesg = nbytes;
	int point_to_buf;
	char newbuf[sizeof(buf)];

	if(inodeptr->file_size < (file_handle_array[file_handle].offset + nbytes)){
        return -1;
	}
	if(isres!=0){
		if(inodeptr->direct_blocks[num_block] != -1){
		
		if ((BLOCKSIZE - isres) < nbytes){
		char tempbuffer[BLOCKSIZE -isres];
		simplefs_readDataBlock(inodeptr->direct_blocks[num_block],tempbuffer);
		memcpy(buf, tempbuffer + isres, (BLOCKSIZE- isres));
		currpoint = BLOCKSIZE- isres;
		num_block++;
		bytesg = bytesg - currpoint;
		}
		else{
			char tempbuffer[nbytes];
		  simplefs_readDataBlock(inodeptr->direct_blocks[num_block],tempbuffer);
          memcpy(buf, tempbuffer + isres, nbytes);
		  currpoint = nbytes;
		  num_block++;
		  bytesg = bytesg - currpoint;
		}
		}
	}

	while(bytesg >= 0){
		if (bytesg <= BLOCKSIZE){
			char tempbuffer[bytesg];
		    simplefs_readDataBlock(inodeptr->direct_blocks[num_block],tempbuffer);
			memcpy(buf + currpoint, tempbuffer,bytesg);
			currpoint = currpoint + bytesg;
            num_block++;
			bytesg = 0;
			return 0;
		}
		char tempbuffer[BLOCKSIZE];	
		if(inodeptr->direct_blocks[num_block] != -1){
		simplefs_readDataBlock(inodeptr->direct_blocks[num_block],tempbuffer);
		memcpy(buf + currpoint, tempbuffer,BLOCKSIZE);
		currpoint = currpoint + BLOCKSIZE;
		num_block++;
		bytesg = bytesg - BLOCKSIZE;}
		else{
            memcpy(buf,newbuf,sizeof(newbuf));
			return -1;
		}
	}
	
    return -1;
}


int simplefs_write(int file_handle, char *buf, int nbytes){
    /*
	    write `nbytes` of data from `buf` to file pointed by `file_handle` starting at current offset
	*/
	int fd = file_handle_array[file_handle].inode_number;
	int offsets = file_handle_array[file_handle].offset;
	struct inode_t *inodeptr = (struct inode_t *)malloc(sizeof(struct inode_t));
    simplefs_readInode(fd, inodeptr);
	int num_block = (file_handle_array[file_handle].offset)/BLOCKSIZE;
	int isres = (file_handle_array[file_handle].offset)% BLOCKSIZE;
    int allocated[MAX_FILE_SIZE];

	int currpoint = 0;   // points to buf size
	int bytesg = nbytes;
	int i = 0; 
	int check = 0;

	if(MAX_FILE_SIZE*BLOCKSIZE < (file_handle_array[file_handle].offset + nbytes)){
        return -1;
	}
    
	if(isres!=0){
		char tempbuffer[BLOCKSIZE];
		char newtempbuffer[BLOCKSIZE];
		simplefs_readDataBlock(inodeptr->direct_blocks[num_block],tempbuffer);
		if ((BLOCKSIZE - isres) < nbytes){
             memcpy(newtempbuffer,tempbuffer,isres);
			 memcpy(newtempbuffer + (offsets%BLOCKSIZE),buf,BLOCKSIZE - isres);
			 simplefs_writeDataBlock(inodeptr->direct_blocks[num_block],newtempbuffer);
			 currpoint += BLOCKSIZE - isres ;
			 num_block++;
			 bytesg -= BLOCKSIZE - isres;
		}
		else{
			memcpy(newtempbuffer,tempbuffer,isres);
			memcpy(newtempbuffer + (offsets%BLOCKSIZE),buf,nbytes);
			simplefs_writeDataBlock(inodeptr->direct_blocks[num_block],newtempbuffer);
			currpoint += nbytes;
			num_block++;
			bytesg -= nbytes;
			inodeptr->file_size += currpoint;
			simplefs_writeInode(fd,inodeptr);
			return 0;
		}
	}


	while(bytesg >= 0){
		if (bytesg <= BLOCKSIZE){
			int indexes = simplefs_allocDataBlock();
			if (indexes != -1){
				char tempbuffer[bytesg];
				memcpy(tempbuffer,buf + currpoint,bytesg);
				inodeptr->direct_blocks[num_block] = indexes;
				simplefs_writeDataBlock(inodeptr->direct_blocks[num_block],tempbuffer);
				allocated[i] = indexes;
				currpoint += bytesg ;
			    num_block++;
			    bytesg -= BLOCKSIZE - isres;
				i++;
                inodeptr->file_size += currpoint;
				simplefs_writeInode(fd,inodeptr);
			    return 0;
			}
			else{
               check = 1;
			   break;
			}
		}
		
		int indexes = simplefs_allocDataBlock();
		if (indexes != -1){
             char tempbuffer[BLOCKSIZE];
			 memcpy(tempbuffer,buf + currpoint,BLOCKSIZE);
			 inodeptr->direct_blocks[num_block] = indexes;
			 simplefs_writeDataBlock(inodeptr->direct_blocks[num_block],tempbuffer);
			 allocated[i] = indexes;
			 currpoint += BLOCKSIZE;
			 num_block++;
			 bytesg -= BLOCKSIZE ;
             i++;
		}
		else{

			check = 1;
			break;
		}

	}

	if (check == 1){
		for (int j=0;j<i;j++){
			simplefs_freeDataBlock(allocated[j]);
			for(int k=0;k<MAX_FILE_SIZE;k++){
				if(inodeptr->direct_blocks[k] == allocated[j] ){
					inodeptr->direct_blocks[k] = -1;
				}
			}
			
		}
		simplefs_writeInode(fd,inodeptr);
	}
	return -1;
}


int simplefs_seek(int file_handle, int nseek){
    /*
	   increase `file_handle` offset by `nseek`
	*/

	if (nseek < 0){
		if ( file_handle_array[file_handle].offset + nseek < 0){
			return -1;
		}
		else{
			file_handle_array[file_handle].offset = file_handle_array[file_handle].offset + nseek;
			return 0;
		}
	}
	else{
		struct inode_t *inodeptr = (struct inode_t *)malloc(sizeof(struct inode_t));
        simplefs_readInode(file_handle_array[file_handle].inode_number, inodeptr);
		if (inodeptr->file_size < file_handle_array[file_handle].offset + nseek){
			return -1;
		}
		else{
			file_handle_array[file_handle].offset = file_handle_array[file_handle].offset + nseek;
			return 0;
		}
	}
    return -1;
}