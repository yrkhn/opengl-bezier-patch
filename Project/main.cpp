// Part3b_Procedural3D_fixed.cpp
// Procedural 3D texture computed in fragment shader using world coordinates.
// Fixed: corrected shader program creation and function signatures.
// Build in Visual Studio 2022: add glad.c to project, link GLFW/GLM, opengl32.lib.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <cmath>

int WIN_W = 1200, WIN_H = 800;
GLFWwindow* window = nullptr;

// ---------- shaders ----------
const char* vs_common = R"GLSL(
#version 330 core
layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNormal;
uniform mat4 uMVP;
uniform mat4 uModel;
out vec3 worldPos;
out vec3 worldNormal;
void main(){
    worldPos = vec3(uModel * vec4(inPos,1.0));
    worldNormal = mat3(transpose(inverse(uModel))) * inNormal;
    gl_Position = uMVP * vec4(inPos,1.0);
}
)GLSL";

const char* fs_proc3d = R"GLSL(
#version 330 core
in vec3 worldPos;
in vec3 worldNormal;
out vec4 FragColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec3 specularColor;
uniform float shininess;

// cheap hash for 3D
float hash32(vec3 p){
    return fract(sin(dot(p, vec3(127.1,311.7,74.7))) * 43758.5453123);
}
float fbm(vec3 p){
    float v=0.0; float a=0.5;
    for(int i=0;i<5;i++){
        v += a * hash32(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}
vec3 proceduralColor3D(vec3 p){
    vec3 q = p * 1.5;
    float noiseVal = fbm(q * 0.8);
    float veins = sin( (q.x + 0.8*noiseVal) * 6.28318 );
    float t = 0.5 + 0.5 * veins;
    vec3 base1 = vec3(0.9,0.85,0.75);
    vec3 base2 = vec3(0.18,0.12,0.06);
    vec3 marble = mix(base1, base2, smoothstep(0.0,1.0,t + 0.2*noiseVal));
    float speck = smoothstep(0.6, 0.9, fract(noiseVal*10.0));
    marble = mix(marble, vec3(0.05,0.02,0.01), speck*0.6);
    return clamp(marble, 0.0, 1.0);
}
void main(){
    vec3 N = normalize(worldNormal);
    vec3 L = normalize(lightPos - worldPos);
    vec3 V = normalize(viewPos - worldPos);
    vec3 R = reflect(-L,N);
    float diff = max(dot(N,L), 0.0);
    float spec = 0.0; if(diff>0.0) spec = pow(max(dot(R,V),0.0), shininess);
    vec3 texcol = proceduralColor3D(worldPos);
    vec3 ambient = 0.06 * texcol;
    vec3 diffuse = diff * texcol;
    vec3 specular = spec * specularColor;
    vec3 col = (ambient + diffuse + specular) * lightColor;
    FragColor = vec4(col, 1.0);
}
)GLSL";

const char* fs_normals_debug = R"GLSL(
#version 330 core
in vec3 worldNormal;
out vec4 FragColor;
void main(){ vec3 N = normalize(worldNormal); FragColor = vec4(N*0.5 + 0.5, 1.0); }
)GLSL";

// ---------- shader helpers ----------
GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[8192]; glGetShaderInfoLog(s, sizeof(log), nullptr, log); std::cerr << "Shader compile error:\n" << log << "\n"; }
    return s;
}
GLuint makeProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[8192]; glGetProgramInfoLog(p, sizeof(log), nullptr, log); std::cerr << "Program link error:\n" << log << "\n"; }
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

// ---------- Bezier patch utilities (same as before) ----------
void bernstein3(float t, float& b0, float& b1, float& b2, float& b3) {
    float it = 1.0f - t;
    b0 = it * it * it; b1 = 3.0f * t * it * it; b2 = 3.0f * t * t * it; b3 = t * t * t;
}
glm::vec3 evalPatch(const std::array<glm::vec3, 16>& ctrl, float u, float v) {
    float bu[4], bv[4]; bernstein3(u, bu[0], bu[1], bu[2], bu[3]); bernstein3(v, bv[0], bv[1], bv[2], bv[3]);
    glm::vec3 P(0.0f);
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) P += ctrl[i * 4 + j] * (bu[i] * bv[j]);
    return P;
}
void buildPatchMesh(const std::array<glm::vec3, 16>& ctrl, int res,
    std::vector<float>& vdata, std::vector<unsigned int>& idx) {
    vdata.clear(); idx.clear();
    int N = res;
    std::vector<glm::vec3> pos((N + 1) * (N + 1));
    for (int i = 0; i <= N; i++) for (int j = 0; j <= N; j++) {
        float u = float(i) / N, v = float(j) / N;
        pos[i * (N + 1) + j] = evalPatch(ctrl, u, v);
    }
    std::vector<glm::vec3> normals((N + 1) * (N + 1), glm::vec3(0.0f));
    for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) {
        int a = i * (N + 1) + j, b = (i + 1) * (N + 1) + j, c = i * (N + 1) + (j + 1), d = (i + 1) * (N + 1) + (j + 1);
        idx.push_back(a); idx.push_back(b); idx.push_back(d);
        idx.push_back(a); idx.push_back(d); idx.push_back(c);
        glm::vec3 pA = pos[a], pB = pos[b], pD = pos[d], pC = pos[c];
        glm::vec3 n1 = glm::normalize(glm::cross(pB - pA, pD - pB));
        glm::vec3 n2 = glm::normalize(glm::cross(pD - pA, pC - pD));
        normals[a] += (n1 + n2);
        normals[b] += n1;
        normals[d] += (n1 + n2);
        normals[c] += n2;
    }
    for (auto& n : normals) if (glm::length(n) > 0.0f) n = glm::normalize(n);
    for (size_t i = 0; i < pos.size(); ++i) {
        vdata.push_back(pos[i].x); vdata.push_back(pos[i].y); vdata.push_back(pos[i].z);
        vdata.push_back(normals[i].x); vdata.push_back(normals[i].y); vdata.push_back(normals[i].z);
    }
}

// ---------- globals ----------
std::array<glm::vec3, 16> ctrlInit = {
    glm::vec3(-1.5f,-1.5f,0.0f), glm::vec3(-0.5f,-1.5f,0.0f), glm::vec3(0.5f,-1.5f,0.0f), glm::vec3(1.5f,-1.5f,0.0f),
    glm::vec3(-1.5f,-0.5f,0.0f), glm::vec3(-0.5f,-0.5f,1.0f), glm::vec3(0.5f,-0.5f,1.0f), glm::vec3(1.5f,-0.5f,0.0f),
    glm::vec3(-1.5f, 0.5f,0.0f), glm::vec3(-0.5f, 0.5f,1.0f), glm::vec3(0.5f, 0.5f,1.0f), glm::vec3(1.5f, 0.5f,0.0f),
    glm::vec3(-1.5f, 1.5f,0.0f), glm::vec3(-0.5f, 1.5f,0.0f), glm::vec3(0.5f, 1.5f,0.0f), glm::vec3(1.5f, 1.5f,0.0f)
};
std::array<glm::vec3, 16> ctrl = ctrlInit;
int selectedCtrl = 0;
int patchRes = 12;

GLuint progProc3D = 0;
GLuint progNormalsDbg = 0;
GLuint vaoPatch = 0, vboPatch = 0, eboPatch = 0;
int idxCount = 0;

float camDist = 6.0f; float yaw = -20.0f, pitch = -20.0f;
bool leftDown = false; double lastX = 0, lastY = 0;
bool wireframe = false;
bool showNormals = false;

void rebuildGL() {
    std::vector<float> vdata; std::vector<unsigned int> idx;
    buildPatchMesh(ctrl, patchRes, vdata, idx);
    idxCount = (int)idx.size();
    if (vaoPatch == 0) glGenVertexArrays(1, &vaoPatch);
    if (vboPatch == 0) glGenBuffers(1, &vboPatch);
    if (eboPatch == 0) glGenBuffers(1, &eboPatch);
    glBindVertexArray(vaoPatch);
    glBindBuffer(GL_ARRAY_BUFFER, vboPatch);
    glBufferData(GL_ARRAY_BUFFER, vdata.size() * sizeof(float), vdata.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboPatch);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    GLsizei stride = 6 * sizeof(float);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
}

void cursor_cb(GLFWwindow*, double x, double y) {
    if (leftDown) {
        float dx = float(x - lastX), dy = float(y - lastY);
        yaw += dx * 0.2f; pitch += dy * 0.2f;
        if (pitch > 89) pitch = 89; if (pitch < -89) pitch = -89;
    }
    lastX = x; lastY = y;
}
void mouse_btn_cb(GLFWwindow* win, int button, int action, int mods) {
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT) leftDown = (action == GLFW_PRESS);
}
void scroll_cb(GLFWwindow*, double, double yoff) {
    camDist -= (float)yoff * 0.5f;
    if (camDist < 1.0f) camDist = 1.0f; if (camDist > 50.0f) camDist = 50.0f;
}
void fb_cb(GLFWwindow*, int w, int h) { WIN_W = w; WIN_H = h; glViewport(0, 0, w, h); }

int main() {
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(WIN_W, WIN_H, "Part3b - Procedural 3D Texture (fixed)", NULL, NULL);
    if (!window) { std::cerr << "Window create failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cerr << "GLAD failed\n"; return -1; }

    glfwSetCursorPosCallback(window, cursor_cb);
    glfwSetMouseButtonCallback(window, mouse_btn_cb);
    glfwSetScrollCallback(window, scroll_cb);
    glfwSetFramebufferSizeCallback(window, fb_cb);

    progProc3D = makeProgram(vs_common, fs_proc3d);
    progNormalsDbg = makeProgram(vs_common, fs_normals_debug);

    rebuildGL();

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    std::cout << "Part3b (fixed): Procedural 3D texture. + / - res, [/] select control, X/Y/Z move, T wireframe, R reset\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;
        double now = glfwGetTime();
        static double lastKey = 0;
        double debounce = 0.12;
        if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS && now - lastKey > debounce) { selectedCtrl = (selectedCtrl - 1 + 16) % 16; std::cout << "sel=" << selectedCtrl << "\n"; lastKey = now; }
        if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS && now - lastKey > debounce) { selectedCtrl = (selectedCtrl + 1) % 16; std::cout << "sel=" << selectedCtrl << "\n"; lastKey = now; }
        float step = 0.03f;
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) { if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ctrl[selectedCtrl].x -= step; else ctrl[selectedCtrl].x += step; rebuildGL(); }
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) { if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ctrl[selectedCtrl].y -= step; else ctrl[selectedCtrl].y += step; rebuildGL(); }
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) { if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ctrl[selectedCtrl].z -= step; else ctrl[selectedCtrl].z += step; rebuildGL(); }
        if ((glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) && now - lastKey > debounce) { patchRes = std::min(64, patchRes + 1); rebuildGL(); std::cout << "res=" << patchRes << "\n"; lastKey = now; }
        if ((glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) && now - lastKey > debounce) { patchRes = std::max(2, patchRes - 1); rebuildGL(); std::cout << "res=" << patchRes << "\n"; lastKey = now; }
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && now - lastKey > debounce) { wireframe = !wireframe; glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL); std::cout << "wire=" << wireframe << "\n"; lastKey = now; }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && now - lastKey > debounce) { ctrl = ctrlInit; patchRes = 12; rebuildGL(); camDist = 6; yaw = -20; pitch = -20; std::cout << "reset\n"; lastKey = now; }

        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -camDist));
        view = glm::rotate(view, glm::radians(pitch), glm::vec3(1, 0, 0));
        view = glm::rotate(view, glm::radians(yaw), glm::vec3(0, 1, 0));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)WIN_W / WIN_H, 0.1f, 100.0f);
        glm::mat4 mvp = proj * view * glm::mat4(1.0f);

        glViewport(0, 0, WIN_W, WIN_H);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 invView = glm::inverse(view);
        glm::vec3 camPos = glm::vec3(invView[3]);

        if (idxCount > 0) {
            glUseProgram(progProc3D);
            glUniformMatrix4fv(glGetUniformLocation(progProc3D, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(glGetUniformLocation(progProc3D, "uModel"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
            glUniform3fv(glGetUniformLocation(progProc3D, "lightPos"), 1, glm::value_ptr(camPos));
            glUniform3fv(glGetUniformLocation(progProc3D, "lightColor"), 1, glm::value_ptr(glm::vec3(1.0f)));
            glUniform3fv(glGetUniformLocation(progProc3D, "viewPos"), 1, glm::value_ptr(camPos));
            glUniform3fv(glGetUniformLocation(progProc3D, "specularColor"), 1, glm::value_ptr(glm::vec3(1.0f)));
            glUniform1f(glGetUniformLocation(progProc3D, "shininess"), 64.0f);
            glBindVertexArray(vaoPatch);
            glDrawElements(GL_TRIANGLES, idxCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    if (vaoPatch) glDeleteVertexArrays(1, &vaoPatch);
    if (vboPatch) glDeleteBuffers(1, &vboPatch);
    if (eboPatch) glDeleteBuffers(1, &eboPatch);
    if (progProc3D) glDeleteProgram(progProc3D);
    if (progNormalsDbg) glDeleteProgram(progNormalsDbg);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
