#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define waysAmount 4  
#define setsAmount 8   
#define blockSize 64   // cantidad de palabras de 16 bits por bloque de 128B 
#define MemoryBlocks 512 // cantidad de bloques en memoria      

typedef struct
{
	int addresses[blockSize];
} MemoryBlock;

typedef struct
{
	int addresses[blockSize];
	int tag;
	int valid;
	int order;

} CacheBlock;

CacheBlock cache[waysAmount][setsAmount];

MemoryBlock memory[MemoryBlocks];

int hits = 0;
int misses = 0;

void init()
{

	for (int a = 0; a<waysAmount; a++)
	{
		for (int b =0; b < setsAmount; b++)
		{
			CacheBlock cb;
			
			for (int c=0; c < blockSize; c++)
			{
				cb.addresses[c] = 0;
			}
			cb.tag = 0;
			cb.valid=0; 
			cb.order=0;
			cache[a][b] = cb;
		}
	}
	for (int d = 0; d<MemoryBlocks; d++)
		{
			MemoryBlock nb;
			
			for (int e=0; e < blockSize; e++)
			{
				nb.addresses[e] = 0;
			}
			memory[d] = nb;
		}
	}


unsigned int find_set(unsigned int address)
{
	//Devuelve el conjunto de cache al que mapea la direccion address.
	return address/ blockSize % setsAmount;
}

unsigned int get_offset(unsigned int address)
{
	//Devuelve el oﬀset del byte del bloque de memoria al que mapea la direccion address
	return address % blockSize;
}
void reorganize(int set, int num){
	//reorganiza los ordenes segun la politica de reemplazo lru 
	for (int i=0; i < waysAmount; i++){
		if (cache[i][set].order < 7 && cache[i][set].order >= num) 
		{
			cache[i][set].order=cache[i][set].order - 1;
		}

	}
}
unsigned int select_oldest(unsigned int set)
{
	//Devuelve la vıa en la que esta el bloque mas “viejo” dentro de un conjunto, 
	//utilizando el campo correspondiente de los metadatos de los bloques del conjunto. 
	int oldest = 0;
	int way = 0;
	int y;
	for (int x = 0; x < waysAmount; x++)
	{
		if (cache[x][set].valid == 0)
			{
				return x;
			}
		else
		{
			y = cache[x][set].order;
			if (oldest < y)
			{
				oldest = y;
				way = x;
			}
		}

	}
	return way;
}

void read_tocache(unsigned int block, unsigned int way, unsigned int set)
{
	//lee el bloque blocknum de memoria y guardarlo en el conjunto y 
	//via indicados en la memoria cache.
	for (int i = 0; i <blockSize; i++)
	{
		cache[way][set].addresses[i] = memory[block].addresses[i];
	}
	cache[way][set].tag = block;
	cache[way][set].valid = 1;
	cache[way][set].order = 7;
}

signed int compare_tag(unsigned int tag, unsigned int set)
{
	//devuelve la vıa en la que se encuentra almacenado el bloque correspondiente 
	//a tag en el conjunto index, o -1 si ninguno de los tags coincide.

	int aux = -1;
	for (int i = 0; i< waysAmount; i++)
	{
		if (cache[i][set].valid == 1 && cache[i][set].tag == tag)
		{
			aux = i;
			break;
		}
	}
	return aux;
}

unsigned char read_byte(unsigned int address)
{
	/*busca el valor del byte correspondiente a la posicion 
	address en la cache; si este no se encuentra en la cache debe cargar ese bloque. 
	El valor de retorno siempre debe ser el valor del byte almacenado en la direccion 
	indicada.*/

	int offset = get_offset(address);
	int set = find_set(address);
	int tag = address / (blockSize*setsAmount);
	int way = compare_tag(tag, set);
	int num = cache[way][set].order;

	if (way == -1)
	{
		read_tocache(tag, select_oldest(set), set);
		misses++;
		//printf("rd produjo miss en set %d, tag %d, offset %d, address %d \n",set, tag, offset, address); 
	}
	else
	{
		cache[way][set].order = 7;
		hits++;
		//printf("rd produjo hit en set %d, tag %d, offset %d, address %d \n",set, tag, offset, address); 
	}
	reorganize(set, num);
	return cache[way][set].addresses[offset];
}

void write_byte(unsigned int address, unsigned char data)
{
	/*Escribe el valor value en la posicion address de memoria, y en la posicion 
	correcta del bloque que corresponde a address, si el bloque se encuentra en la cache. 
	Si no se encuentra, debe escribir el valor solamente en la memoria*/
	
	int offset = get_offset(address);
	int set = find_set(address);
	int tag = address / (blockSize*setsAmount);
		
	if (compare_tag(tag, set) == -1)
	{
		misses++;
		//printf("wr produjo miss en set %d, tag %d, offset %d, address %d \n",set, tag, offset, address); 
	}
	else
	{
		hits++;
		int way = compare_tag(tag, set);
		cache[way][set].addresses[offset] = data;
		//printf("wr produjo hit en set %d, tag %d offset %d, address %d \n",set, tag, offset, address);
	}
	memory[tag].addresses[offset] = data;
}

float get_miss_rate()
{
	//Devuelve el porcentaje de misses desde que se inicializo la cache.
	float MR = 0;
	int cant = hits + misses;
	if (cant > 0)
	{
		MR =(float)misses/(float)cant;
	}
	return MR;
}

void borrar(char *palabra,char caracter)
{
	int counter=0;
	for(int i=0; palabra[i];i++)
	{
		palabra[i]=palabra[i+counter];
		if(palabra[i]==caracter)
		{
			i--;
			counter++;
		}
	}
}

void parser(char linea[256])
{
	if(strstr(linea, "FLUSH ") != NULL)
	{
		init();
	}
	if(strstr(linea, "MR") != NULL)
	{
		printf("El Miss rate fue del: %f \n", get_miss_rate());
	}
	if(strstr(linea, "W ") != NULL)
	{
		
		char *cadena = strtok(linea, " " );
		cadena = strtok(NULL, " " );
		char *cadena2 = strtok(NULL, "," );
		borrar(cadena, ','); 
        borrar(cadena, ' '); 
        borrar(cadena, '\n'); 
        int address = strtol(cadena, (char **)NULL, 10);
        int data = strtol(cadena2, (char **)NULL, 10);
		write_byte(address, data);
		printf("se escribio en la direccion: %d el dato %d \n", address, data);
	}
	if(strstr(linea, "R ") != NULL)
	{
		char * cadena = strtok(linea, " " );
		cadena = strtok(NULL, " " );
        borrar(cadena, '\n');
        int address = strtol(cadena, (char **)NULL, 10);
        read_byte(address);
        printf("se leyo la direccion: %d \n", address); 
	}
}

int main(int argc, char* argv[])
{
	init();
	char const* const archivo = argv[1];
    FILE* prueba = fopen(archivo, "r");
    char linea[256];

    while (fgets(linea, sizeof(linea), prueba)) {
        parser(linea);
    }

    fclose(prueba);
    return 0;
}
