#pragma once

#include <windows.h>
#include <iostream>
#include <chrono>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace windowsUtils {
    inline void setConsoleFont(const wchar_t* fontName, int fontSize) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_FONT_INFOEX fontInfo;
        fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);

        GetCurrentConsoleFontEx(hConsole, FALSE, &fontInfo);
        wcscpy_s(fontInfo.FaceName, fontName);
        fontInfo.dwFontSize.Y = fontSize;
        fontInfo.dwFontSize.X = fontSize;

        SetCurrentConsoleFontEx(hConsole, FALSE, &fontInfo);
    }

    inline void setConsoleSize(int width, int height) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        COORD bufferSize;
        bufferSize.X = width;
        bufferSize.Y = height;
        SetConsoleScreenBufferSize(hConsole, bufferSize);

        SMALL_RECT windowSize;
        windowSize.Left = 0;
        windowSize.Top = 0;
        windowSize.Right = width - 1;
        windowSize.Bottom = height - 1;
        SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
    }
    inline void setConsoleTitle(std::string& title) {
        std::cout << "\033]0;" << title << "\007";
        std::flush(std::cout);
        SetConsoleTitleA(title.c_str());
    }
    inline POINT getCursorRelativeToConsole() {
        POINT screenPos;
        GetCursorPos(&screenPos);  // 1. Позиция на экране

        // 2. Получаем handle окна консоли
        HWND consoleWindow = GetConsoleWindow();

        // 3. Преобразуем экранные координаты в координаты окна
        ScreenToClient(consoleWindow, &screenPos);

        return screenPos;
    }

    inline void disableWindowResize() {
        HWND consoleWindow = GetConsoleWindow();

        // Убираем рамку изменения размера
        LONG_PTR style = GetWindowLongPtr(consoleWindow, GWL_STYLE);
        style &= ~WS_SIZEBOX;
        style &= ~WS_MAXIMIZEBOX;
        SetWindowLongPtr(consoleWindow, GWL_STYLE, style);

        // Применяем изменения
        SetWindowPos(consoleWindow, NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    inline void setFixedWindowSize(int width, int height) {
        HWND consoleWindow = GetConsoleWindow();

        // 1. Устанавливаем размер окна
        RECT rect;
        GetWindowRect(consoleWindow, &rect);
        MoveWindow(consoleWindow, rect.left, rect.top, width, height, TRUE);

        // 2. Запрещаем изменение размера
        disableWindowResize();

        // 3. Устанавливаем минимальный и максимальный размер одинаковым
        LONG_PTR style = GetWindowLongPtr(consoleWindow, GWL_STYLE);
        style |= WS_MINIMIZEBOX; // Оставляем минимизацию, если нужно
        SetWindowLongPtr(consoleWindow, GWL_STYLE, style);

        // 4. Обновляем
        SetWindowPos(consoleWindow, NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    inline long long getHighResolutionTime() {
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    inline bool isLeftMouseButtonPressed() {
        // Получаем состояние всех кнопок мыши
        return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    }

    // Более полная функция с позицией
    struct MouseEvent {
        bool leftPressed;
        bool leftDown;
        POINT position;
    };

    inline void enableWindowResizing() {
        HWND hConsole = GetConsoleWindow();

        if (hConsole) {
            LONG_PTR style = GetWindowLongPtr(hConsole, GWL_STYLE);
            style |= (WS_SIZEBOX | WS_MAXIMIZEBOX);
            SetWindowLongPtr(hConsole, GWL_STYLE, style);

            SetWindowPos(hConsole, NULL, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }

    inline MouseEvent getMouseState() {
        MouseEvent event;

        // Состояние кнопки (событие нажатия)
        event.leftPressed = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        // Позиция курсора на экране
        GetCursorPos(&event.position);

        return event;
    }
    inline void enableMouseInput() {
        HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
        DWORD mode;
        GetConsoleMode(hInput, &mode);
        mode |= ENABLE_MOUSE_INPUT;
        mode &= ~ENABLE_QUICK_EDIT_MODE;
        SetConsoleMode(hInput, mode);
    }
}

namespace BitUtils {

    __device__ __host__ __forceinline__ void setBit(uint8_t* byte, size_t bitIndex, bool value) {
        bitIndex &= 0x7;  // Ограничиваем 0-7

        uint8_t mask = static_cast<uint8_t>(1u << bitIndex);

        if (value) {
            *byte |= mask;
        }
        else {
            *byte &= ~mask;
        }
    }
    __device__ __host__ __forceinline__ bool readBit(uint8_t* const byte, const size_t bitIndex) {
        //Индекс для битов с ну(о)ля начинается!!
        // строго от 0-7, а то пизда!!!
        return *byte >> bitIndex & 1;
    }

    __device__ __host__ __forceinline__ void reverseBit(uint8_t* byte, const size_t bitIndex) {
        setBit(byte, bitIndex, !readBit(byte, bitIndex));
    }
};