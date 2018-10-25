#include <functional>
#include <vector>
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <streambuf>

#include <emscripten.h>
#include <emscripten/html5.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES3/gl3.h>

using namespace std;

// an example of something we will control from the javascript side
bool background_is_black = true;

// the function called by the javascript code
extern "C" void EMSCRIPTEN_KEEPALIVE toggle_background_color() { background_is_black = !background_is_black; }

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

int main()
{
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

    // Create a Vertex Buffer Object and copy the vertex data to it
    GLuint vbo;
    glGenBuffers(1, &vbo);

    const GLfloat x = 0.5f;
    const GLfloat y = 0.5f;
    GLfloat vertices[] = {
      0.0f, y, 0.0f, -y, -x, -y,
      -x, y, -x, -y, 0.0f, y
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
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

    // Specify the layout of the vertex data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

    double startTime = emscripten_get_now();

    loop = [&]
    {
        // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        double elapsed = (emscripten_get_now() - startTime) / 1000.0;

        glUniform1f(u_time, elapsed);

        if( background_is_black ) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        } else {
            glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    };

    emscripten_set_main_loop(main_loop, 0, true);

    return EXIT_SUCCESS;
}
