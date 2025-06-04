#include <iostream>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <omp.h>
#include <chrono>
#include <functional>
#include <cstring> // для memcpy

#pragma pack(push, 1)
struct BMPHeader {
    uint16_t fileType{0x4D42};
    uint32_t fileSize{0};
    uint16_t reserved1{0};
    uint16_t reserved2{0};
    uint32_t offsetData{0};
};

struct BMPInfoHeader {
    uint32_t size{0};
    int32_t width{0};
    int32_t height{0};
    uint16_t planes{1};
    uint16_t bitCount{0};
    uint32_t compression{0};
    uint32_t sizeImage{0};
    int32_t xPixelsPerMeter{0};
    int32_t yPixelsPerMeter{0};
    uint32_t colorsUsed{0};
    uint32_t colorsImportant{0};
};
#pragma pack(pop)

struct Pixel {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
};

class PictureBMP {
public:
    PictureBMP(const std::string &filename);
    ~PictureBMP();
    void Save(const std::string &filename);

    void Rotate90();
    void RotateCounter90();
    void GaussianFilter();

    void Rotate90Sequential();
    void RotateCounter90Sequential();
    void GaussianFilterSequential();

private:
    BMPHeader header;
    BMPInfoHeader infoHeader;
    Pixel* data = nullptr;

    bool Memory(int totalSize);
    void FreeMemory();

    inline int index(int y, int x) const { return y * infoHeader.width + x; }
};

bool PictureBMP::Memory(int totalSize) {
    data = new (std::nothrow) Pixel[totalSize];
    return data != nullptr;
}

void PictureBMP::FreeMemory() {
    delete[] data;
    data = nullptr;
}

PictureBMP::PictureBMP(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("Error open file.");

    file.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (header.fileType != 0x4D42) throw std::runtime_error("File format is not BMP.");

    file.read(reinterpret_cast<char *>(&infoHeader), sizeof(infoHeader));
    infoHeader.height = std::abs(infoHeader.height);
    infoHeader.width = std::abs(infoHeader.width);

    if (infoHeader.width == 0 || infoHeader.height == 0)
        throw std::runtime_error("Invalid image dimensions.");

    file.seekg(header.offsetData, std::ios::beg);

    int totalSize = infoHeader.width * infoHeader.height;
    if (!Memory(totalSize)) throw std::bad_alloc();

    file.read(reinterpret_cast<char *>(data), totalSize * sizeof(Pixel));
    if (!file) throw std::runtime_error("Error reading pixel data.");
}

PictureBMP::~PictureBMP() {
    FreeMemory();
}

void PictureBMP::Save(const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("File save error.");

    file.write(reinterpret_cast<const char *>(&header), sizeof(header));
    file.write(reinterpret_cast<const char *>(&infoHeader), sizeof(infoHeader));

    int totalSize = infoHeader.width * infoHeader.height;
    file.write(reinterpret_cast<const char *>(data), totalSize * sizeof(Pixel));
}

void PictureBMP::Rotate90() {
    int oldWidth = infoHeader.width;
    int oldHeight = infoHeader.height;
    int newSize = oldWidth * oldHeight;

    Pixel* rotatedData = new (std::nothrow) Pixel[newSize];
    if (!rotatedData) {
        throw std::bad_alloc();
    }

#pragma omp parallel for schedule(static, 8)
    for (int y = 0; y < oldHeight; ++y) {
        for (int x = 0; x < oldWidth; ++x) {
            int newY = oldHeight - y - 1;
            rotatedData[x * oldHeight + newY] = data[y * oldWidth + x];
        }
    }

    delete[] data;
    data = rotatedData;
    std::swap(infoHeader.width, infoHeader.height);
}

void PictureBMP::Rotate90Sequential() {
    int oldWidth = infoHeader.width;
    int oldHeight = infoHeader.height;
    int newSize = oldWidth * oldHeight;
    Pixel* rotatedData = new Pixel[newSize];

    for (int y = 0; y < oldHeight; ++y) {
        for (int x = 0; x < oldWidth; ++x) {
            int newY = oldHeight - y - 1;
            rotatedData[x * oldHeight + newY] = data[index(y, x)];
        }
    }

    delete[] data;
    data = rotatedData;
    std::swap(infoHeader.width, infoHeader.height);
}

void PictureBMP::RotateCounter90() {
    int oldWidth = infoHeader.width;
    int oldHeight = infoHeader.height;
    int newSize = oldWidth * oldHeight;
    Pixel* rotatedData = new Pixel[newSize];

    #pragma omp parallel for schedule(static, 8)
    for (int y = 0; y < oldHeight; ++y) {
        for (int x = 0; x < oldWidth; ++x) {
            int newX = oldWidth - x - 1;
            rotatedData[newX * oldHeight + y] = data[index(y, x)];
        }
    }

    delete[] data;
    data = rotatedData;
    std::swap(infoHeader.width, infoHeader.height);
}

void PictureBMP::RotateCounter90Sequential() {
    int oldWidth = infoHeader.width;
    int oldHeight = infoHeader.height;
    int newSize = oldWidth * oldHeight;
    Pixel* rotatedData = new Pixel[newSize];

    for (int y = 0; y < oldHeight; ++y) {
        for (int x = 0; x < oldWidth; ++x) {
            int newX = oldWidth - x - 1;
            rotatedData[newX * oldHeight + y] = data[index(y, x)];
        }
    }

    delete[] data;
    data = rotatedData;
    std::swap(infoHeader.width, infoHeader.height);
}

void PictureBMP::GaussianFilter() {
    const float kernel[3][3] = {
        {1/16.0f, 2/16.0f, 1/16.0f},
        {2/16.0f, 4/16.0f, 2/16.0f},
        {1/16.0f, 2/16.0f, 1/16.0f}
    };

    int width = infoHeader.width;
    int height = infoHeader.height;
    int totalSize = width * height;
    Pixel* tempData = new Pixel[totalSize];

    #pragma omp parallel for schedule(static, 8)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float r = 0, g = 0, b = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int nx = std::clamp(x + kx, 0, width - 1);
                    int ny = std::clamp(y + ky, 0, height - 1);
                    float w = kernel[ky + 1][kx + 1];
                    const Pixel& p = data[ny * width + nx];
                    r += p.red * w;
                    g += p.green * w;
                    b += p.blue * w;
                }
            }
            tempData[y * width + x].red = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
            tempData[y * width + x].green = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
            tempData[y * width + x].blue = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
        }
    }

    delete[] data;
    data = tempData;
}

void PictureBMP::GaussianFilterSequential() {
    const float kernel[3][3] = {
        {1/16.0f, 2/16.0f, 1/16.0f},
        {2/16.0f, 4/16.0f, 2/16.0f},
        {1/16.0f, 2/16.0f, 1/16.0f}
    };

    int width = infoHeader.width;
    int height = infoHeader.height;
    int totalSize = width * height;
    Pixel* tempData = new Pixel[totalSize];

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float r = 0, g = 0, b = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int nx = std::clamp(x + kx, 0, width - 1);
                    int ny = std::clamp(y + ky, 0, height - 1);
                    float w = kernel[ky + 1][kx + 1];
                    const Pixel& p = data[ny * width + nx];
                    r += p.red * w;
                    g += p.green * w;
                    b += p.blue * w;
                }
            }
            tempData[y * width + x].red = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
            tempData[y * width + x].green = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
            tempData[y * width + x].blue = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
        }
    }

    delete[] data;
    data = tempData;
}

// === Вспомогательные функции ===

double measureTime(std::function<void()> func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    try {
        double seq_rotate90 = measureTime([&]() {
            PictureBMP image("input.bmp");
            image.Rotate90Sequential();
            image.Save("output_seq_90.bmp");
        });
        double seq_rotateCounter90 = measureTime([&]() {
            PictureBMP image("input.bmp");
            image.RotateCounter90Sequential();
            image.Save("output_seq_counter90.bmp");
        });
        double seq_gaussian = measureTime([&]() {
            PictureBMP image("input.bmp");
            image.GaussianFilterSequential();
            image.Save("blur_seq.bmp");
        });

        double par_rotate90 = measureTime([&]() {
            PictureBMP image("input.bmp");
            image.Rotate90();
            image.Save("output_par_90.bmp");
        });
        double par_rotateCounter90 = measureTime([&]() {
            PictureBMP image("input.bmp");
            image.RotateCounter90();
            image.Save("output_par_counter90.bmp");
        });
        double par_gaussian = measureTime([&]() {
            PictureBMP image("input.bmp");
            image.GaussianFilter();
            image.Save("blur_par.bmp");
        });

        std::cout << "=== Sequential ===\n";
        std::cout << "Rotate90: " << seq_rotate90 << " ms\n";
        std::cout << "RotateCounter90: " << seq_rotateCounter90 << " ms\n";
        std::cout << "GaussianFilter: " << seq_gaussian << " ms\n";

        std::cout << "=== Parallel ===\n";
        std::cout << "Rotate90: " << par_rotate90 << " ms\n";
        std::cout << "RotateCounter90: " << par_rotateCounter90 << " ms\n";
        std::cout << "GaussianFilter: " << par_gaussian << " ms\n";

        std::cout << "=== Speedup ===\n";
        std::cout << "Rotate90: " << seq_rotate90 / par_rotate90 << "x\n";
        std::cout << "RotateCounter90: " << seq_rotateCounter90 / par_rotateCounter90 << "x\n";
        std::cout << "GaussianFilter: " << seq_gaussian / par_gaussian << "x\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}