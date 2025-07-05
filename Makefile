.PHONY: all clean debug release

all: debug release

clean:
	rm -rf cmake-build*

cmake-build-debug:
	cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug

cmake-build-release:
	cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release

debug: | cmake-build-debug
	cmake --build cmake-build-debug --config Debug

release: | cmake-build-release
	cmake --build cmake-build-release --config Release
