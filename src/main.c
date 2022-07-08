#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>

// Estructura para almacenar la información de la imagen PGM
typedef struct PGMImage
{
    char pgmType[3];
    unsigned char **data;
    unsigned int width;
    unsigned int height;
    unsigned int maxValue;
} PGMImage;

// Estructura para almacenar la información de las distancias euclideanas
typedef struct Euclides
{
    char pgmType[3]; //Innecesario si no se exporta como imagen 
    unsigned long **data;
    unsigned int width;
    unsigned int height;
    unsigned int maxValue; //Innecesario si no se exporta como imagen
} Euclides;

// Abrir una imagen PGM y procesarla para guardar sus datos en una estructura
bool openPGM(PGMImage *pgm, const char *filename)
{
    // Abrir archivo en modo 'read binary'
    FILE *pgmfile = fopen(filename, "rb");

    // Si no existe retorna falso
    if (pgmfile == NULL)
    {
        printf("El archivo que se intenta abrir no existe\n");
        return false;
    }
    if(fscanf(pgmfile, "%s", pgm->pgmType) < 0)
        perror("fscanf()");

    // Se chequea que sea la version P5
    if (strcmp(pgm->pgmType, "P5"))
    {
        fprintf(stderr, "Error en el tipo de archivo PGM!\n");
        exit(EXIT_FAILURE);
    }

    // Detectar las dimensiones de la imagen
    if(fscanf(pgmfile, "%u %u", &(pgm->width), &(pgm->height)) < 0)
        perror("fscanf()");

    // Detectar el maximo nivel de gris
    if(fscanf(pgmfile, "%u", &(pgm->maxValue)) < 0)
        perror("fscanf()");

    // Alocar memoria para la data
    pgm->data = malloc(pgm->height * sizeof(unsigned char *));

    // Almacenar la data en la estructura
    if (pgm->pgmType[1] == '5')
    {
        fgetc(pgmfile);
        for (int i = 0; i < (int)pgm->height; i++)
        {
            pgm->data[i] = malloc(pgm->width * sizeof(unsigned char));

            // Si falla malloc
            if (pgm->data[i] == NULL)
            {
                fprintf(stderr, "Error en malloc()\n");
                exit(EXIT_FAILURE);
            }

            // Se leen los valores y se almacenan en data
            size_t temp = fread(pgm->data[i], sizeof(unsigned char), pgm->width, pgmfile);
            if(temp == 0)
                perror("fread()");
        }
    }

    // Cerrar el archivo
    fclose(pgmfile);
    return true;
}

// Imprimir los detalles de la imagen PGM
void printImageDetails(PGMImage *pgm, const char *filename)
{
    FILE *pgmfile = fopen(filename, "rb");

    // Se checkea la extensión/formato
    char *ext = strrchr(filename, '.');

    printf("\nInformación de la imagen: %s\n", filename);

    if (!ext)
        printf("No se encontró la extensión del archivo %s", filename);
    else
        printf("Format         : %s\n", ext + 1);

    printf("PGM File type  : %s\n", pgm->pgmType);

    // Imprimir el tipo de archivo PGM, ASCII o binario
    if (!strcmp(pgm->pgmType, "P2"))
        printf("PGM File Format: ASCII\n");
    else if (!strcmp(pgm->pgmType, "P5"))
        printf("PGM File Format: Binary\n");

    printf("Width of img   : %d px\n", pgm->width);
    printf("Height of img  : %d px\n", pgm->height);
    printf("Max Gray value : %d\n", pgm->maxValue);

    // Cerrar archivo
    fclose(pgmfile);
}

// Guardar una estructura PGM a un archivo de imagen .pgm
void save_pgm_to_file(Euclides *pgm, const char *filename)
{
    unsigned int temp = 0;
    FILE *pgm_file;
    pgm_file = fopen(filename, "wb");

    fprintf(pgm_file, "%s\n", pgm->pgmType);
    fprintf(pgm_file, "%d %d\n", pgm->width, pgm->height);
    fprintf(pgm_file, "%d\n", pgm->maxValue);

    printf("Escribiendo el archivo: %s\n", filename);
    for (unsigned int i = 0; i < pgm->height; i++)
    {
        printf("\n");
        for (unsigned int j = 0; j < pgm->width; j++)
        {
            temp = (unsigned int) pgm->data[i][j];
            //printf("\t%ld", temp);
            fprintf(pgm_file, "%c", temp);
        }
    }
    printf("\n");
    fclose(pgm_file);
}

// Inicializar los valores de la matriz de distancias euclideanas
void init_euclid(unsigned int euclid_width, unsigned int euclid_height, Euclides *euclid_distances)
{
    strcpy(euclid_distances->pgmType, "P5");
    euclid_distances->width = euclid_width;
    euclid_distances->height = euclid_height;
    euclid_distances->maxValue = 255;
    euclid_distances->data = malloc(euclid_height * sizeof(unsigned long *));
    for(unsigned int i=0; i<euclid_height; i++)
    {
        euclid_distances->data[i] = malloc(euclid_distances->width * sizeof(unsigned long));

        // Si falla malloc
        if (euclid_distances->data[i] == NULL)
        {
            fprintf(stderr, "Error en malloc()\n");
            exit(EXIT_FAILURE);
        }

        //TODO: DEBERIAMOS INICIALIZARLO A ALGUN VALOR ALTO? Tipo 33333 o 55555
    }
}

// Realiza la operación de distancia euclideana entre el pattern y cada posible ventana de la image
void compute_distances(PGMImage *image, PGMImage *pattern, Euclides *euclid_distances, u_int8_t threads)
{
    omp_set_dynamic(0); //Se apaga el seteo dinamico de hilos del sistema operativo
    //printf("threads=%d\n", threads);
    //omp_set_num_threads(threads);
    unsigned int i, j, u, v;
    unsigned long accum = 0;
    //(void) threads;

    printf("\nComenzando a procesar la imagen para encontrar el patrón\n");
    //printf("Cantidad de threads: %d\n", omp_get_num_threads());
    #pragma omp parallel for private(i,j,u,v) num_threads(threads)
    for(i=0; i<euclid_distances->height; i++) //Itera sobre cada fila de la imagen grande
    {
        //printf("Cantidad de threads: %d\n", omp_get_num_threads());
        for(j=0; j<euclid_distances->width; j++) //Itera sobre cada celda de cada fila de la imagen grande
        {
            accum = 0;
            for(u=0; u<pattern->height; u++) //Itera sobre cada fila del patron a encontrar
            {
                for(v=0; v<pattern->width; v++) //Itera sobre cada celda de cada fila del patron a encontrar
                {
                    int temp = (image->data[i+u][j+v] - pattern->data[u][v]); //Se almacena temporalmente el producto de la celdas correspondientes a la matriz image y pattern
                    accum += (unsigned long) (temp * temp); //Se eleva al cuadrado para evitar valores negativos
                }
            }
            if(accum == 0)
            {
                printf("Encontrada distancia 0 en las coords: x=%d y=%d\n", j, i); //Se invierten i j porque i son filas y j columnas
            }
            euclid_distances->data[i][j] = accum; //Se almacena el valor en la estructura de distancias euclideanas
        }
    }
}

int main(int argc, char const *argv[])
{
    PGMImage *image = malloc(sizeof(PGMImage));
    PGMImage *pattern = malloc(sizeof(PGMImage));
    Euclides *euclid_distances = malloc(sizeof(Euclides));
    const char *image_file;
    const char *pattern_file;
    u_int8_t threads = (u_int8_t) atoi(argv[1]);

    if (argc == 4)
    {
        image_file = argv[2];
        pattern_file = argv[3];
    }
    else
    {
        image_file = "./img/II.pgm";
        pattern_file = "./img/TTT.pgm";
    }

    printf("Image file: %s\n", image_file);
    printf("Pattern file: %s\n", pattern_file);

    // Se procesa la imagen y se imprimen los detalles
    if (openPGM(image, image_file))
        printImageDetails(image, image_file);

    if (openPGM(pattern, pattern_file))
        printImageDetails(pattern, pattern_file);

    // Se inicializan los valores de euclid_distances, excepto la data
    const unsigned int euclid_width = (image->width) - (pattern->width) + 1; //Le sumamos 1 para abarcar la ultima ventana
    const unsigned int euclid_height = (image->height) - (pattern->height) + 1;
    init_euclid(euclid_width, euclid_height, euclid_distances);

    compute_distances(image, pattern, euclid_distances, threads);

    //save_pgm_to_file(euclid_distances, "./img/euclid.pgm");

    return 0;
}