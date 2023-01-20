#include <iostream>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <mpi.h>
extern "C"
{
#include "pgmlib.c"
}

#define KERNEL_SIZE 3
#define KERNEL_HALF_SIZE ((KERNEL_SIZE - 1) / 2)

using namespace std;

// Sobel kernels
int Gx[KERNEL_SIZE][KERNEL_SIZE] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
int Gy[KERNEL_SIZE][KERNEL_SIZE] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

int get_pixelVal(unsigned char *first, unsigned char *image, unsigned char *last, int width, int height, int row, int col, int i, int j)
{
    int pixelVal = 0;

    // Check if the pixel is within the bounds of the image

    if (col + j >= 0 && col + j < width)
    {
        if (row + i >= 0 && row + i < height)
        {
            pixelVal = image[(row + i) * width + (col + j)];
        }
        else if ((row + i) == -1 && first != NULL)
        {
            pixelVal = first[col + j];
        }
        else if ((row + i) == height && last != NULL)
        {
            pixelVal = last[col + j];
        }
    }

    return pixelVal;
}

unsigned char *work_image(unsigned char *image, int size, int rank, int width, int height)
{
    // Allocate result Image
    unsigned char *outputImage = (unsigned char *)malloc(sizeof(unsigned char) * height * width);

    // Determine wether there is a first row and last row
    unsigned char *first = image - width;
    unsigned char *last = image + height * width;
    if (rank == 0)
    {
        first = NULL;
    }
    else if (rank == (size - 1))
    {
        last = NULL;
    }

    // Perform edge detection on each row
    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            int gx = 0, gy = 0;
            for (int i = -KERNEL_HALF_SIZE; i <= KERNEL_HALF_SIZE; i++)
            {
                for (int j = -KERNEL_HALF_SIZE; j <= KERNEL_HALF_SIZE; j++)
                {
                    int pixelVal;

                    // get pixel value
                    pixelVal = get_pixelVal(first, image, last, width, height, row, col, i, j);
                    gx += pixelVal * Gx[i + KERNEL_HALF_SIZE][j + KERNEL_HALF_SIZE];
                    gy += pixelVal * Gy[i + KERNEL_HALF_SIZE][j + KERNEL_HALF_SIZE];
                }
            }
            int gVal = round(sqrt(pow(gx, 2) + pow(gy, 2)));
            if (gVal > 255)
            {
                gVal = 255;
            }
            outputImage[row * width + col] = gVal;
        }
    }

    return outputImage;
}

int main(int argc, char *argv[])
{
    // Initialize MPI
    MPI_Init(&argc, &argv);

    // Get the rank and size of the MPI process
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Declare sendcounts, displacements, height, width, work per process, etc...

    int height, width, maxVal, imageSize;
    unsigned char *image = NULL, *outputImage = NULL;
    int *sendcounts = (int *)malloc(sizeof(int) * size), *displs = (int *)malloc(sizeof(int) * size);
    unsigned char *work;

    if (rank == 0)
    {
        // Read in the image file
        if (argc < 2)
        {
            cout << "Usage: mpirun -n [num_processes] sobel [image_file]" << endl;
            return 1;
        }
        const char *inputFile = argv[1];

        PGMImage *pgm = (PGMImage *)malloc(sizeof(PGMImage));
        if (openPGM(pgm, inputFile))
            printImageDetails(pgm, inputFile);
        image = pgm->data;
        height = pgm->height;
        width = pgm->width;
        maxVal = pgm->maxValue;
        imageSize = width * height;
    }

    // Broadcast height and width
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    imageSize = width * height;

    // Define Row type
    MPI_Datatype rtype;
    MPI_Type_contiguous(width, MPI_UNSIGNED_CHAR, &rtype);
    MPI_Type_commit(&rtype);

    // Divide the image into rows for each MPI process
    int rowsPerProcess = height / size;
    int extraRows = height % size;
    int sum = 0;

    // Filling displacements and sendcounts per process
    for (int i = 0; i < size; i++)
    {
        sendcounts[i] = rowsPerProcess;
        if (extraRows > 0)
        {
            sendcounts[i]++;
            extraRows--;
        }
        displs[i] = sum;
        sum += sendcounts[i];
    }

    // Allocate process image space
    work = (unsigned char *)malloc(sizeof(unsigned char) * (sendcounts[rank] + 2) * width);
    work = work + width; // Jump over first line

    MPI_Scatterv(image, sendcounts, displs, rtype, work, sendcounts[rank], rtype, 0, MPI_COMM_WORLD);

    // Communicating upper and lower rows of the work images (Sobel operator kernel)
    MPI_Request req;
    if (rank != 0)
    {
        MPI_Isend(work, 1, rtype, rank - 1, 0, MPI_COMM_WORLD, &req);
        MPI_Recv(work - width, 1, rtype, rank - 1, 0, MPI_COMM_WORLD, 0);
    }

    if (rank != (size - 1))
    {
        MPI_Isend(work + (sendcounts[rank] - 1) * width, 1, rtype, rank + 1, 0, MPI_COMM_WORLD, &req);
        MPI_Recv(work + sendcounts[rank] * width, 1, rtype, rank + 1, 0, MPI_COMM_WORLD, 0);
    }

    // Now everyone has what it takes to work by itself and then gather results
    // -----//
    outputImage = work_image(work, size, rank, width, sendcounts[rank]);
    // -----//

    // Gather the results from each MPI process
    unsigned char *finalImage = NULL;
    if (rank == 0)
    {
        finalImage = (unsigned char *)calloc(imageSize, sizeof(unsigned char));
    }
    MPI_Gatherv(outputImage, sendcounts[rank], rtype, finalImage, sendcounts, displs, rtype, 0, MPI_COMM_WORLD);

    // Write the final image to a file
    if (rank == 0)
    {
        FILE *outFile = fopen("sobel_output.pgm", "wb");
        fprintf(outFile, "P5\n%d %d\n%d\n", width, height, maxVal);
        fwrite(finalImage, sizeof(unsigned char), imageSize, outFile);
        fclose(outFile);
    }

    // Clean up
    work = work - width;
    delete[] image;
    delete[] outputImage;
    delete[] work;
    delete[] sendcounts;
    delete[] displs;
    if (rank == 0)
    {
        delete[] finalImage;
    }

    // Finalize MPI
    MPI_Finalize();

    return 0;
}
