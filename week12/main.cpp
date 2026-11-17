// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week12" && git tag week12 && git push --tags
//
//  Қазіргі күйі: 12-АПТА — бірнеше текстура, текстуралық блоктар, mix() араластыру
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

// 6-апта: GLM
#include <glm/glm.hpp>                    // mat4, vec3 типтері
#include <glm/gtc/matrix_transform.hpp>   // translate, rotate, perspective, lookAt
#include <glm/gtc/type_ptr.hpp>           // value_ptr

// 11-апта: суретті файлдан жүктеу
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
// 7-апта: терезе өлшемі жаһандық айнымалы — aspect осыдан есептеледі
int gWidth  = 1280;
int gHeight = 720;

bool g_wireframe = false;  // Tab — wireframe

// --- Камера (9-апта) ---
glm::vec3 camPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camUp    = glm::vec3(0.0f, 1.0f,  0.0f);

// --- Тінтуір (10-апта) ---
float yaw   = -90.0f;
float pitch =   0.0f;
float lastX = 640.0f;
float lastY = 360.0f;
bool  firstMouse = true;
bool  invertY    = false;   // I — invert Y

// --- Проекция (10-апта) ---
float fov = 45.0f;

// --- 12-апта ---
float blend = 0.5f;          // ↑/↓ — екі текстураның араласу коэффициенті 0 … 1
unsigned int tex1 = 0, tex2 = 0, tex3 = 0;   // container.png, pattern.png, logo.png
unsigned int shader = 0;

// Фрагмент шейдеріндегі режим (uMode): қай араластыру тәсілі
// 0 — mix(a, b, blend), 1 — a * b, 2 — a + b, 3 — clamp(a + b), 4 — логотип (decal.a),
// 5 — mix(a, b, vTexCoord.y), 6 — smoothstep, 7 — үш текстура
int  g_mode        = 0;
bool g_brokenUnits = false;  // 1 — glActiveTexture жолдары «комментарийде» (1-тапсырма)
bool g_wrongId     = false;  // 2 — glUniform1i-ге блок нөмірінің орнына ID (2-тапсырма)
bool g_autoBlend   = false;  // B — blend уақыт бойынша (қосымша Қ2)
bool g_perFace     = false;  // F — әр жаққа бөлек коэффициент (қосымша Қ3)
bool g_rotateLogo  = false;  // L — логотипті айналдыру (қосымша Қ4)

// ---------------------------------------------------------------------
//  Шейдерлер: 11-аптадағы UV + 12-апта: екі (үш) sampler
// ---------------------------------------------------------------------
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
out vec2 vTexCoord;
void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    vTexCoord = aTexCoord;
}
)";

const char* fragmentSrc = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D texture1;   // 0-блок: container.png
uniform sampler2D texture2;   // 1-блок: pattern.png (4-режимде logo.png)
uniform sampler2D texture3;   // 2-блок: logo.png (тек Қ1, үш текстура)
uniform float blend;
uniform int   uMode;
uniform float time;           // Қ4: логотипті бұру бұрышы (0 — бұрылмайды)
void main() {
    vec4 a = texture(texture1, vTexCoord);
    vec4 b = texture(texture2, vTexCoord);

    if (uMode == 0) FragColor = mix(a, b, blend);                   // 1. сызықтық
    if (uMode == 1) FragColor = a * b;                              // 2. көбейту
    if (uMode == 2) FragColor = a + b;                              // 3. қосу
    if (uMode == 3) FragColor = clamp(a + b, 0.0, 1.0);             // 4. қосу, шектеумен

    if (uMode == 4) {
        // 4-тапсырма: логотип альфамен. Қ4: UV-ді центрден бұрамыз
        vec2 c = vTexCoord - 0.5;
        float s = sin(time), co = cos(time);
        vec2 uv = vec2(c.x * co - c.y * s, c.x * s + c.y * co) + 0.5;
        vec4 decal = texture(texture2, uv);
        FragColor = mix(a, decal, decal.a);   // a = 0 — жәшік, a = 1 — логотип
    }

    if (uMode == 5) FragColor = mix(a, b, vTexCoord.y);                          // 5-тапсырма
    if (uMode == 6) FragColor = mix(a, b, smoothstep(0.3, 0.7, vTexCoord.y));    // 5-тапсырма, тік ауысу

    if (uMode == 7) {
        // Қ1: үш текстура — жәшік пен өрнек араласады, үстінен логотип
        vec4 decal = texture(texture3, vTexCoord);
        vec4 surface = mix(a, b, blend);
        FragColor = mix(surface, decal, decal.a);
    }
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
//  10-апта: тінтуір — yaw/pitch-тен camFront
// ---------------------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {  // xpos, ypos: курсордың пиксельдегі орны
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    if (invertY) yoffset = -yoffset;
    lastX = (float)xpos;
    lastY = (float)ypos;

    const float sensitivity = 0.1f;
    yaw   += xoffset * sensitivity;
    pitch += yoffset * sensitivity;

    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    if (yaw >  360.0f) yaw -= 360.0f;
    if (yaw < -360.0f) yaw += 360.0f;

    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camFront = glm::normalize(dir);
}

// ---------------------------------------------------------------------
//  10-апта: дөңгелек — fov
// ---------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {  // yoffset: дөңгелек бағыты (+1 / -1)
    fov -= (float)yoffset;
    if (fov <  1.0f) fov =  1.0f;
    if (fov > 90.0f) fov = 90.0f;
}

// ---------------------------------------------------------------------
//  11-апта: суретті жүктеп, текстура объектісін жасау. Қайтарады: текстураның ID-сі
// ---------------------------------------------------------------------
unsigned int loadTexture(const char* path) {  // path: суреттің толық жолы
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);

    int w, h, n;   // ені, биіктігі, арна саны
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if (data) {
        GLenum fmt = (n == 4) ? GL_RGBA : GL_RGB;   // logo.png — 4 арна (мөлдір фон)
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << path << " " << w << "x" << h << " арна: " << n << std::endl;
    } else {
        std::cerr << "Сурет жүктелмеді: " << path << std::endl;
    }
    stbi_image_free(data);
    return id;
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

    // 9-апта: WASD, Space/Shift, Ctrl — жүгіру
    float camSpeed = 2.5f * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camSpeed *= 2.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += camSpeed * camFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= camSpeed * camFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camPos -= glm::normalize(glm::cross(camFront, camUp)) * camSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camPos += glm::normalize(glm::cross(camFront, camUp)) * camSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camPos += camSpeed * glm::vec3(0.0f, 1.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camPos -= camSpeed * glm::vec3(0.0f, 1.0f, 0.0f);

    // 12-апта: ↑/↓ — blend. Үздіксіз әрекет, сондықтан dt-ға көбейтеміз (секундына 0.5)
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        blend += 0.5f * dt;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        blend -= 0.5f * dt;

    if (blend > 1.0f) blend = 1.0f;
    if (blend < 0.0f) blend = 0.0f;

    // 1-тапсырма: 1 — glActiveTexture-сыз сызу (әдейі қате)
    static bool k1 = false;
    if (keyPressedOnce(window, GLFW_KEY_1, k1)) {
        g_brokenUnits = !g_brokenUnits;
        if (g_brokenUnits) {
            // Бағдарлама басынан glActiveTexture-сыз іске қосылғандай жағдай: 1-блок бос, белсенді — 0-блок
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }
    }

    // 2-тапсырма: 2 — sampler-ге блок нөмірінің орнына текстура ID-сі (әдейі қате)
    static bool k2 = false;
    if (keyPressedOnce(window, GLFW_KEY_2, k2)) {
        g_wrongId = !g_wrongId;
        glUseProgram(shader);   // uniform тек белсенді бағдарламаға жазылады
        if (g_wrongId) glUniform1i(glGetUniformLocation(shader, "texture2"), tex2);   // әдейі қате
        else           glUniform1i(glGetUniformLocation(shader, "texture2"), 1);
        std::cout << "tex1 = " << tex1 << "   tex2 = " << tex2 << std::endl;
    }

    // 3-тапсырма: 3 — төрт араластыру тәсілін кезекпен ауыстыру (0 → 1 → 2 → 3 → 0)
    static bool k3 = false;
    if (keyPressedOnce(window, GLFW_KEY_3, k3)) g_mode = (g_mode >= 3) ? (g_mode + 1) % 4 : g_mode + 1;
    // 4 — логотип, 5 — vTexCoord.y, 6 — smoothstep, 7 — үш текстура (Қ1), 0 — қайта mix(a, b, blend)
    static bool k4 = false, k5 = false, k6 = false, k7 = false, k0 = false;
    if (keyPressedOnce(window, GLFW_KEY_4, k4)) g_mode = 4;
    if (keyPressedOnce(window, GLFW_KEY_5, k5)) g_mode = 5;
    if (keyPressedOnce(window, GLFW_KEY_6, k6)) g_mode = 6;
    if (keyPressedOnce(window, GLFW_KEY_7, k7)) g_mode = 7;
    if (keyPressedOnce(window, GLFW_KEY_0, k0)) g_mode = 0;

    // Қосымша: B — автоматты blend, F — әр жаққа бөлек, L — логотипті айналдыру
    static bool bWas = false, fWas = false, lWas = false;
    if (keyPressedOnce(window, GLFW_KEY_B, bWas)) g_autoBlend = !g_autoBlend;
    if (keyPressedOnce(window, GLFW_KEY_F, fWas)) g_perFace = !g_perFace;
    if (keyPressedOnce(window, GLFW_KEY_L, lWas)) g_rotateLogo = !g_rotateLogo;

    // 10-апта: I — invert Y
    static bool iWas = false;
    if (keyPressedOnce(window, GLFW_KEY_I, iWas)) invertY = !invertY;

    // R — бастапқы күй
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        camPos   = glm::vec3(0.0f, 0.0f,  3.0f);
        camFront = glm::vec3(0.0f, 0.0f, -1.0f);
        yaw = -90.0f;  pitch = 0.0f;  fov = 45.0f;
        blend = 0.5f;
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
                                          "12-апта: бірнеше текстура",
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

    // 10-апта: курсорды ұстау
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwGetFramebufferSize(window, &gWidth, &gHeight);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    // Видеокартадағы текстуралық блоктар саны (OpenGL 3.3 кемінде 16-ға кепілдік береді)
    int units;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &units);
    std::cout << "GL_MAX_TEXTURE_IMAGE_UNITS: " << units << "\n";

    std::cout << "Басқару: Up/Down - blend, 1 - блоктарды шатастыру, 2 - ID қатесі, 3 - араластыру тәсілі,\n"
                 "         4 - логотип, 5 - vTexCoord.y, 6 - smoothstep, 7 - үш текстура, 0 - mix,\n"
                 "         B - автоматты blend, F - әр жақ бөлек, L - логотипті айналдыру,\n"
                 "         тінтуір/дөңгелек/WASD - камера, R - бастапқы күй, Esc - шығу\n";

    // === 8-АПТА: тереңдік тесті мен culling ===
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);


    // === 11-АПТА: куб — 36 вершина, әрқайсысы 5 float (позиция + UV) ===
    float vertices[] = {
        // артқы жақ (z = -0.5)
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        // алдыңғы жақ (z = +0.5)
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        // сол жақ (x = -0.5)
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        // оң жақ (x = +0.5)
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        // астыңғы жақ (y = -0.5)
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        // үстіңгі жақ (y = +0.5)
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f
    };

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // поз: location 0 — 3 float, ығысу 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // UV: location 1 — 2 float, ығысу 3 float
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // === 3-АПТА: шейдерлер компиляциясы ===

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, nullptr);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentSrc, nullptr);
    glCompileShader(fs);

    shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // === 12-АПТА, 1-қадам: екі (үш) текстураны жүктеу — 11-аптадағы loadTexture өзгеріссіз ===
    std::string p1 = std::string(ASSET_DIR) + "/container.png";
    std::string p2 = std::string(ASSET_DIR) + "/pattern.png";
    std::string p3 = std::string(ASSET_DIR) + "/logo.png";
    tex1 = loadTexture(p1.c_str());
    tex2 = loadTexture(p2.c_str());
    tex3 = loadTexture(p3.c_str());

    // Қ4: логотип бұрылғанда шеттері қайталанбасын — сырты мөлдір шекара
    glBindTexture(GL_TEXTURE_2D, tex3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border[] = { 0.0f, 0.0f, 0.0f, 0.0f };   // толық мөлдір
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    // === 12-АПТА, 3-қадам: sampler-лерге блок НӨМІРІН береміз (ID емес!) — glUseProgram-нан КЕЙІН ===
    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "texture1"), 0);
    glUniform1i(glGetUniformLocation(shader, "texture2"), 1);
    glUniform1i(glGetUniformLocation(shader, "texture3"), 2);

    // === uniform орындары — цикл алдында бір рет ===
    int locModel = glGetUniformLocation(shader, "uModel");
    int locView  = glGetUniformLocation(shader, "uView");
    int locProj  = glGetUniformLocation(shader, "uProjection");
    int blendLoc = glGetUniformLocation(shader, "blend");
    int locMode  = glGetUniformLocation(shader, "uMode");
    int locTime  = glGetUniformLocation(shader, "time");

    // === 8-АПТА: бес кубтың орындары ===
    glm::vec3 cubePositions[] = {
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

        // Қ2: blend уақыт бойынша 0 … 1 аралығында тербеледі
        if (g_autoBlend) blend = 0.5f + 0.5f * sin((float)glfwGetTime());

        float angle = frameNow;

        // --- Экранды тазалау ---
        float r = (std::sin(frameNow * 2.0f) + 1.0f) * 0.5f * 0.3f;
        float g = (std::sin(frameNow * 1.2f) + 1.0f) * 0.5f * 0.3f;
        glClearColor(r, g, 0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // === 10-апта: view мен projection ===
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov),
                                                (float)gWidth / (float)gHeight, 0.1f, 100.0f);

        const char* modeName[8] = { "mix(a, b, blend)", "a * b", "a + b", "clamp(a + b)",
                                    "логотип: mix(base, decal, decal.a)", "mix(a, b, vTexCoord.y)",
                                    "smoothstep(0.3, 0.7, y)", "үш текстура" };
        char buf[200];
        std::snprintf(buf, sizeof(buf), "12-апта | %s | blend=%.2f%s%s", modeName[g_mode], blend,
                      g_brokenUnits ? " | glActiveTexture жоқ" : "", g_wrongId ? " | ID қатесі" : "");
        glfwSetWindowTitle(window, buf);

        // === 12-АПТА: render — әр текстура өз блогына ===
        glUseProgram(shader);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniform1f(blendLoc, blend);     // 1f — float, 1i — int
        glUniform1i(locMode, g_mode);
        glUniform1f(locTime, g_rotateLogo ? frameNow : 0.0f);

        // 4-тапсырма: логотип режимінде екінші текстура — pattern.png емес, logo.png
        unsigned int second = (g_mode == 4) ? tex3 : tex2;

        // Әр glBindTexture-дан бұрын glActiveTexture — әйтпесе екеуі де бір блокқа түседі
        if (!g_brokenUnits) glActiveTexture(GL_TEXTURE0);   // 1-тапсырмада бұл жол «жоқ»
        glBindTexture(GL_TEXTURE_2D, tex1);
        if (!g_brokenUnits) glActiveTexture(GL_TEXTURE1);   // 1-тапсырмада бұл жол «жоқ»
        glBindTexture(GL_TEXTURE_2D, second);

        if (g_mode == 7) {
            // Қ1: үшінші текстура — 2-блокқа
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, tex3);
        }

        glBindVertexArray(vao);

        for (int i = 0; i < 5; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            model = glm::rotate(model, angle * (0.5f + i * 0.3f), glm::vec3(0.3f, 1.0f, 0.0f));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

            if (g_perFace) {
                // Қ3: алты жақ бөлек сызылады — әрқайсысы 6 вершина, өз коэффициентімен
                for (int f = 0; f < 6; ++f) {
                    glUniform1f(blendLoc, f / 5.0f);
                    glDrawArrays(GL_TRIANGLES, f * 6, 6);
                }
            } else {
                glDrawArrays(GL_TRIANGLES, 0, 36);
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
    glDeleteTextures(1, &tex1);
    glDeleteTextures(1, &tex2);
    glDeleteTextures(1, &tex3);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
