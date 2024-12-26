#include "imgui.h"
#include <glad/glad.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h> 

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif
static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

#include <learnopengl/shader.h>
#include "CoreHeader.h"
// 三角形的顶点数据 是在NDC范围  远是1 近是0.0
//远处的绿色三角形
float g_green_vertices[] = {
        1.0f,  1.0f, 0.9f, 0.0f, 1.0f, 0.0f,//绿色
        1.0f, -1.0f, 0.9f, 0.0f, 1.0f, 0.0f,//绿色
        -1.0f,  1.0f, 0.9f, 0.0f, 1.0f, 0.0f,//绿色
};
//近处的红色三角形
float g_red_vertices[] = {
    //     ---- 位置 ----         ---- 位置 ----
    1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f,//红色
    1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,//红色
    -1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f,//红色
};
uint32 g_indices[] =
{
      0, 1, 2, 
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
uint32 GenerateVAO(int32 verticesSize, float *vertices,int32 indicesSize=0,uint32 *indices=NULL);

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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(s_width,s_height,"OpenGLDemo",NULL,NULL);
    if(window == NULL)
    {
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 430";
    ImGui_ImplOpenGL3_Init(glsl_version);

    
    uint32 FarGreenTriVAO = GenerateVAO(sizeof(g_green_vertices), g_green_vertices,sizeof(g_indices), g_indices);
    uint32 NearRedTriVAO = GenerateVAO(sizeof(g_red_vertices), g_red_vertices,sizeof(g_indices), g_indices);

    std::string OpenEarlyZ_VSPath = SLN_SOURCE_CODE_DIR + std::string("OpenEarlyZ_VS.glsl");
    std::string OpenEarlyZ_FSPath = SLN_SOURCE_CODE_DIR + std::string("OpenEarlyZ_FS.glsl");
    Shader OpenEarlyZShader(OpenEarlyZ_VSPath.c_str(), OpenEarlyZ_FSPath.c_str());
    
    std::string CloseEarlyZ_VSPath = SLN_SOURCE_CODE_DIR + std::string("CloseEarlyZ_VS.glsl");
    std::string CloseEarlyZ_FSPath = SLN_SOURCE_CODE_DIR + std::string("CloseEarlyZ_FS.glsl");
    Shader CloseEarlyZShader(CloseEarlyZ_VSPath.c_str(), CloseEarlyZ_FSPath.c_str());

    //绑定buffer
    GLuint counterBuffer;
    glGenBuffers(1, &counterBuffer);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counterBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterBuffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);


    while(!glfwWindowShouldClose(window))
    {
        int WindowsWidth, WindowsHeight;
        glfwGetWindowSize(window, &WindowsWidth, &WindowsHeight);
        
        //帧初始化
        processInput(window);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        //绘制
        {
            //深度测试
            if(s_OpenDepthTest)
            {
                glEnable(GL_DEPTH_TEST);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
            else
            {
                glDisable(GL_DEPTH_TEST);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            //片元计数器置位0
            GLuint newValue = 0;
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterBuffer);
            glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &newValue);
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

            //是否开启Earlyz
            if(s_OpenEarlyZ)
            {
                OpenEarlyZShader.use();
            }
            else
            {
                CloseEarlyZShader.use();
            }

            //绘制两个三角形
            if(s_DrawRedTri)
            {
                glBindVertexArray(NearRedTriVAO);
                glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT,0);
            }
            if(s_DrawGreenTri)
            {
                glBindVertexArray(FarGreenTriVAO);
                glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT,0);
            }
                
            // 读取片元着色器执行数量
            s_FragmentCount = 0;
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterBuffer);
            glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &s_FragmentCount);
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
        }

        
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            continue;
        }

        //Imgui
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            {
                ImGui::Begin("Debug");

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::NewLine();
                
                ImGui::Checkbox("OpenEarlyZ",&s_OpenEarlyZ);
                ImGui::Checkbox("OpenDepthTest",&s_OpenDepthTest);
                ImGui::Checkbox("DrawGreenTri",&s_DrawGreenTri);
                ImGui::Checkbox("DrawRedTri",&s_DrawRedTri);
                ImGui::Text("Fragment Count = %d",(int)(s_FragmentCount));

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


uint32 GenerateVAO(int32 verticesSize, float *vertices,int32 indicesSize,uint32 *indices) {
    uint32 VAO;
    uint32 VBO;
    uint32 EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, GL_STATIC_DRAW);

    glGenBuffers(1,&EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize, indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return VAO;
}

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}