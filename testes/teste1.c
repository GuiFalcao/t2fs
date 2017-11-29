#include <stdio.h>
#include <stdlib.h>



int main(){
	int result = opendir2("/");
	printf("%d\n", result);

	result = readdir2("/");
	printf("%d\n", result);
}