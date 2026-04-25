#pragma once

#include <iostream>
#include <stdio.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

//#include "utils.h"

#define HANDLE_ERROR(call) do { \
    cudaError_t err = call; \
    if (err != cudaSuccess) { \
        std::cerr << "CUDA Error at " << __FILE__ << ":" << __LINE__ << " - " \
                  << cudaGetErrorString(err) << " (" << err << ")" << std::endl; \
        std::exit(err); \
    } \
} while(0)


class FastRandom {
private:
    uint32_t state;
public:
    __device__ __forceinline__ FastRandom(uint32_t seed) : state(seed) {}  // передаём seed!

    __device__ __forceinline__ uint32_t next() {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }

    __device__ __forceinline__ float nextFloat() {
        return (next() & 0xFFFFFF) / 16777216.0f;
    }

    __device__ __forceinline__ bool nextBool(float probability = 0.5f) {
        return nextFloat() < probability;
    }
};

__global__ void initField(uint8_t* dev_dstfield, int width, int height, int seed);
__global__ void nextFieldGeneration(uint8_t* dev_dstField, uint8_t* dev_srcField, int width, int height);
__global__ void devReverseBit(uint8_t* dev_srcField, size_t byteIndex, size_t bitIndex);

class GameOfLife {
private:
    uint8_t* dev_currField;
    uint8_t* dev_nextField;

    unsigned int width, height;
    unsigned int step = 0;
    int seed;

public:
    GameOfLife(unsigned int width, unsigned int height, int seed);
    ~GameOfLife();


    __host__ unsigned int getWidth();
    __host__ unsigned int getHeight();
    __host__ uint8_t getDevField();
    __host__ void getField(uint8_t* field);
    __host__ void nextGeneration();
    __host__ void reverseCell(size_t byteIndex, size_t bitIndex);
    __host__ void cudaNextGeneration();
    __host__ void cudaGenerateNewField();
    __host__ void cudaClearField();
    __host__ void cudaGenerateNewField(int seed);
};

class ConsoleOutput {
private:
    int width;
    int height;

    int isFilled = 0;
public:

    ConsoleOutput(unsigned int width, unsigned int height);

    __host__ void fieldOutput(uint8_t* field);
    __host__ void displayTitle(std::string title);
};