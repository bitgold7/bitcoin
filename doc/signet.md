# Signet

This project ships with support for the public signet testing network. Signet
requires blocks to include a signature that satisfies a fixed challenge.

## Challenge

```
512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae
```

## Useful endpoints

* Faucet: <https://signetfaucet.com/>
* Block explorer: <https://mempool.space/signet>

To join signet with a node built from this repository, start `bitgoldd` with the
`-signet` option. The fixed challenge above is compiled in by default.

