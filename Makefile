
all: dist/index.js dist/index.html dist/ui.js

clean:
	rm dist/index.js dist/index.wasm dist/index.html

dist/index.html: src/index.html
	cp src/index.html dist/index.html

dist/ui.js: src/ui.js
	cp src/ui.js dist/ui.js

dist/index.js: cpp/main.cpp shaders/* images/*
	emcc -Icpp/glm cpp/main.cpp \
		-std=c++17 -s WASM=1 -s USE_WEBGL2=1 --preload-file images/ --preload-file shaders/ \
		--use-preload-plugins \
		-o dist/index.js
