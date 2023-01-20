#include <iostream>
#include <cstdlib>
#include <fstream>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " width height" << std::endl;
        return 1;
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    std::ofstream image("image.pgm", std::ios::binary);

    image << "P5" << std::endl;
    image << width << " " << height << std::endl;
    image << "255" << std::endl;

    for (int i = 0; i < width * height; i++)
    {
        image << (unsigned char)(rand() % 256);
    }

    image.close();

    return 0;
}
