// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week10" && git tag week10 && git push --tags
//
//  Қазіргі күйі: 10-АПТА — тінтуірмен еркін камера (yaw, pitch), дөңгелекпен fov
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

// 6-апта: GLM
#include <glm/glm.hpp>                    // mat4, vec3 типтері
#include <glm/gtc/matrix_transform.hpp>   // translate, rotate, perspective, lookAt
#include <glm/gtc/type_ptr.hpp>           // value_ptr

#include <cmath>       // 10-апта: cos, sin
#include <cstdio>
#include <iostream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
// 7-апта: терезе өлшемі жаһандық айнымалы — aspect осыдан есептеледі
int gWidth  = 1280;
int gHeight = 720;

bool g_wireframe = false;  // Tab — wireframe

// --- Камера (9-апта) ---
glm::vec3 camPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);   // 10-апта: енді тінтуірден есептеледі
glm::vec3 camUp    = glm::vec3(0.0f, 1.0f,  0.0f);

// --- Тінтуір (10-апта) ---
float yaw   = -90.0f;     // -90 — камера -Z-ке (кубқа) қарап басталады; 0 болса +X-ке қарайды
float pitch =   0.0f;
float lastX = 640.0f;     // курсордың алдыңғы орны (терезенің ортасы)
float lastY = 360.0f;
bool  firstMouse = true;  // бірінші оқиғада секіріс болмасын

// --- Проекция (10-апта) ---
float fov = 45.0f;

// 10-апта: тапсырмалар
bool g_pitchLimit = true;    // L — pitch шектеуін алып тастау (1-тапсырма, тәжірибе үшін)
bool invertY      = false;   // I — invert Y (3-тапсырма)

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
//  10-АПТА: тінтуір қозғалғанда шақырылады — yaw/pitch-тен camFront есептейміз
// ---------------------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {  // xpos, ypos: курсордың пиксельдегі орны
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;   // кері: экранның y-і төмен қарай өседі, ал pitch жоғары қарай
    if (invertY) yoffset = -yoffset;       // 3-тапсырма: invert Y
    lastX = (float)xpos;
    lastY = (float)ypos;

    // 2-тапсырма: 0.03 — тым баяу, 0.5 — тым жылдам, 0.1 — ыңғайлы
    const float sensitivity = 0.1f;
    yaw   += xoffset * sensitivity;
    pitch += yoffset * sensitivity;

    // Тура жоғары/төмен қарағанда сахна аударылмасын (1-тапсырмада L-мен өшіріледі)
    if (g_pitchLimit) {
        if (pitch >  89.0f) pitch =  89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    // Қосымша Қ4: yaw 360-тан асса, 360-ты шегереміз — сан шексіз өспесін
    if (yaw >  360.0f) yaw -= 360.0f;
    if (yaw < -360.0f) yaw += 360.0f;

    // Екі бұрыштан бағыт векторы (cos/sin радиан күтеді, ал yaw/pitch — градус)
    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camFront = glm::normalize(dir);
}

// ---------------------------------------------------------------------
//  10-АПТА: дөңгелек — fov (алға айналдырса, fov кішірейеді — жақындату)
// ---------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {  // yoffset: дөңгелек бағыты (+1 / -1)
    fov -= (float)yoffset;
    if (fov <  1.0f) fov =  1.0f;
    if (fov > 90.0f) fov = 90.0f;   // 5-тапсырма: жоғарғы шек 45-тен 90-ға көтерілді
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
//  Пернетақтаны өңдеу. dt — алдыңғы кадрдан бергі секунд
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // 9-апта: WASD. W енді камера қарап тұрған жаққа жүреді (camFront тінтуірден келеді)
    float camSpeed = 2.5f * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camSpeed *= 2.0f;   // 9-апта: жүгіру

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += camSpeed * camFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= camSpeed * camFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camPos -= glm::normalize(glm::cross(camFront, camUp)) * camSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camPos += glm::normalize(glm::cross(camFront, camUp)) * camSpeed;

    // 9-апта: Space/Left Shift — әлемнің Y осі бойымен жоғары/төмен
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camPos += camSpeed * glm::vec3(0.0f, 1.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camPos -= camSpeed * glm::vec3(0.0f, 1.0f, 0.0f);

    // 3-тапсырма: I — invert Y. Алдыңғы күйді сақтаймыз, әйтпесе басулы тұрғанда әр кадр ауысады
    static bool iPressedLast = false;
    bool iNow = glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS;
    if (iNow && !iPressedLast) invertY = !invertY;
    iPressedLast = iNow;

    // 1-тапсырма: L — pitch шектеуін өшіру/қосу
    static bool lWas = false;
    if (keyPressedOnce(window, GLFW_KEY_L, lWas)) g_pitchLimit = !g_pitchLimit;

    // 4-тапсырма: P — камераның толық күйін шығару
    static bool pWas = false;
    if (keyPressedOnce(window, GLFW_KEY_P, pWas)) {
        std::printf("camPos   = (%.2f, %.2f, %.2f)\n", camPos.x, camPos.y, camPos.z);
        std::printf("camFront = (%.2f, %.2f, %.2f)\n", camFront.x, camFront.y, camFront.z);
        std::printf("yaw = %.1f   pitch = %.1f   fov = %.1f\n", yaw, pitch, fov);
        std::cout << "length = " << glm::length(camFront) << std::endl;
    }

    // Қосымша Қ3: R — камераны бастапқы күйге қайтару
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        camPos   = glm::vec3(0.0f, 0.0f,  3.0f);
        camFront = glm::vec3(0.0f, 0.0f, -1.0f);
        yaw   = -90.0f;
        pitch =   0.0f;
        fov   =  45.0f;
    }

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
                                          "10-апта: тінтуірмен еркін камера",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады. Видеокарта OpenGL 3.3-ті "
                     "қолдамауы мүмкін.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(0);   // 1-апта: VSync өшірулі

    // -----------------------------------------------------------------
    //  3. GLAD: OpenGL функцияларын жүктеу (контекст белсенді болғаннан КЕЙІН)
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    // === 10-АПТА: курсорды ұстау — GLAD жүктелгеннен кейін ===
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);   // курсор жасырын, терезеден шықпайды
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwGetFramebufferSize(window, &gWidth, &gHeight);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";
    std::cout << "Басқару: тінтуір - қарау, дөңгелек - зум, WASD - жүру, Space/Left Shift - жоғары/төмен,\n"
                 "         Left Ctrl - жүгіру, I - invert Y, L - pitch шектеуі, P - камера күйі,\n"
                 "         R - бастапқы күй, Tab - wireframe, Esc - шығу\n";

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

        processInput(window, dt);

        float angle = frameNow;     // кубтар секундына 1 радиан айналады

        // --- Экранды тазалау ---
        float r = (std::sin(frameNow * 2.0f) + 1.0f) * 0.5f * 0.3f;
        float g = (std::sin(frameNow * 1.2f) + 1.0f) * 0.5f * 0.3f;
        glClearColor(r, g, 0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // === 10-АПТА: view мен projection ===
        // camFront — тінтуірден, fov — дөңгелектен. Формула 9-аптадағыдай
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov),   // 45.0f орнына
                                                (float)gWidth / (float)gHeight, 0.1f, 100.0f);

        // Қосымша Қ2: терезе тақырыбында ағымдағы бұрыштар
        char buf[128];
        std::snprintf(buf, sizeof(buf), "yaw=%.1f  pitch=%.1f  fov=%.1f%s%s", yaw, pitch, fov,
                      invertY ? "  [invert Y]" : "", g_pitchLimit ? "" : "  [pitch шектеусіз]");
        glfwSetWindowTitle(window, buf);

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
