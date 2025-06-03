#include <iostream>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <omp.h>
#include <chrono>
#include <functional>

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
    Pixel** data;
    bool Memory(int height, int width);
    void FreeMemory(int height);
};

bool PictureBMP::Memory(int height,int width) {
    data = new (std::nothrow) Pixel*[height];
    if (!data) return false;

    for (int i = 0; i < height; ++i) {
        data[i] = new (std::nothrow) Pixel[width];
        if (!data[i]) {
            for (int j = 0; j < i; ++j) {
                delete[] data[j];
            }
            delete[] data;
            data = nullptr;
            return false;
        }
    }
    return true;
}

void PictureBMP::FreeMemory(int height) {
    if (data) {
        for (int i = 0; i < height; ++i) {
            delete[] data[i];
        }
        delete[] data;
        data = nullptr;
    }
}

PictureBMP::PictureBMP(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("Error open file.");
    file.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (header.fileType != 0x4D42) throw std::runtime_error("File format is not BMP.");
    file.read(reinterpret_cast<char *>(&infoHeader), sizeof(infoHeader));
    infoHeader.height = std::abs(infoHeader.height);
    infoHeader.width = std::abs(infoHeader.width);
    if (infoHeader.width == 0 || infoHeader.height == 0) throw std::runtime_error("unexceptable file size.");
    file.seekg(header.offsetData, file.beg);

    Memory(infoHeader.height, infoHeader.width);

    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            file.read(reinterpret_cast<char *>(&data[i][j]), sizeof(Pixel));
            if (!file) throw std::runtime_error("Error file read.");
        }
    }

    file.close();
}

PictureBMP::~PictureBMP() {
    FreeMemory(infoHeader.height);
}

void PictureBMP::Save(const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("File save error.");

    file.write(reinterpret_cast<const char *>(&header), sizeof(header));
    file.write(reinterpret_cast<const char *>(&infoHeader), sizeof(infoHeader));

    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            file.write(reinterpret_cast<const char *>(&data[i][j]), sizeof(Pixel));
        }
    }

    file.close();
}

void PictureBMP::Rotate90Sequential() {
    Pixel** rotatedData = new Pixel*[infoHeader.width];
    for (int i = 0; i < infoHeader.width; ++i) {
        rotatedData[i] = new Pixel[infoHeader.height];
    }

    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            rotatedData[j][infoHeader.height - i - 1] = data[i][j];
        }
    }

    FreeMemory(infoHeader.height);
    data = rotatedData;
    std::swap(infoHeader.width, infoHeader.height);
}

void PictureBMP::Rotate90() {
    Pixel** rotatedData = new Pixel*[infoHeader.width];
    for (int i = 0; i < infoHeader.width; ++i) {
        rotatedData[i] = new Pixel[infoHeader.height];
    }

    #pragma omp parallel for
    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            rotatedData[j][infoHeader.height - i - 1] = data[i][j];
        }
    }

    FreeMemory(infoHeader.height);

    data = rotatedData;
    std::swap(infoHeader.width, infoHeader.height);
}
void PictureBMP::RotateCounter90Sequential() {
    Pixel** rotatedData = new Pixel*[infoHeader.width];
    for (int i = 0; i < infoHeader.width; ++i) {
        rotatedData[i] = new Pixel[infoHeader.height];
    }

    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            rotatedData[infoHeader.width - j - 1][i] = data[i][j];
        }
    }

    FreeMemory(infoHeader.height);
    data = rotatedData;
    std::swap(infoHeader.width, infoHeader.height);
}

void PictureBMP::RotateCounter90() {
    Pixel** rotatedData = new Pixel*[infoHeader.width];
    for (int i = 0; i < infoHeader.width; ++i) {
        rotatedData[i] = new Pixel[infoHeader.height];
    }

    #pragma omp parallel for
    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            rotatedData[infoHeader.width - j - 1][i] = data[i][j];
        }
    }

    FreeMemory(infoHeader.height);

    data = rotatedData;
    std::swap(infoHeader.width, infoHeader.height);
}
void PictureBMP::GaussianFilterSequential() {
    const float kernel[3][3] = {
        {1/16.0f, 2/16.0f, 1/16.0f},
        {2/16.0f, 4/16.0f, 2/16.0f},
        {1/16.0f, 2/16.0f, 1/16.0f}
    };

    Pixel** tempData = new Pixel*[infoHeader.height];
    for (int i = 0; i < infoHeader.height; ++i) {
        tempData[i] = new Pixel[infoHeader.width];
    }

    for (int y = 0; y < infoHeader.height; ++y) {
        for (int x = 0; x < infoHeader.width; ++x) {
            float sumRed = 0, sumGreen = 0, sumBlue = 0;

            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int nx = std::clamp(x + kx, 0, infoHeader.width - 1);
                    int ny = std::clamp(y + ky, 0, infoHeader.height - 1);

                    float weight = kernel[ky + 1][kx + 1];
                    sumRed   += data[ny][nx].red * weight;
                    sumGreen += data[ny][nx].green * weight;
                    sumBlue  += data[ny][nx].blue * weight;
                }
            }

            tempData[y][x] = {
                static_cast<uint8_t>(std::clamp(sumBlue, 0.0f, 255.0f)),
                static_cast<uint8_t>(std::clamp(sumGreen, 0.0f, 255.0f)),
                static_cast<uint8_t>(std::clamp(sumRed, 0.0f, 255.0f))
            };
        }
    }

    FreeMemory(infoHeader.height);
    data = tempData;
}

void PictureBMP::GaussianFilter() {
    float kernel[3][3] = {
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f},
        {2 / 16.0f, 4 / 16.0f, 2 / 16.0f},
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f}
    };

    Pixel** tempData = new Pixel*[infoHeader.height];
    for (int i = 0; i < infoHeader.height; ++i) {
        tempData[i] = new Pixel[infoHeader.width];
    }

    #pragma omp parallel for
    for (int y = 0; y < infoHeader.height; ++y) {
        for (int x = 0; x < infoHeader.width; ++x) {
            float sumRed = 0, sumGreen = 0, sumBlue = 0;

            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int nx = x + kx;
                    int ny = y + ky;

                    if (nx < 0) nx = 0;
                    if (nx >= infoHeader.width) nx = infoHeader.width - 1;
                    if (ny < 0) ny = 0;
                    if (ny >= infoHeader.height) ny = infoHeader.height - 1;

                    float weight = kernel[ky + 1][kx + 1];
                    sumRed += data[ny][nx].red * weight;
                    sumGreen += data[ny][nx].green * weight;
                    sumBlue += data[ny][nx].blue * weight;
                }
            }

            tempData[y][x].red = static_cast<uint8_t>(std::clamp(sumRed, 0.0f, 255.0f));
            tempData[y][x].green = static_cast<uint8_t>(std::clamp(sumGreen, 0.0f, 255.0f));
            tempData[y][x].blue = static_cast<uint8_t>(std::clamp(sumBlue, 0.0f, 255.0f));
        }
    }

    for (int y = 0; y < infoHeader.height; ++y) {
        for (int x = 0; x < infoHeader.width; ++x) {
            data[y][x] = tempData[y][x];
        }
    }

    for (int i = 0; i < infoHeader.height; ++i) {
        delete[] tempData[i];
    }
    delete[] tempData;
}


double measureTime(std::function<void()> func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}
int main() {
    try {
        // Тестируем последовательные версии
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

        // Тестируем параллельные версии
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

        // Вывод результатов
        std::cout << "=== Sequential ===\n";
        std::cout << "Rotate90: " << seq_rotate90 << " ms\n";
        std::cout << "RotateCounter90: " << seq_rotateCounter90 << " ms\n";
        std::cout << "GaussianFilter: " << seq_gaussian << " ms\n\n";

        std::cout << "=== Parallel ===\n";
        std::cout << "Rotate90: " << par_rotate90 << " ms\n";
        std::cout << "RotateCounter90: " << par_rotateCounter90 << " ms\n";
        std::cout << "GaussianFilter: " << par_gaussian << " ms\n\n";

        std::cout << "=== Speedup ===\n";
        std::cout << "Rotate90: " << seq_rotate90/par_rotate90 << "x\n";
        std::cout << "RotateCounter90: " << seq_rotateCounter90/par_rotateCounter90 << "x\n";
        std::cout << "GaussianFilter: " << seq_gaussian/par_gaussian << "x\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}