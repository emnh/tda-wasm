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

class State {
public:
  float width = 1.0;
  float height = 1.0;
  bool movingDown = false;
  bool movingUp = false;
  bool movingLeft = false;
  bool movingRight = false;
  float fov = 45.0;
  double pitch = -89.0;
  double yaw = 0.0;;
  int numTextures = 0;
};

State state;

extern "C" void EMSCRIPTEN_KEEPALIVE
setSize(float w, float h) {
  state.width = w;
  state.height = h;
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

extern "C" void EMSCRIPTEN_KEEPALIVE
addZoom(double fov) {
  state.fov += fov;
  if (state.fov > 45.0f) {
    state.fov = 45.0f;
  }
  if (state.fov < 1.0f) {
    state.fov = 1.0f;
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
  geometry.normal =
    (GLfloat*)
    EM_ASM_INT({
          return UI.getNormal();
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
  glm::vec3 cameraPos   = glm::vec3(0.0f, 10.0f,  0.0f);
  glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f,  0.0f);
  glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

  glm::mat4 getView() {
    return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
  }

  glm::mat4 getMVP()
  {
    glm::mat4 Projection = glm::perspective(glm::radians(state.fov), state.width / state.height, 0.1f, 100000.f);
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

class UniformArgs {
public:
  double startTime;
  double lastTime;
  double now;
  double tick;
  double elapsed;
  Camera camera;
  glm::vec3 light = glm::vec3(1.0, 1000.0, 1.0);
};

class Material {
public:
  const char* vertexSourceFile;
	const char* fragmentSourceFile;
  GLuint shaderProgram;
  GLint u_time;
  GLint u_light;
  GLint u_mvp;
  GLint u_tex;
  GLuint textureID;
  GLuint activeTextureID;

  void update(UniformArgs uniformArgs) {
    // uniformArgs.light.x = cos(uniformArgs.elapsed);
    // uniformArgs.light.z = sin(uniformArgs.elapsed);

    // Set uniforms
    glUniform1f(u_time, uniformArgs.elapsed);
    glUniform3fv(u_light, 1, glm::value_ptr(uniformArgs.light));
    glUniform1i(u_tex, activeTextureID);
    glm::mat4 mvp = uniformArgs.camera.getMVP();
    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

    // Activate texture
    // cerr << "activating tex: " << activeTextureID << " with " << textureID << endl;
    // glActiveTexture(GL_TEXTURE0 + activeTextureID);
    // glBindTexture(GL_TEXTURE_2D, textureID);
  }

  void init() {
    // Read shader sources
		std::string* vertexSourceStr = new string();
		readFile(vertexSourceFile, *vertexSourceStr);
		const char* vertexSource = vertexSourceStr->c_str();
		std::string* fragmentSourceStr = new string();
		readFile(fragmentSourceFile, *fragmentSourceStr);
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
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkLinkError(shaderProgram);
    glUseProgram(shaderProgram);

    // Get uniform locations
    u_time = glGetUniformLocation(shaderProgram, "u_time");
    u_light = glGetUniformLocation(shaderProgram, "u_light");
    u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
    // Get texture uniform
    u_tex = glGetUniformLocation(shaderProgram, "u_tex");
    
    // Initialize texture
    initTexture();
  }
  
  virtual void initTexture() {
    int imgWidth;
    int imgHeight;
    char* imageData = emscripten_get_preloaded_image_data("images/grass.jpg", &imgWidth, &imgHeight);
    //cerr << "imageData: " << (int) imageData << " " << imgWidth << " " << imgHeight << endl;
    
    // Create one OpenGL texture
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    activeTextureID = state.numTextures;
    glActiveTexture(GL_TEXTURE0 + activeTextureID);
    state.numTextures++;
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
};

class CubeMaterial : public Material {
  virtual void initTexture() {
    const char* images[] = {
      "images/Box_Right.jpg",
      "images/Box_Left.jpg",
      "images/Box_Top.jpg",
      "images/Box_Bottom.jpg",
      "images/Box_Front.jpg",
      "images/Box_Back.jpg"
    };
    GLint targets[] = {
       GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 
       GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 
       GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 
    };
    glGenTextures(1, &textureID);
    activeTextureID = state.numTextures;
    glActiveTexture(GL_TEXTURE0 + activeTextureID);
    cerr << "cube active: " << activeTextureID << endl;
    state.numTextures++;
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    for (int j = 0; j < 6; j++) {
        int imgWidth;
        int imgHeight;
        char* imageData = emscripten_get_preloaded_image_data(images[j], &imgWidth, &imgHeight);
        // cerr << "imgsize: " << imgWidth << " " << imgHeight << endl;
        glTexImage2D(targets[j], 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
  }
};


class Mesh {
public:
  Material material;
  Geometry geometry;

  GLuint vao;
  GLuint vbo;
  GLuint vboUV;
  GLuint vboNormal;
  GLuint vindex;

  Mesh() {
  }

  void init() {
    // Use program
    glUseProgram(material.shaderProgram);

    // Create vertex array object
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // Create a Vertex Buffer Object and copy the vertex data to it
    glGenBuffers(1, &vbo);

    // Upload position data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, geometry.count * 3 * sizeof(GLfloat), geometry.position, GL_STATIC_DRAW);

    // Specify the layout of the vertex position data
    GLint posAttrib = glGetAttribLocation(material.shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Create a Vertex Buffer Object and copy the vertex data to it
    glGenBuffers(1, &vboUV);
    
    // Upload UV data
    glBindBuffer(GL_ARRAY_BUFFER, vboUV);
    glBufferData(GL_ARRAY_BUFFER, geometry.count * 2 * sizeof(GLfloat), geometry.uv, GL_STATIC_DRAW);

    // Specify the layout of the vertex uvs 
    GLint uvAttrib = glGetAttribLocation(material.shaderProgram, "uv");
    glEnableVertexAttribArray(uvAttrib);
    glVertexAttribPointer(uvAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    // Create a Vertex Buffer Object and copy the vertex data to it
    glGenBuffers(1, &vboNormal);
    
    // Upload UV data
    glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
    glBufferData(GL_ARRAY_BUFFER, geometry.count * 3 * sizeof(GLfloat), geometry.normal, GL_STATIC_DRAW);

    // Specify the layout of the vertex normals 
    GLint normalAttrib = glGetAttribLocation(material.shaderProgram, "normal");
    glEnableVertexAttribArray(normalAttrib);
    glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
    // Index
    glGenBuffers(1, &vindex);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vindex);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, geometry.indexCount * 3 * sizeof(GLint), geometry.index, GL_STATIC_DRAW);
  }

  void draw(UniformArgs uniformArgs) {
    // Use program
    glUseProgram(material.shaderProgram);
    // Bind vertex array object
    glBindVertexArray(vao);
    // Bind main buffer again for drawing
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vindex);
    // Update material
    material.update(uniformArgs);
    // Draw
    glDrawElements(GL_TRIANGLES, 3 * geometry.indexCount, GL_UNSIGNED_INT, (void*) 0);
  }
};

int main()
{
    EM_ASM(UI.allReady());
    
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

    // Create geometry
    EM_ASM({
          const THREE = window.UI.THREE;
          var sz = 100.0;
          window.UI.geometry = new THREE.PlaneBufferGeometry(sz, sz, 256, 256);
        });
    Geometry geometry = loadGeometry();
    
    // Create material
    Material material;
    material.vertexSourceFile = "shaders/vertex.glsl";
    material.fragmentSourceFile = "shaders/fragment.glsl";
    material.init();

    // Create mesh
    Mesh terrain;
    terrain.geometry = geometry;
    terrain.material = material;
    terrain.init();

    // Create another geometry
    EM_ASM({
          const THREE = window.UI.THREE;
          window.UI.geometry = new THREE.SphereBufferGeometry(2, 32, 20);
        });
    Geometry geometry2 = loadGeometry();
    
    // Create material
    Material material2;
    material2.vertexSourceFile = "shaders/defaultVertex.glsl";
    material2.fragmentSourceFile = "shaders/defaultFragment.glsl";
    material2.init();

    // Create mesh
    Mesh sphere;
    sphere.geometry = geometry2;
    sphere.material = material2;
    sphere.init();

    // Create another geometry
    EM_ASM({
          const THREE = window.UI.THREE;
          var sz = 100000.0;
          window.UI.geometry = new THREE.BoxBufferGeometry(sz, sz, sz, 1, 1, 1);
        });
    Geometry geometry3 = loadGeometry();
    
    // Create material
    CubeMaterial material3;
    material3.vertexSourceFile = "shaders/skyVertex.glsl";
    material3.fragmentSourceFile = "shaders/skyFragment.glsl";
    material3.init();

    // Create mesh
    Mesh skybox;
    skybox.geometry = geometry3;
    skybox.material = material3;
    skybox.init();

    // Misc loop vars
    UniformArgs uniformArgs;
    uniformArgs.startTime = emscripten_get_now();
    uniformArgs.lastTime = uniformArgs.startTime;
    
    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);
    // Set clear color
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    loop = [&]
    {
        uniformArgs.now = emscripten_get_now();
        uniformArgs.tick = (uniformArgs.now - uniformArgs.lastTime) / 1000.0;
        uniformArgs.lastTime = uniformArgs.now;
        uniformArgs.elapsed = (uniformArgs.now - uniformArgs.startTime) / 1000.0;
        uniformArgs.camera.update(uniformArgs.tick);

        glViewport(0, 0, state.width, state.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        terrain.draw(uniformArgs);
        sphere.draw(uniformArgs);
        skybox.draw(uniformArgs);
    };

    emscripten_set_main_loop(main_loop, 0, true);

    return EXIT_SUCCESS;
}
