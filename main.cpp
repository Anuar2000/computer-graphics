// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week02" && git tag week02 && git push --tags
//
//  Қазіргі күйі: 2-АПТА — үшбұрыш (VBO + VAO)
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int WIDTH  = 1280;   // 1-апта, задание 1: было 800
const int HEIGHT = 720;    // 1-апта, задание 1: было 600

// Пирамида из треугольников (как на фото): N рядов, закрашены только
// "верхние" (вершиной вверх) треугольники, перевёрнутые остаются пустыми.
int         ROWS       = 5;   // число рядов, спрашиваем у пользователя при запуске
const int   MAX_ROWS   = 200; // верхняя граница, чтобы не завесить программу
const float FILL_COLOR[3] = {0.1f, 0.9f, 0.7f};   // цвет заливки (на фото - чёрный: {0,0,0})
const float LINE_COLOR[3] = {1.0f, 1.0f, 1.0f};   // цвет линий сетки

// Глобальные переменные состояния (1-апта, задание 3; 2-апта, задания 2-3)
bool g_whiteBackground = false;  // Пробел зажат -> белый фон
bool g_lineLoopMode    = false;  // клавиша 2: GL_LINE_LOOP вместо GL_TRIANGLES
bool g_wireframe       = false;  // клавиша W: glPolygonMode wireframe

// ---------------------------------------------------------------------
//  2-АПТА: шейдеры (пока самые простые, в 3-й неделе улучшим)
// ---------------------------------------------------------------------
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
void main() { gl_Position = vec4(aPos, 1.0); }
)";

// Цвет теперь задаётся через uniform uColor (заливка и линии рисуются разными цветами)
const char* fragmentSrc = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() { FragColor = vec4(uColor, 1.0); }
)";

// ---------------------------------------------------------------------
//  Терезе өлшемі өзгергенде шақырылады
// ---------------------------------------------------------------------
void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------
//  Пернетақтаны тексеру. Әр кадрда шақырылады.
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // 1-апта, задание 3: пока пробел нажат, фон белый
    g_whiteBackground = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);

    // 2-апта, задание 2: 1 = GL_TRIANGLES, 2 = GL_LINE_LOOP
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) g_lineLoopMode = false;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) g_lineLoopMode = true;

    // 2-апта, задание 3: W переключает wireframe (по одному нажатию)
    static bool wWasPressed = false;
    bool wPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    if (wPressed && !wWasPressed) {
        g_wireframe = !g_wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);
    }
    wWasPressed = wPressed;
}

// =====================================================================
//  MAIN
// =====================================================================
int main() {

    // -----------------------------------------------------------------
    //  0. Спрашиваем число рядов (1 - обычный треугольник, 2 - два ряда, ...)
    // -----------------------------------------------------------------
    while (true) {
        std::cout << "Сколько рядов треугольников (1-" << MAX_ROWS << ")? ";
        if (std::cin >> ROWS && ROWS >= 1 && ROWS <= MAX_ROWS) break;
        if (std::cin.eof()) return -1;          // ввод закрыт (Ctrl+D)
        std::cin.clear();                        // сбросить ошибку ввода
        std::cin.ignore(10000, '\n');            // выбросить неверную строку
        std::cout << "Введите целое число от 1 до " << MAX_ROWS << ".\n";
    }

    // -----------------------------------------------------------------
    //  1. GLFW-ны іске қосу
    // -----------------------------------------------------------------
    if (!glfwInit()) {
        std::cerr << "GLFW іске қосылмады\n";
        return -1;
    }

    // Қандай OpenGL нұсқасы керек екенін айтамыз.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    // -----------------------------------------------------------------
    //  2. Терезе жасау
    // -----------------------------------------------------------------
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
                                          "Компьютерлік графика",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады. Видеокарта OpenGL 3.3-ті "
                     "қолдамауы мүмкін.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);              // осы терезенің контексі белсенді
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(0);   // 1-апта, задание 4: VSync выключен (было 1), чтобы увидеть реальный FPS

    // -----------------------------------------------------------------
    //  3. GLAD: OpenGL функцияларын жүктеу
    //     Контекст белсенді болғаннан КЕЙІН ғана. Ретін бұзсаң — бәрі құлайды.
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";
    std::cout << "Управление: 1 - GL_TRIANGLES, 2 - GL_LINE_LOOP, "
                 "W - wireframe, Пробел - белый фон, Esc - выход\n";


    // === 2-АПТА: үшбұрыштың деректері мен буферлері ===

    // Вершинные данные в NDC (-1 .. 1). Строим сетку точек P(i, j):
    // ряд i = 0..ROWS (сверху вниз), в ряду i есть i+1 точек (j = 0..i).
    // Размеры подобраны так, чтобы треугольник был равносторонним в окне 1280x720.
    const float topY = 0.8f, botY = -0.8f, halfW = 0.52f;
    auto P = [&](int i, int j, std::vector<float>& out) {
        out.push_back(-halfW * i / ROWS + j * (2.0f * halfW / ROWS));
        out.push_back(topY - i * (topY - botY) / ROWS);
        out.push_back(0.0f);
    };

    std::vector<float> fillVerts;   // только закрашенные (верхние) треугольники
    std::vector<float> lineVerts;   // все треугольники - для линий сетки
    for (int r = 0; r < ROWS; ++r) {
        for (int j = 0; j <= r; ++j) {           // вершиной вверх
            P(r, j, fillVerts);  P(r + 1, j, fillVerts);  P(r + 1, j + 1, fillVerts);
            P(r, j, lineVerts);  P(r + 1, j, lineVerts);  P(r + 1, j + 1, lineVerts);
        }
        for (int j = 0; j < r; ++j) {            // перевёрнутые (не закрашиваем)
            P(r, j, lineVerts);  P(r, j + 1, lineVerts);  P(r + 1, j + 1, lineVerts);
        }
    }
    const int fillCount = (int)fillVerts.size() / 3;   // 15 треугольников * 3
    const int lineCount = (int)lineVerts.size() / 3;   // 25 треугольников * 3

    // Два набора VAO/VBO: [0] - заливка, [1] - линии
    unsigned int vao[2], vbo[2];
    glGenVertexArrays(2, vao);
    glGenBuffers(2, vbo);
    std::vector<float>* data[2] = { &fillVerts, &lineVerts };
    for (int k = 0; k < 2; ++k) {
        glBindVertexArray(vao[k]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[k]);
        glBufferData(GL_ARRAY_BUFFER, data[k]->size() * sizeof(float),
                     data[k]->data(), GL_STATIC_DRAW);
        // location=0, 3 float, нормаланбаған, қадам 3 float, ығысу 0
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }
    glBindVertexArray(0);

    // === 3-АПТА: (уақытша) шейдерлер компиляциясы ===

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, nullptr);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentSrc, nullptr);
    glCompileShader(fs);

    unsigned int shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);
    int colorLoc = glGetUniformLocation(shader, "uColor");


    // -----------------------------------------------------------------
    //  4. Негізгі цикл
    // -----------------------------------------------------------------
    // Для счётчика FPS (1-апта, задание 4)
    double lastFpsTime = glfwGetTime();
    int    frameCount  = 0;

    while (!glfwWindowShouldClose(window)) {

        processInput(window);   // проверка клавиш ДО glClear (1-апта, задание 3)

        // --- Экранды тазалау ---
        float t = (float)glfwGetTime();
        // 1-апта, задание 2: частота выросла (0.5 -> 2.0, 0.3 -> 1.2), амплитуда 0.3 прежняя
        float r = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
        float g = (std::sin(t * 1.2f) + 1.0f) * 0.5f * 0.3f;

        if (g_whiteBackground) glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        else                   glClearColor(r, g, 0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // === 2-АПТА: сызу командасы ===
        glUseProgram(shader);

        // 1) заливка закрашенных треугольников
        glUniform3f(colorLoc, FILL_COLOR[0], FILL_COLOR[1], FILL_COLOR[2]);
        glBindVertexArray(vao[0]);
        if (g_lineLoopMode) {
            for (int i = 0; i < fillCount / 3; ++i)
                glDrawArrays(GL_LINE_LOOP, i * 3, 3);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, fillCount);
        }

        // 2) линии сетки поверх (контур каждого из 25 треугольников)
        glUniform3f(colorLoc, LINE_COLOR[0], LINE_COLOR[1], LINE_COLOR[2]);
        glBindVertexArray(vao[1]);
        for (int i = 0; i < lineCount / 3; ++i)
            glDrawArrays(GL_LINE_LOOP, i * 3, 3);

        glfwSwapBuffers(window);   // дайын кадрды экранға шығару
        glfwPollEvents();          // пернетақта/тінтуір оқиғаларын өңдеу

        // FPS: выводим раз в секунду (1-апта, задание 4)
        ++frameCount;
        double now = glfwGetTime();
        if (now - lastFpsTime >= 1.0) {
            std::cout << "FPS: " << frameCount / (now - lastFpsTime) << "\n";
            frameCount  = 0;
            lastFpsTime = now;
        }
    }

    // -----------------------------------------------------------------
    //  5. Тазалау
    // -----------------------------------------------------------------
    // === 2-АПТА: буферлер өшіріледі ===
    glDeleteVertexArrays(2, vao);
    glDeleteBuffers(2, vbo);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}