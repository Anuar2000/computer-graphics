// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week02" && git tag week02 && git push --tags
//
//  Қазіргі күйі: 6-АПТА — түрлендіру матрицалары (GLM)
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

// 6-апта: GLM
#include <glm/glm.hpp>                    // mat4, vec3 типтері
#include <glm/gtc/matrix_transform.hpp>   // translate, rotate, scale
#include <glm/gtc/type_ptr.hpp>           // value_ptr

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
const float RADIUS_MAX     = 0.6f;   // 0.6 + 0.25 (айналған төртбұрыштың жарты диагоналі) < 1.0
float g_speed  = SPEED_DEFAULT;
float g_radius = RADIUS_DEFAULT;
float g_angle  = 0.0f;               // орбитадағы бұрыш (радиан)

// 6-апта: режим (T пернесі): 0 — негізгі T·R·S, 1/2/3 — тапсырмалар
int g_mode = 0;

// ---------------------------------------------------------------------
//  6-АПТА: шейдерлер. uOffset пен uScale орнына бір mat4
// ---------------------------------------------------------------------
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 uTransform;
out vec3 vColor;
void main() {
    gl_Position = uTransform * vec4(aPos, 1.0);
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

    // 6-апта: T — режимді ауыстыру (негізгі → 1 → 2 → 3-тапсырма)
    static bool tWasPressed = false;
    bool tPressed = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
    if (tPressed && !tWasPressed) g_mode = (g_mode + 1) % 4;
    tWasPressed = tPressed;

    // Қосымша: R — бәрін бастапқы күйге қайтару
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        g_speed  = SPEED_DEFAULT;
        g_radius = RADIUS_DEFAULT;
        g_angle  = 0.0f;
    }
}

// ---------------------------------------------------------------------
//  6-апта: төртбұрышты сызу (VAO мен матрица алдын ала орнатылған)
// ---------------------------------------------------------------------
void drawQuad() {
    if (g_lineLoopMode) {
        // GL_LINE_LOOP барлық индексті бір контурға тұйықтайды,
        // сондықтан әр үшбұрышты бөлек сызамыз (3 индекстен)
        glDrawElements(GL_LINE_LOOP, 3, GL_UNSIGNED_INT, (void*)0);
        glDrawElements(GL_LINE_LOOP, 3, GL_UNSIGNED_INT, (void*)(3 * sizeof(unsigned int)));
    } else {
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);   // 6 — индекс саны
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
                                          "6-апта: түрлендіру матрицалары (GLM)",
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
                 "W/S - скорость, Q/E - радиус орбиты, R - сброс, T - режим\n";


    // === 2-АПТА: төртбұрыштың деректері мен буферлері ===

    // Вершинные данные в NDC (-1 .. 1): x y z  r g b.
    // 5-апта: төртбұрыш = 4 вершина + 6 индекс (6 вершинаның орнына).
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

    // 6-апта: бір ғана VAO — барлық режимде тек матрица өзгереді
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

    const char* modeName[4] = { "негізгі: T · R · S (орбита + өз центрінен айналу)",
                                "1-тапсырма: translate пен rotate орны ауыстырылған",
                                "2-тапсырма: сол төменгі бұрыштан айналу",
                                "3-тапсырма: үш төртбұрыш, үш матрица" };
    std::cout << "T - режимді ауыстыру. Қазір: " << modeName[g_mode] << "\n";

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

    // === 6-АПТА: uniform орнын цикл алдында БІР рет табамыз ===
    int locTransform = glGetUniformLocation(shader, "uTransform");
    float lastFrame = (float)glfwGetTime();   // НАЗАР: нөл емес, әйтпесе 1-кадрда dt өте үлкен


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
        float angle = g_angle;

        // --- Экранды тазалау ---
        float t = (float)glfwGetTime();
        // 1-апта, задание 2: частота выросла (0.5 -> 2.0, 0.3 -> 1.2), амплитуда 0.3 прежняя
        float r = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
        float g = (std::sin(t * 1.2f) + 1.0f) * 0.5f * 0.3f;

        if (g_whiteBackground) glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        else                   glClearColor(r, g, 0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // T басылса — консольге жаңа режимнің атын шығарамыз
        static int lastMode = g_mode;
        if (g_mode != lastMode) {
            std::cout << "Режим: " << modeName[g_mode] << "\n";
            lastMode = g_mode;
        }

        // === 2-АПТА: сызу командасы ===
        glUseProgram(shader);   // МІНДЕТТІ, әрі БІРІНШІ: uniform орнатудан бұрын
        glBindVertexArray(vao);

        if (g_mode == 0) {
            // 6-апта, негізгі: T · R · S. Соңғы жазылғаны вершинаға бірінші әсер етеді:
            // 1) кішірейту, 2) өз центрінен бұру, 3) орбитаға апару
            glm::mat4 m = glm::mat4(1.0f);   // бірлік матрица, ЦИКЛ ІШІНДЕ
            m = glm::translate(m, glm::vec3(std::cos(angle) * g_radius,
                                            std::sin(angle) * g_radius, 0.0f));
            m = glm::rotate(m, angle, glm::vec3(0.0f, 0.0f, 1.0f));
            m = glm::scale(m, glm::vec3(0.6f, 0.6f, 1.0f));

            glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(m));
            drawQuad();
        }

        if (g_mode == 1) {
            // 1-тапсырма: rotate мен translate орны ауыстырылды → R · T · S.
            // Енді жылжыту векторының өзі angle-ге бұрылады: орны (cos 2a, sin 2a) * радиус
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::rotate(m, angle, glm::vec3(0.0f, 0.0f, 1.0f));
            m = glm::translate(m, glm::vec3(std::cos(angle) * g_radius,
                                            std::sin(angle) * g_radius, 0.0f));
            m = glm::scale(m, glm::vec3(0.6f, 0.6f, 1.0f));

            glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(m));
            drawQuad();
        }

        if (g_mode == 2) {
            // 2-тапсырма: сол төменгі бұрыш (-0.3, -0.3) айналу нүктесі болады.
            // Вершинаға әсер ету реті: бұрышты центрге апару → бұру → кері қайтару → кішірейту
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::scale(m, glm::vec3(0.6f, 0.6f, 1.0f));
            m = glm::translate(m, glm::vec3(-0.3f, -0.3f, 0.0f));   // 3) кері қайтару
            m = glm::rotate(m, angle, glm::vec3(0.0f, 0.0f, 1.0f)); // 2) бұру
            m = glm::translate(m, glm::vec3(0.3f, 0.3f, 0.0f));     // 1) бұрышты (0,0)-ге апару

            glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(m));
            drawQuad();
        }

        if (g_mode == 3) {
            // 3-тапсырма: бір VAO, үш матрица, үш сызу. Геометрия қайта жіберілмейді

            // 1-төртбұрыш: ортада, өз центрінен айналады
            glm::mat4 m1 = glm::mat4(1.0f);
            m1 = glm::rotate(m1, angle, glm::vec3(0.0f, 0.0f, 1.0f));
            m1 = glm::scale(m1, glm::vec3(0.5f, 0.5f, 1.0f));
            glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(m1));
            drawQuad();

            // 2-төртбұрыш: 0.6 радиуспен орбитада, кері бағытта екі есе жылдам айналады
            glm::mat4 m2 = glm::mat4(1.0f);
            m2 = glm::translate(m2, glm::vec3(std::cos(angle) * 0.6f,
                                              std::sin(angle) * 0.6f, 0.0f));
            m2 = glm::rotate(m2, -2.0f * angle, glm::vec3(0.0f, 0.0f, 1.0f));
            m2 = glm::scale(m2, glm::vec3(0.3f, 0.3f, 1.0f));
            glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(m2));
            drawQuad();

            // 3-төртбұрыш: орбитаның қарсы жағында (angle + PI), бұрылмайды
            glm::mat4 m3 = glm::mat4(1.0f);
            m3 = glm::translate(m3, glm::vec3(std::cos(angle + 3.14159265f) * 0.6f,
                                              std::sin(angle + 3.14159265f) * 0.6f, 0.0f));
            m3 = glm::scale(m3, glm::vec3(0.3f, 0.3f, 1.0f));
            glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(m3));
            drawQuad();
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
    glDeleteBuffers(1, &ebo);      // 5-апта: EBO да өшіріледі
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
