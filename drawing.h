#pragma once
#include "utils.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>

constexpr float SCREENWIDTH = 1920;
constexpr float SCREENHEIGHT = 1080;

const size_t floatPerPoint = 6;

struct NDC {
    float x, y, z = 0.0f;
};

/*------------------------------ВЕРШИНЫ, ИХ ПРЕДСТАВЛЕНИЕ И РАБОТА С НИМИ---------------------------*/
class VertexBuffer {
private:
    std::vector<float> vertices;
public:
    // Ёмкость (количество зарезервированных float) 
    size_t getReservedArrayLength() const {
        return vertices.capacity();
    }

    // Количество float на одну вершину
    size_t getFloatPerPoint() const {
        return floatPerPoint;
    }

    // Количество используемых float
    size_t getUsedArrayLength() const {
        return vertices.size();
    }

    // Указатель на данные
    float* getVertices() {
        return vertices.data();
    }

    const float* getVertices() const {
        return vertices.data();
    }

    // Количество вершин
    size_t getVerticesCount() const {
        return vertices.size() / floatPerPoint;
    }

    // Очистка
    void clear() {
        vertices.clear();
    }

    // Резервирование памяти
    void reserve(size_t newCapacity) {
        if (newCapacity < vertices.size()) {
            std::cerr << "Ошибка: новая ёмкость меньше текущего размера\n";
            return;
        }
        vertices.reserve(newCapacity);
    }

    // Автоматическое увеличение ёмкости
    void fastAddReserved() {
        size_t newCap = vertices.capacity() * 1.5 + 100;
        reserve(newCap);
    }

    // Добавление одной точки (floatPerPoint чисел)
    void addPoint(const float* point) {
        if (!point) return;
        if (vertices.size() + floatPerPoint > vertices.capacity()) {
            fastAddReserved();
        }
        vertices.insert(vertices.end(), point, point + floatPerPoint);
    }

    // Добавление фигуры (vertexCount вершин)
    void addFigure(const float* figure, size_t vertexCount) {
        if (!figure) return;
        size_t elementsToAdd = vertexCount * floatPerPoint;
        if (vertices.size() + elementsToAdd > vertices.capacity()) {
            // Рассчитываем новую ёмкость
            size_t newCap = vertices.capacity();
            while (vertices.size() + elementsToAdd > newCap) {
                newCap = newCap * 1.5 + 100;
            }
            reserve(newCap);
        }
        vertices.insert(vertices.end(), figure, figure + elementsToAdd);
    }

    // Для обратной совместимости со старым именем setReserveLength
    void setReserveLength(size_t newCapacity) {
        reserve(newCapacity);
    }

    void addReserveLength(size_t add) {
        reserve(vertices.capacity() + add);
    }
};
/*-----------------------------ОТОБРАЖЕНИЕ ИНФОРМАЦИИ В КОНСОЛЬ ДЛЯ ОТЛАДКИ-------------------------*/
class InfoDisplay {
protected:
    inline long long getHighResolutionTime() {
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    long double toSeconds(long double microSeconds) {
        return microSeconds / 1000000.0l;
    }
private:
    VertexBuffer& vertexBuffer;

    long double startTime = getTime();       // момент создания объекта
    long double firstTime = getTime() - startTime;  // время первого кадра 
    long double secondTime = startTime;              // текущее время 
    long double lastCycleTime = startTime;           // время последнего вывода информации
    long double outputDelay = 1.0l;                   // задержка между выводами 


public:
    InfoDisplay(VertexBuffer& vertexBuffer) : vertexBuffer(vertexBuffer) {
    }

    long double getTime() {
        return toSeconds(getHighResolutionTime());
    }

    void displayInfoWithDelay() {
        if (getTime() > lastCycleTime + outputDelay) {
            lastCycleTime = getTime();
            getVertexBufferInfo();

        }
    }

    void displayTitle(const std::string& title) {
        std::cout << "\033]0;" << title << "\007";
    }

    void getVertexBufferInfo() {
        std::cout << "Используемый  " << vertexBuffer.getUsedArrayLength();
        std::cout << "  зарезервированный размер массива: " << vertexBuffer.getReservedArrayLength() << std::endl;
    }

    void getopenGLInfo() {
        std::cout << "GLAD initialized successfully!" << std::endl;
        std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    }

    void updateTitleInfo() {
        displayTitle(getInfo()); // отладка
    }
    std::string getInfo() {
        secondTime = getTime() - startTime;
        std::string info =
            " GLFW Test"
            " time: " + std::to_string(secondTime / 1) +
            " FPS: " + std::to_string(1 / (secondTime - firstTime)); // отладка
        firstTime = getTime() - startTime;
        return info;


    }
};
/*----------------------------------------------ШЕЙДЕР----------------------------------------------*/
class ShaderProgram {
public:
    /*                                VARIABLES                                */
    /*/////////////////////////////////////////////////////////////////////////*/
    /*--------------------------vertex Variables Shader------------------------*/
    std::string vertShaderCode;
    const char* vShaderCode;

    std::string vertexCode;
    std::ifstream vShaderFile;
    unsigned int vertexShader;
    /*--------------------------fragmen Variables Shader------------------------*/
    std::string fragShaderCode;
    const char* fShaderCode;

    std::string fragmentCode;
    std::ifstream fShaderFile;
    unsigned int fragmentShader;
    /*--------------------------Shader Variable------------------------*/
    unsigned int shaderProgram;
    /*//////////////////////////////////////////////////////////////////////////*/

    /*--------------------------vertex Shader------------------------*/
    std::string fileReadVertexShader() {
        try {
            vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            vShaderFile.open("shader.vert");
            std::stringstream vShaderStream;
            vShaderStream << vShaderFile.rdbuf();
            vShaderFile.close();
            vertexCode = vShaderStream.str();
        }
        catch (const std::ifstream::failure& e) {
            std::cerr << "ERROR::SHADER::FILE_NOT_READ: " << e.what() << '\n';
            std::cerr << "Could not open shader.vert" << std::endl;
        }
        return  vertexCode;
    }
    void loadVertexShader() {
        vertShaderCode = fileReadVertexShader();
        vShaderCode = vertShaderCode.c_str();
        int success;
        char infoLog[512];
        vertexShader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(vertexShader, 1, &vShaderCode, NULL);
        glCompileShader(vertexShader);

        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
    }

    /*--------------------------fragment Shader------------------------*/

    std::string fileReadFragmentShader() {
        try {
            fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            fShaderFile.open("shader.frag");
            std::stringstream fShaderStream;
            fShaderStream << fShaderFile.rdbuf();
            fShaderFile.close();
            fragmentCode = fShaderStream.str();
        }
        catch (const std::ifstream::failure& e) {
            std::cerr << "ERROR::SHADER::FILE_NOT_READ: " << e.what() << '\n';
            std::cerr << "Could not open shader.frag" << std::endl;
        }
        return fragmentCode;
    }
    void loadFragmentShader() {

        fragShaderCode = fileReadFragmentShader();
        fShaderCode = fragShaderCode.c_str();

        int success;
        char infoLog[512];

        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
        glCompileShader(fragmentShader);

        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
    }
    /*--------------------------common Shader code------------------------*/
    void createShaderProgram() {
        loadVertexShader();
        loadFragmentShader();

        int success;
        char infoLog[512];

        shaderProgram = glCreateProgram();

        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);

        glLinkProgram(shaderProgram);

        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    /*-----------------------------uniforms---------------------------*/

    void loadUniform() {
        int uni_loc = glGetUniformLocation(shaderProgram, "scr_aspect");
        if (uni_loc == -1) {
            std::cerr << "Uniform 'scr_aspect' not found!" << std::endl;
            return;
        }
        glUniform1f(uni_loc, (float)SCREENHEIGHT / SCREENWIDTH);
    }

    /*-------------------------------litarly use shader lol----------------------------------*/
    void useShader() {
        glUseProgram(shaderProgram);
    }
};
/*-------------------------------------------MESH(VAO,VBO)------------------------------------------*/
class Mesh {
private:
    unsigned int VBO;
    unsigned int VAO;
public:

    unsigned int initVAO() {
        //vertex aaray
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        return VAO;
    }

    unsigned int initVBO() {

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        return VBO;
    }

    void setupAttributes(VertexBuffer vertexBuffer) {
        glBufferData(GL_ARRAY_BUFFER, vertexBuffer.getVerticesCount() * vertexBuffer.getFloatPerPoint() * sizeof(float), vertexBuffer.getVertices(), GL_STATIC_DRAW);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        //position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        //color
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
};

class GLFW {
private:
    GLFWwindow* window;

public:

    GLFW(){
        // 1. Инициализация GLFW
        if (!glfwInit()) {
            std::cout << "Failed to initialize GLFW" << std::endl;
            std::exit(-1);
        }

        // 2. Настройка параметров окна
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // 3. Создание окна (ВАЖНО: ДО GLAD!)
        createWindow();

        // 4. Делаем контекст OpenGL текущим
        glfwMakeContextCurrent(window);

        // 5. ТОЛЬКО ТЕПЕРЬ инициализируем GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            std::exit(-1);
        }

    }
    void createWindow() {
        // Создание окн
        window = glfwCreateWindow(SCREENWIDTH, SCREENHEIGHT, "Hello GLFW", NULL, NULL);
        if (!window)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            std::exit(-1);
        }
        glfwMakeContextCurrent(window);
    }
    GLFWwindow* getWindow() {
        return window;
    }
};

class Geometry {
private:
    VertexBuffer& vertexBuffer;

    NDC screenToNDC(POINT point) {
        // Нормализуем от 0 до 1
        float normalizedX = point.x / SCREENWIDTH;
        float normalizedY = point.y / SCREENHEIGHT;

        // Преобразуем в NDC [-1, 1]  инверсией Y
        float ndcX = normalizedX * 2.0f - 1.0f;
        float ndcY = 1.0f - normalizedY * 2.0f;

        return { ndcX, ndcY, 0.0f };
    }
    POINT NDCToScreen(const NDC& ndc, int screenWidth, int screenHeight) {
        // Проверка на выход за пределы NDC
        float clampedX = max(-1.0f, min(1.0f, ndc.x));
        float clampedY = max(-1.0f, min(1.0f, ndc.y));

        // Преобразование из NDC [-1,1] в экранные координаты [0, width/height]
        // Сначала переводим в диапазон [0,1], затем умножаем на размер экрана
        float screenX = (clampedX + 1.0f) * 0.5f * screenWidth;
        float screenY = (1.0f - clampedY) * 0.5f * screenHeight;  // Инверсия Y

        // Округление до ближайшего целого
        long x = static_cast<long>(screenX + 0.5f);
        long y = static_cast<long>(screenY + 0.5f);

        // Клиппинг к границам экрана (0 до width-1, height-1)
        x = max(0L, min(static_cast<long>(screenWidth - 1), x));
        y = max(0L, min(static_cast<long>(screenHeight - 1), y));

        return { static_cast<LONG>(x), static_cast<LONG>(y) };
    };

public:
    Geometry(VertexBuffer& vertexBuffer) : vertexBuffer(vertexBuffer) {
    }

    inline void createRectangle(const NDC& topleft, const NDC& lowright) { // врехне-левая, нижне-правая грань.
        NDC lowleft = { topleft.x, lowright.y };
        NDC topright = { lowright.x, topleft.y };

        createTriangle(lowright, topright, lowleft);
        createTriangle(lowleft, topright, topleft);
    }

    inline void createRectangle(const POINT& topleft, const POINT& lowright) { // врехне-левая, нижне-правая грань.
        createRectangle(screenToNDC(topleft), screenToNDC(lowright));
    }

    inline void createCircle(float radius, const POINT& pos, int sideNumber) {
        // Преобразуем центр экрана в NDC
        NDC center = screenToNDC(pos);
        const float pi = 3.1415926535f;

        for (int i = 0; i < sideNumber; ++i) {
            // Углы текущей и следующей точки
            float angle1 = 2 * pi * i / sideNumber;
            float angle2 = 2 * pi * (i + 1) / sideNumber;

            // Локальные координаты точек на окружности (относительно центра)
            float x1 = radius * cos(angle1);
            float y1 = radius * sin(angle1);
            float x2 = radius * cos(angle2);
            float y2 = radius * sin(angle2);

            // Вершины в NDC
            NDC p1 = { center.x + x1, center.y + y1, 0.5f };
            NDC p2 = { center.x + x2, center.y + y2, 0.5f };

            // Формируем вершины (позиция + атрибуты)
            float triangle[] = {
                center.x, center.y, 0.5f, 0.0f, 0.0f, 0.0f,
                p1.x, p1.y, 0.5f, x1, y1, x1 * y1,
                p2.x, p2.y, 0.5f, x2, y2, x2 * y2
            };

            vertexBuffer.addFigure(triangle, 3);
        }
    }

    inline void createTriangle(const POINT& p1, const POINT& p2, const POINT& p3) {
        // Преобразуем экранные координаты в NDC
        NDC ndc1 = screenToNDC(p1);
        NDC ndc2 = screenToNDC(p2);
        NDC ndc3 = screenToNDC(p3);

        //создание треугольника
        float triangle[] = {
             ndc1.x, ndc1.y, 0.5f, 0.75f, 0.75f, 0.78f,
             ndc2.x, ndc2.y, 0.5f, 0.75f, 0.75f, 0.78f,
             ndc3.x, ndc3.y, 0.5f, 0.75f, 0.75f, 0.78f
        };

        //отправка в буффер
        vertexBuffer.addFigure(triangle, 3);
    }

    inline void createTriangle(const NDC& ndc1, const NDC& ndc2, const NDC& ndc3) {

        //создание треугольника
        float triangle[] = {
             ndc1.x, ndc1.y, 0.5f, 0.75f, 0.75f, 0.78f,
             ndc2.x, ndc2.y, 0.5f, 0.75f, 0.75f, 0.78f,
             ndc3.x, ndc3.y, 0.5f, 0.75f, 0.75f, 0.78f
        };

        //отправка в буффер
        vertexBuffer.addFigure(triangle, 3);
    }
};

