#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAIN_MEMORY_SIZE (64 * 1024) // 64 KB
#define CACHE_SIZE (4 * 1024)
#define BLOCK_SIZE 128
#define SET_AMOUNT 8
#define WAYS_AMOUNT 4
#define INVALID 0
#define VALID 1

#define BLOCKS_COUNT (CACHE_SIZE / WAYS_AMOUNT / BLOCK_SIZE)
#define ADDRESS 16
#define OFFSET (log(BLOCK_SIZE) / log(2))
#define INDEX (log(SET_AMOUNT) / log(2))
#define TAG (ADDRESS - INDEX - OFFSET)

typedef struct {
    unsigned char data[MAIN_MEMORY_SIZE];
} main_memory_t;

typedef struct {
    unsigned int tag;
    int counter;
    bool valid;
    unsigned char data[BLOCK_SIZE];
} cache_block_t;

typedef struct {
    cache_block_t blocks[SET_AMOUNT][WAYS_AMOUNT];
    int miss_count;
    int hit_count;
} cache_t;

main_memory_t main_memory;
cache_t cache;

//-------------------------------------AUX----------------------------------------//

unsigned char read_cache(unsigned int set, unsigned int way, unsigned int offset) {
	// Lee valor de memoria caché correspondiente al set, vía y offset indicados. 
	// A sus vez setea el counter del bloque en cero.

    cache_block_t block = cache.blocks[set][way];
    block.counter = 0;

    return block.data[offset];
}

void inc_block_counter(unsigned int set) {
	// Adiciona uno al atributo counter de todos los bloques de memoria caché del set indicado.

    cache_block_t *blocks_in_set = cache.blocks[set];
    for (size_t i = 0; i < WAYS_AMOUNT; i++) {
        blocks_in_set[i].counter += 1;
    }
}

unsigned int get_block(unsigned int address) { 
	// Obtiene el bloque de memoria caché correspondiente a la dirección indicada.

	return address / BLOCK_SIZE; 
}

unsigned int get_tag(unsigned int address) {
	// Obtiene el tag del bloque de la memoria caché correspondiente a la dirección indicada.

    unsigned int int_for_set_and_offset = pow(2, INDEX + OFFSET);
    return address / int_for_set_and_offset;
}

void inc_rates(bool isHit) {
	// Actualiza el valor de hit o miss de la caché.
	
    if (isHit) {
    	cache.hit_count += 1;
    } else {
        cache.miss_count += 1;
    }
}

//------------------------------------MAIN----------------------------------------//

void init() {

	printf("------------------------------------------------------------ \n");
    memset(main_memory.data, 0, MAIN_MEMORY_SIZE);

    memset(cache.blocks, INVALID, sizeof(cache.blocks));
    cache.miss_count = 0;
    cache.hit_count = 0;
}

unsigned int find_set(unsigned int address) {
	//Devuelve el conjunto de cache al que mapea la direccion address.

    return address / BLOCK_SIZE % SET_AMOUNT;
}

unsigned int get_offset(unsigned int address) { 
	//Devuelve el oﬀset del byte del bloque de memoria al que mapea la direccion address.

	return address % BLOCK_SIZE;
}

unsigned int select_oldest(unsigned int set) {
	//Devuelve la vıa en la que esta el bloque mas “viejo” dentro de un conjunto, 
	//utilizando el campo correspondiente de los metadatos de los bloques del conjunto.

	cache_block_t *blocks_in_set = cache.blocks[set];
    unsigned int oldest = 0;
    for (size_t i = 1; i < WAYS_AMOUNT; i++) {
        if (blocks_in_set[oldest].counter < blocks_in_set[i].counter) {
            oldest = i;
        }
    }
    return oldest;
}

void read_tocache(unsigned int blocknum, unsigned int way, unsigned int set) {
	//lee el bloque blocknum de memoria y guardarlo en el conjunto y 
	//via indicados en la memoria cache.

	int block_starting_address = blocknum * BLOCK_SIZE;
    cache_block_t *block = &cache.blocks[set][way];
    for (int i = 0; i < BLOCK_SIZE; i++) {
        block->data[i] = main_memory.data[block_starting_address + i];
    }
    block->valid = VALID;
    block->tag = get_tag(block_starting_address);
    block->counter = 0;
}

signed int compare_tag(unsigned int tag, unsigned int set) {
	//devuelve la vıa en la que se encuentra almacenado el bloque correspondiente 
	//a tag en el conjunto index, o -1 si ninguno de los tags coincide.

	cache_block_t *blocks_in_set = cache.blocks[set];
    cache_block_t block;
    for (int i = 0; i < WAYS_AMOUNT; i++) {
        block = blocks_in_set[i];
        if (block.valid && block.tag == tag) {
            return i;
        }
    }
    return -1;
}

void write_tocache(unsigned int address, unsigned char value) {
	//Como la cache es WT/¬WA, si escribimos en la cache, esto nos garantiza que el bloque correspondiente a la
	//direccion se encuentra en la misma.

    unsigned int tag = get_tag(address);
    unsigned int set = find_set(address);
    unsigned int offset = get_offset(address);
    int way = compare_tag(tag, set);
    cache_block_t *block = &cache.blocks[set][way];
    block->data[offset] = value;
    block->counter = 0;
}

unsigned char read_byte(unsigned int address) {
	/*busca el valor del byte correspondiente a la posicion 
	address en la cache; si este no se encuentra en la cache debe cargar ese bloque. 
	El valor de retorno siempre debe ser el valor del byte almacenado en la direccion 
	indicada.*/

	unsigned int tag = get_tag(address);
    unsigned int set = find_set(address);
    int way = compare_tag(tag, set);
    unsigned char byte_read;
    bool is_in_cache = way >= 0;
    inc_block_counter(set);
    if (!is_in_cache) {
        unsigned int block_num = get_block(address);
        way = select_oldest(set);
        read_tocache(block_num, way, set);
    }

    unsigned int offset = get_offset(address);
    byte_read = read_cache(set, way, offset);
    inc_rates(is_in_cache);

    printf("Se leyó en la dirección %d, el dato => %u \n", address, byte_read);
    return byte_read;
}

void write_byte(unsigned int address, unsigned char value) {
	/*Escribe el valor value en la posicion address de memoria, y en la posicion 
	correcta del bloque que corresponde a address, si el bloque se encuentra en la cache. 
	Si no se encuentra, debe escribir el valor solamente en la memoria*/
	
	unsigned int tag = get_tag(address);
    unsigned int set = find_set(address);
    int way = compare_tag(tag, set);
    bool is_in_cache = way >= 0;

    inc_block_counter(set);
    main_memory.data[address] = value;
    if (is_in_cache) {
        write_tocache(address, value);
    }
    inc_rates(is_in_cache);
    printf("Se escribió en la dirección %d, el dato => %u \n", address, value);
}

float get_miss_rate() {
	//Devuelve el porcentaje de misses desde que se inicializo la cache.

	if (cache.miss_count == 0) return 0;
    int total_count = cache.miss_count + cache.hit_count;
    float mr = cache.miss_count / (float)total_count * 100;
    printf("MISS RATE: %0.2f%% \n", mr);
    printf("------------------------------------------------------------ \n");
    return mr;
}

//--------------------------------------------------------------------------------//

static int read_address(char *line) {
    unsigned int address;
    int result = sscanf(line, "R %u", &address);
    if (result != 1) {
        fprintf(stderr, "Error al leer la dirección.\n");
        return 1;
    } else if (address >= MAIN_MEMORY_SIZE) {
        fprintf(stderr, "Error: La dirección %d es muy grande. \n", address);
    } else {
        read_byte(address);
    }
    return 0;
}

static int write_address(char *line) {
    unsigned int address;
    unsigned int value;
    int result = sscanf(line, "W %u, %u", &address, &value);
    if (result != 2) {
        fprintf(stderr, "Error al leer la dirección y el valor.\n");
        return 1;
    } else if (address >= MAIN_MEMORY_SIZE) {
        fprintf(stderr, "Error: La dirección %d es muy grande. \n", address);
    } else {
        write_byte(address, value);
    }
    return 0;
}

int parser(const char *filename) {
	FILE *commands_file;
    if (!(commands_file = fopen(filename, "r"))) {
        fprintf(stderr, "Error al abrir el archivo.\n");
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    int result = 0;

    while (getline(&line, &len, commands_file) != -1) {
        if (strncmp(line, "FLUSH", 5) == 0) {
            init();
        } else if (strncmp(line, "R", 1) == 0) {
            result = read_address(line);
        } else if (strncmp(line, "W", 1) == 0) {
            result = write_address(line);
        } else if (strncmp(line, "MR", 2) == 0) {
            get_miss_rate();
        } else {
            fprintf(stderr, "Error: línea inválida.\n");
            result = 1;
        }

        if (result == 1) return result;
    }
    fclose(commands_file);
    return result;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
        fprintf(stderr, "Argunmentos invalidos.\n");
        return 1;
    }

    init();

    int result = parser(argv[1]);

    return result;
}
