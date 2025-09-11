See [doc/build-\*.md](/doc)

Optional Bulletproofs support depends on `libsecp256k1_zkp` version 0.6.1 or
newer built with:

```
./configure --enable-experimental --enable-module-bppp \
            --with-ecmult-window=15 --with-ecmult-gen-precision=4
```

Disable Bulletproofs with `--without-bulletproofs` or by passing
`-DENABLE_BULLETPROOFS=OFF` to CMake.
