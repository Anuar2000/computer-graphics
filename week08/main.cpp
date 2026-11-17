// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week08" && git tag week08 && git push --tags
//
//  Қазіргі күйі: 8-АПТА — тереңдік буфері, back-face culling, бірнеше объект
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

// 6-апта: GLM
#include <glm/glm.hpp>                    // mat4, vec3 типтері
#include <glm/gtc/matrix_transform.hpp>   // translate, rotate, scale, perspective, lookAt
#include <glm/gtc/type_ptr.hpp>           // value_ptr

#include <cmath>
#include <cstdio>
#include <iostream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
// 7-апта: терезе өлшемі жаһандық айнымалы — aspect осыдан есептеледі
int gWidth  = 1280;
int gHeight = 720;

bool g_whiteBackground = false;  // Пробел басылып тұрса — ақ фон
bool g_wireframe       = false;  // Tab — wireframe

// 4-апта: айналу жылдамдығы — радиан/СЕКУНД
const float SPEED_DEFAULT = 1.0f;
const float SPEED_MIN     = 0.1f;
const float SPEED_MAX     = 10.0f;
float g_speed = SPEED_DEFAULT;
float g_angle = 0.0f;

float g_fov = 45.0f;     // 7-апта: дөңгелекпен 10°–90°

// 8-апта: тапсырмалар (сан пернелерімен қосылады)
bool g_depthTest  = true;   // 1 — тереңдік тесті қосу/өшіру (1-тапсырма)
bool g_zFight     = false;  // 2 — Z-fighting сахнасы: екі куб бетпе-бет (2-тапсырма)
bool g_tinyNear   = true;   // 3 — near: 0.0001 / 0.1 (2-тапсырмада ғана әсер етеді)
bool g_cullFront  = false;  // 4 — glCullFace: GL_BACK / GL_FRONT (3-тапсырма)
int  g_depthFunc  = 0;      // 5 — glDepthFunc: 0 GL_LESS, 1 GL_GREATER, 2 GL_ALWAYS (қосымша)
bool g_tenCubes   = false;  // 6 — он куб шеңбер бойымен (қосымша)
bool g_flat       = false;  // 7 — жазық фигура: кубтың бір қыры ғана (қосымша)
bool g_showDepth  = false;  // 8 — түстің орнына gl_FragCoord.z (қосымша)

// ---------------------------------------------------------------------
//  Шейдерлер: 7-аптадағы MVP + 8-апта, қосымша: тереңдікті көзбен көру
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
uniform bool uShowDepth;
void main() {
    if (uShowDepth) FragColor = vec4(vec3(gl_FragCoord.z), 1.0);   // 0 — қара (жақын), 1 — ақ (алыс)
    else            FragColor = vec4(vColor, 1.0);
}
)";

// ---------------------------------------------------------------------
//  Терезе өлшемі өзгергенде шақырылады
// ---------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    gWidth  = width;
    gHeight = height;
    glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------
//  7-апта: тінтуір дөңгелегі fov-ты 10°–90° аралығында өзгертеді
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

    g_whiteBackground = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);

    // Tab — wireframe (қосымша: culling-пен бірге қалай жұмыс істейтінін көру)
    static bool tabWas = false;
    if (keyPressedOnce(window, GLFW_KEY_TAB, tabWas)) {
        g_wireframe = !g_wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);
    }

    // 4-апта: W/S — айналу жылдамдығы
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) g_speed += 2.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) g_speed -= 2.0f * dt;
    g_speed = std::fmin(std::fmax(g_speed, SPEED_MIN), SPEED_MAX);

    // 8-апта: 1 — тереңдік тесті (1-тапсырма)
    static bool k1 = false;
    if (keyPressedOnce(window, GLFW_KEY_1, k1)) {
        g_depthTest = !g_depthTest;
        if (g_depthTest) glEnable(GL_DEPTH_TEST);
        else             glDisable(GL_DEPTH_TEST);
    }

    // 8-апта: 2 — Z-fighting сахнасы, 3 — near-ды ауыстыру (2-тапсырма)
    static bool k2 = false, k3 = false;
    if (keyPressedOnce(window, GLFW_KEY_2, k2)) g_zFight = !g_zFight;
    if (keyPressedOnce(window, GLFW_KEY_3, k3)) g_tinyNear = !g_tinyNear;

    // 8-апта: 4 — GL_BACK / GL_FRONT (3-тапсырма)
    static bool k4 = false;
    if (keyPressedOnce(window, GLFW_KEY_4, k4)) {
        g_cullFront = !g_cullFront;
        glCullFace(g_cullFront ? GL_FRONT : GL_BACK);
    }

    // Қосымша: 5 — glDepthFunc сынағы
    static bool k5 = false;
    if (keyPressedOnce(window, GLFW_KEY_5, k5)) {
        g_depthFunc = (g_depthFunc + 1) % 3;
        if (g_depthFunc == 0) glDepthFunc(GL_LESS);
        if (g_depthFunc == 1) glDepthFunc(GL_GREATER);
        if (g_depthFunc == 2) glDepthFunc(GL_ALWAYS);
    }

    // Қосымша: 6 — он куб, 7 — жазық фигура, 8 — тереңдікті көзбен көру
    static bool k6 = false, k7 = false, k8 = false;
    if (keyPressedOnce(window, GLFW_KEY_6, k6)) g_tenCubes = !g_tenCubes;
    if (keyPressedOnce(window, GLFW_KEY_7, k7)) g_flat = !g_flat;
    if (keyPressedOnce(window, GLFW_KEY_8, k8)) g_showDepth = !g_showDepth;

    // R — бәрін бастапқы күйге қайтару
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        g_speed = SPEED_DEFAULT;
        g_angle = 0.0f;
        g_fov = 45.0f;
        g_depthTest = true;   glEnable(GL_DEPTH_TEST);
        g_cullFront = false;  glCullFace(GL_BACK);
        g_depthFunc = 0;      glDepthFunc(GL_LESS);
        g_zFight = false;
        g_tinyNear = true;
        g_tenCubes = false;
        g_flat = false;
        g_showDepth = false;
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
                                          "8-апта: тереңдік буфері, culling",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады. Видеокарта OpenGL 3.3-ті "
                     "қолдамауы мүмкін.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSwapInterval(0);   // 1-апта: VSync өшірулі

    // -----------------------------------------------------------------
    //  3. GLAD: OpenGL функцияларын жүктеу (контекст белсенді болғаннан КЕЙІН)
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    glfwGetFramebufferSize(window, &gWidth, &gHeight);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    // 8-апта: терезеде тереңдік буфері бар ма? 24 немесе 32 болуы керек, 0 — буфер жоқ
    int bits = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH,
        GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &bits);
    std::cout << "depth bits: " << bits << "\n";

    std::cout << "Басқару: 1 - тереңдік тесті, 2 - Z-fighting сахнасы, 3 - near 0.0001/0.1,\n"
                 "         4 - GL_BACK/GL_FRONT, 5 - glDepthFunc, 6 - он куб, 7 - жазық фигура,\n"
                 "         8 - тереңдікті көру, Tab - wireframe, дөңгелек - fov,\n"
                 "         W/S - жылдамдық, R - бастапқы күй, Пробел - ақ фон, Esc - шығу\n";

    // === 8-АПТА: тереңдік тесті мен culling — циклге дейін, бір рет ===
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);   // кері қырларды тастау
    glCullFace(GL_BACK);      // қайсысын тастау — артқысын
    glFrontFace(GL_CCW);      // «алдыңғы» — сағат тіліне қарсы (5-аптадан бергі әдет)


    // === 7-АПТА: куб — 36 вершина, әрқайсысы 6 float (позиция + түс) ===
    // Әр қыр сырттан қарағанда сағат тіліне қарсы (CCW) — culling дұрыс жұмыс істейді.
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

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

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

    // === 7-АПТА: uniform орындары — цикл алдында бір рет ===
    int locModel     = glGetUniformLocation(shader, "uModel");
    int locView      = glGetUniformLocation(shader, "uView");
    int locProj      = glGetUniformLocation(shader, "uProjection");
    int locShowDepth = glGetUniformLocation(shader, "uShowDepth");   // 8-апта, қосымша

    // === 8-АПТА: бірнеше объект — позициялар массиві ===
    glm::vec3 positions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  0.5f, -2.0f),
        glm::vec3(-1.8f, -0.3f, -1.0f),
        glm::vec3( 1.2f, -0.8f, -3.5f),
        glm::vec3(-2.2f,  1.0f, -4.0f)
    };

    float lastFrame = (float)glfwGetTime();


    // -----------------------------------------------------------------
    //  4. Негізгі цикл
    // -----------------------------------------------------------------
    double lastFpsTime = glfwGetTime();
    int    frameCount  = 0;

    while (!glfwWindowShouldClose(window)) {

        // 4-апта: delta time
        float frameNow = (float)glfwGetTime();
        float dt = frameNow - lastFrame;
        lastFrame = frameNow;

        processInput(window, dt);

        g_angle += g_speed * dt;
        float angle = g_angle;

        // --- Экранды тазалау ---
        float t = (float)glfwGetTime();
        float r = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
        float g = (std::sin(t * 1.2f) + 1.0f) * 0.5f * 0.3f;

        if (g_whiteBackground) glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        else                   glClearColor(r, g, 0.35f, 1.0f);
        // 8-апта: тереңдік буферін де ӘР кадрда тазалаймыз (әйтпесе 1-кадрдан кейін экран қатып қалады)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // === 7-АПТА: projection мен view ===
        float nearPlane = 0.1f;
        float farPlane  = 100.0f;
        if (g_zFight) {
            // 2-тапсырма: near өте кішкентай — буфердің жартысы 0.0001 … 0.0002 аралығына кетеді
            nearPlane = g_tinyNear ? 0.0001f : 0.1f;
            farPlane  = 1000.0f;
        }
        glm::mat4 projection = glm::perspective(glm::radians(g_fov),
                                                (float)gWidth / (float)gHeight, nearPlane, farPlane);
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                                     glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));

        // Терезе тақырыбында қазіргі күй
        const char* funcName[3] = { "GL_LESS", "GL_GREATER", "GL_ALWAYS" };
        char title[200];
        std::snprintf(title, sizeof(title),
                      "8-апта | depth %s | cull %s | %s | near=%g far=%g",
                      g_depthTest ? "ON" : "OFF", g_cullFront ? "GL_FRONT" : "GL_BACK",
                      funcName[g_depthFunc], nearPlane, farPlane);
        glfwSetWindowTitle(window, title);

        // === Сызу: projection мен view бір рет, model әрқайсысына бөлек ===
        glUseProgram(shader);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniform1i(locShowDepth, g_showDepth);
        glBindVertexArray(vao);

        // 7-қосымша: жазық фигура — кубтың алдыңғы қыры ғана (6-шы вершинадан 6 вершина)
        int first = g_flat ? 6 : 0;
        int count = g_flat ? 6 : 36;

        if (g_zFight) {
            // 2-тапсырма: екі куб бетпе-бет, z-тері 0.001-ге ғана айырылады.
            // Екі кубты бірге баяу бұрамыз — жыпылықтау өрнегі өзгергені көрінсін.
            for (int i = 0; i < 2; i++) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::rotate(model, angle * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::translate(model, glm::vec3(i * 0.4f, i * 0.3f, i * 0.001f));
                // Екінші кубты 180° бұрамыз — алдына қызыл қыры шығады, жыпылықтау түстен көрінсін
                if (i == 1) model = glm::rotate(model, 3.14159265f, glm::vec3(0.0f, 1.0f, 0.0f));
                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, first, count);
            }
        } else if (g_tenCubes) {
            // 6-қосымша: он куб шеңбер бойымен (центрі (0, 0, -4), радиусы 3), әрқайсысы өз жылдамдығымен
            for (int i = 0; i < 10; i++) {
                float a = i * 2.0f * 3.14159265f / 10.0f;
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(std::cos(a) * 3.0f, std::sin(a) * 1.5f,
                                                        -4.0f + std::sin(a) * 2.0f));
                model = glm::rotate(model, angle * (0.3f + i * 0.2f), glm::vec3(1.0f, 0.3f, 0.5f));
                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, first, count);
            }
        } else {
            // Негізгі: бес куб, әрқайсысы өз орнында, өз жылдамдығымен айналады
            for (int i = 0; i < 5; i++) {
                glm::mat4 model = glm::mat4(1.0f);   // ЦИКЛ ІШІНДЕ — әр куб бірліктен басталады
                model = glm::translate(model, positions[i]);
                model = glm::rotate(model, angle * (0.5f + i * 0.3f), glm::vec3(0.3f, 1.0f, 0.0f));

                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, first, count);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        // FPS: секундына бір рет
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
