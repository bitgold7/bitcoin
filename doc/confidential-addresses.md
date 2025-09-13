# Confidential Addresses

Confidential addresses extend standard wallet addresses with additional
metadata describing a blinding key. This repository exposes two RPC
commands for working with these addresses.

## Generating an address

```bash
$ bitcoin-cli getnewconfidentialaddress
{
  "address": "baseaddress",
  "blinding_key": "0123456789abcdef",
  "confidential_address": "baseaddress:0123456789abcdef"
}
```

## Validating an address

```bash
$ bitcoin-cli validateconfidentialaddress "baseaddress:0123456789abcdef"
{
  "isvalid": true,
  "address": "baseaddress",
  "blinding_key": "0123456789abcdef"
}
```
