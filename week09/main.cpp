// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week09" && git tag week09 && git push --tags
//
//  Қазіргі күйі: 9-АПТА — камера моделі, LookAt, пернетақтамен жылжу
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

// 6-апта: GLM
#include <glm/glm.hpp>                    // mat4, vec3 типтері
#include <glm/gtc/matrix_transform.hpp>   // translate, rotate, perspective, lookAt
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

bool g_wireframe = false;  // Tab — wireframe

float g_fov = 45.0f;       // 7-апта: дөңгелекпен 10°–90°

// === 9-АПТА: камера — үш айнымалы, жаһандық, цикл сыртында ===
glm::vec3 camPos   = glm::vec3(0.0f, 0.0f,  3.0f);   // камера қайда тұр — WASD осыны өзгертеді
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);   // қай бағытқа қарайды (OpenGL-де алға — -Z)
glm::vec3 camUp    = glm::vec3(0.0f, 1.0f,  0.0f);   // камераның «төбесі»

// 9-апта: тапсырмалар
bool g_fixedTarget = false;  // 1 — lookAt-тың 2-параметрі тұрақты нүкте (қате нұсқа, 1-тапсырма)
bool g_worldUp     = false;  // 2 — Space/Shift: камераның төбесі / әлемнің Y осі (2-тапсырма)
bool g_topCamera   = false;  // C — екінші камера: жоғарыдан қарайды (қосымша)

// ---------------------------------------------------------------------
//  Шейдерлер: 7-аптадағы MVP
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
//  9-АПТА: пернетақтаны өңдеу. dt — алдыңғы кадрдан бергі секунд
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // 2.5 — бірлік/секунд, dt-ға көбейткенде кадрға шаққандағы қадам шығады
    float speed = 2.5f * dt;
    // 3-тапсырма: Left Control басулы тұрса — екі есе жылдам (жүгіру)
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed *= 2.0f;

    // W/S — алға/артқа (камера қарап тұрған бағыт бойымен)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += speed * camFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= speed * camFront;
    // A/D — солға/оңға: cross оң жаққа қарайтын векторды береді, normalize — ұзындығын 1-ге келтіреді
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camPos -= glm::normalize(glm::cross(camFront, camUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camPos += glm::normalize(glm::cross(camFront, camUp)) * speed;

    // 2-тапсырма: Space — жоғары, Left Shift — төмен
    glm::vec3 upDir = camUp;                                       // камераның төбесі бойымен
    if (g_worldUp) upDir = glm::vec3(0.0f, 1.0f, 0.0f);            // әлемнің Y осі бойымен
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)      camPos += speed * upDir;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camPos -= speed * upDir;

    // Қосымша: камера сахнадан тым алыс кетпесін — camPos ұзындығы 20-дан аспайды
    if (glm::length(camPos) > 20.0f) camPos = glm::normalize(camPos) * 20.0f;

    // 1 — lookAt-тың 2-параметрі: camPos + camFront / тұрақты нүкте (1-тапсырма)
    static bool k1 = false;
    if (keyPressedOnce(window, GLFW_KEY_1, k1)) g_fixedTarget = !g_fixedTarget;

    // 2 — жоғары-төмен бағыты: camUp / әлемнің Y осі (2-тапсырма)
    static bool k2 = false;
    if (keyPressedOnce(window, GLFW_KEY_2, k2)) g_worldUp = !g_worldUp;

    // Қосымша: C — екінші камера (жоғарыдан)
    static bool cWas = false;
    if (keyPressedOnce(window, GLFW_KEY_C, cWas)) g_topCamera = !g_topCamera;

    // Қосымша: R — камераны бастапқы орнына қайтару
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        camPos   = glm::vec3(0.0f, 0.0f,  3.0f);
        camFront = glm::vec3(0.0f, 0.0f, -1.0f);
        camUp    = glm::vec3(0.0f, 1.0f,  0.0f);
        g_fov = 45.0f;
    }

    // Қосымша: P — камераның координатын консольге шығару (бір рет басқанда)
    static bool pWas = false;
    if (keyPressedOnce(window, GLFW_KEY_P, pWas))
        std::cout << camPos.x << ", " << camPos.y << ", " << camPos.z << "\n";

    // Tab — wireframe
    static bool tabWas = false;
    if (keyPressedOnce(window, GLFW_KEY_TAB, tabWas)) {
        g_wireframe = !g_wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);
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
                                          "9-апта: камера, LookAt, WASD",
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
    std::cout << "Басқару: WASD - жүру, Space/Left Shift - жоғары/төмен, Left Ctrl - жүгіру,\n"
                 "         1 - lookAt-тың тұрақты нүктесі, 2 - camUp/әлем Y осі, C - жоғарғы камера,\n"
                 "         R - камераны қайтару, P - координат, Tab - wireframe, дөңгелек - fov,\n"
                 "         Esc - шығу\n";

    // === 8-АПТА: тереңдік тесті мен culling — циклге дейін, бір рет ===
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);


    // === 7-АПТА: куб — 36 вершина, әрқайсысы 6 float (позиция + түс) ===
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
    int locModel = glGetUniformLocation(shader, "uModel");
    int locView  = glGetUniformLocation(shader, "uView");
    int locProj  = glGetUniformLocation(shader, "uProjection");

    // === 8-АПТА: бес кубтың орындары ===
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

        processInput(window, dt);   // 9-апта: dt-дан КЕЙІН, сызудан бұрын

        float angle = frameNow;     // кубтар секундына 1 радиан айналады

        // --- Экранды тазалау ---
        float r = (std::sin(frameNow * 2.0f) + 1.0f) * 0.5f * 0.3f;
        float g = (std::sin(frameNow * 1.2f) + 1.0f) * 0.5f * 0.3f;
        glClearColor(r, g, 0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // === 9-АПТА: view матрицасы — тұрақты сандардың орнына айнымалылар ===
        // 7-апта: glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)) — жойылды
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);   // дұрыс — камера ұшады
        if (g_fixedTarget) {
            view = glm::lookAt(camPos, glm::vec3(0.0f), camUp);           // қате — камера айналады
        }
        if (g_topCamera) {
            // Қосымша: екінші камера — сахнаның үстінен төмен қарайды (жоғары бағыты — -Z)
            view = glm::lookAt(glm::vec3(0.0f, 12.0f, -2.0f),
                               glm::vec3(0.0f, 0.0f, -2.0f),
                               glm::vec3(0.0f, 0.0f, -1.0f));
        }

        glm::mat4 projection = glm::perspective(glm::radians(g_fov),
                                                (float)gWidth / (float)gHeight, 0.1f, 100.0f);

        char title[200];
        std::snprintf(title, sizeof(title),
                      "9-апта | camPos = (%.2f, %.2f, %.2f) | %s | %s%s",
                      camPos.x, camPos.y, camPos.z,
                      g_fixedTarget ? "lookAt(camPos, 0, up)" : "lookAt(camPos, camPos + camFront, up)",
                      g_worldUp ? "up: әлем Y" : "up: camUp",
                      g_topCamera ? " | жоғарғы камера" : "");
        glfwSetWindowTitle(window, title);

        // === Сызу: projection мен view бір рет, model әрқайсысына бөлек ===
        glUseProgram(shader);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glBindVertexArray(vao);

        for (int i = 0; i < 5; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, positions[i]);
            model = glm::rotate(model, angle * (0.5f + i * 0.3f), glm::vec3(0.3f, 1.0f, 0.0f));

            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Қосымша: жоғарғы камерада еркін камераның орнын кішкентай кубпен белгілейміз
        if (g_topCamera) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), camPos);
            model = glm::scale(model, glm::vec3(0.3f));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
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
