# SSE Implementations

Implementations of the following [searchable symmetric encryption](https://en.wikipedia.org/wiki/Searchable_symmetric_encryption) (SSE) schemes:
- PiBas ([Cash et al., NDSS'14](https://eprint.iacr.org/2014/853.pdf)) (specifically the result-hiding variant used in [Demertzis et al., NDSS'20](https://www.ndss-symposium.org/wp-content/uploads/2020/02/24423-paper.pdf) figure 12, similar to PiBasRo)
- NLogN ([Asharov et al., STOC'16](https://eprint.iacr.org/2016/251.pdf), approach #3 "Improving the Cash–Tessaro Scheme")
- Logarithmic-SRC ([Demertzis et al., SIGMOD'16](https://idemertzis.com/Papers/sigmod16.pdf))
- Logarithmic-SRC-i ([Demertzis et al., SIGMOD'16](https://idemertzis.com/Papers/sigmod16.pdf))
- Logarithmic-SRC-i\* ([Demertzis et al., TODS'18](https://dl.acm.org/doi/pdf/10.1145/3167971))
- SDa ([Demertzis et al., NDSS'20](https://www.ndss-symposium.org/wp-content/uploads/2020/02/24423-paper.pdf))

Since many of these can be instantiated with various underlying schemes, the following instantiations are possible (not all of these are secure, though!):
- PiBas
- NLogN
- Logarithmic-SRC[PiBas]
- Logarithmic-SRC[NLogN]
- Logarithmic-SRC-i[PiBas]
- Logarithmic-SRC-i[NLogN]
- Logarithmic-SRC-i\*
- SDa[PiBas]
- SDa[NLogN]
- SDa[Logarithmic-SRC[PiBas]]
- SDa[Logarithmic-SRC[NLogN]]
- SDa[Logarithmic-SRC-i[PiBas]]
- SDa[Logarithmic-SRC-i[NLogN]]
- SDa[Logarithmic-SRC-i\*]

See [src/main.cpp](src/main.cpp), [src/app/sse_factory.cpp](src/app/sse_factory.cpp), and [src/app/experiments/](src/app/experiments/) for usage examples :3

# Requirements

- CMake (tested with CMake 4.3.4, 3.22.1)
- Conan 2 (tested with Conan 2.28.1, 2.18.1)
- A C++ compiler that supports C++20 and `std::format` (tested with GCC 15.3.0, 15.2.0; theortically should work with GCC/g++ >=13 but doesn't seem to compile on GCC/g++ 13; not tested with Clang)

Only tested on Linux (NixOS, Ubuntu). To run on Windows, don't. (ok, fine, WSL works :p)

# Running

1. Generate two Conan profiles for debug and release respectively (names must match those in the [Makefile](Makefile)!):
    ```
    conan profile detect --name=sse_implementations_debug
    conan profile detect --name=sse_implementations_release
    ```
2. Edit both Conan profiles (by default at `./conan2/profiles`, or `~/.conan2/profiles/` if the `.conanrc` didn't work):
    - Set `build_type=Debug` for the debugging profile and `build_type=Release` for the release profile!!
    - Make sure `compiler.cppstd=20` is set (`gnu20` is fine too if using GCC or Clang).
    - If your "default" compiler (usually `/usr/bin/c++`, which is usually symlinked to `/usr/bin/g++`) is not the correct version and something like `g++-15` was separately installed (e.g. to `/usr/bin/g++-15`), add the following to the bottom of both profiles to specify the compiler executable:
        ```
        [conf]
        tools.build:compiler_executables={"cpp": "<path to compiler executable>"}
        ```

        Make sure to use the C++, not C compiler! (E.g. `g++` instead of `gcc` or `clang++` instead of `clang`.)
3. Verify that the compiler executable set for `CMAKE_CXX_COMPILER` in [CMakeLists.txt](CMakeLists.txt) (as described above) matches the one set in the Conan profiles. If using default, you can comment this line out in CMakeLists.txt (in which case CMake will use the default `/usr/bin/c++`).
4. In the base directory of this project/repo, run
    ```
    make
    ```
    and then run either the debug (non-compiler-optimized) version with
    ```
    build_debug/main
    ```
    or the release (compiler-optimized) version with
    ```
    build_release/main
    ```

Adjust the basic configuration options in [src/config.h](src/config.h) (e.g. whether to enable benchmarking), as well as in each experiment in [src/app/experiments/](src/app/experiments/) (when applicable), as desired. Note that currently these values are only applied at compile time, so you must recompile after making changes.

## NixOS

If you're using NixOS, there is a `flake.nix` provided that installs the packages listed in the "Requirements" section above. Run `nix develop` in this project's base directory to install them and then proceed with the steps above. (Note that the flake only installs the build tools; Conan is still responsible for managing the project's dependencies.)

# Notes

- This is NOT intended for actual, real-world use! It's more a proof of concept or a simulation for doing experimental evaluation.
- The client-server distinction is fairly minimal and is only meant for benchmarking things like network communication. This implementation does not actually run across two separate hosts or have a well-defined client/server program.
    - The "client" classes for each scheme also function as the "controller", exposing the SSE API and implementing the client-side logic. These classes in turn may own "server" classes, which mostly serve to just store and perform basic retrieval operations on encrypted indexes.
    - At the moment, only the "most underlying" schemes like static point SSEs—PiBas and NLogN—have a server class; other schemes just keep one or more instances of these underlying schemes (specifically, of the underlying schemes' client classes, which hence also includes the servers).
- Ids and keywords MUST be nonnegative integral values. Otherwise, Bad Things may happen.
- While database tuples each possess a range of keywords instead of just one for sake of generality (for range scheme underlying indexes), they must still only have a singular keyword in the input database, meaning the start and end of each keyword range must be the same.
- Keyword search is supported (i.e. one document can have multiple keywords), but only for non-range schemes (as range queries for documents with multiple "keywords" or attribute values are not well-defined). To insert such documents into the dataset, put in one document per keyword all with the same id. Attempting to do this for the range schemes may result in undefined behavior; only insert one document per id for those.
- Profile a section of code inside an SSE scheme's class or encrypted indexes using `this->benchmarks->startProfile(<profile display name>)` and `this->benchmarks->endProfile(<profile display name>)`; then the total time stored in the `<profile display name>` profile will be printed out in the benchmarking info.
- i have pain

# dev notes/conventions

- uh mostly just keep the existing conventions ig
- generally, each class should define constructors/destructors/init methods/clear methods etc. that are responsible for the members defined by that class. children inheriting from these classes should call each parent's version of these methods in their own implementation.
- see [src/utils/types/MOVE_SEMANTICS.md](src/utils/types/MOVE_SEMANTICS.md) for notes about move semantics and the big five.
- currently, one-line getters/setters/very simple methods like `bool IDb::empty()` are implemented inside the class declaration in the header. very broad interfaces like in [src/schemes/interfaces/](src/schemes/interfaces/) have all implementations of methods in the header and have no .cpp file, which avoids massive explicit template instantiation.
- public destructors in classes that are meant to be inherited from should almost always be `virtual`.
- call parent `clear()` methods at the end, in reverse order of `init()` or constructor (just like destructor/constructor). if no `init()` or constructor to reference, the unofficial convention i'm using is the reverse order of inheritance.
- currently, files are almost always include what you use, i.e. include everything that has a relevant symbol in the file. also, includes should almost always be relative to [./src/](./src/), and just keep the existing include ordering/formatting.
