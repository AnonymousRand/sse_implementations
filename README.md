# SSE Implementations

Implementations of the following [searchable symmetric encryption](https://en.wikipedia.org/wiki/Searchable_symmetric_encryption) (SSE) schemes:

- PiBas ([Cash et al. NDSS'14](https://eprint.iacr.org/2014/853.pdf))
- Logarithmic-SRC ([Demertzis et al. SIGMOD'16](https://idemertzis.com/Papers/sigmod16.pdf))
- Logarithmic-SRC-i ([Demertzis et al. SIGMOD'16](https://idemertzis.com/Papers/sigmod16.pdf))
- Logarithmic-SRC-i\* ([Demertzis et al. 2018](https://dl.acm.org/doi/pdf/10.1145/3167971))
- SDa ([Demertzis et al. NDSS'20](https://www.ndss-symposium.org/wp-content/uploads/2020/02/24423-paper.pdf))

# Requirements

- CMake
- [Conan 2](https://docs.conan.io/2/installation.html)
- A C++ compiler that supports C++20 (ideally g++ version 10 or above; install with `apt install g++-10` if using `apt`)
- Only tested on Linux systems

# Running

1. Install CMake and Conan 2.
2. Generate two Conan profiles for debug and release respectively:
    ```
    conan profile detect --name=sse_implementations_debug
    conan profile detect --name=sse_implementations_release
    ```
3. Edit both Conan profiles (by default at `~/.conan2/profiles/`) to ensure that C++20 is used:
    - Make sure `compiler.cppstd=20` is set (`gnu20` is fine too if using `compiler=gnu`).
    - If the "default" compiler (e.g. `/usr/bin/g++`) is not the correct version and something like `g++-10` was separately installed (e.g. to `/usr/bin/g++-10`), add the following to the bottom of both profiles to specify the compiler executable:
        ```
        [conf]
        tools.build:compiler_executables={"cpp": "<path to compiler executable>"}
        ```
4. In the base directory of this project/repo, run
    ```
    make
    ```
    and then run either the non-compiler-optimized (debug) version with
    ```
    ./build-debug/main
    ```
    or the compiler-optimized (release) version with
    ```
    ./build-release/main
    ```

# Notes

- Ids and keywords must be nonnegative integral values. Otherwise, Bad Things may happen.
- Keyword search is supported (i.e. one document can have multiple keywords), but only for non-range-query schemes (as otherwise range queries are not well-defined). To insert such documents into the dataset, put in one document per keyword all with the same id. Attempting to do this for the range query schemes Logarithmic-SRC and especially Logarithmic-SRC-i may result in undefined and generally confusing behavior; only insert one document per id for those.
