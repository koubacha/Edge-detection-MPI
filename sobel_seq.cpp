#include <iostream>
#include <cstdio>
#include <cstring>
#include <cmath>

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

int get_pixelVal(unsigned char *image, int width, int height, int row, int col, int i, int j)
{
    int pixelVal = 0;

    // Check if the pixel is within the bounds of the image

    if (col + j >= 0 && col + j < width)
    {
        if (row + i >= 0 && row + i < height)
        {
            pixelVal = image[(row + i) * width + (col + j)];
        }
    }

    return pixelVal;
}

unsigned char *work_image(unsigned char *image, int width, int height)
{
    // Allocate result Image
    unsigned char *outputImage = (unsigned char *)malloc(sizeof(unsigned char) * height * width);

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
                    pixelVal = get_pixelVal(image, width, height, row, col, i, j);
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

    // Declare sendcounts, displacements, height, width, work per process, etc...

    int height, width, maxVal, imageSize;
    unsigned char *image = NULL, *outputImage = NULL;

    // Read in the image file
    if (argc < 2)
    {
        cout << "sobel_seq [image_file]" << endl;
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

    // Now everyone has what it takes to work by itself and then gather results
    // -----//
    outputImage = work_image(image, width, height);
    // -----//

    FILE *outFile = fopen("sobel_output.pgm", "wb");
    fprintf(outFile, "P5\n%d %d\n%d\n", width, height, maxVal);
    fwrite(outputImage, sizeof(unsigned char), imageSize, outFile);
    fclose(outFile);

    // Clean up
    delete[] image;
    delete[] outputImage;

    return 0;
}