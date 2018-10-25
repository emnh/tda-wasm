
all: dist/index.js dist/index.html

clean:
	rm dist/index.js dist/index.wasm dist/index.html

dist/index.html: src/index.html
	cp src/index.html dist/index.html

dist/index.js: cpp/main.cpp
	emcc cpp/main.cpp -std=c++17 -s WASM=1 -s USE_WEBGL2=1 -o dist/index.js
