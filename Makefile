
all: dist/index.js dist/index.html

clean:
	rm dist/index.js dist/index.wasm dist/index.html

dist/index.html: src/index.html
	cp src/index.html dist/index.html

#dist/ui.js: src/ui.js
#	cp src/ui.js dist/ui.js

dist/index.js: cpp/main.cpp shaders/* images/*
	emcc -Icpp/glm -Icpp/imgui cpp/imgui/imgui.cpp cpp/main.cpp \
		-std=c++17 -s WASM=1 -s USE_WEBGL2=1 -s ALLOW_MEMORY_GROWTH=1 \
		-s TOTAL_MEMORY=128MB \
		--preload-file images/ --preload-file shaders/ \
		--use-preload-plugins --no-heap-copy \
		-o dist/index.js
