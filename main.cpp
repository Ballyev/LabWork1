#include <iostream>
#include "bmp.h"

int main() {
    try {
        BMP image("input.bmp");

        image.Rotate90();
        image.Save("rotated90.bmp");

        image.RotateCounter90();
        image.Save("rotatedCounter90.bmp");

        image.GaussianFilter();
        image.Save("filtered.bmp");
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}