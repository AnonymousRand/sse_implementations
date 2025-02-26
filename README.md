# SSE Implementations

Implementations of the following searchable symmetric encryption (SSE) schemes:

- PiBas ([Cash et al. 2014](https://eprint.iacr.org/2014/853.pdf))
- Logarithmic-SRC ([Demertzis et al. 2016](https://idemertzis.com/Papers/sigmod16.pdf))
- Logarithmic-SRC-i ([Demertzis et al. 2016](https://idemertzis.com/Papers/sigmod16.pdf))
- SDa ([Demertzis et al. 2020](https://www.ndss-symposium.org/wp-content/uploads/2020/02/24423-paper.pdf))

# Requirements

- C++20 is required, which requires at least G++ version 8. To check the currently installed G++ version, use `g++ --version` at the command line.
    - To update G++ with `apt`, try something like `apt install g++-10`, and then use `g++-10` instead of `g++` on the command line during compilation.
- The OpenSSL library needed to compile the program can be installed with `apt install libssl-dev` (if using `apt`).

# Compiling

```
cd src/
g++ main.cpp pi_bas.cpp sda.cpp log_src.cpp log_srci.cpp util/*.cpp -lcrypto -std=c++20
./a.out
```

Using GCC instead of G++ to compile will probably produce a wall of errors (different linking mechanics?).

# Notes

- Keyword search is supported (i.e. one document can have multiple keywords). To insert such documents into the dataset, put in one tuple per keyword, all with the same id.
- Ids and keywords must be integers. In addition, ids must be positive and consecutive. Otherwise, bad things may happen.
