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

const int maxTexturesPerMaterial = 10;
const int maxUniformsPerMaterial = 20;

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
  // 10.0
  glm::vec3 cameraPos   = glm::vec3(0.0f, 30.0f,  0.0f);
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
  int width;
  int height;
  double startTime;
  double lastTime;
  double now;
  double tick;
  double elapsed;
  Camera camera;
  glm::vec3 light = glm::vec3(0.0, -1000.0, 0.0);
  glm::vec2 axis;
};

class TextureInfo {
public:
  GLuint id;
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
  GLint u_resolution;
  GLint u_light;
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
    glUniform2f(u_resolution, uniformArgs.width, uniformArgs.height);
    glUniform3fv(u_light, 1, glm::value_ptr(uniformArgs.light));
    for (int i = 0; i < uniformCount; i++) {
      uniforms[i].updater(uniforms[i].index, uniformArgs);
    }
    for (int i = 0; i < textureCount; i++) {
      glUniform1i(textures[i].u_texture, textures[i].activeID);
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
    u_resolution = glGetUniformLocation(shaderProgram, "u_resolution");
    u_light = glGetUniformLocation(shaderProgram, "u_light");
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
  virtual void initRest() {
    addUniform("u_axis", [](GLint index, UniformArgs& uniformArgs) {
        glUniform2fv(index, 1, glm::value_ptr(uniformArgs.axis));
    });
  }
};

class TerrainMaterial : public Material {
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

  Mesh(const char* vertexSourceFile, const char* fragmentSourceFile, Material& material) {
    geometry = loadGeometry();
    
    // Create material
    material.vertexSourceFile = vertexSourceFile;
    material.fragmentSourceFile = fragmentSourceFile;
    material.init();
    this->material = material;

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

    // Create terrain
    EM_ASM({
          const THREE = window.UI.THREE;
          var sz = 100.0;
          window.UI.geometry = new THREE.PlaneBufferGeometry(sz, sz, 256, 256);
        });
    TerrainMaterial terrainMaterial;
    Mesh terrain("shaders/vertex.glsl", "shaders/fragment.glsl", terrainMaterial);

    // Create sphere
    EM_ASM({
          const THREE = window.UI.THREE;
          window.UI.geometry = new THREE.SphereBufferGeometry(2, 32, 20);
        });
    Material material;
    Mesh sphere("shaders/defaultVertex.glsl", "shaders/defaultFragment.glsl", material);

    // Create skybox
    EM_ASM({
          const THREE = window.UI.THREE;
          var sz = 100000.0;
          window.UI.geometry = new THREE.BoxBufferGeometry(sz, sz, sz, 1, 1, 1);
        });
    CubeMaterial cubeMaterial;
    Mesh skybox("shaders/skyVertex.glsl", "shaders/skyFragment.glsl", cubeMaterial);
    
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
    Material heightMapMaterial;
    Mesh heightMapMesh("shaders/heightmapVertex.glsl", "shaders/heightmapFragment.glsl", heightMapMaterial);
    
    // Create water map render targets
    RenderTarget waterMap1(sz, sz);
    waterMap1.init();
    RenderTarget waterMap2(sz, sz);
    waterMap2.init();

    // Create water map mesh
    // Uses same geometry
    WaterMaterial waterMapMaterial;
    Mesh waterMapMesh("shaders/watermapVertex.glsl", "shaders/watermapFragment.glsl", waterMapMaterial);
    waterMapMesh.material.setTexture("u_heightmap", heightMap.textureID, heightMap.activeTextureID);
    int waterIndex =
      waterMapMesh.material.setTexture("u_watermap", waterMap2.textureID, waterMap2.activeTextureID);
    TextureInfo& waterTextureInfo = waterMapMesh.material.textures[waterIndex];

    // Create copy mesh
    // Uses same geometry
    Material copyMaterial;
    Mesh fullscreenQuad("shaders/copyVertex.glsl", "shaders/copyFragment.glsl", copyMaterial);
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
    
    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);
    // Set clear color
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    bool first = true;

    loop = [&]
    {
        uniformArgs.now = emscripten_get_now();
        uniformArgs.tick = (uniformArgs.now - uniformArgs.lastTime) / 1000.0;
        uniformArgs.lastTime = uniformArgs.now;
        uniformArgs.elapsed = (uniformArgs.now - uniformArgs.startTime) / 1000.0;
        uniformArgs.camera.update(uniformArgs.tick);

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
        for (int axisIndex = 0; axisIndex < 2; axisIndex++) {
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
          terrainWaterTextureInfo.id = waterMapTarget.textureID;
          terrainWaterTextureInfo.activeID = waterMapTarget.activeTextureID;
        }

        // Render
        uniformArgs.width = state.width;
        uniformArgs.height = state.height;
        glViewport(0, 0, uniformArgs.width, uniformArgs.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        terrain.draw(uniformArgs);
        //sphere.draw(uniformArgs);
        skybox.draw(uniformArgs);
        //fullscreenQuad.draw(uniformArgs);
        
        first = false;
    };

    emscripten_set_main_loop(main_loop, 0, true);

    return EXIT_SUCCESS;
}
