# SSE-Implementations

Implementations of the following searchable symmetric encryption (SSE) schemes:

- `PiBas` ([Cash et al. 2014](https://eprint.iacr.org/2014/853.pdf))
- `Log-SRC` ([Demertzis et al. 2016](https://idemertzis.com/Papers/sigmod16.pdf))
- `Log-SRC-i` ([Demertzis et al. 2016](https://idemertzis.com/Papers/sigmod16.pdf))

# Requirements

- C++20 is required, which requires at least G++ version 8. To check the currently installed G++ version, use `g++ --version` at the command line.
- The OpenSSL library needed to compile the program can be installed with `sudo apt install libssl-dev` (if using `apt`).

# Compiling

```
cd src/
g++ main.cpp pi_bas.cpp log_src.cpp log_srci.cpp util/*.cpp -lcrypto -std=c++20
```

Using GCC instead of G++ to compile will probably produce a wall of errors (different linking mechanics?).
