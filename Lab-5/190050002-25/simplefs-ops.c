#include "simplefs-ops.h"
extern struct filehandle_t file_handle_array[MAX_OPEN_FILES]; // Array for storing opened files

int simplefs_create(char *filename)
{
	/*
		Create file with name `filename` from disk
	*/
	struct inode_t *inode = malloc(sizeof(struct inode_t));

	for (int i = 0; i < NUM_INODES; i++)
	{
		simplefs_readInode(i, inode);
		if (inode->status == INODE_IN_USE && strcmp(inode->name, filename) == 0)
		{
			free(inode);
			return -1;
		}
	}

	int inode_number = simplefs_allocInode();
	if (inode_number == -1)
	{
		free(inode);
		return -1;
	}

	simplefs_readInode(inode_number, inode);

	inode->status = INODE_IN_USE;
	strcpy(inode->name, filename);
	inode->file_size = 0;
	memset(inode->direct_blocks, -1, sizeof(inode->direct_blocks));

	simplefs_writeInode(inode_number, inode);
	return inode_number;

	free(inode);
	return -1;
}

void simplefs_delete(char *filename)
{
	/*
		delete file with name `filename` from disk
	*/
	struct inode_t *inode = malloc(sizeof(struct inode_t));

	for (int i = 0; i < NUM_INODES; i++)
	{
		simplefs_readInode(i, inode);
		if (inode->status == INODE_IN_USE && strcmp(inode->name, filename) == 0)
		{
			for (int j = 0; j < NUM_INODE_BLOCKS; j++)
				if (inode->direct_blocks[j] != -1)
					simplefs_freeDataBlock(inode->direct_blocks[j]);
			memset(inode->name, 0, MAX_NAME_STRLEN);
			simplefs_freeInode(i);
			return;
		}
	}

	free(inode);
}

int simplefs_open(char *filename)
{
	/*
		open file with name `filename`
	*/
	struct inode_t *inode = malloc(sizeof(struct inode_t));
	int node_found = -1;

	for (int i = 0; i < NUM_INODES; i++)
	{
		simplefs_readInode(i, inode);
		if (strcmp(inode->name, filename) == 0)
		{
			node_found = i;
			break;
		}
	}

	if (node_found == -1)
		return -1;

	for (int i = 0; i < MAX_OPEN_FILES; i++)
		if (file_handle_array[i].inode_number == -1)
		{
			file_handle_array[i].inode_number = node_found;
			file_handle_array[i].offset = 0;
			return i;
		}

	free(inode);
	return -1;
}

void simplefs_close(int file_handle)
{
	/*
		close file pointed by `file_handle`
	*/
	if (file_handle < MAX_OPEN_FILES)
	{
		file_handle_array[file_handle].inode_number = -1;
		file_handle_array[file_handle].offset = 0;
	}
}

int simplefs_read(int file_handle, char *buf, int nbytes)
{
	/*
		read `nbytes` of data into `buf` from file pointed by `file_handle` starting at current offset
	*/
	struct inode_t *inode = malloc(sizeof(struct inode_t));
	simplefs_readInode(file_handle_array[file_handle].inode_number, inode);

	int offset = file_handle_array[file_handle].offset;
	if (offset + nbytes > inode->file_size)
		return -1;

	int block_idx = offset / BLOCKSIZE;
	memset(buf, 0, nbytes);
	char *block_content = malloc(BLOCKSIZE);

	while (nbytes > 0)
	{
		simplefs_readDataBlock(inode->direct_blocks[block_idx], block_content);
		int s = offset % BLOCKSIZE;
		int e = s + nbytes;
		if (e > BLOCKSIZE)
			e = BLOCKSIZE;
		strncat(buf, &block_content[s], e - s);
		nbytes -= e - s;
		offset += e - s;
		block_idx++;
	}

	free(block_content);
	free(inode);
	return 0;
}

int simplefs_write(int file_handle, char *buf, int nbytes)
{
	/*
		write `nbytes` of data from `buf` to file pointed by `file_handle` starting at current offset
	*/
	struct inode_t *inode = malloc(sizeof(struct inode_t));
	int inode_number = file_handle_array[file_handle].inode_number;
	simplefs_readInode(inode_number, inode);

	int offset = file_handle_array[file_handle].offset;
	if (offset + nbytes > MAX_FILE_SIZE * BLOCKSIZE)
		return -1;

	int block_idx = offset / BLOCKSIZE;
	int curr_block = inode->file_size / BLOCKSIZE - inode->file_size % BLOCKSIZE;

	int nbytes_ = nbytes - (BLOCKSIZE - offset % BLOCKSIZE);
	char *tmp = malloc(BLOCKSIZE);
	for (int i = block_idx; i < NUM_INODE_BLOCKS; i++)
	{
		if (inode->direct_blocks[i] == -1)
		{
			if ((inode->direct_blocks[i] = simplefs_allocDataBlock()) == -1)
			{
				nbytes_ = 1;
				break;
			}
			memset(tmp, 0, BLOCKSIZE);
			simplefs_writeDataBlock(inode->direct_blocks[i], tmp);
		}
		if (nbytes_ <= 0)
			break;
		nbytes_ -= BLOCKSIZE;
	}
	free(tmp);

	if (nbytes_ > 0)
	{
		for (int i = curr_block + 1; i < NUM_INODE_BLOCKS; i++)
			if (inode->direct_blocks[i] != -1)
			{
				simplefs_freeDataBlock(inode->direct_blocks[i]);
				inode->direct_blocks[i] = -1;
			}
		return -1;
	}

	char *block_content = malloc(BLOCKSIZE);
	int buf_index = 0;

	while (nbytes > 0)
	{
		simplefs_readDataBlock(inode->direct_blocks[block_idx], block_content);
		int s = offset % BLOCKSIZE;
		int e = s + nbytes;
		if (e > BLOCKSIZE)
			e = BLOCKSIZE;
		strncpy(&block_content[s], &buf[buf_index], e - s);
		simplefs_writeDataBlock(inode->direct_blocks[block_idx], block_content);
		nbytes -= e - s;
		offset += e - s;
		buf_index += e - s;
		inode->file_size += e - s;
		block_idx++;
	}

	simplefs_writeInode(inode_number, inode);

	free(block_content);
	free(inode);
	return 0;
}

int simplefs_seek(int file_handle, int nseek)
{
	/*
	   increase `file_handle` offset by `nseek`
	*/
	// TODO
	struct filehandle_t *filehandle = &file_handle_array[file_handle];
	struct inode_t *inode = malloc(sizeof(struct inode_t));

	simplefs_readInode(filehandle->inode_number, inode);

	if (filehandle->offset + nseek <= inode->file_size)
	{
		filehandle->offset += nseek;
		return 0;
	}

	return -1;
}