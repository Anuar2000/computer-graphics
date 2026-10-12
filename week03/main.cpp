// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — 3-АПТА
//  Тақырып: шейдерлер, GLSL, интерполяция
//
//  Базалық бөлім:
//    - makeShader(): компиляция + линковка, ішінде ҚАТЕ ТЕКСЕРУ
//    - түс атрибуты: әр вершина 6 float (позиция + түс), STRIDE = 6 * float
//
//  Тапсырмаларды көрсету (пернелер):
//    I     — 1-тапсырма: түстерді төңкеру (1.0 - vColor)
//    C     — 2-тапсырма: үш вершинаға бірдей түс (градиент жоғалады)
//    Y     — 3-тапсырма: gl_Position.y * -1 (үшбұрыш төңкеріледі)
//    P     — қосымша: процедуралық шахмат тақтасы (gl_FragCoord, mod, step)
//    4-тапсырма: бағдарламаны "--broken" аргументімен іске қос:
//          cg_week03 --broken
//      әдейі қате шейдер компиляцияланады, makeShader хабарламаны шығарады.
//
//  1-2 апталардан қалғандар: Space — ақ фон, W — wireframe, Esc — шығу,
//  консольге секундына бір рет FPS.
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstring>
#include <iostream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int WIDTH  = 1280;
const int HEIGHT = 720;

// Пернелер күйі (processInput жазады, негізгі цикл оқиды)
bool g_whiteBackground = false;  // Space басулы тұрғанда
bool g_wireframe       = false;  // W — қосу/өшіру
bool g_invert          = false;  // I — 1-тапсырма
bool g_sameColor       = false;  // C — 2-тапсырма
bool g_flipY           = false;  // Y — 3-тапсырма
bool g_checker         = false;  // P — қосымша

// ---------------------------------------------------------------------
//  ШЕЙДЕРЛЕР
// ---------------------------------------------------------------------

// Негізгі vertex шейдер: позиция мен түсті алады, түсті fragment-ке береді.
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
void main() {
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}
)";

// 3-тапсырма: y-ті -1-ге көбейтеміз. Экрандағы орынды тек gl_Position анықтайды.
const char* vertexFlipSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
void main() {
    gl_Position = vec4(aPos, 1.0);
    gl_Position.y *= -1.0;
    vColor = aColor;
}
)";

// Негізгі fragment шейдер: интерполяцияланған түс.
const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

// 1-тапсырма: түстерді төңкеру. Амал покомпонентті — цикл керек емес.
const char* fragmentInvertSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0 - vColor, 1.0);
}
)";

// Қосымша: процедуралық шахмат тақтасы. gl_FragCoord — пиксельдің экрандағы орны.
const char* fragmentCheckerSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    vec2 cell = floor(gl_FragCoord.xy / 40.0);              // 40 пиксельдік шаршылар
    float k = step(0.5, mod(cell.x + cell.y, 2.0));         // 0 немесе 1
    FragColor = vec4(vColor * mix(0.25, 1.0, k), 1.0);      // күңгірт / ашық шаршы
}
)";

// 4-тапсырма: ӘДЕЙІ қате — бірінші жолдың соңында ";" жоқ, әрі vec5 деген тип жоқ.
const char* fragmentBrokenSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    vec5 c = vec5(vColor, 1.0, 1.0)
    FragColor = vec4(vColor, 1.0);
}
)";

// ---------------------------------------------------------------------
//  makeShader: өткен аптадағы 16 жолдық блок енді функцияда.
//  Компиляция тексеру (glGetShaderiv) мен линковка тексеру (glGetProgramiv)
//  — екі бөлек функция, шатастыруға болмайды.
// ---------------------------------------------------------------------
unsigned int compileShader(GLenum type, const char* src, const char* name) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);          // компиляция үшін
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        std::cerr << "ШЕЙДЕР КОМПИЛЯЦИЯСЫНЫҢ ҚАТЕСІ (" << name << "):\n"
                  << log << "\n";
        glDeleteShader(id);
        return 0;
    }
    return id;
}

unsigned int makeShader(const char* vsSrc, const char* fsSrc) {
    unsigned int vs = compileShader(GL_VERTEX_SHADER,   vsSrc, "vertex");
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fsSrc, "fragment");
    if (vs == 0 || fs == 0) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    int ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);          // линковка үшін
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "ЛИНКОВКА ҚАТЕСІ:\n" << log << "\n";
        glDeleteProgram(prog);
        prog = 0;
    }

    // Программаға қосылған соң шейдер объектілері енді керек емес
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ---------------------------------------------------------------------
//  Терезе өлшемі өзгергенде шақырылады
// ---------------------------------------------------------------------
void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// Перне бір рет басылғанын анықтау (toggle): алдыңғы кадрдағы күйді есте сақтаймыз,
// әйтпесе секундына жүздеген рет ауысады.
bool pressedOnce(GLFWwindow* window, int key, bool& wasPressed) {
    bool now = glfwGetKey(window, key) == GLFW_PRESS;
    bool result = now && !wasPressed;
    wasPressed = now;
    return result;
}

// ---------------------------------------------------------------------
//  Пернетақтаны тексеру. Әр кадрда шақырылады (glClear-дан БҰРЫН).
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    g_whiteBackground = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);

    static bool wPrev = false, iPrev = false, cPrev = false, yPrev = false, pPrev = false;
    if (pressedOnce(window, GLFW_KEY_W, wPrev)) {
        g_wireframe = !g_wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);
    }
    if (pressedOnce(window, GLFW_KEY_I, iPrev)) g_invert    = !g_invert;
    if (pressedOnce(window, GLFW_KEY_C, cPrev)) g_sameColor = !g_sameColor;
    if (pressedOnce(window, GLFW_KEY_Y, yPrev)) g_flipY     = !g_flipY;
    if (pressedOnce(window, GLFW_KEY_P, pPrev)) g_checker   = !g_checker;
}

// VAO + VBO жасау: әр вершина 6 float (позиция + түс)
void makeTriangle(const float* vertices, size_t bytes, unsigned int& vao, unsigned int& vbo) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);                                  // 1. VAO бірінші
    glBindBuffer(GL_ARRAY_BUFFER, vbo);                      // 2. содан соң VBO
    glBufferData(GL_ARRAY_BUFFER, bytes, vertices, GL_STATIC_DRAW);

    const int STRIDE = 6 * sizeof(float);                    // НАЗАР: екеуінде де 6
    // location = 0: позиция, ығысу 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    // location = 1: түс, ығысу 3 float (байтпен!)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// =====================================================================
//  MAIN
// =====================================================================
int main(int argc, char** argv) {

    // 4-тапсырма: "--broken" берілсе, әдейі қате шейдерді компиляциялаймыз
    bool brokenDemo = (argc > 1 && std::strcmp(argv[1], "--broken") == 0);

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
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
                                          "3-апта: шейдерлер, GLSL, интерполяция",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады. Видеокарта OpenGL 3.3-ті "
                     "қолдамауы мүмкін.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);              // осы терезенің контексі белсенді
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(0);                         // VSync өшірулі (1-апта)

    // -----------------------------------------------------------------
    //  3. GLAD: OpenGL функцияларын жүктеу
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    // === 4-ТАПСЫРМА: әдейі қате ===
    if (brokenDemo) {
        std::cout << "\n--broken: әдейі қате fragment шейдер компиляцияланады...\n\n";
        unsigned int bad = makeShader(vertexSrc, fragmentBrokenSrc);
        if (bad == 0) {
            std::cout << "makeShader 0 қайтарды: тексеру жұмыс істейді, қате ұсталды.\n";
        } else {
            glDeleteProgram(bad);
        }
        glfwTerminate();
        return 0;
    }

    std::cout << "Басқару: I - түсті төңкеру, C - бірдей түс, Y - төңкеру,\n"
                 "         P - шахмат, W - wireframe, Space - ақ фон, Esc - шығу\n";

    // -----------------------------------------------------------------
    //  ВЕРШИНА ДЕРЕКТЕРІ: x y z  r g b
    // -----------------------------------------------------------------
    // Базалық: үш бұрышы үш түсті — арасы градиентпен толады
    float vertices[] = {
        // позиция             // түс
        -0.5f, -0.5f, 0.0f,    1.0f, 0.0f, 0.0f,   // сол төменгі — қызыл
         0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,   // оң төменгі  — жасыл
         0.0f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f    // жоғарғы     — көк
    };

    // 2-тапсырма: үш вершинаға бірдей түс (қызғылт сары)
    float verticesSame[] = {
        -0.5f, -0.5f, 0.0f,    1.0f, 0.5f, 0.2f,
         0.5f, -0.5f, 0.0f,    1.0f, 0.5f, 0.2f,
         0.0f,  0.5f, 0.0f,    1.0f, 0.5f, 0.2f
    };

    unsigned int vaoGradient, vboGradient, vaoSame, vboSame;
    makeTriangle(vertices,     sizeof(vertices),     vaoGradient, vboGradient);
    makeTriangle(verticesSame, sizeof(verticesSame), vaoSame,     vboSame);

    // -----------------------------------------------------------------
    //  ШЕЙДЕР ПРОГРАММАЛАРЫ — бәрі makeShader арқылы
    //  [vertex: 0 = қалыпты, 1 = төңкерілген][fragment: 0 = қалыпты, 1 = инверсия, 2 = шахмат]
    // -----------------------------------------------------------------
    const char* vsList[2] = { vertexSrc, vertexFlipSrc };
    const char* fsList[3] = { fragmentSrc, fragmentInvertSrc, fragmentCheckerSrc };
    unsigned int programs[2][3];
    for (int v = 0; v < 2; ++v) {
        for (int f = 0; f < 3; ++f) {
            programs[v][f] = makeShader(vsList[v], fsList[f]);
            if (programs[v][f] == 0) {
                std::cerr << "Шейдер жасалмады — бағдарлама тоқтайды.\n";
                glfwTerminate();
                return -1;
            }
        }
    }

    // -----------------------------------------------------------------
    //  4. Негізгі цикл
    // -----------------------------------------------------------------
    double lastFpsTime = glfwGetTime();
    int    frameCount  = 0;

    while (!glfwWindowShouldClose(window)) {

        processInput(window);   // пернелер glClear-дан БҰРЫН

        // --- Экранды тазалау (1-аптадағы анимацияланған фон) ---
        float t = (float)glfwGetTime();
        float r = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
        float g = (std::sin(t * 1.2f) + 1.0f) * 0.5f * 0.3f;
        if (g_whiteBackground) glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        else                   glClearColor(r, g, 0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // --- Қай шейдер мен қай деректер ---
        int v = g_flipY ? 1 : 0;
        int f = g_checker ? 2 : (g_invert ? 1 : 0);

        glUseProgram(programs[v][f]);
        glBindVertexArray(g_sameColor ? vaoSame : vaoGradient);
        glDrawArrays(GL_TRIANGLES, 0, 3);

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
    glDeleteVertexArrays(1, &vaoGradient);
    glDeleteVertexArrays(1, &vaoSame);
    glDeleteBuffers(1, &vboGradient);
    glDeleteBuffers(1, &vboSame);
    for (int v = 0; v < 2; ++v)
        for (int f = 0; f < 3; ++f)
            glDeleteProgram(programs[v][f]);

    glfwTerminate();
    return 0;
}
