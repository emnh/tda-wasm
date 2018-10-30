
all: dist/index.js dist/index.html

clean:
	rm dist/index.js dist/index.wasm dist/index.html

dist/index.html: src/index.html
	cp src/index.html dist/index.html

#dist/ui.js: src/ui.js
#	cp src/ui.js dist/ui.js

dist/index.js: cpp/main.cpp shaders/* images/* Makefile
	emcc -Icpp/glm \
		-ISDL2 \
		-Icpp/imgui \
		-Icpp/imgui/examples \
		-Icpp/imgui/examples/sdl_emscripten_example \
		cpp/imgui/imgui.cpp cpp/imgui/imgui_draw.cpp cpp/imgui/imgui_widgets.cpp \
		cpp/imgui/imgui_demo.cpp \
		cpp/imgui/examples/sdl_emscripten_example/imgui_impl_sdl.cpp \
		cpp/imgui/examples/imgui_impl_opengl3.cpp \
		cpp/main.cpp \
		-std=c++17 -s WASM=1 -s USE_SDL=2 -s USE_WEBGL2=1 -s ALLOW_MEMORY_GROWTH=1 \
		-s TOTAL_MEMORY=128MB \
		-Werror \
		--preload-file images/ --preload-file shaders/ \
		--use-preload-plugins --no-heap-copy \
		-o dist/index.js
		#cpp/imgui/imgui.cpp cpp/imgui/imgui_draw.cpp cpp/imgui/imgui_widgets.cpp \
		#-o test/index.html
		#cpp/imgui/examples/sdl_emscripten_example/main.cpp \
