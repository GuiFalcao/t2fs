#include <stdio.h>
#include <string.h>
#include "../include/apidisk.h"
#include "../include/t2fs.h"

#define system_init = 0;

typedef struct file_struct {
    char    name[MAX_FILE_NAME_SIZE]; 	/* Nome do arquivo. : string com caracteres ASCII (0x21 até 0x7A), case sensitive.*/
    DWORD   firstCluster;		/* Número do primeiro cluster de dados correspondente a essa entrada de diretório */
    int offset; /*pra checar aonde está dentro do diretório*/
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


/*função que procura o diretório buscado pelo pathname, podendo este ser absoluto ou relativo.
  Recebe um pathname a ser procurado, e quatro variaveis auxiliares para serem colocados os registros.
  Ao fim, estarão as entradas de diretório do dir procurado em entries
  O cluster do diretório pai ao do procurado em temp_cluster_father_dir
  A entrada de diretório relativa ao dir procurado em temp_current_dir
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
				//procura no diretório pai
				temp_current_dir->firstCluster = clusterOfFatherDirectory;
			}
			//é no "."
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

	//Entries tem agora as entradas de diretório do primeiro diretório referenciado, /, . ou ..
	entries  = (struct t2fs_record*) cluster_buffer;
	
	//procura nas entradas o diretório pai, seta o cluster do diretório pai pra ele
	for(i=0;i<DIRENT_PER_SECTOR;i++){
		if(strcmp(entries[i].name, "..")==0){
			temp_cluster_father_dir = entries[i].firstCluster;
		}
	}

	/*TEMOS AGORA:
	1) Array entries tem as entradas de diretório do primeiro diretório procurado
	2) temp_cluster_father_dir tem o cluster do diretório pai
	3) temp_current_dir tem a entrada de diretório do primeiro diretório achado, /, . ou ..*/

	char* word;
	//word is the current directory name being looked for
	for (word = strtok(pathname, "/"); word; word = strtok(NULL, "/"))
	{
		found = 0;
		//searchs for the given name in the directory records' array
		for(i=0; i<DIRENT_PER_SECTOR;i++){
			//se achar o nome do diretório com o nome da pasta
			if(strcmp(word, entries[i].name) == 0){
				if(entries[i].TypeVal != 2){
					if(debug = 1){
						printf("nao é um diretório\n");
					}
					return -1;
				}
				//if found the directory, reads its cluster
				//associa o cluster do pai a este cluster, já que vai trocar de diretório
				int j = 0;
				for(j=0;j<DIRENT_PER_SECTOR;j++){
					if(strcmp(entries[i].name, ".")==0){
						temp_cluster_father_dir = entries[j].firstCluster;
					}
				}
				//le o cluster; agora entries tem as informações do novo cluster
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

	/*entries tem o conteúdo do diretório procurado. temp_current_dir tem a entrada de diretório dele,
	temp_current_dir_path tem o caminho dele, temp_cluster_father_dir tem o cluster do diretório pai*/
	

}



/*FUNCOES DE DIRETORIO:*/




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
	//AQUI O ARRAY ENTRIES TEM AS ENTRADAS DE DIRETÓRIO DO ROOT
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
						printf("nao é um diretório\n");
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
			//NAO ACHOU O NOME DA PASTA -> ou ela não existe, ou é a que precisa ser criada
			if(strtok(NULL, "/")==NULL)
				{
					//então acabou a palavra, é o diretório que tu precisa criar
					for(i=0; i<DIRENT_PER_SECTOR;i++){
						//percorre o array de entries pra achar uma entrada livre
						if(entries[i].TypeVal = 0x00){
						//se o tipo de valor é 0, o arquivo não está sendo usado
						//escreve o cluster atualizado do diretório pai com a nova entrada de diretório
							entries[i].TypeVal = 0x02;
							strncpy(entries[i].name, word, 55);
							entries[i].bytesFileSize = DIRENT_PER_SECTOR;
							int free_c = findsFreeCluster(2);
							entries[i].firstCluster = free_c;
							write_cluster(clusterOfFatherDirectory, (char*) entries);

							//aloca um buffer pra escrever as entradas do novo diretório
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
					//não achou entrada livre, retorna erro
					return -1;
				}
			else{
				//o caminho dado não existe, retorna erro
				return -1;
			}
		}
	}
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
	struct t2fs_record* entries;

	int i = 0;
	int temp_cluster_father_dir = -1;
	int found = 0;
	int vazio = 0;

	struct t2fs_record* temp_current_dir;


	if(strcmp(pathname,"/") == 0){
		//NÃO É POSSÍVEL DELETAR O /
		return -1;
	}

	findPath(pathname, temp_cluster_father_dir, temp_current_dir, entries);

	//ACHOU TODO CAMINHO ATE O DIRETORIO
	for(i=0; i<DIRENT_PER_SECTOR;i++){
		if(!(entries[i].TypeVal == 0x00)){
			//SE ALGUMA ENTRADA NÃO FOR VAZIA E NAO FOR O . E O .. RETORNA ERRO
			if((strcmp(entries[i].name, ".")!=0)&&(strcmp(entries[i].name, "..")!=0)){
				return -1;
			}
		}
	}
	//ACHOU O DIRETORIO E ESTA VAZIO
	//zera o cluster do diretório
	cluster_buffer = NULL;
	write_cluster(entries[i].firstCluster, cluster_buffer);

	//apaga a entrada do cluster na fat
	fat[entries[i].firstCluster] = 0;

	free(cluster_buffer);

	return 0;

}


/*-----------------------------------------------------------------------------
Função:	Altera o current path
	O novo caminho do diretório a ser usado como current path é aquele informado pelo parâmetro "pathname".
	São considerados erros quaisquer situações que impeçam a operação.
		Isso inclui:
			(a) "pathname" não existente;
			(b) algum dos componentes do "pathname" não existe (caminho inválido);
			(c) o "pathname" indicado não é um diretório;

Entra:	pathname -> caminho do diretório a ser criado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int chdir2 (char *pathname){
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
Função:	Função que informa o diretório atual de trabalho.
	O T2FS deve copiar o pathname do diretório de trabalho, incluindo o ‘\0’ do final do string, para o buffer indicado por "pathname".
	Essa cópia não pode exceder o tamanho do buffer, informado pelo parâmetro "size".

Entra:	pathname -> ponteiro para buffer onde copiar o pathname
	size -> tamanho do buffer "pathname" (número máximo de bytes a serem copiados).

Saída:
	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Caso ocorra algum erro, a função retorna um valor diferente de zero.
	São considerados erros quaisquer situações que impeçam a cópia do pathname do diretório de trabalho para "pathname",
	incluindo espaço insuficiente, conforme informado por "size".
-----------------------------------------------------------------------------*/
int getcwd2 (char *pathname, int size){
	if(current_directory == NULL){
		if(debug = 1){
			printf("nao está em diretorio nenhum\n");
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
	struct	t2fs_record* entries;
	int temp_cluster_father_dir;
	struct t2fs_record* temp_current_dir;
	int i = 0;
	int found = 0;

	findPath(pathname, temp_cluster_father_dir, temp_current_dir, entries);


	/*achou o diretório, instancia uma nova file_struct, coloca na
	primeira posição livre do array de open directories e retorna o índice*/
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
Função:	Realiza a leitura das entradas do diretório identificado por "handle".
	A cada chamada da função é lida a entrada seguinte do diretório representado pelo identificador "handle".
	Algumas das informações dessas entradas devem ser colocadas no parâmetro "dentry".
	Após realizada a leitura de uma entrada, o ponteiro de entradas (current entry) é ajustado para a próxima entrada válida
	São considerados erros:
		(a) término das entradas válidas do diretório identificado por "handle".
		(b) qualquer situação que impeça a realização da operação

Entra:	handle -> identificador do diretório cujas entradas deseja-se ler.
	dentry -> estrutura de dados onde a função coloca as informações da entrada lida.

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor negativo
		Se o diretório chegou ao final, retorna "-END_OF_DIR" (-1)
		Outros erros, será retornado um outro valor negativo
-----------------------------------------------------------------------------*/
int readdir2 (DIR2 handle, DIRENT2 *dentry)
{
	if(system == 0){
		init_system();
	}

	char* cluster_buffer = (char *)malloc(256*sb->SectorsPerCluster);

	int offset = open_directories[handle].offset;
	if(offset >= DIRENT_PER_SECTOR - 1){
		//quer dizer que já chegou no final do diretório, pois já passou todas as entradas de diretório
		return END_OF_DIR;
	}
	//vai ler o diretório apontado pelo handle
	read_cluster(open_directories[handle].firstCluster, cluster_buffer);

	strncpy(dentry->fileType, cluster_buffer[offset*64], 1);
	strncpy(dentry->name, cluster_buffer[offset*64 + 1], 55);
	strncpy(dentry->fileSize, cluster_buffer[offset*64 + 56], 4);


	open_directories[handle].offset += 1;
	return 0;
}


/*-----------------------------------------------------------------------------
Função:	Fecha o diretório identificado pelo parâmetro "handle".

Entra:	handle -> identificador do diretório que se deseja fechar (encerrar a operação).

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int closedir2 (DIR2 handle){
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
Função: Criar um novo arquivo.
	O nome desse novo arquivo é aquele informado pelo parâmetro "filename".
	O contador de posição do arquivo (current pointer) deve ser colocado na posição zero.
	Caso já exista um arquivo ou diretório com o mesmo nome, a função deverá retornar um erro de criação.
	A função deve retornar o identificador (handle) do arquivo.
	Esse handle será usado em chamadas posteriores do sistema de arquivo para fins de manipulação do arquivo criado.

Entra:	filename -> path absoluto para o arquivo a ser criado. Todo o "path" deve existir.

Saída:	Se a operação foi realizada com sucesso, a função retorna o handle do arquivo (número positivo).
	Em caso de erro, deve ser retornado um valor negativo.
-----------------------------------------------------------------------------*/
FILE2 create2 (char *filename){

	return 0;
}


/*-----------------------------------------------------------------------------
Função:	Apagar um arquivo do disco.
	O nome do arquivo a ser apagado é aquele informado pelo parâmetro "filename".

Entra:	filename -> nome do arquivo a ser apagado.

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int delete2 (char *filename){

	return 0;
}


/*-----------------------------------------------------------------------------
Função:	Abre um arquivo existente no disco.
	O nome desse novo arquivo é aquele informado pelo parâmetro "filename".
	Ao abrir um arquivo, o contador de posição do arquivo (current pointer) deve ser colocado na posição zero.
	A função deve retornar o identificador (handle) do arquivo.
	Esse handle será usado em chamadas posteriores do sistema de arquivo para fins de manipulação do arquivo criado.
	Todos os arquivos abertos por esta chamada são abertos em leitura e em escrita.
	O ponto em que a leitura, ou escrita, será realizada é fornecido pelo valor current_pointer (ver função seek2).

Entra:	filename -> nome do arquivo a ser apagado.

Saída:	Se a operação foi realizada com sucesso, a função retorna o handle do arquivo (número positivo)
	Em caso de erro, deve ser retornado um valor negativo
-----------------------------------------------------------------------------*/
FILE2 open2 (char *filename){

	return 0;
}


/*-----------------------------------------------------------------------------
Função:	Fecha o arquivo identificado pelo parâmetro "handle".

Entra:	handle -> identificador do arquivo a ser fechado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int close2 (FILE2 handle){

	return 0;
}


/*-----------------------------------------------------------------------------
Função:	Realiza a leitura de "size" bytes do arquivo identificado por "handle".
	Os bytes lidos são colocados na área apontada por "buffer".
	Após a leitura, o contador de posição (current pointer) deve ser ajustado para o byte seguinte ao último lido.

Entra:	handle -> identificador do arquivo a ser lido
	buffer -> buffer onde colocar os bytes lidos do arquivo
	size -> número de bytes a serem lidos

Saída:	Se a operação foi realizada com sucesso, a função retorna o número de bytes lidos.
	Se o valor retornado for menor do que "size", então o contador de posição atingiu o final do arquivo.
	Em caso de erro, será retornado um valor negativo.
-----------------------------------------------------------------------------*/
int read2 (FILE2 handle, char *buffer, int size){

	return 0;
}


/*-----------------------------------------------------------------------------
Função:	Realiza a escrita de "size" bytes no arquivo identificado por "handle".
	Os bytes a serem escritos estão na área apontada por "buffer".
	Após a escrita, o contador de posição (current pointer) deve ser ajustado para o byte seguinte ao último escrito.

Entra:	handle -> identificador do arquivo a ser escrito
	buffer -> buffer de onde pegar os bytes a serem escritos no arquivo
	size -> número de bytes a serem escritos

Saída:	Se a operação foi realizada com sucesso, a função retorna o número de bytes efetivamente escritos.
	Em caso de erro, será retornado um valor negativo.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size){

	return 0;
}


/*-----------------------------------------------------------------------------
Função:	Função usada para truncar um arquivo.
	Remove do arquivo todos os bytes a partir da posição atual do contador de posição (CP)
	Todos os bytes a partir da posição CP (inclusive) serão removidos do arquivo.
	Após a operação, o arquivo deverá contar com CP bytes e o ponteiro estará no final do arquivo

Entra:	handle -> identificador do arquivo a ser truncado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int truncate2 (FILE2 handle){

	return 0;
}


/*-----------------------------------------------------------------------------
Função:	Reposiciona o contador de posições (current pointer) do arquivo identificado por "handle".
	A nova posição é determinada pelo parâmetro "offset".
	O parâmetro "offset" corresponde ao deslocamento, em bytes, contados a partir do início do arquivo.
	Se o valor de "offset" for "-1", o current_pointer deverá ser posicionado no byte seguinte ao final do arquivo,
		Isso é útil para permitir que novos dados sejam adicionados no final de um arquivo já existente.

Entra:	handle -> identificador do arquivo a ser escrito
	offset -> deslocamento, em bytes, onde posicionar o "current pointer".

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int seek2 (FILE2 handle, unsigned int offset){

	return 0;
}


