#include "cudaFunctionInit.h"
#include "utils.h"

 __constant__ int8_t neighbour[8][2] = {
    { 1, 1},{ 0, 1},{-1, 1},
    { 1, 0},        {-1, 0},
    { 1,-1},{ 0,-1},{-1,-1}
};

__global__ void initField(uint8_t* dev_dstfield, int width, int height, int seed) {
    unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int totalCells = width * height;

    if (tid < totalCells) {
        // Каждый поток получает УНИКАЛЬНЫЙ seed!

        FastRandom rng(tid ^ (seed * 0x9e3779b9));

        for (int bitIndex = 0; bitIndex < 8; bitIndex++) {
            BitUtils::setBit(&dev_dstfield[tid], bitIndex, rng.nextBool(0.1f));
        }

    }
}

__global__ void clearField(uint8_t* dev_dstfield, int width, int height) {
    unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int totalCells = width * height;

    if (tid < totalCells) {
        // Каждый поток получает УНИКАЛЬНЫЙ seed!

        for (int bitIndex = 0; bitIndex < 8; bitIndex++) {
            BitUtils::setBit(&dev_dstfield[tid], bitIndex, 0);
        }

    }
}

__global__ void nextFieldGeneration(uint8_t* dev_dstField, uint8_t* dev_srcField, int width, int height) {
    unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int totalByteGrids = width * height;

    if (tid < totalByteGrids) {
        bool value;

        for (unsigned int bitIndex = 0; bitIndex < 8; bitIndex++) {

            unsigned int xPos = tid % width * 8 + bitIndex;
            unsigned int yPos = tid / width;

            unsigned int neighbourCount = 0;

            for (int i = 0; i < 8; i++) {
                unsigned int nx = (xPos + neighbour[i][0] + (width * 8)) % (width * 8);
                unsigned int ny = (yPos + neighbour[i][1] + height) % height;


                neighbourCount += BitUtils::readBit(&dev_srcField[ny * width + (nx / 8)], nx % 8);
            }

            bool cellAlive = BitUtils::readBit(&dev_srcField[yPos * width + (xPos / 8)], xPos % 8);

            bool newAlive =
                (cellAlive && (neighbourCount == 2 || neighbourCount == 3)) || //если жива
                (!cellAlive && neighbourCount == 3);//если мертва
            BitUtils::setBit(&dev_dstField[yPos * width + (xPos / 8)], xPos % 8, newAlive);
        }
    }

}

__global__ void devReverseBit(uint8_t* dev_srcField, size_t byteIndex, size_t bitIndex) {
    BitUtils::reverseBit(&dev_srcField[byteIndex], bitIndex);
}

GameOfLife::GameOfLife(unsigned int width, unsigned int height, int seed) : width(width), height(height), seed(seed) {
    // Выделяем память на GPU
    unsigned int length = width * height;
    cudaMalloc(&dev_currField, length * sizeof(uint8_t));
    cudaMalloc(&dev_nextField, length * sizeof(uint8_t));

    cudaMemset(dev_currField, 0, length * sizeof(uint8_t));
    cudaMemset(dev_nextField, 0, length * sizeof(uint8_t));

    cudaGenerateNewField(seed);


}

GameOfLife::~GameOfLife() {
    cudaFree(dev_currField);
    //printf("Память освобождена\n");
}

__host__ unsigned int GameOfLife::getWidth() {
    return width;
}

__host__ unsigned int GameOfLife::getHeight() {
    return height;
}

__host__ void GameOfLife::getField(uint8_t* field) {
    cudaMemcpy(field, dev_currField, width * height, cudaMemcpyDeviceToHost);
}

__host__ void GameOfLife::reverseCell(size_t x, size_t y) {
    // Проверка границ
    if ((x >= width * 8)|| y >= height) {
        return;
    }

    // Вычисляем позицию в битовом массиве
    size_t cellIndex = y * (width * 8) + x;           // индекс клетки
    size_t byteIndex = cellIndex / 8;           // индекс байта
    size_t bitIndex = cellIndex % 8;            // индекс бита в байте

    // Запускаем ядро для инвертирования бита
    devReverseBit << <1, 1 >> > (dev_currField, byteIndex, bitIndex);

    // Желательно добавить синхронизацию, если нужно сразу читать результат
    // cudaDeviceSynchronize();
}

__host__ void GameOfLife::nextGeneration() {
    step++;
    cudaNextGeneration();
}

__host__ void GameOfLife::cudaNextGeneration() {
    unsigned int threads = 256;
    unsigned int blocks = (width * height + threads - 1) / threads;

    nextFieldGeneration << <blocks, threads >> > (dev_nextField, dev_currField, width, height);
    std::swap(dev_currField, dev_nextField);
}

__host__ void GameOfLife::cudaGenerateNewField() {
    unsigned int threads = 256;
    unsigned int blocks = (width * height + threads - 1) / threads;

    int seed = 0;

    initField << <blocks, threads >> > (dev_currField, width, height, seed);
}

__host__ void GameOfLife::cudaGenerateNewField(int seed) {
    int threads = 256;
    int blocks = (width * height + threads - 1) / threads;

    initField << <blocks, threads >> > (dev_currField, width, height, seed);
}

__host__ void GameOfLife::cudaClearField() {
    int threads = 256;
    int blocks = (width * height + threads - 1) / threads;

    clearField << <blocks, threads >> > (dev_currField, width, height);
}

ConsoleOutput::ConsoleOutput(unsigned int width, unsigned int height) : 
    width(width), 
    height(height) { }

__host__ void ConsoleOutput::fieldOutput(uint8_t* field) {
    static std::string outputBuff = "";
    outputBuff.clear();
    bool bit;


    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            for (unsigned int bitIndex = 0; bitIndex < 8; bitIndex++) {
                bit = BitUtils::readBit(&field[y * width + x], bitIndex);
                outputBuff = outputBuff + (bit ? "# " : "  ");
            }
        }
        outputBuff += '\n';
    }
    isFilled += height;

    if (isFilled > (9001)) {
        system("cls");
        isFilled = 0;
    }

    outputBuff = "\033[2J\033[H" + outputBuff;
    std::cout << outputBuff;
}
__host__ void ConsoleOutput::displayTitle(std::string title) {
    std::cout << "\033]0;" << title << "\007";
}
