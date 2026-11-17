// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week11" && git tag week11 && git push --tags
//
//  Қазіргі күйі: 11-АПТА — текстуралар (stb_image, UV, wrapping, фильтрлеу, mipmap)
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

// 6-апта: GLM
#include <glm/glm.hpp>                    // mat4, vec3 типтері
#include <glm/gtc/matrix_transform.hpp>   // translate, rotate, perspective, lookAt
#include <glm/gtc/type_ptr.hpp>           // value_ptr

// 11-апта: суретті файлдан жүктеу. #define-сыз stbi_load табылмайды (undefined reference)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>       // 10-апта: cos, sin
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

// --- 11-апта: тапсырмалар ---
unsigned int g_texture = 0;   // container.png текстурасы (пернелер оның параметрлерін өзгертеді)
int   g_wrapMode   = 0;       // 1 — орау режимі: REPEAT / MIRRORED_REPEAT / CLAMP_TO_EDGE / CLAMP_TO_BORDER
bool  g_uvTimes2   = false;   // 2 — UV мәндері 2.0-ге көбейтілген VAO (1-тапсырма)
bool  g_nearest    = false;   // N — MAG_FILTER: GL_LINEAR / GL_NEAREST (2-тапсырма)
bool  g_noMipmap   = false;   // M — MIN_FILTER: GL_LINEAR_MIPMAP_LINEAR / GL_LINEAR (3-тапсырма)
bool  g_blackCube  = false;   // B — MAG_FILTER-ге mipmap тұрақтысы, әдейі қате (4-тапсырма)
float uvScale      = 1.0f;    // [ ] — UV масштабы 0.5 … 8.0 (5-тапсырма)
bool  g_tint       = false;   // T — текстураны түспен көмкеру (қосымша Қ2)
bool  g_flow       = false;   // F — ағып жатқан су әсері (қосымша Қ3)
bool  g_aniso      = false;   // K — анизотропты фильтрация (қосымша Қ4)

// ---------------------------------------------------------------------
//  11-АПТА: шейдерлер. Түстің орнына UV, фрагментте texture()
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
uniform sampler2D ourTexture;
uniform float uvScale;   // 5-тапсырма
uniform vec3  tint;      // Қ2: (1, 1, 1) — өзгеріссіз
uniform float time;      // Қ3: 0 — қозғалыссыз
void main() {
    vec2 uv = vTexCoord * uvScale + vec2(0.0, time * 0.1);
    FragColor = texture(ourTexture, uv) * vec4(tint, 1.0);
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
    float yoffset = lastY - (float)ypos;   // экранның y-і төмен қарай өседі
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
//  11-АПТА: суретті жүктеп, текстура объектісін жасау. Қайтарады: текстураның ID-сі
// ---------------------------------------------------------------------
unsigned int loadTexture(const char* path) {  // path: суреттің толық жолы
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);   // бұдан кейінгі glTexParameteri мен glTexImage2D осы текстураға

    // Орау (UV 0..1-ден шыққанда) және фильтрлеу (үлкейту/кішірейту)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Суреттің (0, 0)-і жоғарғы сол бұрышта, ал OpenGL-де төменгі сол — сондықтан аударамыз
    stbi_set_flip_vertically_on_load(true);

    int w, h, n;   // ені, биіктігі, арна саны
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if (data) {
        GLenum fmt = (n == 4) ? GL_RGBA : GL_RGB;   // PNG мөлдір болса 4 арна, әйтпесе 3
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);           // glTexImage2D-дан КЕЙІН, әйтпесе кішірейтетін ештеңе жоқ
        std::cout << path << " " << w << "x" << h << " арна: " << n << std::endl;
    } else {
        std::cerr << "Сурет жүктелмеді: " << path << std::endl;
    }
    stbi_image_free(data);   // сурет GPU-ға көшірілді, CPU-дағы көшірме енді керек емес
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

    // 10-апта: I — invert Y
    static bool iWas = false;
    if (keyPressedOnce(window, GLFW_KEY_I, iWas)) invertY = !invertY;

    // Текстура параметрлерін өзгерту алдында оны байлаймыз
    glBindTexture(GL_TEXTURE_2D, g_texture);

    // 1-тапсырма: 1 — орау режимін кезекпен ауыстыру
    static bool k1 = false;
    if (keyPressedOnce(window, GLFW_KEY_1, k1)) {
        g_wrapMode = (g_wrapMode + 1) % 4;
        GLint modes[4] = { GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER };
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, modes[g_wrapMode]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, modes[g_wrapMode]);
        // GL_CLAMP_TO_BORDER үшін шекара түсі бөлек беріледі
        float border[] = { 1.0f, 0.0f, 0.0f, 1.0f };   // қызыл
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    }

    // 1-тапсырма: 2 — UV × 2.0 бар VAO-ға ауысу
    static bool k2 = false;
    if (keyPressedOnce(window, GLFW_KEY_2, k2)) g_uvTimes2 = !g_uvTimes2;

    // 2-тапсырма: N — GL_LINEAR / GL_NEAREST (үлкейту фильтрі)
    static bool nWas = false;
    if (keyPressedOnce(window, GLFW_KEY_N, nWas)) {
        g_nearest = !g_nearest;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, g_nearest ? GL_NEAREST : GL_LINEAR);
    }

    // 3-тапсырма: M — mipmap бар / жоқ (кішірейту фильтрі)
    static bool mWas = false;
    if (keyPressedOnce(window, GLFW_KEY_M, mWas)) {
        g_noMipmap = !g_noMipmap;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        g_noMipmap ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
    }

    // 4-тапсырма: B — MAG_FILTER-ге mipmap тұрақтысы (әдейі қате) / қайта GL_LINEAR
    static bool bWas = false;
    if (keyPressedOnce(window, GLFW_KEY_B, bWas)) {
        g_blackCube = !g_blackCube;
        if (g_blackCube)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);   // әдейі қате
        else
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, g_nearest ? GL_NEAREST : GL_LINEAR);
        // glGetError — қатені табудың бір жолы (0 = қате жоқ, 1280 = GL_INVALID_ENUM)
        std::cout << "glGetError() = " << glGetError() << std::endl;
    }

    // 5-тапсырма: [ ] — UV масштабы. Үздіксіз әрекет, сондықтан dt-ға көбейтеміз
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) uvScale += 2.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)  uvScale -= 2.0f * dt;
    if (uvScale < 0.1f) uvScale = 0.1f;   // 2-тапсырма үшін 0.1-ге дейін кішірейтуге болады
    if (uvScale > 8.0f) uvScale = 8.0f;

    // Қосымша: T — түспен көмкеру, F — ағып жатқан су, K — анизотропты фильтрация
    static bool tWas = false, fWas = false, kWas = false;
    if (keyPressedOnce(window, GLFW_KEY_T, tWas)) g_tint = !g_tint;
    if (keyPressedOnce(window, GLFW_KEY_F, fWas)) g_flow = !g_flow;
    if (keyPressedOnce(window, GLFW_KEY_K, kWas)) {
        g_aniso = !g_aniso;
        float maxAniso = 1.0f;
        if (g_aniso) glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
        std::cout << "anisotropy = " << maxAniso << std::endl;
    }

    // 4-апта: P — камера күйі
    static bool pWas = false;
    if (keyPressedOnce(window, GLFW_KEY_P, pWas)) {
        std::printf("camPos = (%.2f, %.2f, %.2f)  yaw = %.1f  pitch = %.1f  fov = %.1f  uvScale = %.2f\n",
                    camPos.x, camPos.y, camPos.z, yaw, pitch, fov, uvScale);
    }

    // R — бастапқы күй (камера және UV масштабы)
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        camPos   = glm::vec3(0.0f, 0.0f,  3.0f);
        camFront = glm::vec3(0.0f, 0.0f, -1.0f);
        yaw = -90.0f;  pitch = 0.0f;  fov = 45.0f;
        uvScale = 1.0f;
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
                                          "11-апта: текстуралар",
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
    std::cout << "Басқару: тінтуір/дөңгелек/WASD - камера, 1 - орау режимі, 2 - UV x2,\n"
                 "         N - NEAREST/LINEAR, M - mipmap, B - қара куб, [ ] - UV масштабы,\n"
                 "         T - түс, F - ағыс, K - анизотропия, P - күй, R - бастапқы күй, Esc - шығу\n";

    // === 8-АПТА: тереңдік тесті мен culling ===
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);


    // === 11-АПТА: куб — 36 вершина, әрқайсысы 5 float (позиция + UV) ===
    // Позициялар 10-аптадағыдай — тек әр жолдың соңындағы үш түс саны екі UV санына ауысты.
    // Алты жақта да UV реті бірдей: (0,0) (1,0) (1,1) — бірінші үшбұрыш, (1,1) (0,1) (0,0) — екінші.
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

    // 1-тапсырма: дәл сол куб, бірақ барлық UV 2.0-ге көбейтілген (1.0f орнына 2.0f)
    float vertices2[36 * 5];
    for (int i = 0; i < 36 * 5; i++) {
        vertices2[i] = vertices[i];
        if (i % 5 >= 3) vertices2[i] = vertices[i] * 2.0f;   // әр вершинаның 4- және 5-саны — UV
    }

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 11-апта: stride 6 емес, 5 float
    // поз: location 0 — 3 float, ығысу 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // UV: location 1 — 2 float, ығысу 3 float
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 1-тапсырма үшін екінші VAO (UV × 2)
    unsigned int vao2, vbo2;
    glGenVertexArrays(1, &vao2);
    glGenBuffers(1, &vbo2);

    glBindVertexArray(vao2);
    glBindBuffer(GL_ARRAY_BUFFER, vbo2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
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

    unsigned int shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // === 11-АПТА: текстураны жүктеу. ASSET_DIR — CMakeLists.txt береді, қай бумадан іске қосса да табылады ===
    std::string path = std::string(ASSET_DIR) + "/container.png";
    g_texture = loadTexture(path.c_str());

    // === uniform орындары — цикл алдында бір рет ===
    int locModel   = glGetUniformLocation(shader, "uModel");
    int locView    = glGetUniformLocation(shader, "uView");
    int locProj    = glGetUniformLocation(shader, "uProjection");
    int locUvScale = glGetUniformLocation(shader, "uvScale");
    int locTint    = glGetUniformLocation(shader, "tint");
    int locTime    = glGetUniformLocation(shader, "time");

    // Бір текстура болса да sampler-ге 0-блокты нақты айтамыз — 12-аптада екі текстура болады
    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "ourTexture"), 0);

    // === 8-АПТА: бес кубтың орындары + 3-тапсырма: z = -40 маңында тағы алты куб ===
    glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,   0.0f),
        glm::vec3( 2.0f,  0.5f,  -2.0f),
        glm::vec3(-1.8f, -0.3f,  -1.0f),
        glm::vec3( 1.2f, -0.8f,  -3.5f),
        glm::vec3(-2.2f,  1.0f,  -4.0f),
        // алыстағы кубтар (mipmap әсерін көру үшін)
        glm::vec3(-18.0f,  2.0f, -40.0f),
        glm::vec3(-12.0f, -3.0f, -38.0f),
        glm::vec3( -7.0f,  9.0f, -42.0f),
        glm::vec3(  7.0f, -9.0f, -39.0f),
        glm::vec3( 12.0f,  3.0f, -41.0f),
        glm::vec3( 18.0f, -2.0f, -40.0f)
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

        const char* wrapName[4] = { "GL_REPEAT", "GL_MIRRORED_REPEAT", "GL_CLAMP_TO_EDGE", "GL_CLAMP_TO_BORDER" };
        char buf[200];
        std::snprintf(buf, sizeof(buf), "11-апта | %s | UV%s | MAG %s | MIN %s | uvScale=%.2f",
                      wrapName[g_wrapMode], g_uvTimes2 ? " x2" : "", g_nearest ? "NEAREST" : "LINEAR",
                      g_noMipmap ? "LINEAR" : "MIPMAP", uvScale);
        glfwSetWindowTitle(window, buf);

        // === 11-АПТА: render — шейдер, текстура, VAO ===
        glUseProgram(shader);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniform1f(locUvScale, uvScale);

        // Қ2: tint уақыт бойынша өзгереді; Қ3: time — UV-ді жылжытады
        glm::vec3 tint = glm::vec3(1.0f);
        if (g_tint) tint = glm::vec3(0.6f + 0.4f * std::sin(frameNow), 0.6f + 0.4f * std::sin(frameNow + 2.0f), 1.0f);
        glUniform3fv(locTint, 1, glm::value_ptr(tint));
        glUniform1f(locTime, g_flow ? frameNow : 0.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_texture);

        glBindVertexArray(g_uvTimes2 ? vao2 : vao);

        for (int i = 0; i < 11; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            model = glm::rotate(model, angle * (0.5f + (i % 5) * 0.3f), glm::vec3(0.3f, 1.0f, 0.0f));

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
    glDeleteVertexArrays(1, &vao2);
    glDeleteBuffers(1, &vbo2);
    glDeleteTextures(1, &g_texture);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
