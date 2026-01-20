// 3DTD.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>
#include <Windows.h> // для Windows API

// Глобальная переменная для отслеживания завершения
std::atomic<bool> g_stopProgram(false);

// Обработчик Ctrl+C в консоли
void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        g_stopProgram = true;
        std::cout << "\nInterrupt signal received. Stopping...\n";
    }
}

// Функция для проверки нажатия ESC в консоли (Windows специфичная)
bool isConsoleEscapePressed()
{
#ifdef _WIN32
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
    {
        return true;
    }
#endif
    return false;
}

void run_3D_gpu_stress()
{
    // Устанавливаем обработчик сигналов
    std::signal(SIGINT, signalHandler);

    // Проверяем ESC в консоли перед запуском
    std::cout << "========================================\n";
    std::cout << "GPU Stress Test - Load Level Selection\n";
    std::cout << "========================================\n";
    std::cout << "Press ESC anytime to exit (in window or console)\n";
    std::cout << "1 - Light (50 iterations)\n";
    std::cout << "2 - Medium (100 iterations) (will be in the table)\n";
    std::cout << "3 - High (200 iterations)\n";
    std::cout << "4 - Very High (400 iterations)\n";
    std::cout << "5 - Maximum (800 iterations)\n";
    std::cout << "========================================\n";
    std::cout << "Enter number 1-5: ";

    // Проверка ESC во время ввода
    for (int i = 0; i < 100; i++)
    {
        if (isConsoleEscapePressed())
        {
            std::cout << "\nESC pressed in console. Exiting...\n";
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    int loadLevel;
    std::cin >> loadLevel;

    // Проверка нажатия ESC после ввода
    if (g_stopProgram || isConsoleEscapePressed())
    {
        std::cout << "\nProgram terminated by user.\n";
        return;
    }

    int iterations;
    std::string levelName;

    // Обработка выбора пользователя через switch-case
    switch (loadLevel)
    {
    case 1:
        iterations = 50;
        levelName = "Light (50 iterations)";
        break;
    case 2:
        iterations = 100;
        levelName = "Medium (100 iterations)";
        break;
    case 3:
        iterations = 200;
        levelName = "High (200 iterations)";
        break;
    case 4:
        iterations = 400;
        levelName = "Very High (400 iterations)";
        break;
    case 5:
        iterations = 800;
        levelName = "Maximum (800 iterations)";
        break;
    default:
        std::cout << "Invalid input! Using default: Level 3 (200 iterations)\n";
        iterations = 200;
        levelName = "High (200 iterations)";
        break;
    }

    std::cout << "\nSelected level: " << levelName << "\n";
    std::cout << "Starting test in 2 seconds...\n";
    std::cout << "Press ESC in window or console to exit immediately.\n\n";

    // Пауза перед запуском с проверкой ESC
    for (int i = 0; i < 200; i++)
    { // 2 секунды
        if (g_stopProgram || isConsoleEscapePressed())
        {
            std::cout << "\nStart cancelled by user.\n";
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Инициализация GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to init GLFW\n";
        return;
    }

    // Настройка OpenGL контекста
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Создание окна
    GLFWwindow *window = glfwCreateWindow(800, 600, "GPU Stress 3D Test", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // Выключение VSync для максимальной нагрузки

    // Инициализация GLAD
    if (!gladLoadGL())
    {
        std::cerr << "Failed to init GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }

    // Создание простого fullscreen quad
    float quad[] = {
        -1.f, -1.f,
        1.f, -1.f,
        -1.f, 1.f,
        1.f, 1.f};

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // Вершинный шейдер
    const char *vertexShaderSrc = R"(
    #version 330 core
    layout(location = 0) in vec2 pos;
    out vec2 uv;
    void main() {
        uv = pos*0.5 + 0.5;
        gl_Position = vec4(pos,0,1);
    }
    )";

    // Динамическое создание фрагментного шейдера с выбранным количеством итераций
    std::string fragmentShaderBase = R"(
    #version 330 core
    in vec2 uv;
    out vec4 color;
    uniform float time;
    void main() {
        vec2 p = uv*10.0;
        float acc=0.0;
        for(int i=0;i<)";

    std::string fragmentShaderEnd = R"(;i++){
            p=vec2(sin(p.y+acc), cos(p.x-acc));
            acc += length(p);
        }
        float c=sin(acc);
        color = vec4(c,c*0.5,1.0-c,1.0);
    }
    )";

    std::string fragmentShaderSrcStr = fragmentShaderBase + std::to_string(iterations) + fragmentShaderEnd;
    const char *fragmentShaderSrc = fragmentShaderSrcStr.c_str();

    // Лямбда-функция для компиляции шейдеров
    auto compileShader = [](GLenum type, const char *src)
    {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);

        // Проверка успешности компиляции
        GLint success;
        glGetShaderiv(s, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(s, 512, nullptr, infoLog);
            std::cerr << "Shader compilation error:\n"
                      << infoLog << "\n";
        }

        return s;
    };

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    // Проверка линковки программы
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Program linking error:\n"
                  << infoLog << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    glUseProgram(program);

    GLint timeLoc = glGetUniformLocation(program, "time");

    // Инициализация переменных для подсчета FPS
    auto lastTime = std::chrono::high_resolution_clock::now();
    int frames = 0;

    std::cout << "GPU stress test running. Level: " << levelName << "\n";
    std::cout << "Press ESC in window or console to exit.\n";

    // Главный цикл рендеринга
    while (!glfwWindowShouldClose(window) && !g_stopProgram)
    {
        // Проверка ESC в окне
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            std::cout << "\nESC pressed in window. Stopping...\n";
            break;
        }

        // Проверка ESC в консоли (периодически)
        static int consoleCheckCounter = 0;
        consoleCheckCounter++;
        if (consoleCheckCounter >= 10)
        { // Проверяем каждые ~10 кадров
            consoleCheckCounter = 0;
            if (isConsoleEscapePressed())
            {
                std::cout << "\nESC pressed in console. Stopping...\n";
                break;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
        float t = (float)glfwGetTime();
        glUniform1f(timeLoc, t);

        // 50-кратный overdraw для создания нагрузки
        for (int i = 0; i < 50; i++)
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // Подсчет и вывод FPS каждую секунду
        frames++;
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - lastTime).count();
        if (elapsed >= 1.0)
        {
            std::cout << "\rFPS: " << frames << "       " << std::flush;
            frames = 0;
            lastTime = now;
        }
    }

    std::cout << "\nExiting GPU stress test.\n";

    // Очистка ресурсов
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glfwDestroyWindow(window);
    glfwTerminate();
}