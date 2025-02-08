# SSE-Implementations

To install the OpenSSL libraries required to run this: `sudo apt install libssl-dev`

Thanks to <https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption#C.2B.2B_Programs> for a rough guide to encryption and decryption in OpenSSL, specifically with more C++-friendly syntax.

Thanks to <https://stackoverflow.com/a/34624592> for providing me a stupidly simple way to make said C++-friendly syntax work after hours of soul-searching and hair-pulling.

# Temp notes for writeup

- Ultimately went for `std::list` instead of `std::vector` for TDAG stuff; `std::list` is significantly faster than `std::vector` with splicing/merging, but slower for iterating because of worse memory locality.
    - Lots of other misc optimization choices, like experimentally testing many different ways to do TDAG functions or buildIndex methods
