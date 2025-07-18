# SSE Implementations

Implementations of the following [searchable symmetric encryption](https://en.wikipedia.org/wiki/Searchable_symmetric_encryption) (SSE) schemes:
- PiBas ([Cash et al. NDSS'14](https://eprint.iacr.org/2014/853.pdf)) (but specifically the result-hiding variant used in [Demertzis et al. NDSS'20](https://www.ndss-symposium.org/wp-content/uploads/2020/02/24423-paper.pdf) (in figure 12), similar to PiBasRo)
- Logarithmic-SRC ([Demertzis et al. SIGMOD'16](https://idemertzis.com/Papers/sigmod16.pdf))
- Logarithmic-SRC-i ([Demertzis et al. SIGMOD'16](https://idemertzis.com/Papers/sigmod16.pdf))
- Logarithmic-SRC-i\* ([Demertzis et al. 2018](https://dl.acm.org/doi/pdf/10.1145/3167971))
- SDa ([Demertzis et al. NDSS'20](https://www.ndss-symposium.org/wp-content/uploads/2020/02/24423-paper.pdf))

Since many of these can be instantiated with various underlying schemes, the following instantiations are possible:
- PiBas
- Logarithmic-SRC[PiBas]
- Logarithmic-SRC-i[PiBas]
- Logarithmic-SRC-i\*[PiBas]
- SDa[PiBas]
- SDa[Logarithmic-SRC[PiBas]]
- SDa[Logarithmic-SRC-i[PiBas]]
- SDa[Logarithmic-SRC-i\*[PiBas]]

See [src/main.cpp](src/main.cpp) for usage examples.

# Requirements

- CMake
- [Conan 2](https://docs.conan.io/2/installation.html)
- A C++ compiler that supports C++20 (ideally g++ version 10 or above; install with `apt install g++-10` if using `apt`)
- Only tested on Linux systems. To run on Windows, don't.
  
  (ok wsl exists)

# Running

1. Generate two Conan profiles for debug and release respectively:
    ```
    conan profile detect --name=sse_implementations_debug
    conan profile detect --name=sse_implementations_release
    ```
2. Edit both Conan profiles (by default at `~/.conan2/profiles/`):
    - Set `build_type=Debug` for the debugging profile and `build_type=Release` for the release profile.
    - Make sure `compiler.cppstd=20` is set (`gnu20` is fine too if using `compiler=gnu`).
    - If your "default" compiler (e.g. `/usr/bin/g++`) is not the correct version and something like `g++-10` was separately installed (e.g. to `/usr/bin/g++-10`), add the following to the bottom of both profiles to specify the compiler executable:
        ```
        [conf]
        tools.build:compiler_executables={"cpp": "<path to compiler executable>"}
        ```
3. In the base directory of this project/repo, run
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
- While each database tuple possess a range of keywords instead of just one for sake of generality (for range scheme underlying indexes), they must still only have a singular keyword in the input database, meaning the start and end of each keyword range must be the same.
- Keyword search is supported (i.e. one document can have multiple keywords), but only for non-range-query schemes (as otherwise range queries are not well-defined). To insert such documents into the dataset, put in one document per keyword all with the same id. Attempting to do this for the range query schemes Logarithmic-SRC and especially Logarithmic-SRC-i may result in undefined and generally confusing behavior; only insert one document per id for those.
- Currently, [src/main.cpp](src/main.cpp) implements four experiments:
    - A debugging experiment that prints out the results for each scheme and acts as a basic test case/sanity check.
    - Experiment 1, which times range queries of varying sizes on a fixed-size db.
    - Experiment 2, which times a fixed range query on dbs of varying sizes.
    - Experiment 3, which demonstrates the advantage of Logarithmic-SRC-i over Logarithmic-SRC when a lot of false positives are generated.
