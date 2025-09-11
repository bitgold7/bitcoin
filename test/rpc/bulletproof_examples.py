#!/usr/bin/env python3
"""Example RPC calls for Bulletproof functionality.

This script demonstrates how to call the Bulletproof RPC methods using
HTTP. A bitcoind instance with RPC enabled must be running locally.
If no node is running the script will exit gracefully.
"""

import json
import sys
import http.client
from base64 import b64encode

RPC_USER = "user"
RPC_PASSWORD = "pass"
RPC_PORT = 8332


def rpc_call(method, params=None):
    if params is None:
        params = []
    headers = {
        "Authorization": "Basic " + b64encode(f"{RPC_USER}:{RPC_PASSWORD}".encode()).decode(),
        "Content-Type": "application/json",
    }
    payload = json.dumps({"jsonrpc": "1.0", "id": "example", "method": method, "params": params})
    conn = http.client.HTTPConnection("127.0.0.1", RPC_PORT)
    try:
        conn.request("POST", "/", payload, headers)
        response = conn.getresponse()
        return json.loads(response.read())
    finally:
        conn.close()


def main():
    try:
        created = rpc_call("createrawbulletprooftransaction", [[], {}])
        proof = created.get("result", {}).get("hex")
        verified = rpc_call("verifybulletproof", [proof])
        print("create:", created)
        print("verify:", verified)
    except Exception as err:
        print("RPC example skipped:", err)


if __name__ == "__main__":
    main()
