// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week07" && git tag week07 && git push --tags
//
//  Қазіргі күйі: 7-АПТА — координаттар кеңістіктері, MVP, проекция
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

// 6-апта: GLM
#include <glm/glm.hpp>                    // mat4, vec3 типтері
#include <glm/gtc/matrix_transform.hpp>   // translate, rotate, scale, perspective, ortho, lookAt
#include <glm/gtc/type_ptr.hpp>           // value_ptr

#include <cmath>
#include <cstdio>
#include <iostream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
// 7-апта: терезе өлшемі енді жаһандық айнымалы — aspect осыдан есептеледі
int gWidth  = 1280;   // 1-апта, задание 1: было 800
int gHeight = 720;    // 1-апта, задание 1: было 600

bool g_whiteBackground = false;  // Пробел басылып тұрса — ақ фон
bool g_wireframe       = false;  // 7-апта, қосымша: Tab — wireframe (бұрын F)

// 4-апта: айналу жылдамдығы — радиан/СЕКУНД (кадрға емес!)
const float SPEED_DEFAULT = 1.0f;
const float SPEED_MIN     = 0.1f;
const float SPEED_MAX     = 10.0f;
float g_speed = SPEED_DEFAULT;
float g_angle = 0.0f;                // кубтың бұрышы (радиан)

// 7-апта, 1-тапсырма: проекция параметрлері (1, 2, 3 пернелері кезекпен ауыстырады)
const float FOV_LIST[4]  = { 30.0f, 45.0f, 90.0f, 120.0f };
const float NEAR_LIST[4] = { 0.1f, 1.0f, 2.5f, 4.0f };
const float FAR_LIST[3]  = { 100.0f, 5.0f, 2.0f };
int g_fovIndex  = 1;     // 45°
int g_nearIndex = 0;     // 0.1
int g_farIndex  = 0;     // 100.0
float g_fov = 45.0f;     // градус; тінтуір дөңгелегі де осыны өзгертеді

bool g_ortho     = false;  // 7-апта, 2-тапсырма: O — ортографиялық проекция
bool g_threeCubes = false; // 7-апта, 3-тапсырма: T — үш куб
bool g_twoAxes   = false;  // 7-апта, қосымша: X — X пен Y осінен бірге айналу
bool g_pulse     = false;  // 7-апта, қосымша: P — масштаб пульсациясы

// ---------------------------------------------------------------------
//  7-АПТА: шейдерлер. Бір uTransform орнына үш матрица — M, V, P
// ---------------------------------------------------------------------
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
out vec3 vColor;
void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
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
void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    gWidth  = width;     // 7-апта: aspect келесі кадрда жаңа өлшеммен есептеледі
    gHeight = height;
    glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------
//  7-апта, қосымша: тінтуір дөңгелегі fov-ты 10°–90° аралығында өзгертеді
// ---------------------------------------------------------------------
void scroll_callback(GLFWwindow*, double xoffset, double yoffset) {  // yoffset: дөңгелек бағыты (+1 / -1)
    g_fov -= (float)yoffset * 2.0f;
    if (g_fov < 10.0f) g_fov = 10.0f;
    if (g_fov > 90.0f) g_fov = 90.0f;
}

// ---------------------------------------------------------------------
//  Перне бір рет басылғанын тексеру (басып тұрғанда қайталанбайды)
// ---------------------------------------------------------------------
bool keyPressedOnce(GLFWwindow* window, int key, bool& wasPressed) {  // wasPressed: алдыңғы кадрдағы күй
    bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
    bool once = pressed && !wasPressed;
    wasPressed = pressed;
    return once;
}

// ---------------------------------------------------------------------
//  Пернетақтаны тексеру. Әр кадрда шақырылады.
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // 1-апта: пробел басылып тұрғанда фон ақ
    g_whiteBackground = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);

    // 7-апта, қосымша: Tab — wireframe, артқы қырлар алдыңғыларының алдында сызылғаны көрінеді
    static bool tabWas = false;
    if (keyPressedOnce(window, GLFW_KEY_TAB, tabWas)) {
        g_wireframe = !g_wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);
    }

    // 4-апта: W/S — айналу жылдамдығы (жеделдетуге де dt керек)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) g_speed += 2.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) g_speed -= 2.0f * dt;
    g_speed = std::fmin(std::fmax(g_speed, SPEED_MIN), SPEED_MAX);

    // 7-апта, 1-тапсырма: 1 — fov, 2 — near, 3 — far мәндерін кезекпен ауыстыру
    static bool k1 = false, k2 = false, k3 = false;
    if (keyPressedOnce(window, GLFW_KEY_1, k1)) {
        g_fovIndex = (g_fovIndex + 1) % 4;
        g_fov = FOV_LIST[g_fovIndex];
    }
    if (keyPressedOnce(window, GLFW_KEY_2, k2)) g_nearIndex = (g_nearIndex + 1) % 4;
    if (keyPressedOnce(window, GLFW_KEY_3, k3)) g_farIndex  = (g_farIndex + 1) % 3;

    // 7-апта, 2-тапсырма: O — перспектива / ортография
    static bool oWas = false;
    if (keyPressedOnce(window, GLFW_KEY_O, oWas)) g_ortho = !g_ortho;

    // 7-апта, 3-тапсырма: T — бір куб / үш куб
    static bool tWas = false;
    if (keyPressedOnce(window, GLFW_KEY_T, tWas)) g_threeCubes = !g_threeCubes;

    // 7-апта, қосымша: X — екі ось, P — пульсация
    static bool xWas = false, pWas = false;
    if (keyPressedOnce(window, GLFW_KEY_X, xWas)) g_twoAxes = !g_twoAxes;
    if (keyPressedOnce(window, GLFW_KEY_P, pWas)) g_pulse   = !g_pulse;

    // R — бәрін бастапқы күйге қайтару
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        g_speed = SPEED_DEFAULT;
        g_angle = 0.0f;
        g_fovIndex = 1;  g_fov = 45.0f;
        g_nearIndex = 0;
        g_farIndex = 0;
        g_ortho = false;
        g_threeCubes = false;
        g_twoAxes = false;
        g_pulse = false;
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    // -----------------------------------------------------------------
    //  2. Терезе жасау
    // -----------------------------------------------------------------
    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight,
                                          "7-апта: координаттар кеңістіктері, MVP",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады. Видеокарта OpenGL 3.3-ті "
                     "қолдамауы мүмкін.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);   // 7-апта, қосымша: дөңгелекпен fov
    glfwSwapInterval(0);   // 1-апта, задание 4: VSync өшірулі

    // -----------------------------------------------------------------
    //  3. GLAD: OpenGL функцияларын жүктеу (контекст белсенді болғаннан КЕЙІН)
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    // Терезе жасалғанда нақты framebuffer өлшемін аламыз (масштабы 100%-дан өзгеше экранда басқа болуы мүмкін)
    glfwGetFramebufferSize(window, &gWidth, &gHeight);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";
    std::cout << "Басқару: 1 - fov, 2 - near, 3 - far, O - ортография, T - үш куб,\n"
                 "         Tab - wireframe, X - екі ось, P - пульсация, дөңгелек - fov,\n"
                 "         W/S - жылдамдық, R - бастапқы күй, Пробел - ақ фон, Esc - шығу\n";


    // === 7-АПТА: куб — 36 вершина, әрқайсысы 6 float (позиция + түс) ===

    // Әр қырдың өз түсі бар, сондықтан қырлардың шекарасы көрінеді.
    // Әр қыр — 2 үшбұрыш, сырттан қарағанда сағат тіліне қарсы (CCW) — 8-аптада culling үшін керек.
    float vertices[] = {
        // артқы қыр (z = -0.5) — қызыл
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,

        // алдыңғы қыр (z = +0.5) — жасыл
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,

        // сол қыр (x = -0.5) — көк
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,

        // оң қыр (x = +0.5) — сары
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,

        // астыңғы қыр (y = -0.5) — күлгін
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,

        // үстіңгі қыр (y = +0.5) — көгілдір
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f
    };

    // 7-апта: EBO жоқ — куб бұрышы үш қырға ортақ, ал үш қырдың түсі әртүрлі
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);                                  // VAO бірінші

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    const int STRIDE = 6 * sizeof(float);
    // location=0: позиция (3 float), ығысу 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    // location=1: түс (3 float), ығысу 3 float
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // === 3-АПТА: шейдерлер компиляциясы ===

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

    // === 7-АПТА: үш uniform орнын цикл алдында БІР рет табамыз ===
    // int locTransform = glGetUniformLocation(shader, "uTransform");   // 6-апта
    int locModel = glGetUniformLocation(shader, "uModel");
    int locView  = glGetUniformLocation(shader, "uView");
    int locProj  = glGetUniformLocation(shader, "uProjection");

    // 7-апта, 3-тапсырма: үш кубтың орындары
    glm::vec3 positions[] = {
        glm::vec3(-1.5f, 0.0f,  0.0f),
        glm::vec3( 0.0f, 0.0f, -1.0f),
        glm::vec3( 1.5f, 0.0f,  0.0f)
    };
    float cubeSpeed[] = { 1.0f, 0.5f, 2.0f };   // әр кубтың өз жылдамдығы (angle-ге көбейткіш)

    float lastFrame = (float)glfwGetTime();   // НАЗАР: нөл емес, әйтпесе 1-кадрда dt өте үлкен


    // -----------------------------------------------------------------
    //  4. Негізгі цикл
    // -----------------------------------------------------------------
    double lastFpsTime = glfwGetTime();
    int    frameCount  = 0;

    while (!glfwWindowShouldClose(window)) {

        // 4-апта: delta time — алдыңғы кадрдан бергі уақыт (секунд)
        float frameNow = (float)glfwGetTime();
        float dt = frameNow - lastFrame;
        lastFrame = frameNow;

        processInput(window, dt);   // пернелер glClear-дан БҰРЫН

        g_angle += g_speed * dt;    // радиан/сек * сек = радиан
        float angle = g_angle;

        // --- Экранды тазалау ---
        float t = (float)glfwGetTime();
        float r = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
        float g = (std::sin(t * 1.2f) + 1.0f) * 0.5f * 0.3f;

        if (g_whiteBackground) glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        else                   glClearColor(r, g, 0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);   // тереңдік буфері әлі жоқ — 8-аптада

        // === 7-АПТА: цикл ішінде үш матрица ===
        float nearPlane = NEAR_LIST[g_nearIndex];
        float farPlane  = FAR_LIST[g_farIndex];
        float aspect = (float)gWidth / (float)gHeight;   // тұрақты 800/600 емес — терезе өлшемінен

        // P: перспектива (fov, aspect, near, far) немесе ортография (2-тапсырма)
        glm::mat4 projection = glm::perspective(glm::radians(g_fov), aspect, nearPlane, farPlane);
        if (g_ortho) {
            projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, 0.1f, 100.0f);
        }

        // V: камера (0, 0, 3)-те, координат басына қарайды, «жоғары» — +Y
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                                     glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));

        // Терезе тақырыбында қазіргі параметрлер — скриншотта көрінсін
        char title[200];
        std::snprintf(title, sizeof(title),
                      "7-апта | %s | fov=%.0f near=%.1f far=%.1f | %s",
                      g_ortho ? "ortho" : "perspective", g_fov, nearPlane, farPlane,
                      g_threeCubes ? "3 куб" : "1 куб");
        glfwSetWindowTitle(window, title);

        // === 2-АПТА: сызу командасы ===
        glUseProgram(shader);   // МІНДЕТТІ, әрі БІРІНШІ: uniform орнатудан бұрын

        // projection мен view барлық кубқа ортақ — бір рет жібереміз
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(vao);

        // Пульсация (қосымша): масштаб 0.8 … 1.2 аралығында
        float s = 1.0f;
        if (g_pulse) s = 1.0f + 0.2f * std::sin(t * 3.0f);

        if (!g_threeCubes) {
            // Базалық: бір куб Y осінен айналады
            glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
            if (g_twoAxes) model = glm::rotate(model, angle * 0.7f, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(s, s, s));

            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);   // 36 — вершина саны
        } else {
            // 3-тапсырма: бір VAO, цикл ішінде «матрица → uniform → сызу» үш рет
            for (int i = 0; i < 3; i++) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, positions[i]);
                model = glm::rotate(model, angle * cubeSpeed[i], glm::vec3(0.0f, 1.0f, 0.0f));
                if (g_twoAxes) model = glm::rotate(model, angle * 0.7f, glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::scale(model, glm::vec3(s, s, s));

                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        glfwSwapBuffers(window);   // дайын кадрды экранға шығару
        glfwPollEvents();          // пернетақта/тінтуір оқиғаларын өңдеу

        // FPS: секундына бір рет (1-апта, задание 4)
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
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
