gen:
	export CC=/usr/bin/clang; cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -S ./ -B ./build

build:
	cmake --build ./build
	chmod +x ./build/apps/aether-c

clean:
	rm -r ./build
