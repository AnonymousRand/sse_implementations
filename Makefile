debug_output = build-debug/
release_output = build-release/

.PHONY: all clean debug release

all: debug release

clean:
	rm -rf build-*

debug:
	conan install . --output-folder=$(debug_output) --build=missing --profile:build=sse_implementations_debug --profile:host=sse_implementations_debug
	cmake -S . -B $(debug_output) -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="$(debug_output)/conan_toolchain.cmake" --debug-output
	cmake --build $(debug_output) --config Debug

release:
	conan install . --output-folder=$(release_output) --build=missing --profile:build=sse_implementations_release --profile:host=sse_implementations_release
	cmake -S . -B $(release_output) -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$(release_output)/conan_toolchain.cmake"
	cmake --build $(release_output) --config Release
