// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week02" && git tag week02 && git push --tags
//
//  Қазіргі күйі: 5-АПТА — индекс буфері (EBO)
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int WIDTH  = 1280;   // 1-апта, задание 1: было 800
const int HEIGHT = 720;    // 1-апта, задание 1: было 600

// Глобальные переменные состояния (1-апта, задание 3; 2-апта, задания 2-3)
bool g_whiteBackground = false;  // Пробел зажат -> белый фон
bool g_lineLoopMode    = false;  // клавиша 2: GL_LINE_LOOP вместо GL_TRIANGLES
bool g_wireframe       = false;  // клавиша F: glPolygonMode wireframe (W теперь занята скоростью)

// 4-апта: қозғалыс күйі. speed — радиан/СЕКУНД (кадрға емес!)
const float SPEED_DEFAULT  = 1.5f;
const float RADIUS_DEFAULT = 0.4f;
const float SPEED_MIN      = 0.1f;
const float SPEED_MAX      = 10.0f;
const float RADIUS_MIN     = 0.0f;
const float RADIUS_MAX     = 0.6f;   // 0.3 (үшбұрыш жартылай ені) + радиус < 1.0
float g_speed  = SPEED_DEFAULT;
float g_radius = RADIUS_DEFAULT;
float g_angle  = 0.0f;               // орбитадағы бұрыш (радиан)

// ---------------------------------------------------------------------
//  4-АПТА: шейдерлер (uOffset, uScale uniform-дары + түс атрибуты)
// ---------------------------------------------------------------------
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform vec2  uOffset;
uniform float uScale;
out vec3 vColor;
void main() {
    gl_Position = vec4(aPos.xy * uScale + uOffset, aPos.z, 1.0);
    vColor = aColor;
}
)";

const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); }
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
void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // 1-апта, задание 3: пока пробел нажат, фон белый
    g_whiteBackground = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);

    // 2-апта, задание 2: 1 = GL_TRIANGLES, 2 = GL_LINE_LOOP
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) g_lineLoopMode = false;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) g_lineLoopMode = true;

    // 2-апта, задание 3: F переключает wireframe (по одному нажатию)
    // (раньше была W, но в 4-апта W/S управляют скоростью)
    static bool fWasPressed = false;
    bool fPressed = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    if (fPressed && !fWasPressed) {
        g_wireframe = !g_wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);
    }
    fWasPressed = fPressed;

    // 4-апта, 3-тапсырма: W/S — айналу жылдамдығы.
    // Жеделдетуге де dt керек: әйтпесе жылдамдықтың өзгеру жылдамдығы FPS-ке тәуелді.
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) g_speed += 2.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) g_speed -= 2.0f * dt;
    g_speed = std::fmin(std::fmax(g_speed, SPEED_MIN), SPEED_MAX);   // қосымша: clamp

    // Қосымша: Q/E — орбита радиусы (dt-мен, себебі ол да «бірлік/секунд»)
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) g_radius -= 0.5f * dt;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) g_radius += 0.5f * dt;
    g_radius = std::fmin(std::fmax(g_radius, RADIUS_MIN), RADIUS_MAX);

    // Қосымша: R — бәрін бастапқы күйге қайтару
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        g_speed  = SPEED_DEFAULT;
        g_radius = RADIUS_DEFAULT;
        g_angle  = 0.0f;
    }
}

// =====================================================================
//  MAIN
// =====================================================================
int main() {

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
                 "F - wireframe, Пробел - белый фон, Esc - выход\n"
                 "W/S - скорость, Q/E - радиус орбиты, R - сброс\n";


    // === 2-АПТА: үшбұрыштың деректері мен буферлері ===

    // Вершинные данные в NDC (-1 .. 1): x y z  r g b.
    // 5-апта: төртбұрыш = 4 вершина + 6 индекс (6 вершинаның орнына).
    // 0.3f: орбита радиусы (макс. 0.6) + 0.3 * uScale (макс. 1.0) < 1.0 — NDC-ден шықпайды.
    float vertices[] = {
        // позиция          // түс
         0.3f,  0.3f, 0.0f,  1.0f, 0.0f, 0.0f,   // 0 — оң жоғарғы
         0.3f, -0.3f, 0.0f,  0.0f, 1.0f, 0.0f,   // 1 — оң төменгі
        -0.3f, -0.3f, 0.0f,  0.0f, 0.0f, 1.0f,   // 2 — сол төменгі
        -0.3f,  0.3f, 0.0f,  1.0f, 1.0f, 0.0f    // 3 — сол жоғарғы
    };

    // Индекстер вершина НӨМІРЛЕРІН көрсетеді. Диагональ 1–3, барлығы сағат тіліне қарсы.
    unsigned int indices[] = {
        0, 1, 3,    // бірінші үшбұрыш
        1, 2, 3     // екінші үшбұрыш
    };
    const int indexCount    = 6;
    const int triangleCount = indexCount / 3;

    unsigned int vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);                                  // VAO бірінші

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);              // 5-апта: EBO VAO байланған кезде
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    const int STRIDE = 6 * sizeof(float);
    // location=0: позиция (3 float), ығысу 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    // location=1: түс (3 float), ығысу 3 float
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    // НАЗАР: GL_ELEMENT_ARRAY_BUFFER-ды босатпаймыз (VAO ішіндегі EBO жоғалып кетпесін)

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

    // === 4-АПТА: uniform орындарын цикл алдында БІР рет табамыз ===
    int locOffset = glGetUniformLocation(shader, "uOffset");
    int locScale  = glGetUniformLocation(shader, "uScale");
    float lastFrame = (float)glfwGetTime();   // НАЗАР: нөл емес, әйтпесе 1-кадрда dt өте үлкен
    // g_angle, g_speed — жаһандық (processInput-тан R/W/S басқарады)


    // -----------------------------------------------------------------
    //  4. Негізгі цикл
    // -----------------------------------------------------------------
    // Для счётчика FPS (1-апта, задание 4)
    double lastFpsTime = glfwGetTime();
    int    frameCount  = 0;

    while (!glfwWindowShouldClose(window)) {

        // 4-апта: delta time — алдыңғы кадрдан бергі уақыт (секунд)
        float frameNow = (float)glfwGetTime();
        float dt = frameNow - lastFrame;
        lastFrame = frameNow;

        processInput(window, dt);   // проверка клавиш ДО glClear (1-апта, задание 3)

        g_angle += g_speed * dt;    // радиан/сек * сек = радиан (кадр жиілігінен тәуелсіз)

        // --- Экранды тазалау ---
        float t = (float)glfwGetTime();
        // 1-апта, задание 2: частота выросла (0.5 -> 2.0, 0.3 -> 1.2), амплитуда 0.3 прежняя
        float r = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
        float g = (std::sin(t * 1.2f) + 1.0f) * 0.5f * 0.3f;

        if (g_whiteBackground) glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        else                   glClearColor(r, g, 0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // === 2-АПТА: сызу командасы ===
        glUseProgram(shader);   // МІНДЕТТІ, әрі БІРІНШІ: uniform орнатудан бұрын
        // Пульсация: sin (-1..1) -> (0..1) -> 0.5..1.0
        float scale = 0.75f + 0.25f * std::sin(t * 3.0f);
        glUniform1f(locScale, scale);
        glBindVertexArray(vao);

        // 5-апта, 3-тапсырма: бір VAO, екі glUniform2f + glDrawElements.
        // Екінші төртбұрыш орбитаның қарсы жағында (бұрыш + PI).
        for (int k = 0; k < 2; ++k) {
            float a = g_angle + k * 3.14159265f;
            glUniform2f(locOffset, std::cos(a) * g_radius, std::sin(a) * g_radius);

            if (g_lineLoopMode) {
                // GL_LINE_LOOP замыкает ВСЕ индексы в один контур, поэтому
                // рисуем каждый треугольник отдельным вызовом (по 3 индекса).
                for (int i = 0; i < triangleCount; ++i)
                    glDrawElements(GL_LINE_LOOP, 3, GL_UNSIGNED_INT,
                                   (void*)(i * 3 * sizeof(unsigned int)));
            } else {
                // 6 — ИНДЕКС саны (вершина саны емес)
                glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
            }
        }

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
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}