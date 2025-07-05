# SSE Implementations

Implementations of the following [searchable symmetric encryption](https://en.wikipedia.org/wiki/Searchable_symmetric_encryption) (SSE) schemes:

- PiBas ([Cash et al. NDSS'14](https://eprint.iacr.org/2014/853.pdf))
- Logarithmic-SRC ([Demertzis et al. SIGMOD'16](https://idemertzis.com/Papers/sigmod16.pdf))
- Logarithmic-SRC-i ([Demertzis et al. SIGMOD'16](https://idemertzis.com/Papers/sigmod16.pdf))
- Logarithmic-SRC-i\* ([Demertzis et al. 2018](https://dl.acm.org/doi/pdf/10.1145/3167971))
- SDa ([Demertzis et al. NDSS'20](https://www.ndss-symposium.org/wp-content/uploads/2020/02/24423-paper.pdf))

# Requirements

- C++20 and `concepts` are required, which seems to require at least G++ version 10. To check the currently installed G++ version, use `g++ --version` at the command line.
    - To update G++ with `apt`, try something like `apt install g++-10`, and then use `g++-10` instead of `g++` on the command line during compilation.
- The OpenSSL library needed to compile the program can be installed with `apt install libssl-dev` (if using `apt`).

# Compiling

```
cd src/
g++ *.cpp utils/*.cpp -lcrypto -o out/a.out -std=c++20
./out/a.out
```

Using GCC instead of G++ to compile will probably produce a wall of errors (different linking mechanics?).

~~too lazy to set up CMake or whatever right now~~

# Notes

- Ids and keywords must be nonnegative integral values. Otherwise, Bad Things may happen.
- Keyword search is supported (i.e. one document can have multiple keywords), but only for non-range-query schemes (as otherwise range queries are not well-defined). To insert such documents into the dataset, put in one document per keyword all with the same id. Attempting to do this for the range query schemes Logarithmic-SRC and especially Logarithmic-SRC-i may result in undefined and generally confusing behavior; only insert one document per id for those.
