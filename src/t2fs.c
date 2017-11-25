#include <stdio.h>
#include <string.h>
#include "../include/apidisk.h"
#include "../include/t2fs.h"

#define system_init = 0;
t2fs_record *root = NULL;
int *fat = NULL;
t2fs_record *current_directory = NULL;
t2fs_record *current_file = NULL;

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
	strncpy(sb->DataSectorCluster, buffer+28, 4);

	return;

}

void fat_init(){
	int i = 0, y = 0, total;
	int fat[];
	char *buffer;

	int numberOfSectors = (int *) sb->DataSectorStart - (int *) sb->pFATSectorStart;
	//for(i=0;i<numberOfSectors;i++){
	//	read_sector(pFATSectorStart+i, fat+i*256);
	//}

	//total é a quantidade de bytes que tem na fat
	total = numberOfSectors * SECTOR_SIZE;
	for(i=0; i < total; i++){
	   	fat[i] = 0x00000000;
	 }
	 fat[0] = 0x00000001;
	 fat[1] = 0x00000001;
}

void root_init(){
	root->TypeVal = 0x00;
	root->name = "/";
	root->bytesFileSize = 256*SectorsPerCluster;
	root->firstCluster = sb->RootDirCluster;

}

void init_system(){
	superbloco_inst();
	fat_init();
	root_init();
	system_init = 1;
}

int read_cluster (int cluster, unsigned char *buffer)
{
	int i = 0;
	for(i=0;i<sb->SectorsPerCluster;i++){
		read_sector(cluster+i*256, buffer+i*256)
	}

	return 0;
}

int write_cluster (int cluster, unsigned char *buffer)
{
	int i = 0;
	for(i=0;i<sb->SectorsPerCluster;i++){
		write_sector(cluster+i*256, buffer+i*256)
	}

	return 0;
}


//Função que percorre a FAT pra achar um cluster livre, ocupa o valor e retorna o indice
int findsFreeCluster(int type){
	int i=0;
	for(i=0; i<sizeof(fat); i++){
		if(fat[i] == 0){
			fat[i] = type;
			return i;
		}
	}
	return -1;
}
/*-----------------------------------------------------------------------------
Função:	Criar um novo diretório.
	O caminho desse novo diretório é aquele informado pelo parâmetro "pathname".
		O caminho pode ser ser absoluto ou relativo.
	São considerados erros de criação quaisquer situações em que o diretório não possa ser criado.
		Isso inclui a existência de um arquivo ou diretório com o mesmo "pathname".

Entra:	pathname -> caminho do diretório a ser criado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int mkdir2 (char *pathname)
{

	char* cluster_buffer = (char *)malloc(256*sb->SectorsPerCluster);
	t2fs_record* entries;
	int i = 0;
	int clusterOfFatherDirectory;
	int found = 0;

	//se for caminho relativo
	if(pathname[0] == ".")
	{
		if(current_directory == NULL){
			printf("nao ha diretorio aberto\n");
			return -1;
		}
		read_cluster(current_directory->firstCluster, cluster_buffer);
		clusterOfFatherDirectory = current_directory->firstCluster;

	}
	//se for absoluto
	else{
		read_cluster(root->firstCluster,cluster_buffer);
		clusterOfFatherDirectory = root->firstCluster;
	}
	//array to save all direrctory records from the cluster
	//AQUI O ARRAY ENTRIES TEM AS ENTRADAS DE DIRETÓRIO DO ROOT
	entries = (t2fs_record*) cluster_buffer;

	//if(strcmp(pathname, "/") != 0){
		char* word;
		//word is the current directory name being looked for
		for (word = strtok(pathname, "/"); word; word = strtok(NULL, "/"))
		{
			found = 0;
			//searchs for the given name in the directory records' array
			for(i=0, i<SectorsPerCluster*4;i++){
				if(strcmp(word, entries[i]->name) == 0){
					//if found the directory, reads its cluster
					clusterOfFatherDirectory = entries[i]->firstCluster;
					read_cluster(entries[i]->firstCluster, cluster_buffer);
					entries = (t2fs_record*) cluster_buffer;
					found = 1;
				}
			}
			if(found == 0){
				//NAO ACHOU O NOME DA PASTA -> ou ela não existe, ou é a que precisa ser criada
				if(strtok(NULL, "/")==NULL)
					{
						//então acabou a palavra, é o diretório que tu precisa criar
						for(i=0; i<SectorsPerCluster*4;i++){
							//percorre o array de entries pra achar uma entrada livre
							if(entries[i]->TypeVal = 0x00){
							//se o tipo de valor é 0, o arquivo não está sendo usado
							//escreve o cluster atualizado do diretório pai com a nova entrada de diretório
								entries[i]->TypeVal = 0x02;
								strncpy(entries[i]->name, word, 55);
								entries[i]->bytesFileSize = SectorsPerCluster*4;
								int free = findsFreeCluster(2);
								entries[i]->firstCluster = free;
								write_cluster(clusterOfFatherDirectory, (char*) entries);

								//aloca um buffer pra escrever as entradas do novo diretório
								t2fs_record* new_cluster_buffer = (t2fs_record *)malloc(256*sb->SectorsPerCluster);

								//escreve a entrada .
								new_cluster_buffer[0]->TypeVal = 0x02;
								strncpy(new_cluster_buffer[0]->name, ".", 55);
								new_cluster_buffer[0]->bytesFileSize = SectorsPerCluster*4;
								new_cluster_buffer[0]->firstCluster = free;

								//escreve a entrada ..
								new_cluster_buffer[1]->TypeVal = 0x02;
								strncpy(new_cluster_buffer[1]->name, "..", 55);
								new_cluster_buffer[1]->bytesFileSize = SectorsPerCluster*4;
								new_cluster_buffer[1]->firstCluster = clusterOfFatherDirectory;

								//escreve o novo cluster
								write_cluster(free, (char *) new_cluster_buffer);
								free(new_cluster_buffer);
								return 0;
							}
						}
						//não achou entrada livre, retorna erro
						return -1;
					}
				else{
					//o caminho dado não existe, retorna erro
					return -1;
				}
			}
		}

	//}

	/*for(i=0, i<SectorsPerCluster*4;i++){
		if(strcmp(pathname, entries[i]->name)){
			//ACHOU NO ROOT
		}
	}
	//NAO ACHOU NO ROOT*/
}

/*-----------------------------------------------------------------------------
Função:	Apagar um subdiretório do disco.
	O caminho do diretório a ser apagado é aquele informado pelo parâmetro "pathname".
	São considerados erros quaisquer situações que impeçam a operação.
		Isso inclui:
			(a) o diretório a ser removido não está vazio;
			(b) "pathname" não existente;
			(c) algum dos componentes do "pathname" não existe (caminho inválido);
			(d) o "pathname" indicado não é um arquivo;

Entra:	pathname -> caminho do diretório a ser apagado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int rmdir2 (char *pathname)
{

	char* cluster_buffer = (char *)malloc(256*sb->SectorsPerCluster);
	//array to save all direrctory records from the cluster
	t2fs_record* entries;

	int i = 0;
	int clusterOfFatherDirectory = -1;
	int found = 0;
	int vazio = 0;

	//se for caminho relativo
	if(pathname[0] == ".")
	{
		if(current_directory == NULL){
			printf("nao ha diretorio aberto\n");
			return -1;
		}
		read_cluster(current_directory->firstCluster, cluster_buffer);
		entries  = (t2fs_record*) cluster_buffer;
		for(i=0;i<SectorsPerCluster*4;i++){
			if(strncmp(entries[i], "..")==0){
				clusterOfFatherDirectory = entries[i]->firstCluster;
			}
		}
		if(clusterOfFatherDirectory == -1){
			return -1;
		}
	}
	else{
	//se for absoluto
		read_cluster(root->firstCluster,cluster_buffer);
		clusterOfFatherDirectory = root->firstCluster;

	}
	

	if(strcmp(pathname,"/") == 0){
		//NÃO É POSSÍVEL DELETAR O /
		return -1;
	}
	char* word;
	//word is the current directory name being looked for
	for (word = strtok(path, "/"); word; word = strtok(NULL, "/"))
	{
		found = 0;
		//searchs for the given name in the directory records' array
		for(i=0, i<SectorsPerCluster*4;i++){
			if(strcmp(word, entries[i]->name) == 0){
				//if found the directory, reads its cluster
				clusterOfFatherDirectory = entries[i]->firstCluster;
				read_cluster(entries[i]->firstCluster, cluster_buffer);
				entries = (t2fs_record*) cluster_buffer;
				found = 1;
			}
		}
		if(found == 0){
			//NAO ACHOU O NOME DA PASTA RETORNA ERRO
			return -1;
		}
	}
	//ACHOU TODO CAMINHO ATE O DIRETORIO
	for(i=0, i<SectorsPerCluster*4;i++){
		if(!(entries[i]->TypeVal == 0x00)){
			//SE ALGUMA ENTRADA NÃO FOR VAZIA E NAO FOR O . E O .. RETORNA ERRO
			if((strcmp(entries[i]->name, ".")!=0)&&(strcmp(entries[i]->name, "..")!=0))
				return -1
		}
	}
	//ACHOU O DIRETORIO E ESTA VAZIO
	//zera o cluster do diretório
	cluster_buffer = {0};
	write_cluster(entries[i]->firstCluster, cluster_buffer);

	//apaga a entrada do cluster na fat
	fat[entries[i]->firstCluster] = 0;

	return 0;
	
}



/*-----------------------------------------------------------------------------
Função:	Abre um diretório existente no disco.
	O caminho desse diretório é aquele informado pelo parâmetro "pathname".
	Se a operação foi realizada com sucesso, a função:
		(a) deve retornar o identificador (handle) do diretório
		(b) deve posicionar o ponteiro de entradas (current entry) na primeira posição válida do diretório "pathname".
	O handle retornado será usado em chamadas posteriores do sistema de arquivo para fins de manipulação do diretório.

Entra:	pathname -> caminho do diretório a ser aberto

Saída:	Se a operação foi realizada com sucesso, a função retorna o identificador do diretório (handle).
	Em caso de erro, será retornado um valor negativo.
-----------------------------------------------------------------------------*/
DIR2 opendir2 (char *pathname)
{

	char* cluster_buffer = (char *)malloc(256*sb->SectorsPerCluster);
	t2fs_record* entries;
	int i = 0;
	int clusterOfFatherDirectory;
	int found = 0;

	//se for caminho relativo
	if(pathname[0] == ".")
	{
		if(current_directory == NULL){
			printf("nao ha diretorio aberto\n");
			return -1;
		}
		read_cluster(current_directory->firstCluster, cluster_buffer);
		clusterOfFatherDirectory = current_directory->firstCluster;

	}
	//se for absoluto
	else{
		read_cluster(root->firstCluster,cluster_buffer);
		clusterOfFatherDirectory = root->firstCluster;
	}
	//array to save all direrctory records from the cluster
	//AQUI O ARRAY ENTRIES TEM AS ENTRADAS DE DIRETÓRIO DO ROOT
	entries = (t2fs_record*) cluster_buffer;

	//if(strcmp(pathname, "/") != 0){
		char* word;
		//word is the current directory name being looked for
		for (word = strtok(pathname, "/"); word; word = strtok(NULL, "/"))
		{
			found = 0;

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


//IDENTIFY PRONTO!!!
int identify2 (char *name, int size)
{
	char group_id[] = "Guillermo Falcao - 217434\nHermes Tessaro - 218317\nRafael Allegretti - 215020";
  if(size < sizeof(group_id)) return -1;
  else
  {
    strncpy(name, grupo_id, sizeof(group_id));
    return 0;
  }
}