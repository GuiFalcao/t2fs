#include <stdio.h>
#include <string.h>
#include "../include/apidisk.h"
#include "../include/t2fs.h"

#define system_init = 0;

typedef struct file_struct {
    char    name[MAX_FILE_NAME_SIZE]; 	/* Nome do arquivo. : string com caracteres ASCII (0x21 ate 0x7A), case sensitive.*/
    DWORD   firstCluster;		/* Numero do primeiro cluster de dados correspondente a essa entrada de diretorio */
    int offset; /*pra checar aonde esta dentro do diretorio*/
};

struct t2fs_record *root = NULL;
int *fat = NULL;
struct t2fs_record *current_directory = NULL;
char *current_directory_path;
int clusterOfFatherDirectory;
struct t2fs_record *current_file = NULL;
struct file_struct open_directories[10] = {0};
struct file_struct open_files[10] = {0};
struct t2fs_superbloco *sb;

int DIRENT_PER_SECTOR;
int debug = 1;
int system = 0;


/*FUNCOES AUXILIARES*/



void superbloco_inst()
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
	strncpy(sb->DataSectorStart, buffer+28, 4);
	DIRENT_PER_SECTOR = sb->SectorsPerCluster*4;

	return;

}

void fat_init(){
	int i = 0, y = 0, total;

	int numberOfSectors = (int *) sb->DataSectorStart - (int *) sb->pFATSectorStart;
	//for(i=0;i<numberOfSectors;i++){
	//	read_sector(pFATSectorStart+i, fat+i*256);
	//}

	//total e a quantidade de bytes que tem na fat
	total = numberOfSectors * SECTOR_SIZE;
	for(i=0; i < total; i++){
	   	fat[i] = 0x00000000;
	 }
	 fat[0] = 0x00000001;
	 fat[1] = 0x00000001;
}

void root_init(){
	root->TypeVal = 0x00;
	strncpy(root->name,"/",55);
	root->bytesFileSize = 256*sb->SectorsPerCluster;
	root->firstCluster = sb->RootDirCluster;

}

void init_system(){
	superbloco_inst();
	fat_init();
	root_init();
	system = 1;
}

int read_cluster (int cluster, unsigned char *buffer)
{
	int i = 0;
	for(i=0;i<sb->SectorsPerCluster;i++){
		read_sector(cluster+i*256, buffer+i*256);
	}

	return 0;
}

int write_cluster (int cluster, unsigned char *buffer)
{
	int i = 0;
	for(i=0;i<sb->SectorsPerCluster;i++){
		write_sector(cluster+i*256, buffer+i*256);
	}

	return 0;
}
//pei

//Funcao que percorre a FAT pra achar um cluster livre, ocupa o valor e retorna o indice
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


/*funcao que procura o diretorio buscado pelo pathname, podendo este ser absoluto ou relativo.
  Recebe um pathname a ser procurado, e quatro variaveis auxiliares para serem colocados os registros.
  Ao fim, estarao as entradas de diretorio do dir procurado em entries
  O cluster do diretorio pai ao do procurado em temp_cluster_father_dir
  A entrada de diretorio relativa ao dir procurado em temp_current_dir
*/
void findPath(char* pathname, int temp_cluster_father_dir, struct t2fs_record* temp_current_dir, struct t2fs_record* entries)
{
	int found = 0;
	int i = 0;
	char* cluster_buffer = (char *)malloc(256*sb->SectorsPerCluster);

	//se for caminho relativo
	if(pathname[0] != "/")
	{
		if(pathname[0] == "."){
			if(pathname[1] == "."){
				//procura no diretorio pai
				temp_current_dir->firstCluster = clusterOfFatherDirectory;
			}
			//e no "."
			if(current_directory == NULL){
				printf("nao ha diretorio aberto\n");
				return -1;
			}
			temp_current_dir->firstCluster = current_directory->firstCluster;
		}

		read_cluster(temp_current_dir->firstCluster, cluster_buffer);
	}
	//se for absoluto
	else{
		read_cluster(root->firstCluster,cluster_buffer);
	}

	//Entries tem agora as entradas de diretorio do primeiro diretorio referenciado, /, . ou ..
	entries  = (struct t2fs_record*) cluster_buffer;

	//procura nas entradas o diretorio pai, seta o cluster do diretorio pai pra ele
	for(i=0;i<DIRENT_PER_SECTOR;i++){
		if(strcmp(entries[i].name, "..")==0){
			temp_cluster_father_dir = entries[i].firstCluster;
		}
	}

	/*TEMOS AGORA:
	1) Array entries tem as entradas de diretorio do primeiro diretorio procurado
	2) temp_cluster_father_dir tem o cluster do diretorio pai
	3) temp_current_dir tem a entrada de diretorio do primeiro diretorio achado, /, . ou ..*/

	char* word;
	//word is the current directory name being looked for
	for (word = strtok(pathname, "/"); word; word = strtok(NULL, "/"))
	{
		found = 0;
		//searchs for the given name in the directory records' array
		for(i=0; i<DIRENT_PER_SECTOR;i++){
			//se achar o nome do diretorio com o nome da pasta
			if(strcmp(word, entries[i].name) == 0){
				if(entries[i].TypeVal != 2){
					if(debug = 1){
						printf("nao e um diretorio\n");
					}
					return -1;
				}
				//if found the directory, reads its cluster
				//associa o cluster do pai a este cluster, ja que vai trocar de diretorio
				int j = 0;
				for(j=0;j<DIRENT_PER_SECTOR;j++){
					if(strcmp(entries[i].name, ".")==0){
						temp_cluster_father_dir = entries[j].firstCluster;
					}
				}
				//le o cluster; agora entries tem as informacoes do novo cluster
				read_cluster(entries[i].firstCluster, cluster_buffer);
				entries = (struct t2fs_record*) cluster_buffer;
				found = 1;
			}
		}
		if(found == 0){
			if(debug == 1){
				printf("nao achou o nome da pasta\n");
			}
			//NAO ACHOU O NOME DA PASTA RETORNA ERRO
			return -1;
		}
	}

	/*entries tem o conteudo do diretorio procurado. temp_current_dir tem a entrada de diretorio dele,
	temp_current_dir_path tem o caminho dele, temp_cluster_father_dir tem o cluster do diretorio pai*/


}



/*FUNCOES DE DIRETORIO:*/




/*-----------------------------------------------------------------------------
Funcao:	Criar um novo diretorio.
	O caminho desse novo diretorio eh aquele informado pelo parametro "pathname".
		O caminho pode ser ser absoluto ou relativo.
	Sao considerados erros de criacao quaisquer situacoes em que o diretorio nao possa ser criado.
		Isso inclui a existencia de um arquivo ou diretorio com o mesmo "pathname".

Entra:	pathname -> caminho do diretorio a ser criado

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna "0" (zero).
	Em caso de erro, sera retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int mkdir2 (char *pathname)
{

	if(system == 0){
		init_system();
	}

	char* cluster_buffer = (char *)malloc(256*sb->SectorsPerCluster);
	struct t2fs_record* entries;
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
	//AQUI O ARRAY ENTRIES TEM AS ENTRADAS DE DIREToRIO DO ROOT
	entries = (struct t2fs_record*) cluster_buffer;

	char* word;
	//word is the current directory name being looked for
	for (word = strtok(pathname, "/"); word; word = strtok(NULL, "/"))
	{
		found = 0;
		//searchs for the given name in the directory records' array
		for(i=0; i<DIRENT_PER_SECTOR;i++){
			if(strcmp(word, entries[i].name) == 0){
				if(entries[i].TypeVal != 2){
					if(debug = 1){
						printf("nao eh um diretorio\n");
					}
					return -1;
				}
				//if found the directory, reads its cluster
				clusterOfFatherDirectory = entries[i].firstCluster;
				read_cluster(entries[i].firstCluster, cluster_buffer);
				entries = (struct t2fs_record*) cluster_buffer;
				found = 1;
			}
		}
		if(found == 0){
			//NAO ACHOU O NOME DA PASTA -> ou ela nao existe, ou eh a que precisa ser criada
			if(strtok(NULL, "/")==NULL)
				{
					//entao acabou a palavra, eh o diretorio que tu precisa criar
					for(i=0; i<DIRENT_PER_SECTOR;i++){
						//percorre o array de entries pra achar uma entrada livre
						if(entries[i].TypeVal = 0x00){
						//se o tipo de valor e 0, o arquivo nao esta sendo usado
						//escreve o cluster atualizado do diretorio pai com a nova entrada de diretorio
							entries[i].TypeVal = 0x02;
							strncpy(entries[i].name, word, 55);
							entries[i].bytesFileSize = DIRENT_PER_SECTOR;
							int free_c = findsFreeCluster(2);
							entries[i].firstCluster = free_c;
							write_cluster(clusterOfFatherDirectory, (char*) entries);

							//aloca um buffer pra escrever as entradas do novo diretorio
							struct t2fs_record* new_cluster_buffer = (struct t2fs_record *)malloc(256*sb->SectorsPerCluster);

							//escreve a entrada .
							new_cluster_buffer[0].TypeVal = 0x02;
							strncpy(new_cluster_buffer[0].name, ".", 55);
							new_cluster_buffer[0].bytesFileSize = DIRENT_PER_SECTOR;
							new_cluster_buffer[0].firstCluster = free_c;

							//escreve a entrada ..
							new_cluster_buffer[1].TypeVal = 0x02;
							strncpy(new_cluster_buffer[1].name, "..", 55);
							new_cluster_buffer[1].bytesFileSize = DIRENT_PER_SECTOR;
							new_cluster_buffer[1].firstCluster = clusterOfFatherDirectory;

							//escreve o novo cluster
							write_cluster(free_c, (char *) new_cluster_buffer);
							free(new_cluster_buffer);
							return 0;
						}
					}
					//nao achou entrada livre, retorna erro
					return -1;
				}
			else{
				//o caminho dado nao existe, retorna erro
				return -1;
			}
		}
	}
}

/*-----------------------------------------------------------------------------
Funcao:	Apagar um subdiretorio do disco.
	O caminho do diretorio a ser apagado eh aquele informado pelo parametro "pathname".
	Sao considerados erros quaisquer situacoes que impecam a operacao.
		Isso inclui:
			(a) o diretorio a ser removido nao esta vazio;
			(b) "pathname" nao existente;
			(c) algum dos componentes do "pathname" nao existe (caminho invalido);
			(d) o "pathname" indicado nao eh um arquivo;

Entra:	pathname -> caminho do diretorio a ser apagado

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna "0" (zero).
	Em caso de erro, sera retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int rmdir2 (char *pathname)
{
	if(system == 0){
		init_system();
	}

	char* cluster_buffer = (char *)malloc(256*sb->SectorsPerCluster);
	//array to save all direrctory records from the cluster
	struct t2fs_record* entries;

	int i = 0;
	int temp_cluster_father_dir = -1;
	int found = 0;
	int vazio = 0;

	struct t2fs_record* temp_current_dir;


	if(strcmp(pathname,"/") == 0){
		//NAO EH POSSiVEL DELETAR O /
		return -1;
	}

	findPath(pathname, temp_cluster_father_dir, temp_current_dir, entries);

	//ACHOU TODO CAMINHO ATE O DIRETORIO
	for(i=0; i<DIRENT_PER_SECTOR;i++){
		if(!(entries[i].TypeVal == 0x00)){
			//SE ALGUMA ENTRADA NAO FOR VAZIA E NAO FOR O . E O .. RETORNA ERRO
			if((strcmp(entries[i].name, ".")!=0)&&(strcmp(entries[i].name, "..")!=0)){
				return -1;
			}
		}
	}
	//ACHOU O DIRETORIO E ESTA VAZIO
	//zera o cluster do diretorio
	cluster_buffer = NULL;
	write_cluster(entries[i].firstCluster, cluster_buffer);

	//apaga a entrada do cluster na fat
	fat[entries[i].firstCluster] = 0;

	free(cluster_buffer);

	return 0;

}


/*-----------------------------------------------------------------------------
Funcao:	Altera o current path
	O novo caminho do diretorio a ser usado como current path eh aquele informado pelo parametro "pathname".
	Sao considerados erros quaisquer situacoes que impecam a operacao.
		Isso inclui:
			(a) "pathname" nao existente;
			(b) algum dos componentes do "pathname" nao existe (caminho invalido);
			(c) o "pathname" indicado nao e um diretorio;

Entra:	pathname -> caminho do diretorio a ser criado

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna "0" (zero).
	Em caso de erro, sera retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int chdir2 (char *pathname){
	if(system == 0){
		init_system();
	}
	char* cluster_buffer = (char *)malloc(256*sb->SectorsPerCluster);
	//array to save all directory records from the cluster
	struct t2fs_record* entries;
	int temp_cluster_father_dir;
	struct t2fs_record* temp_current_dir;

	int i = 0;
	int found = 0;

	findPath(pathname, temp_cluster_father_dir, temp_current_dir, entries);


	//ACHOU TODO CAMINHO ATE O DIRETORIO
	strcpy(current_directory_path, pathname);
	//current_directory->TypeVal = 0x02;
	//current_directory->bytesFileSize = 256*sb->SectorsPerCluster;
	//strncpy(current_directory->name, ".");
	//current_directory->firstCluster = entries[0]->firstCluster;
	current_directory = temp_current_dir;
	clusterOfFatherDirectory = temp_cluster_father_dir;

	return 0;
}


/*-----------------------------------------------------------------------------
Funcao:	Funcao que informa o diretorio atual de trabalho.
	O T2FS deve copiar o pathname do diretorio de trabalho, incluindo o ‘\0’ do final do string, para o buffer indicado por "pathname".
	Essa copia nao pode exceder o tamanho do buffer, informado pelo parametro "size".

Entra:	pathname -> ponteiro para buffer onde copiar o pathname
	size -> tamanho do buffer "pathname" (numero maximo de bytes a serem copiados).

Saida:
	Se a operacao foi realizada com sucesso, a funcao retorna "0" (zero).
	Caso ocorra algum erro, a funcao retorna um valor diferente de zero.
	Sao considerados erros quaisquer situacoes que impecam a copia do pathname do diretorio de trabalho para "pathname",
	incluindo espaco insuficiente, conforme informado por "size".
-----------------------------------------------------------------------------*/
int getcwd2 (char *pathname, int size){
	if(system == 0){
		init_system();
	}
	if(current_directory == NULL){
		if(debug = 1){
			printf("nao esta em diretorio nenhum\n");
		}
		return -1;
	}

	if(strlen(current_directory_path) > size){
		if(debug = 1){
			printf("tamanho insuficiente\n");
		}
		return -1;
	}

	strncpy(pathname, current_directory_path, size);

	return 0;

}



/*-----------------------------------------------------------------------------
Funcao:	Abre um diretorio existente no disco.
	O caminho desse diretorio e aquele informado pelo parametro "pathname".
	Se a operacao foi realizada com sucesso, a funcao:
		(a) deve retornar o identificador (handle) do diretorio
		(b) deve posicionar o ponteiro de entradas (current entry) na primeira posicao valida do diretorio "pathname".
	O handle retornado sera usado em chamadas posteriores do sistema de arquivo para fins de manipulacao do diretorio.

Entra:	pathname -> caminho do diretorio a ser aberto

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna o identificador do diretorio (handle).
	Em caso de erro, sera retornado um valor negativo.
-----------------------------------------------------------------------------*/
DIR2 opendir2 (char *pathname)
{

	if(system == 0){
		init_system();
	}
	char* cluster_buffer = (char *)malloc(256*sb->SectorsPerCluster);
	struct	t2fs_record* entries;
	int temp_cluster_father_dir;
	struct t2fs_record* temp_current_dir;
	int i = 0;
	int found = 0;

	findPath(pathname, temp_cluster_father_dir, temp_current_dir, entries);


	/*achou o diretorio, instancia uma nova file_struct, coloca na
	primeira posicao livre do array de open directories e retorna o indice*/
	struct file_struct open_dir;
	current_directory = temp_current_dir;
	strncpy(open_dir.name,current_directory->name, 56);
	open_dir.firstCluster, current_directory->firstCluster;
	open_dir.offset = 0;

	i=0;
	while(open_directories[i].name!=NULL){
		i++;
	}
	open_directories[i]=open_dir;


	return i;
}


/*-----------------------------------------------------------------------------
Funcao:	Realiza a leitura das entradas do diretorio identificado por "handle".
	A cada chamada da funcao e lida a entrada seguinte do diretorio representado pelo identificador "handle".
	Algumas das informacoes dessas entradas devem ser colocadas no parametro "dentry".
	Apos realizada a leitura de uma entrada, o ponteiro de entradas (current entry) e ajustado para a proxima entrada valida
	Sao considerados erros:
		(a) termino das entradas validas do diretorio identificado por "handle".
		(b) qualquer situacao que impeca a realizacao da operacao

Entra:	handle -> identificador do diretorio cujas entradas deseja-se ler.
	dentry -> estrutura de dados onde a funcao coloca as informacoes da entrada lida.

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna "0" (zero).
	Em caso de erro, sera retornado um valor negativo
		Se o diretorio chegou ao final, retorna "-END_OF_DIR" (-1)
		Outros erros, sera retornado um outro valor negativo
-----------------------------------------------------------------------------*/
int readdir2 (DIR2 handle, DIRENT2 *dentry)
{
	if(system == 0){
		init_system();
	}

	char* cluster_buffer = (char *)malloc(256*sb->SectorsPerCluster);

	int offset = open_directories[handle].offset;
	if(offset >= DIRENT_PER_SECTOR - 1){
		//quer dizer que ja chegou no final do diretorio, pois ja passou todas as entradas de diretorio
		return END_OF_DIR;
	}
	//vai ler o diretorio apontado pelo handle
	read_cluster(open_directories[handle].firstCluster, cluster_buffer);

	strncpy(dentry->fileType, cluster_buffer[offset*64], 1);
	strncpy(dentry->name, cluster_buffer[offset*64 + 1], 55);
	strncpy(dentry->fileSize, cluster_buffer[offset*64 + 56], 4);


	open_directories[handle].offset += 1;
	return 0;
}


/*-----------------------------------------------------------------------------
Funcao:	Fecha o diretorio identificado pelo parametro "handle".

Entra:	handle -> identificador do diretorio que se deseja fechar (encerrar a operacao).

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna "0" (zero).
	Em caso de erro, sera retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int closedir2 (DIR2 handle){
	if(system == 0){
		init_system();
	}
	if(open_directories[handle].name !=  NULL){
		char *string = NULL;
		strcpy(open_directories[handle].name, string);
		return 0;
	}
	return -1;
}

//IDENTIFY PRONTO!!!
int identify2 (char *name, int size)
{
	char group_id[] = "Guillermo Falcao - 217434\nHermes Tessaro - 218317\nRafael Allegretti - 215020";
  if(size < sizeof(group_id)) return -1;
  else
  {
    strncpy(name, group_id, sizeof(group_id));
    return 0;
  }
}







/*FUNCOES DE ARQUIVO:*/



/*-----------------------------------------------------------------------------
Funcao: Criar um novo arquivo.
	O nome desse novo arquivo e aquele informado pelo parametro "filename".
	O contador de posicao do arquivo (current pointer) deve ser colocado na posicao zero.
	Caso ja exista um arquivo ou diretorio com o mesmo nome, a funcao devera retornar um erro de criacao.
	A funcao deve retornar o identificador (handle) do arquivo.
	Esse handle sera usado em chamadas posteriores do sistema de arquivo para fins de manipulacao do arquivo criado.

Entra:	filename -> path absoluto para o arquivo a ser criado. Todo o "path" deve existir.

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna o handle do arquivo (numero positivo).
	Em caso de erro, deve ser retornado um valor negativo.
-----------------------------------------------------------------------------*/
FILE2 create2 (char *filename){

	return 0;
}


/*-----------------------------------------------------------------------------
Funcao:	Apagar um arquivo do disco.
	O nome do arquivo a ser apagado e aquele informado pelo parametro "filename".

Entra:	filename -> nome do arquivo a ser apagado.

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna "0" (zero).
	Em caso de erro, sera retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int delete2 (char *filename){

	return 0;
}


/*-----------------------------------------------------------------------------
Funcao:	Abre um arquivo existente no disco.
	O nome desse novo arquivo e aquele informado pelo parametro "filename".
	Ao abrir um arquivo, o contador de posicao do arquivo (current pointer) deve ser colocado na posicao zero.
	A funcao deve retornar o identificador (handle) do arquivo.
	Esse handle sera usado em chamadas posteriores do sistema de arquivo para fins de manipulacao do arquivo criado.
	Todos os arquivos abertos por esta chamada sao abertos em leitura e em escrita.
	O ponto em que a leitura, ou escrita, sera realizada e fornecido pelo valor current_pointer (ver funcao seek2).

Entra:	filename -> nome do arquivo a ser apagado.

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna o handle do arquivo (numero positivo)
	Em caso de erro, deve ser retornado um valor negativo
-----------------------------------------------------------------------------*/
FILE2 open2 (char *filename){

	return 0;
}


/*-----------------------------------------------------------------------------
Funcao:	Fecha o arquivo identificado pelo parametro "handle".

Entra:	handle -> identificador do arquivo a ser fechado

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna "0" (zero).
	Em caso de erro, sera retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int close2 (FILE2 handle){

	return 0;
}


/*-----------------------------------------------------------------------------
Funcao:	Realiza a leitura de "size" bytes do arquivo identificado por "handle".
	Os bytes lidos sao colocados na area apontada por "buffer".
	Apos a leitura, o contador de posicao (current pointer) deve ser ajustado para o byte seguinte ao ultimo lido.

Entra:	handle -> identificador do arquivo a ser lido
	buffer -> buffer onde colocar os bytes lidos do arquivo
	size -> numero de bytes a serem lidos

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna o numero de bytes lidos.
	Se o valor retornado for menor do que "size", entao o contador de posicao atingiu o final do arquivo.
	Em caso de erro, sera retornado um valor negativo.
-----------------------------------------------------------------------------*/
int read2 (FILE2 handle, char *buffer, int size){

	return 0;
}


/*-----------------------------------------------------------------------------
Funcao:	Realiza a escrita de "size" bytes no arquivo identificado por "handle".
	Os bytes a serem escritos estao na area apontada por "buffer".
	Apos a escrita, o contador de posicao (current pointer) deve ser ajustado para o byte seguinte ao ultimo escrito.

Entra:	handle -> identificador do arquivo a ser escrito
	buffer -> buffer de onde pegar os bytes a serem escritos no arquivo
	size -> numero de bytes a serem escritos

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna o numero de bytes efetivamente escritos.
	Em caso de erro, sera retornado um valor negativo.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size){

	return 0;
}


/*-----------------------------------------------------------------------------
Funcao:	Funcao usada para truncar um arquivo.
	Remove do arquivo todos os bytes a partir da posicao atual do contador de posicao (CP)
	Todos os bytes a partir da posicao CP (inclusive) serao removidos do arquivo.
	Apos a operacao, o arquivo devera contar com CP bytes e o ponteiro estara no final do arquivo

Entra:	handle -> identificador do arquivo a ser truncado

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna "0" (zero).
	Em caso de erro, sera retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int truncate2 (FILE2 handle){

	return 0;
}


/*-----------------------------------------------------------------------------
Funcao:	Reposiciona o contador de posicoes (current pointer) do arquivo identificado por "handle".
	A nova posicao e determinada pelo parametro "offset".
	O parametro "offset" corresponde ao deslocamento, em bytes, contados a partir do inicio do arquivo.
	Se o valor de "offset" for "-1", o current_pointer devera ser posicionado no byte seguinte ao final do arquivo,
		Isso e util para permitir que novos dados sejam adicionados no final de um arquivo ja existente.

Entra:	handle -> identificador do arquivo a ser escrito
	offset -> deslocamento, em bytes, onde posicionar o "current pointer".

Saida:	Se a operacao foi realizada com sucesso, a funcao retorna "0" (zero).
	Em caso de erro, sera retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int seek2 (FILE2 handle, unsigned int offset){

	return 0;
}
