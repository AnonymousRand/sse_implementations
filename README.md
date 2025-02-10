# SSE-Implementations

To install the OpenSSL libraries required to run this: `sudo apt install libssl-dev`

Thanks to <https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption#C.2B.2B_Programs> for a rough guide to encryption and decryption in OpenSSL, specifically with more C++-friendly syntax.

Thanks to <https://stackoverflow.com/a/34624592> for providing me a stupidly simple way to make said C++-friendly syntax work after hours of soul-searching and hair-pulling.

# Temp notes for writeup

- `buildTdag()` from leafs: with every iteration of the outer while loop, the list size shrinks by 1 as two nodes are combined into one. Thus, we have O(n) for n leaves, assuming few iterations of inner for loop (which seems to be true experimentally).
- Ultimately went for `std::list` instead of `std::vector` for TDAG stuff; `std::list` is significantly faster than `std::vector` with splicing/merging, but slower for iterating because of worse memory locality.
    - Lots of other misc optimization choices, like experimentally testing many different ways to do TDAG functions or buildIndex methods
- Refactored from sse->pibas->logsrc/logsrci inhertance chain to sse->rangesse->logsrc/logsrci with:
    - sse classes no longer holding data like key, db; only tdag and other internal things to make api more consistent
        - also allows functions to be defined more like original
    - rangesse has a sse instead of is an sse (ok it still is an sse, but we no longer use parent functions to call underlying sse)
        - much better template experience for logsrci annoying templates (since it uses two keys, two indexes): can instantiate logsrc and srci as template specializations of rangesse which performs no functions, instead of instantiating logsrc and srci as specializations of pibas which will then need to be templated, and then: how do we define template generic returns for buildIndex(), for example, in pi_bas.cpp?
        - composing also just cleaner and probably the better approach in the long run, inheritance messy and this situation isnt really the perfect one for inheritance
- GAH why does srci just have to mess up the otherwise consistent sse api? me when interactive
- Supports keyword search: `Id`s in database can repeat; i.e., a document can contain multiple `KwRange`s
