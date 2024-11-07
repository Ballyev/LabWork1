#include "bmp.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <algorithm>

template <typename T>
T clamp (T v, T lo, T hi){
    return std::min(hi, std::max(lo, v));
}
BMP::BMP(const std::string &filename) {
    LoadFromFile(filename);
}

void BMP::LoadFromFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Error opening file.");
    }

    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.fileType != 0x4D42) {
        throw std::runtime_error("File format is not BMP.");
    }

    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    infoHeader.height = std::abs(infoHeader.height);
    infoHeader.width = std::abs(infoHeader.width);

    if (infoHeader.width == 0 || infoHeader.height == 0) {
        throw std::runtime_error("Unacceptable file size.");
    }

    AllocateMemory(infoHeader.width, infoHeader.height);

    file.seekg(header.offsetData, file.beg);

    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            file.read(reinterpret_cast<char*>(&data[i][j]), sizeof(Pixel));
            if (!file) {
                throw std::runtime_error("Error reading file.");
            }
        }
    }

    file.close();
}

void BMP::AllocateMemory(int width, int height) {
    data.resize(height, std::vector<Pixel>(width));
}

void BMP::Save(const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Error saving file.");
    }

    int byteCount = infoHeader.height * infoHeader.width * sizeof(Pixel);
    std::cout << "File " << filename << " uses " << byteCount << " bytes." << std::endl;

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    for (int i = 0; i < infoHeader.height; ++i) {
        file.write(reinterpret_cast<const char*>(data[i].data()), infoHeader.width * sizeof(Pixel));
    }

    file.close();
}

void BMP::Rotate90() {
    std::vector<std::vector<Pixel>> rotatedData(infoHeader.width, std::vector<Pixel>(infoHeader.height));

    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            rotatedData[j][infoHeader.height - i - 1] = data[i][j];
        }
    }

    data = rotatedData;
    std::swap(infoHeader.width, infoHeader.height);
}

void BMP::RotateCounter90() {
    std::vector<std::vector<Pixel>> rotatedData(infoHeader.width, std::vector<Pixel>(infoHeader.height));

    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            rotatedData[infoHeader.width - j - 1][i] = data[i][j];
        }
    }

    data = rotatedData;
    std::swap(infoHeader.width, infoHeader.height);
}

void BMP::GaussianFilter() {
    float kernel[3][3] = {
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f},
        {2 / 16.0f, 4 / 16.0f, 2 / 16.0f},
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f}
    };

    std::vector<std::vector<Pixel>> tempData(infoHeader.height, std::vector<Pixel>(infoHeader.width));
    
    for (int y = 0; y < infoHeader.height; ++y) {
        for (int x = 0; x < infoHeader.width; ++x) {
            float sumRed = 0, sumGreen = 0, sumBlue = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int ny = clamp(y + ky, 0, infoHeader.height - 1);
                    int nx = clamp(x + kx, 0, infoHeader.width - 1);
                    sumRed += data[ny][nx].red * kernel[ky + 1][kx + 1];
                    sumGreen += data[ny][nx].green * kernel[ky + 1][kx + 1];
                    sumBlue += data[ny][nx].blue * kernel[ky + 1][kx + 1];
                }
            }
            tempData[y][x].red = static_cast<unsigned char>(clamp(sumRed, 0.0f, 255.0f));
            tempData[y][x].green = static_cast<unsigned char>(clamp(sumGreen, 0.0f, 255.0f));
            tempData[y][x].blue = static_cast<unsigned char>(clamp(sumBlue, 0.0f, 255.0f));
        }
    }

    data = tempData;
}