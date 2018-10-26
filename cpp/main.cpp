#include <functional>
#include <vector>
#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <fstream>
#include <streambuf>

#include <emscripten.h>
#include <emscripten/html5.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES3/gl3.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr

using namespace std;

// an example of something we will control from the javascript side
bool background_is_black = true;
float width = 1.0;
float height = 1.0;

class State {
public:
  bool movingDown = false;
  bool movingUp = false;
  bool movingLeft = false;
  bool movingRight = false;
  double pitch;
  double yaw;
};

State state;

// the function called by the javascript code
extern "C" void EMSCRIPTEN_KEEPALIVE toggle_background_color() { background_is_black = !background_is_black; }

extern "C" void EMSCRIPTEN_KEEPALIVE
setSize(float w, float h) {
  width = w;
  height = h;
}

extern "C" void EMSCRIPTEN_KEEPALIVE
moveDown(bool on) {
  state.movingDown = on;
}

extern "C" void EMSCRIPTEN_KEEPALIVE
moveUp(bool on) {
  state.movingUp = on;
}

extern "C" void EMSCRIPTEN_KEEPALIVE
moveLeft(bool on) {
  state.movingLeft = on;
}

extern "C" void EMSCRIPTEN_KEEPALIVE
moveRight(bool on) {
  state.movingRight = on;
}

extern "C" void EMSCRIPTEN_KEEPALIVE
addYawPitch(double yaw, double pitch) {
  state.yaw += yaw;
  state.pitch += pitch;
  if (state.pitch > 89.0f) {
    state.pitch = 89.0f;
  }
  if (state.pitch < -89.0f) {
    state.pitch = -89.0f;
  }
}

std::function<void()> loop;
void main_loop() { loop(); }

void readFile(const char* fname, string& str) {
	std::ifstream t(fname);

	t.seekg(0, std::ios::end);
	str.reserve(t.tellg());
	t.seekg(0, std::ios::beg);

	str.assign((std::istreambuf_iterator<char>(t)),
							std::istreambuf_iterator<char>());
}

void checkShaderError(GLuint shader) {
  GLint isCompiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
  if(isCompiled == GL_FALSE)
  {
    GLint maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

    for (unsigned int i = 0; i < errorLog.size(); i++) {
      cerr << errorLog[i];
    }
    cerr << endl;

    // Provide the infolog in whatever manor you deem best.
    // Exit with failure.
    glDeleteShader(shader); // Don't leak the shader.
    return;
  }
}

void checkLinkError(GLuint program) {
  GLint isLinked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
  if (isLinked == GL_FALSE)
  {
    GLint maxLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> infoLog(maxLength);
    glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

    for (unsigned int i = 0; i < infoLog.size(); i++) {
      cerr << infoLog[i];
    }
    cerr << endl;

    // The program is useless now. So delete it.
    glDeleteProgram(program);

    // Provide the infolog in whatever manner you deem best.
    // Exit with failure.
    return;
  }
}

class Geometry {
public:
  int count;
  int indexCount;
  GLfloat* position;
  GLfloat* uv;
  GLfloat* normal;
  GLint* index;

  Geometry() {
    count = 0;
    indexCount = 0;
    position = NULL;
    uv = NULL;
    normal = NULL;
    index = NULL;
  }

};

Geometry loadGeometry() {
  Geometry geometry;
  geometry.count =
    EM_ASM_INT({
          return UI.getPositionCount();
        });
  geometry.indexCount =
    EM_ASM_INT({
          return UI.getIndexCount();
        });
  geometry.position =
    (GLfloat*)
    EM_ASM_INT({
          return UI.getPosition();
        });
  geometry.index =
    (GLint*)
    EM_ASM_INT({
          return UI.getIndex();
        });
  geometry.uv =
    (GLfloat*)
    EM_ASM_INT({
          return UI.getUV();
        });
  /*
  for (int i = 0; i < geometry.count * 3; i++) {
    cerr << "pos: " << geometry.position[i] << endl;
  }
  for (int i = 0; i < geometry.indexCount * 3; i++) {
    cerr << "pos: " << geometry.index[i] << endl;
  }
  */
  return geometry;
}

class Camera {
public:
  glm::vec3 cameraPos   = glm::vec3(0.0f, 1.0f,  0.0f);
  glm::vec3 cameraFront = glm::vec3(0.0f, -1.0f,  0.0f);
  glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

  glm::mat4 getView() {
    return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
  }

  glm::mat4 getMVP(float Translate, glm::vec2 const & Rotate)
  {
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.f);
    /*
    glm::mat4 View = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -Translate));
    View = glm::rotate(View, Rotate.y, glm::vec3(-1.0f, 0.0f, 0.0f));
    View = glm::rotate(View, Rotate.x, glm::vec3(0.0f, 1.0f, 0.0f));
    */
    glm::mat4 View = getView();
    glm::mat4 Model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    return Projection * View * Model;
  }

  void update(double tick) {
    glm::vec3 front;
    front.x = cos(glm::radians(state.pitch)) * cos(glm::radians(state.yaw));
    front.y = sin(glm::radians(state.pitch));
    front.z = cos(glm::radians(state.pitch)) * sin(glm::radians(state.yaw));
    cameraFront = glm::normalize(front);

    float cameraSpeed = 5.0f * tick; // adjust accordingly
    if (state.movingDown) {
        cameraPos -= cameraSpeed * cameraFront;
    }
    if (state.movingUp) {
        cameraPos += cameraSpeed * cameraFront;
    }
    if (state.movingLeft) {
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
    if (state.movingRight) {
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
  }
};

int main()
{
    EM_ASM(UI.allReady());
    Geometry geometry = loadGeometry();
    GLfloat* cubeData = geometry.position;
    GLfloat* cubeUVs = geometry.uv;
    GLint* index = geometry.index;
    //cerr << "positionCount: " << geometry.count << endl;
    //cerr << "indexCount: " << geometry.indexCount << endl;

    // Context configurations
    EmscriptenWebGLContextAttributes attrs;
    attrs.explicitSwapControl = 0;
    attrs.depth = 1;
    attrs.stencil = 1;
    attrs.antialias = 1;
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
    context = emscripten_webgl_create_context("canvas", &attrs);
    emscripten_webgl_make_context_current(context);


    /*
    const GLfloat x = 0.5f;
    const GLfloat y = 0.5f;
    GLfloat vertices[] = {
      0.0f, y, 0.0f, -y, -x, -y,
      -x, y, -x, -y, 0.0f, y
    };
    */

		std::string* vertexSourceStr = new string();
		readFile("shaders/vertex.glsl", *vertexSourceStr);
		const char* vertexSource = vertexSourceStr->c_str();
		std::string* fragmentSourceStr = new string();
		readFile("shaders/fragment.glsl", *fragmentSourceStr);
		const char* fragmentSource = fragmentSourceStr->c_str();

    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);
    checkShaderError(vertexShader);

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    checkShaderError(fragmentShader);

    // Link the vertex and fragment shader into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkLinkError(shaderProgram);
    glUseProgram(shaderProgram);

    GLint u_time = glGetUniformLocation(shaderProgram, "u_time");
    GLint u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");

    // Create vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // Create a Vertex Buffer Object and copy the vertex data to it
    GLuint vbo;
    glGenBuffers(1, &vbo);

    // Upload position data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, geometry.count * 3 * sizeof(GLfloat), cubeData, GL_STATIC_DRAW);

    // Specify the layout of the vertex position data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Create a Vertex Buffer Object and copy the vertex data to it
    GLuint vboUV;
    glGenBuffers(1, &vboUV);
    
    // Upload UV data
    glBindBuffer(GL_ARRAY_BUFFER, vboUV);
    glBufferData(GL_ARRAY_BUFFER, geometry.count * 2 * sizeof(GLfloat), cubeUVs, GL_STATIC_DRAW);

    // Specify the layout of the vertex uvs 
    GLint uvAttrib = glGetAttribLocation(shaderProgram, "uv");
    glEnableVertexAttribArray(uvAttrib);
    glVertexAttribPointer(uvAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    // Index
    GLuint vindex;
    glGenBuffers(1, &vindex);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vindex);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, geometry.indexCount * 3 * sizeof(GLint), geometry.index, GL_STATIC_DRAW);
    

    // Bind main buffer again for drawing
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vindex);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // TEXTURE
    
    int imgWidth;
    int imgHeight;
    char* imageData = emscripten_get_preloaded_image_data("images/grass.jpg", &imgWidth, &imgHeight);
    //cerr << "imageData: " << (int) imageData << " " << imgWidth << " " << imgHeight << endl;
    
    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);
    //cerr << "textureID: " << textureID << endl;

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

    // Texture parameters
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // When MAGnifying the image (no bigger mipmap available), use LINEAR filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // When MINifying the image, use a LINEAR blend of two mipmaps, each filtered LINEARLY too
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // Generate mipmaps, by the way.
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Get texture uniform
    GLint u_tex = glGetUniformLocation(shaderProgram, "u_tex");

    // Misc loop vars
    double startTime = emscripten_get_now();
    double lastTime = startTime;
    Camera camera;

    loop = [&]
    {
        // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        double now = emscripten_get_now();
        double tick = (now - lastTime) / 1000.0;
        lastTime = now;
        double elapsed = (now - startTime) / 1000.0;

        camera.update(tick);

        glViewport(0, 0, width, height);

        glUniform1f(u_time, elapsed);
        glUniform1i(u_tex, textureID);
        glActiveTexture(GL_TEXTURE0 + textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glm::vec2 rotate(0.0, M_PI * 3.0 / 4.0); // + (M_PI / 4.0) * (sin(elapsed) + 1.0) / 2.0);
        glm::mat4 mvp = camera.getMVP(1.0, rotate);
        glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

        if( background_is_black ) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        } else {
            glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, 3 * geometry.indexCount, GL_UNSIGNED_INT, (void*) 0);
    };

    emscripten_set_main_loop(main_loop, 0, true);

    return EXIT_SUCCESS;
}
