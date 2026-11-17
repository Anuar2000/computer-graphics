# Компьютерлік графика — апталық тапсырмалар

OpenGL 3.3 Core · C++17 · GLFW · GLAD

Әр апта — бөлек папка, ішінде өз `main.cpp` және `README.md` (тапсырмалар, скриншоттар,
қорғауға жауаптар). Бір `cmake` барлық апталарды бөлек бағдарлама етіп құрастырады.

| Апта | Тақырып | Папка | Бағдарлама |
|------|---------|-------|------------|
| 1 | Терезе ашу, негізгі цикл | [`week01/`](week01) | `cg_week01` |
| 2 | Вершина буферлері (VBO, VAO), үшбұрыштар | [`week02/`](week02) | `cg_week02` |
| 3 | Шейдерлер, GLSL, интерполяция | [`week03/`](week03) | `cg_week03` |
| 4 | Uniform, уақыт, delta time | [`week04/`](week04) | `cg_week04` |
| 5 | Индекс буфері (EBO), примитив түрлері | [`week05/`](week05) | `cg_week05` |
| 6 | Түрлендіру матрицалары (GLM: translate, rotate, scale) | [`week06/`](week06) | `cg_week06` |
| 7 | Координаттар кеңістіктері, MVP, проекция (куб) | [`week07/`](week07) | `cg_week07` |
| 8 | Тереңдік буфері, back-face culling, бірнеше объект | [`week08/`](week08) | `cg_week08` |
| 9 | Камера моделі, LookAt, WASD | [`week09/`](week09) | `cg_week09` |
| 10 | Тінтуірмен еркін камера (yaw, pitch), fov зум | [`week10/`](week10) | `cg_week10` |
| 11 | Текстуралар: stb_image, UV, wrapping, фильтрлеу, mipmap | [`week11/`](week11) | `cg_week11` |
| 12 | Бірнеше текстура, текстуралық блоктар, mix() | [`week12/`](week12) | `cg_week12` |

## Құрылым

```
computer-graphics/
├── week01/ … week12/     әр аптаның коды, README және screenshots/
├── docs/SETUP.md         ортаны баптау (Windows / Linux / macOS)
├── external/             glad, stb (GLFW мен GLM-ді CMake өзі жүктейді)
├── assets/               текстуралар (11-аптадан бастап)
└── CMakeLists.txt
```

## Құрастыру және іске қосу

Ортаны баптау: [docs/SETUP.md](docs/SETUP.md).

```
cmake -B build
cmake --build build
```

Linux / macOS:

```
./build/bin/cg_week03
```

Windows (Visual Studio): `build\bin\Debug\cg_week03.exe`, немесе VS-те
**File → Open → Folder**, жоғарғы тізімнен керек аптаны (`cg_week01` … `cg_week12`) таңдап, F5.
