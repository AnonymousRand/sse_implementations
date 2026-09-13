debug_output = build_debug/
release_output = build_release/

.PHONY: all clean debug release

all: debug release

clean:
	rm -rf build_*

debug:
	conan install . --output-folder=$(debug_output) --build=missing --profile:build=sse_implementations_debug --profile:host=sse_implementations_debug
	# note: i think `-DCMAKE_BUILD_TYPE` is required for `NDEBUG` flag to be set when using Makefile
	cmake -S . -B $(debug_output) -DCMAKE_TOOLCHAIN_FILE="$(debug_output)/conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Debug
	# whereas `--config Debug` sets it for multi-configuration generators like IDEs
	cmake --build $(debug_output) --config Debug

release:
	conan install . --output-folder=$(release_output) --build=missing --profile:build=sse_implementations_release --profile:host=sse_implementations_release
	cmake -S . -B $(release_output) -DCMAKE_TOOLCHAIN_FILE="$(release_output)/conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
	cmake --build $(release_output) --config Release
