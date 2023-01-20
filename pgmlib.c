// C Program to read a PGMB image taken from GEEKSFORGEEKS
// and print its parameters
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure for storing the
// image data
typedef struct PGMImage
{
    char pgmType[3];
    unsigned char *data;
    unsigned int width;
    unsigned int height;
    unsigned int maxValue;
} PGMImage;

// Function to ignore any comments
// in file
void ignoreComments(FILE *fp)
{
    int ch;
    char line[100];

    // Ignore any blank lines
    while ((ch = fgetc(fp)) != EOF && isspace(ch))
        ;

    // Recursively ignore comments
    // in a PGM image commented lines
    // start with a '#'
    if (ch == '#')
    {
        fgets(line, sizeof(line), fp);
        ignoreComments(fp);
    }
    else
        fseek(fp, -1, SEEK_CUR);
}

// Function to open the input a PGM
// file and process it
bool openPGM(PGMImage *pgm,
             const char *filename)
{
    // Open the image file in the
    // 'read binary' mode
    FILE *pgmfile = fopen(filename, "rb");

    // If file does not exist,
    // then return
    if (pgmfile == NULL)
    {
        printf("File does not exist\n");
        return false;
    }

    ignoreComments(pgmfile);
    fscanf(pgmfile, "%s\n",
           pgm->pgmType);

    // Check for correct PGM Binary
    // file type
    if (strcmp(pgm->pgmType, "P5"))
    {
        fprintf(stderr,
                "Wrong file type!\n");
        exit(EXIT_FAILURE);
    }

    ignoreComments(pgmfile);

    // Read the image dimensions
    fscanf(pgmfile, "%d %d\n",
           &(pgm->width),
           &(pgm->height));

    ignoreComments(pgmfile);

    // Read maximum gray value
    fscanf(pgmfile, "%d\n", &(pgm->maxValue));
    ignoreComments(pgmfile);

    // Allocating memory to store
    // img info in defined struct
    pgm->data = (unsigned char *)malloc(pgm->height * pgm->width * sizeof(unsigned char));

    // Storing the pixel info in
    // the struct
    if (pgm->pgmType[1] == '5')
    {

        fgetc(pgmfile);
        fread(pgm->data,
              sizeof(unsigned char),
              pgm->width * pgm->height, pgmfile);
    }

    // Close the file
    fclose(pgmfile);

    return true;
}

// Function to print the file details
void printImageDetails(PGMImage *pgm,
                       const char *filename)
{
    FILE *pgmfile = fopen(filename, "rb");

    // Retrieving the file extension
    const char *ext = strrchr(filename, '.');

    if (!ext)
        printf("No extension found"
               "in file %s",
               filename);
    else
        printf("File format"
               " : %s\n",
               ext + 1);

    printf("PGM File type : %s\n",
           pgm->pgmType);

    // Print type of PGM file, in ascii
    // and binary format
    if (!strcmp(pgm->pgmType, "P2"))
        printf("PGM File Format:"
               "ASCII\n");
    else if (!strcmp(pgm->pgmType,
                     "P5"))
        printf("PGM File Format:"
               " Binary\n");

    printf("Width of img : %d px\n",
           pgm->width);
    printf("Height of img : %d px\n",
           pgm->height);
    printf("Max Gray value : %d\n",
           pgm->maxValue);

    // close file
    fclose(pgmfile);
}

// Driver Code
// int main(int argc, char const *argv[])
// {
//     PGMImage *pgm = malloc(sizeof(PGMImage));
//     const char *ipfile;

//     if (argc == 2)
//         ipfile = argv[1];
//     else
//         ipfile = "gfg_logo.pgm";

//     printf("\tip file : %s\n", ipfile);

//     // Process the image and print
//     // its details
//     if (openPGM(pgm, ipfile))
//         printImageDetails(pgm, ipfile);

//     return 0;
// }
