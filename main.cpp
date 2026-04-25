#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <string>
#include <vector>
#include <windows.h>
#include <iomanip>


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "cudaFunctionInit.h"
#include "drawing.h"

#include "utils.h"

long double time() {
    return windowsUtils::getHighResolutionTime() / 1000000.0l;
}

class MainGameOfLife {
private:
    VertexBuffer& vertexBuffer; //Не удалять в деструкторе!!
    uint8_t* field; //Не удалять в деструкторе!!
    int BLOCKWIDTH;
    int BLOCKHEIGHT;

public:

    MainGameOfLife(VertexBuffer& vertexBuffer, uint8_t* field, int BLOCKWIDTH, int BLOCKHEIGHT) :
        vertexBuffer(vertexBuffer),
        field(field),
        BLOCKWIDTH(BLOCKWIDTH),
        BLOCKHEIGHT(BLOCKHEIGHT) { }
    
    void drawField() {
        Geometry geometry(vertexBuffer);
        bool bit;

        float cellWidthNDC = 2.0f / (BLOCKWIDTH * 8);
        float cellHeightNDC = -2.0f / BLOCKHEIGHT;

        for (unsigned int y = 0; y < BLOCKHEIGHT; y++) {
            for (unsigned int x = 0; x < BLOCKWIDTH; x++) {
                if (field[y * BLOCKWIDTH + x] == 0) continue;
                for (unsigned int bitIndex = 0; bitIndex < 8; bitIndex++) {

                    if (BitUtils::readBit(&field[y * BLOCKWIDTH + x], bitIndex)) {
                        NDC lowLeft{ (- 1.0f + cellWidthNDC * ((x * 8) + bitIndex + 1)), cellHeightNDC* y + 1.0f};
                        NDC topRight{ ( - 1.0f + cellWidthNDC * ((x * 8) + bitIndex)), cellHeightNDC * (y + 1) + 1.0f};

                        geometry.createRectangle(lowLeft, topRight);
                    }
                }
            }
        }
    }
};

void getKeys(GLFWwindow* window, GameOfLife& gameOfLife) {
    double latency = 0.00;
    double timeNow = glfwGetTime();

    //--------------------------SPACE---------------------------------
    //Следующий этаап генераци с задержкой
    {
        static double lasExecutionTime = 0;
        if ((timeNow > lasExecutionTime + latency)
            && (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)) {

            gameOfLife.nextGeneration();
            lasExecutionTime = glfwGetTime();
        }
    }
    //--------------------------'N'---------------------------------
    //Следующий этап генерации с ожиданием повторного ввода
    {
        static double lasExecutionTime = 0;
        static bool wasPressed = false;

        bool isPressed = (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS);

        if (isPressed && !wasPressed) {

            gameOfLife.nextGeneration();
            lasExecutionTime = glfwGetTime();
        }
        wasPressed = isPressed;
    }
    //--------------------------'C'---------------------------------
    //Создание нового поля
    {
        static double lasExecutionTime = 0;
        static bool wasPressed = false;

        bool isPressed = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);

        if (isPressed && !wasPressed) {

            gameOfLife.cudaGenerateNewField(time() * 1000);
        }
        wasPressed = isPressed;
    }

    //--------------------------'X'---------------------------------
    //Очистка поля
    {
        static double lasExecutionTime = 0;
        static bool wasPressed = false;

        bool isPressed = (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS);

        if (isPressed && !wasPressed) {

            gameOfLife.cudaClearField();
        }
        wasPressed = isPressed;
    }

    //--------------------------LEFT CLICK---------------------------------
    //Изменение состояния клетки
    {
        static double lasExecutionTime = 0;
        static bool wasPressed = false;

        bool isPressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

        if (isPressed && !wasPressed) {
            long double width = gameOfLife.getWidth() * 8;
            long double  height = gameOfLife.getHeight();

            double mouseX = 0, mouseY = 0;

            glfwGetCursorPos(window, &mouseX, &mouseY);

            unsigned int cellX = mouseX * (width / 1920);
            unsigned int cellY =  mouseY * (height / 1080);

            gameOfLife.reverseCell(cellX, cellY);
            lasExecutionTime = glfwGetTime();
        }
        wasPressed = isPressed;
    }
}


int main() {
    //----Инициализация всего говна----
    VertexBuffer vertexBuffer;
    ShaderProgram shaderProgram;
    Mesh mesh;
    InfoDisplay infoDisplay(vertexBuffer);
    Geometry geometry(vertexBuffer);


    GLFW glfw;
 
    infoDisplay.getopenGLInfo();

    unsigned int VBO = mesh.initVBO();
    unsigned int VAO = mesh.initVAO();

    mesh.setupAttributes(vertexBuffer);

    shaderProgram.createShaderProgram();

    GLFWwindow* window = glfw.getWindow();

    constexpr unsigned int gridSize = 3;

    constexpr unsigned int BLOCKHEIGHT = 27 * gridSize;
    constexpr unsigned int BLOCKWIDTH = 6 * gridSize;

    /*
    constexpr unsigned int BLOCKHEIGHT = 1080;
    constexpr unsigned int BLOCKWIDTH = 1920 / 8;
    */

    GameOfLife gameOfLife(BLOCKWIDTH, BLOCKHEIGHT, time());
    
    ConsoleOutput consoleOutput(BLOCKWIDTH, BLOCKHEIGHT);

    /*
    //---cuda init---
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    */

    shaderProgram.loadUniform();

    uint8_t* field = new uint8_t[BLOCKWIDTH * BLOCKHEIGHT];

    MainGameOfLife mainGameOfLife(vertexBuffer, field, BLOCKWIDTH, BLOCKHEIGHT);

    long double startTime = time();
    long double firstTime;
    long double secondTime = startTime;

    int frame = 0;

    while (!glfwWindowShouldClose(window)) {
        vertexBuffer.clear();

        frame++;
        firstTime = time() - startTime;

        //----Запуск генерации и ожидание следующего поля----
        
        getKeys(window, gameOfLife);

        gameOfLife.getField(field);
        mainGameOfLife.drawField();
        //----отображение поля в консоль----


        //consoleOutput.fieldOutput(field);

        infoDisplay.updateTitleInfo();
        infoDisplay.displayInfoWithDelay();

        //input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Загружаем новые данные в VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexBuffer.getVerticesCount() * vertexBuffer.getFloatPerPoint() * sizeof(float), vertexBuffer.getVertices(), GL_DYNAMIC_DRAW);

        glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);

        shaderProgram.useShader();


        glDrawArrays(GL_TRIANGLES, 0, vertexBuffer.getVerticesCount());

        glfwSwapBuffers(window);
        glfwPollEvents();



        /*
        secondTime = time() - startTime;
        consoleOutput.displayTitle(
            "  Windows Flower Test"
            "  time: " + std::to_string(secondTime / 1) +
            "  FPS: " + std::to_string(1 / (secondTime - firstTime)) +
            "  Middle FPS: " + std::to_string(1 / (secondTime / frame)) +
            "  Frame: " + std::to_string(frame));
        */

    }
    glfwTerminate();

    return 0;
}
