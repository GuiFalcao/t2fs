#include <stdio.h>
#include <string.h>
#include "../include/apidisk.h"
#include "../include/t2fs.h"

void superbloco_inst(struct t2fs_superbloco *sb)
{

	char *buffer;
	buffer = (char *) malloc(SECTOR_SIZE);

	read_sector(0, buffer);

	strncpy(sb->id, buffer, 4);			//writes id from buffer (0-3)
	strncpy(sb->version, buffer+4, 2);	//writes version from buffer (4-6)
	strncpy(sb->SuperBlockSize, buffer+6, 2); //writes size of sb from buffer (6-8)
	strncpy(sb->DiskSize, buffer+8, 4);
	strncpy(sb->NofSectors, buffer+12, 4);
	strncpy(sb->SectorsPerCluster, buffer+16, 4);
	strncpy(sb->pFATSectorStart, buffer+20, 4);
	strncpy(sb->RootDirCluster, buffer+24, 4);
	strncpy(sb->version, buffer+28, 4);

	return;

}

DIR2 opendir2 (char *pathname)
{
	char *buffer;
	struct t2fs_superbloco *sb;

	sb = (struct t2fs_superbloco *) malloc(32);
	superbloco_inst(sb);
	//memory allocation for the cluster
	buffer = (char *) malloc(SECTOR_SIZE*sb->SectorsPerCluster);

	int i=0;
	int firstSector = sb->RootDirCluster *sb->SectorsPerCluster;

	for (i = 0; i < sb->SectorsPerCluster; ++i){
		read_sector(firstSector + i, buffer + i*SECTOR_SIZE);
	}

	//register with info about the directory
	DIRENT2 *rootdir = (DIRENT2 *)malloc(sizeof(DIRENT2));
	strncpy(rootdir->name, buffer, 56);
	strncpy(rootdir->fileType, buffer+56, 4);
	strncpy(rootdir->fileSize, buffer+60, 4);

	printf("%s", buffer);
}



int main()
{
	char *pathname;
	pathname = "blabla";
	opendir2(pathname);
}