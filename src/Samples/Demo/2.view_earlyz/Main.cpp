#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include <glad/glad.h>
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#include <stb_image.h>

#include "HShader.h"

#include <GLFW/glfw3.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) &&                                 \
    !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif
static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

#include "CoreHeader.h"
#include <learnopengl/shader.h>
#include <learnopengl/filesystem.h>
// 三角形的顶点数据 是在NDC范围  远是1 近是0.0
// 远处的绿色三角形
float g_green_vertices[] = {
    //     ---- 位置 ----         ---- 颜色绿色 ----     ---- UV ----
    1.0f, 1.0f, 0.9f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 0.9f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.9f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
};
// 近处的红色三角形
float g_red_vertices[] = {
    //     ---- 位置 ----         ---- 颜色红色 ----     ---- UV ----
    1.0f, 1.0f, 0.9f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 0.9f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.9f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};
uint32 g_indices[] = {
    0,
    1,
    2,
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
uint32 GenerateVAO(int32 verticesSize, float* vertices, int32 indicesSize = 0,
                   uint32* indices = nullptr);

uint32 GenerateTexture(const char* picPath, int texDiskFormat, int texFormat, int filter)
{
    uint32 texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    int width, height, nrChanneLs;
    uint8* data = stbi_load(FileSystem::getPath(picPath).c_str(), &width, &height, &nrChanneLs, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);
    return texture;
}


static int s_width = 1000;
static int s_height = 1000;

// imgui 变量
static bool s_OpenEarlyZ = true;
static bool s_OpenDepthTest = true;
static bool s_DrawGreenTri = true;
static bool s_DrawRedTri = true;
static GLuint s_FragmentCount = 0;

int main()
{
    glfwSetErrorCallback(glfw_error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window =
        glfwCreateWindow(s_width, s_height, "OpenGLDemo", nullptr, nullptr);
    if (window == nullptr)
    {
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    auto glsl_version = "#version 430";
    ImGui_ImplOpenGL3_Init(glsl_version);

    uint32 FarGreenTriVAO = GenerateVAO(
        sizeof(g_green_vertices), g_green_vertices, sizeof(g_indices), g_indices);
    uint32 NearRedTriVAO = GenerateVAO(sizeof(g_red_vertices), g_red_vertices,
                                       sizeof(g_indices), g_indices);

    std::string EarlyZ_VSPath =
        SLN_SOURCE_CODE_DIR + std::string("EarlyZ_VS.glsl");
    std::string EarlyZ_FSPath =
        SLN_SOURCE_CODE_DIR + std::string("EarlyZ_FS.glsl");
    HShader EarlyZShader(EarlyZ_VSPath.c_str(),
                         EarlyZ_FSPath.c_str());
    EarlyZShader.SetInt("Texture0", 0);

    // 绑定buffer
    GLuint counterBuffer;
    glGenBuffers(1, &counterBuffer);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counterBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterBuffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr,
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    uint32 texture = GenerateTexture("resources/textures/wood.png", 0, 0, 0);

    while (!glfwWindowShouldClose(window))
    {
        int WindowsWidth, WindowsHeight;
        glfwGetWindowSize(window, &WindowsWidth, &WindowsHeight);

        // 帧初始化
        processInput(window);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        // 绘制
        {
            // 深度测试
            if (s_OpenDepthTest)
            {
                glEnable(GL_DEPTH_TEST);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
            else
            {
                glDisable(GL_DEPTH_TEST);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            // 片元计数器置位0
            GLuint newValue = 0;
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterBuffer);
            glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &newValue);
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

            // 是否开启Earlyz
            EarlyZShader.ModifyOrAddMacro("OPEN_EARLY_Z", s_OpenEarlyZ);
            EarlyZShader.Compile();
            EarlyZShader.Use();

            //纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);

            // 绘制两个三角形
            if (s_DrawRedTri)
            {
                glBindVertexArray(NearRedTriVAO);
                glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
            }
            if (s_DrawGreenTri)
            {
                glBindVertexArray(FarGreenTriVAO);
                glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
            }

            // 读取片元着色器执行数量
            s_FragmentCount = 0;
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterBuffer);
            glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint),
                               &s_FragmentCount);
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
        }

        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            continue;
        }

        // Imgui
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            {
                ImGui::Begin("Debug");

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                            1000.0f / io.Framerate, io.Framerate);
                ImGui::NewLine();

                ImGui::Checkbox("OpenEarlyZ", &s_OpenEarlyZ);
                ImGui::Checkbox("OpenDepthTest", &s_OpenDepthTest);
                ImGui::Checkbox("DrawGreenTri", &s_DrawGreenTri);
                ImGui::Checkbox("DrawRedTri", &s_DrawRedTri);
                ImGui::Text("Fragment Count = %d", static_cast<int>(s_FragmentCount));

                ImGui::NewLine();
                ImGui::Text("width = %d, height = %d", WindowsWidth, WindowsHeight);
                ImGui::Text("TotalPixel = %d", WindowsWidth * WindowsHeight);

                ImGui::End();
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

uint32 GenerateVAO(int32 verticesSize, float* vertices, int32 indicesSize,
                   uint32* indices)
{
    uint32 VAO;
    uint32 VBO;
    uint32 EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize, indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), static_cast<void*>(nullptr));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return VAO;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
