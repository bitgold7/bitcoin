#!/usr/bin/env python3
"""Functional tests for Bulletproof RPC calls."""

import json
import unittest
import http.client
from base64 import b64encode

RPC_USER = "user"
RPC_PASSWORD = "pass"
RPC_PORT = 8332

class BulletproofRPCTest(unittest.TestCase):
    def rpc_call(self, method, params=None, path="/"):
        if params is None:
            params = []
        headers = {
            "Authorization": "Basic " + b64encode(f"{RPC_USER}:{RPC_PASSWORD}".encode()).decode(),
            "Content-Type": "application/json",
        }
        payload = json.dumps({"jsonrpc": "1.0", "id": "test", "method": method, "params": params})
        conn = http.client.HTTPConnection("127.0.0.1", RPC_PORT)
        try:
            conn.request("POST", path, payload, headers)
            response = conn.getresponse()
            return json.loads(response.read())
        finally:
            conn.close()

    def setUp(self):
        try:
            self.rpc_call("getblockcount")
        except Exception as e:
            self.skipTest(f"RPC server not available: {e}")

    def test_create_and_verify(self):
        created = self.rpc_call("createrawbulletprooftransaction", [[], {"data": "00"}])
        proof = created.get("result", {}).get("proof")
        self.assertIsInstance(proof, str)
        verified = self.rpc_call("verifybulletproof", [proof])
        self.assertTrue(verified.get("result"))

    def test_invalid_proof(self):
        # A random string should not verify as a valid Bulletproof.
        invalid = "00"
        res = self.rpc_call("verifybulletproof", [invalid])
        self.assertFalse(res.get("result"))

    def test_activation_and_wallet(self):
        created = self.rpc_call("createrawbulletprooftransaction", [[], {"data": "00"}])
        tx_hex = created.get("result", {}).get("hex")
        if tx_hex is None:
            self.skipTest("Bulletproof RPC not available")

        # Mempool acceptance will fail if Bulletproofs are not yet active.
        r = self.rpc_call("testmempoolaccept", [[tx_hex]])
        result = r.get("result", [{}])[0]
        if result.get("allowed"):
            self.assertTrue(result.get("allowed"))
        else:
            self.assertEqual(result.get("reject-reason"), "bad-txns-bulletproof-premature")

        # Wallet RPCs should still function when Bulletproof support is built in.
        self.rpc_call("createwallet", ["bpwallet"], path="/")
        addr = self.rpc_call("getnewaddress", path="/wallet/bpwallet").get("result")
        self.assertIsInstance(addr, str)

if __name__ == '__main__':
    unittest.main()
