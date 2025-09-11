See [doc/build-\*.md](/doc)

Bulletproofs support in BitGold depends on `libsecp256k1_zkp` version 0.6.1 or
newer built with:

```
./configure --enable-experimental --enable-module-bppp \
            --with-ecmult-window=15 --with-ecmult-gen-precision=4
```

