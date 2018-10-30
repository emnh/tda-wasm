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

#include <imgui.h>

using namespace std;


// BEGIN IMGUI STUFF


// ImGui - standalone example application for SDL2 + OpenGL ES 2 + Emscripten

#include <imgui.h>
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

bool g_done = false;
SDL_Window* g_window;
bool g_show_test_window = true;
bool g_show_another_window = false;
ImVec4 g_clear_color = ImColor(114, 144, 154);



// END IMGUI STUFF


const int maxTexturesPerMaterial = 10;
const int maxUniformsPerMaterial = 30;

class State {
public:
  float width = 1.0;
  float height = 1.0;
  bool movingDown = false;
  bool movingUp = false;
  bool movingLeft = false;
  bool movingRight = false;
  float fov = 45.0;
  // pitch, yaw: -23.3653 -53.9898
  double pitch = -23.3653;
  double yaw = -53.9898;
  int numTextures = 0;
};

State state;

extern "C" void EMSCRIPTEN_KEEPALIVE
setSize(float w, float h) {
  state.width = w;
  state.height = h;
  SDL_SetWindowSize(g_window, state.width, state.height);
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
  // 10.0
  // cameraPos: -25.7092 23.5091 26.9642
  glm::vec3 cameraPos   = glm::vec3(-21.5f, 22.8f,  27.5f);
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
      //cerr << "cameraPos: " << cameraPos.x << " " << cameraPos.y << " " << cameraPos.z << endl;
      //cerr << "pitch, yaw: " << state.pitch << " " << state.yaw << endl;
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
  int width;
  int height;
  double startTime;
  double lastTime;
  double now;
  double tick;
  double elapsed;
  Camera camera;
  glm::vec3 light = glm::vec3(-1.0, -2.0, 1.0);
  glm::vec2 axis;
};

class TextureInfo {
public:
  GLuint id;
  GLint type;
  GLuint activeID;
  GLint u_texture;
};

class UniformInfo {
public:
  GLint index;
  std::function<void(GLint, UniformArgs&)> updater;
};

class Material {
public:
  const char* vertexSourceFile;
	const char* fragmentSourceFile;
  GLuint shaderProgram;
  GLint u_time;
  GLint u_tick;
  GLint u_resolution;
  GLint u_light;
  GLint u_eye;
  GLint u_mvp;
  int uniformCount = 0;
  UniformInfo uniforms[maxUniformsPerMaterial];
  int textureCount = 0;
  TextureInfo textures[maxTexturesPerMaterial];

  virtual void update(UniformArgs& uniformArgs) {
    // uniformArgs.light.x = cos(uniformArgs.elapsed);
    // uniformArgs.light.z = sin(uniformArgs.elapsed);

    // Set uniforms
    glUniform1f(u_time, uniformArgs.elapsed);
    glUniform1f(u_tick, uniformArgs.tick);
    glUniform2f(u_resolution, uniformArgs.width, uniformArgs.height);
    glUniform3fv(u_light, 1, glm::value_ptr(uniformArgs.light));
    glUniform3fv(u_eye, 1, glm::value_ptr(uniformArgs.camera.cameraPos));
    for (int i = 0; i < uniformCount; i++) {
      uniforms[i].updater(uniforms[i].index, uniformArgs);
    }
    for (int i = 0; i < textureCount; i++) {
      glUniform1i(textures[i].u_texture, textures[i].activeID);
      glActiveTexture(GL_TEXTURE0 + textures[i].activeID);
      glBindTexture(textures[i].type, textures[i].id);
    }
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
    u_tick = glGetUniformLocation(shaderProgram, "u_tick");
    u_resolution = glGetUniformLocation(shaderProgram, "u_resolution");
    u_light = glGetUniformLocation(shaderProgram, "u_light");
    u_eye = glGetUniformLocation(shaderProgram, "u_eye");
    u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
    
    // Initialize texture
    initTexture();
    initRest();
  }

  virtual void initRest() {
  }

  int setTexture(const char* uniform, GLint id, int activeID) {
    int index = textureCount;
    textureCount++;
    TextureInfo& ti = textures[index];
    ti.id = id;
    ti.activeID = activeID;
    ti.type = GL_TEXTURE_2D;

    // Get texture uniform
    glUseProgram(shaderProgram);
    ti.u_texture = glGetUniformLocation(shaderProgram, uniform);
    return index;
  }

  int addUniform(const char* uniform, const std::function<void(GLint, UniformArgs&)>& updater) {
    int index = uniformCount;
    uniformCount++;
    uniforms[index].index = glGetUniformLocation(shaderProgram, uniform);
    uniforms[index].updater = updater;
    return index;
  }

  int addTexture(const char* uniform, GLint type) {
    int index = textureCount;
    textureCount++;
    TextureInfo& ti = textures[index];

    // Get texture uniform
    ti.u_texture = glGetUniformLocation(shaderProgram, uniform);
    ti.type = type;

    // Create one OpenGL texture
    glGenTextures(1, &ti.id);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    ti.activeID = state.numTextures;
    glActiveTexture(GL_TEXTURE0 + ti.activeID);
    state.numTextures++;
    glBindTexture(type, ti.id);
    return index;
  }

  virtual void initTexture() {
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

class WaterMaterial : public Material {
public:
  float centralTurbulence1 = 1.0;
  float centralTurbulence2 = 1.0;
  float rain = 1.0;
  float evaporation = 1.0;
  float waveDecay = 1.0;
  float viscosity = 0.0;
  float waterTransfer = 1.0;

  virtual void initRest() {
    addUniform("u_axis", [](GLint index, UniformArgs& uniformArgs) {
        glUniform2fv(index, 1, glm::value_ptr(uniformArgs.axis));
    });
    addUniform("u_centralTurbulence1", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, centralTurbulence1);
    });
    addUniform("u_centralTurbulence2", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, centralTurbulence2);
    });
    addUniform("u_rain", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, rain);
    });
    addUniform("u_evaporation", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, evaporation);
    });
    addUniform("u_waveDecay", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, waveDecay);
    });
    addUniform("u_viscosity", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, viscosity);
    });
    addUniform("u_waterTransfer", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, waterTransfer);
    });
  }
};

class TerrainMaterial : public Material {
public:
	float sunIntensity = 0.7;
	float lightsIntensity = 1.0;
	float beamIntensity = 1.0;
  float heightMultiplier = 20.0;
	int refractMethod = 1;
	int numLights = 9;
	float lightRadius = 15.0;
	float waterNormalFactor = 0.0;
	float fresnel = 0.02;
	float occlusion = 1.0;
	// TODO: use this in geometry creation instead of constant
	glm::vec2 mapSize = glm::vec2(100.0, 100.0); 

  virtual void initTexture() {
    // TODO: addImage function
    int imgWidth;
    int imgHeight;
    char* imageData = emscripten_get_preloaded_image_data("images/grass.jpg", &imgWidth, &imgHeight);
    addTexture("u_tex", GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    imageData = emscripten_get_preloaded_image_data("images/water.jpg", &imgWidth, &imgHeight);
    addTexture("u_waterTex", GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
  }

	virtual void initRest() {
    addUniform("u_sunIntensity", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, sunIntensity);
    });
    addUniform("u_lightsIntensity", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, lightsIntensity);
    });
    addUniform("u_beamIntensity", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, beamIntensity);
    });
    addUniform("u_heightMultiplier", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, heightMultiplier);
    });
    addUniform("u_refractMethod", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1i(index, refractMethod);
    });
    addUniform("u_numLights", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1i(index, numLights);
    });
    addUniform("u_lightRadius", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, lightRadius);
    });
    addUniform("u_waterNormalFactor", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, waterNormalFactor);
    });
    addUniform("u_fresnel", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, fresnel);
    });
    addUniform("u_occlusion", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform1f(index, occlusion);
    });
    addUniform("u_mapSize", [this](GLint index, UniformArgs& uniformArgs) {
        glUniform2fv(index, 1, glm::value_ptr(mapSize));
    });
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
    addTexture("u_tex", GL_TEXTURE_CUBE_MAP);
    for (int j = 0; j < 6; j++) {
        int imgWidth;
        int imgHeight;
        char* imageData = emscripten_get_preloaded_image_data(images[j], &imgWidth, &imgHeight);
        glTexImage2D(targets[j], 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
  }
};

// TODO: other way around
class CopyMaterial : public Material {
  virtual void initTexture() {
  }
};

template <class TMaterial>
class Mesh {
public:
  TMaterial material;
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

  void draw(UniformArgs& uniformArgs) {
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

  Mesh(const char* vertexSourceFile, const char* fragmentSourceFile) {
    geometry = loadGeometry();
    
    // Create material
    material.vertexSourceFile = vertexSourceFile;
    material.fragmentSourceFile = fragmentSourceFile;
    material.init();

    init();
  }
};

class RenderTarget {
public:
  int imgWidth;
  int imgHeight;
  int activeTextureID;
  GLuint textureID;
  GLuint framebuffer;

  RenderTarget(int width, int height) {
    imgWidth = width;
    imgHeight = height;
  }

  void init() {
    // Get extension
    emscripten_webgl_enable_extension(emscripten_webgl_get_current_context(), "EXT_color_buffer_float");
    emscripten_webgl_enable_extension(emscripten_webgl_get_current_context(), "OES_texture_float_linear");

    // Create and bind texture
    glGenTextures(1, &textureID);
    activeTextureID = state.numTextures;
    glActiveTexture(GL_TEXTURE0 + activeTextureID);
    state.numTextures++;
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Initialize empty texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, imgWidth, imgHeight, 0, GL_RGBA, GL_FLOAT, NULL);

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create and bind framebuffer
    glGenFramebuffers(1, &framebuffer);
    activate();

    // Attach texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);

    // Reset
    deactivate();
  }

  void activate() {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  }

  void deactivate() {
    glBindFramebuffer(GL_FRAMEBUFFER, NULL);
  }
};

int main()
{
    EM_ASM(UI.allReady());

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    g_window = SDL_CreateWindow("Tower Defense: Analysis", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 200, 200, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GLContext glcontext = SDL_GL_CreateContext(g_window);
    
    // Setup ImGui binding
    //ImGui_ImplSdl_Init(g_window);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplSDL2_InitForOpenGL(g_window, glcontext);
    const char* glsl_version = "#version 300 es";
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsDark();
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE sdlContext = emscripten_webgl_get_current_context();
    
    EM_ASM(UI.resize());
    
    // Context configurations
    EmscriptenWebGLContextAttributes attrs;
    attrs.explicitSwapControl = 0;
    attrs.depth = 1;
    attrs.stencil = 1;
    attrs.antialias = 1;
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = sdlContext;
    //context = emscripten_webgl_create_context("canvas2", &attrs);
    //emscripten_webgl_make_context_current(context);

    // Create terrain
    EM_ASM({
          const THREE = window.UI.THREE;
					// TODO: pass sz as parameter
          var sz = 100.0;
          window.UI.geometry = new THREE.PlaneBufferGeometry(sz, sz, 256, 256);
        });
    Mesh<TerrainMaterial> terrain("shaders/vertex.glsl", "shaders/fragment.glsl");

    // Create sphere
    EM_ASM({
          const THREE = window.UI.THREE;
          window.UI.geometry = new THREE.SphereBufferGeometry(2, 32, 20);
        });
    Mesh<Material> sphere("shaders/defaultVertex.glsl", "shaders/defaultFragment.glsl");

    // Create skybox
    EM_ASM({
          const THREE = window.UI.THREE;
          var sz = 100000.0;
          window.UI.geometry = new THREE.BoxBufferGeometry(sz, sz, sz, 1, 1, 1);
        });
    Mesh<CubeMaterial> skybox("shaders/skyVertex.glsl", "shaders/skyFragment.glsl");
    
    // Create height map render target
    int sz = 256;
    RenderTarget heightMap(sz, sz);
    heightMap.init();

    // Create height map mesh
    EM_ASM({
          const THREE = window.UI.THREE;
          var sz = 2.0;
          window.UI.geometry = new THREE.PlaneBufferGeometry(sz, sz, 1, 1);
        });
    Mesh<Material> heightMapMesh("shaders/heightmapVertex.glsl", "shaders/heightmapFragment.glsl");
    
    // Create water map render targets
    RenderTarget waterMap1(sz, sz);
    waterMap1.init();
    RenderTarget waterMap2(sz, sz);
    waterMap2.init();

    // Create water map mesh
    // Uses same geometry
    Mesh<WaterMaterial> waterMapMesh("shaders/watermapVertex.glsl", "shaders/watermapFragment.glsl");
    waterMapMesh.material.setTexture("u_heightmap", heightMap.textureID, heightMap.activeTextureID);
    int waterIndex =
      waterMapMesh.material.setTexture("u_watermap", waterMap2.textureID, waterMap2.activeTextureID);
    TextureInfo& waterTextureInfo = waterMapMesh.material.textures[waterIndex];
    
    // Create water wave mesh
    // Uses same geometry
    Mesh<WaterMaterial> waterWavesMesh("shaders/watermapVertex.glsl", "shaders/waterwavesFragment.glsl");
    waterWavesMesh.material.setTexture("u_heightmap", heightMap.textureID, heightMap.activeTextureID);
    int waterIndex2 =
      waterWavesMesh.material.setTexture("u_watermap", waterMap2.textureID, waterMap2.activeTextureID);
    TextureInfo& waterTextureInfo2 = waterWavesMesh.material.textures[waterIndex2];

    // Create copy mesh
    // Uses same geometry
    Mesh<Material> fullscreenQuad("shaders/copyVertex.glsl", "shaders/copyFragment.glsl");
    //fullscreenQuad.material.setTexture("u_tex", heightMap.textureID, heightMap.activeTextureID);
    fullscreenQuad.material.setTexture("u_tex", waterMap1.textureID, waterMap1.activeTextureID);

    // Set terrain height map
    terrain.material.setTexture("u_heightmap", heightMap.textureID, heightMap.activeTextureID);
    int terrainWaterIndex =
      terrain.material.setTexture("u_watermap", waterMap1.textureID, waterMap1.activeTextureID);
    TextureInfo& terrainWaterTextureInfo = terrain.material.textures[terrainWaterIndex];

    // Misc loop vars
    UniformArgs uniformArgs;
    uniformArgs.startTime = emscripten_get_now();
    uniformArgs.lastTime = uniformArgs.startTime;
    int currentWaterRT = 0;
    
    // Set clear color
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    bool first = true;

    // Setup UI
    
    //char buf[100];
    /*
    float f;
    ImGui::Text("Hello, world %d", 123);
    if (ImGui::Button("Save"))
    {
        // do stuff
    }
    //ImGui::InputText("string", buf, IM_ARRAYSIZE(buf));
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    */
		int tickIndex = 0;
		float ticks[60];

    loop = [&]
    {
        /*
        emscripten_webgl_make_context_current(sdlContext);
        main_loop2();
        emscripten_webgl_make_context_current(context);
        */

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                g_done = true;
						}
        }
				ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(g_window);
        ImGui::NewFrame();

        uniformArgs.now = emscripten_get_now();
        uniformArgs.tick = (uniformArgs.now - uniformArgs.lastTime) / 1000.0;
        uniformArgs.lastTime = uniformArgs.now;
        uniformArgs.elapsed = (uniformArgs.now - uniformArgs.startTime) / 1000.0;
        uniformArgs.camera.update(uniformArgs.tick);
	
				ticks[tickIndex] = uniformArgs.tick;
				tickIndex = (tickIndex + 1) % 60;

        // Enable depth test
        glEnable(GL_DEPTH_TEST);
        // Accept fragment if it closer to the camera than the former one
        glDepthFunc(GL_LESS);
        // Disable blend, ImgUI enables it
        glDisable(GL_BLEND);

        // Height map
        if (first) {
          uniformArgs.width = heightMap.imgWidth;
          uniformArgs.height = heightMap.imgHeight;
          heightMap.activate();
          glViewport(0, 0, uniformArgs.width, uniformArgs.height);
          glClear(GL_COLOR_BUFFER_BIT);
          heightMapMesh.draw(uniformArgs);
          heightMap.deactivate();
        }

        // Water
        int iterCount = 2; // + 2 * int(1.0 * uniformArgs.tick * 60.0);
        uniformArgs.tick /= (float) iterCount;
        //cerr << "iterCount: " << iterCount << endl;
        for (int axisIndex = 0; axisIndex < iterCount; axisIndex++) {
          currentWaterRT = (currentWaterRT + 1) % 2;
          RenderTarget& waterMapSource = currentWaterRT == 0 ? waterMap1 : waterMap2;
          RenderTarget& waterMapTarget = currentWaterRT == 0 ? waterMap2 : waterMap1;
          uniformArgs.axis =
            axisIndex % 2 == 0 ?
            glm::vec2(1.0 / static_cast<double>(waterMapSource.imgWidth), 0.0) :
            glm::vec2(0.0, 1.0 / static_cast<double>(waterMapSource.imgHeight));
          waterTextureInfo.id = waterMapSource.textureID;
          waterTextureInfo.activeID = waterMapSource.activeTextureID;
          uniformArgs.width = waterMapSource.imgWidth;
          uniformArgs.height = waterMapSource.imgHeight;
          waterMapTarget.activate();
          glViewport(0, 0, uniformArgs.width, uniformArgs.height);
          glClear(GL_COLOR_BUFFER_BIT);
          waterMapMesh.draw(uniformArgs);
          waterMapTarget.deactivate();
        }
        uniformArgs.tick *= (float) iterCount;

        // Water waves
        for (int axisIndex = 0; axisIndex < 1; axisIndex++) {
          currentWaterRT = (currentWaterRT + 1) % 2;
          RenderTarget& waterMapSource = currentWaterRT == 0 ? waterMap1 : waterMap2;
          RenderTarget& waterMapTarget = currentWaterRT == 0 ? waterMap2 : waterMap1;
          uniformArgs.axis =
            axisIndex % 2 == 0 ?
            glm::vec2(1.0 / static_cast<double>(waterMapSource.imgWidth), 0.0) :
            glm::vec2(0.0, 1.0 / static_cast<double>(waterMapSource.imgHeight));
          waterTextureInfo2.id = waterMapSource.textureID;
          waterTextureInfo2.activeID = waterMapSource.activeTextureID;
          uniformArgs.width = waterMapSource.imgWidth;
          uniformArgs.height = waterMapSource.imgHeight;
          waterMapTarget.activate();
          glViewport(0, 0, uniformArgs.width, uniformArgs.height);
          glClear(GL_COLOR_BUFFER_BIT);
          waterWavesMesh.draw(uniformArgs);
          waterMapTarget.deactivate();

          // Set terrain water map
          terrainWaterTextureInfo.id = waterMapTarget.textureID;
          terrainWaterTextureInfo.activeID = waterMapTarget.activeTextureID;
        }


        // Render terrain
        uniformArgs.width = state.width;
        uniformArgs.height = state.height;
        glViewport(0, 0, uniformArgs.width, uniformArgs.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        terrain.draw(uniformArgs);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_ZERO, GL_ZERO);
        skybox.draw(uniformArgs);
        //sphere.draw(uniformArgs);
        //fullscreenQuad.draw(uniformArgs);
        
        // 1. Show a simple window
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
        /*
        {
            static float f = 0.0f;
            ImGui::Text("Hello, world!");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&g_clear_color);
            if (ImGui::Button("Test Window")) g_show_test_window ^= 1;
            if (ImGui::Button("Another Window")) g_show_another_window ^= 1;
        }
        */

        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (true || g_show_another_window)
        {
          const int w = 350;
          const int h = state.height - 10;
          ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiSetCond_FirstUseEver);
          ImGui::SetNextWindowPos(ImVec2(state.width - w - 5, 5), ImGuiSetCond_FirstUseEver);
          ImGui::Begin("Configuration", &g_show_another_window);
          ImGui::Text("https://github.com/emnh/tda-wasm");
          ImGui::Text("Right click + WASD to move camera");
          ImGui::Text("Avg %.3f ms/frame (%.1f FPS)",
						1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
					ImGui::PlotLines("Frame Times", ticks, IM_ARRAYSIZE(ticks));
          if (ImGui::Button("Side View")) {
            state.pitch = -23.3653;
            state.yaw = -53.9898;
            uniformArgs.camera.cameraPos = glm::vec3(-21.5f, 22.8f,  27.5f);
					};
          if (ImGui::Button("Top View")) {
						state.pitch = -89.0;
            state.yaw = 0.0;
            uniformArgs.camera.cameraPos = glm::vec3(0.0, 60.0, 0.0);
					};
          ImGui::Text("Uniforms");
					ImGui::SliderFloat("Sun", &terrain.material.sunIntensity, 0.01f, 10.0f);
					ImGui::SliderInt("Light Count", &terrain.material.numLights, 0, 100);
					ImGui::SliderFloat("Lights", &terrain.material.lightsIntensity, 0.0f, 20.0f);
					ImGui::SliderFloat("Light Radius", &terrain.material.lightRadius, 0.0f, 30.0f);
					ImGui::SliderFloat("Beam Light", &terrain.material.beamIntensity, 0.01f, 2.0f);
					ImGui::SliderFloat("Map Height", &terrain.material.heightMultiplier, 0.01f, 100.0f);
					ImGui::SliderFloat("Centre Wave 1", &waterMapMesh.material.centralTurbulence1, 0.0f, 5.0f);
					ImGui::SliderFloat("Centre Wave 2", &waterMapMesh.material.centralTurbulence2, 0.0f, 5.0f);
					ImGui::SliderFloat("Rain", &waterMapMesh.material.rain, 0.0f, 100.0f);
					ImGui::SliderFloat("Evaporation", &waterMapMesh.material.evaporation, 0.0f, 20.0f);
					ImGui::SliderFloat("Wave Decay", &waterMapMesh.material.waveDecay, 0.0f, 20.0f);
					ImGui::SliderFloat("Viscosity", &waterMapMesh.material.viscosity, 0.0f, 1.0f);
					ImGui::SliderFloat("Water Transfer", &waterMapMesh.material.waterTransfer, 0.0f, 10.0f);
          ImGui::Combo("Refract Method",
	        	&terrain.material.refractMethod, "First\0Second\0Third\0Fourth\0Average\0");
					ImGui::SliderFloat("Water Normal", &terrain.material.waterNormalFactor, 0.0f, 1.0f);
					ImGui::SliderFloat("Fresnel", &terrain.material.fresnel, 0.0f, 1.0f);
					ImGui::SliderFloat("Occlusion", &terrain.material.occlusion, 0.0f, 5.0f);
          ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (false && g_show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow();
        }

        // Rendering
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        //glClearColor(g_clear_color.x, g_clear_color.y, g_clear_color.z, g_clear_color.w);
        //glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);
        ImGui::Render();
        //SDL_GL_MakeCurrent(g_window, glcontext);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(g_window);
        first = false;
    };

    emscripten_set_main_loop(main_loop, 0, true);

    // Cleanup
	  ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return 0;
}
