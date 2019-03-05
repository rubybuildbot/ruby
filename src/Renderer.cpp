#include "Renderer.hpp"
#include <iostream>
#include <glad/glad.h>
#include <fstream>
#include <streambuf>
#include <vector>

using namespace std;

Renderer::Renderer() : verticesCount(0) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        cout << "Error initializing SDL: " << SDL_GetError() << endl;
        exit(1);
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

    window = SDL_CreateWindow("ルビィ", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 512, SDL_WINDOW_OPENGL);
    glContext = SDL_GL_CreateContext(window);

    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress)) {
        cout << "Failed to initialize the OpenGL context." << std::endl;
        exit(1);
    }

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(window);

    vertexShader = compileShader("./glsl/vertex.glsl", GL_VERTEX_SHADER);
    fragmentShader = compileShader("./glsl/fragment.glsl", GL_FRAGMENT_SHADER);
    glProgram = linkProgram();
    glUseProgram(glProgram);

    GLuint vertexArrayRef;
    glGenVertexArrays(1, &vertexArrayRef);
    glBindVertexArray(vertexArrayRef);
    vertexArrayObject = vertexArrayRef;

    pointsBuffer = make_unique<RendererBuffer<Point>>();
    GLuint pointsIndex = findProgramAttribute("vertex_point");
    glEnableVertexAttribArray(pointsIndex);
    glVertexAttribIPointer(pointsIndex, 2, GL_SHORT, 0, NULL);

    colorsBuffer = make_unique<RendererBuffer<Color>>();
    GLuint colorIndex = findProgramAttribute("vertex_color");
    glEnableVertexAttribArray(colorIndex);
    glVertexAttribIPointer(colorIndex, 3, GL_UNSIGNED_BYTE, 0, NULL);
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &vertexArrayObject);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteProgram(glProgram);
    SDL_Quit();
}

string Renderer::openShaderSource(string filePath) const {
    ifstream file(filePath);
    string source;

    file.seekg(0, std::ios::end);
    source.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    source.assign((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    return source;
}

GLuint Renderer::compileShader(string filePath, GLenum shaderType) const {
    string source = openShaderSource(filePath);
    GLuint shader = glCreateShader(shaderType);
    const GLchar *src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        cout << "Shader compilation error." << endl;
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
        cout << string(errorLog.begin(), errorLog.end()) << endl;
        exit(1);
    }
    return shader;
}

GLuint Renderer::linkProgram() const {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        cout << "OpenGL program link error." << endl;
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
        vector<GLchar> errorLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, &errorLog[0]);
        cout << string(errorLog.begin(), errorLog.end()) << endl;
        exit(1);
    }
    return program;
}

GLuint Renderer::findProgramAttribute(string attribute) const {
    const GLchar *attrib = attribute.c_str();
    GLint index = glGetAttribLocation(glProgram, attrib);
    if (index < 0) {
        cout << "Attribute " << attribute << " not found in main OpenGL program." << endl;
        exit(1);
    }
    return index;
}

void Renderer::pushTriangle(Point (&points)[3], Color (&colors)[3]) {
    if (verticesCount + 3 > RENDERER_BUFFER_SIZE) {
        cout << "Renderer buffer full, forcing draw!" << endl;
        draw();
    }

    for (uint8_t i = 0; i < 3; i++) {
        pointsBuffer->set(verticesCount, points[i]);
        colorsBuffer->set(verticesCount, colors[i]);
        verticesCount++;
    }
    return;
}

void Renderer::draw() {
    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verticesCount);
    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    while (true) {
        GLenum result = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000);
        if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
            break;
        }
    }
    verticesCount = 0;
}

void Renderer::display() {
    draw();
    SDL_GL_SwapWindow(window);
}
