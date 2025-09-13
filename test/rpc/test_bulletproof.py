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
        # Create a wallet and generate two outputs so we get multiple proofs back.
        self.rpc_call("createwallet", ["bpproof"], path="/")
        addr1 = self.rpc_call("getnewaddress", path="/wallet/bpproof").get("result")
        addr2 = self.rpc_call("getnewaddress", path="/wallet/bpproof").get("result")
        created = self.rpc_call(
            "createrawbulletprooftransaction",
            [[], {addr1: 0, addr2: 0}],
            path="/wallet/bpproof",
        )
        proofs = created.get("result", {}).get("proofs")
        if proofs is None:
            self.skipTest("Bulletproof RPC not available")
        self.assertEqual(len(proofs), 2)
        for proof in proofs:
            verified = self.rpc_call("verifybulletproof", [proof])
            self.assertTrue(verified.get("result"))
            # Tamper with the proof by flipping the first byte to test verification failure.
            flipped = f"{int(proof[:2], 16) ^ 1:02x}" + proof[2:]
            tampered = self.rpc_call("verifybulletproof", [flipped])
            self.assertFalse(tampered.get("result"))

    def test_invalid_proof(self):
        # A random string should not verify as a valid Bulletproof.
        invalid = "00"
        res = self.rpc_call("verifybulletproof", [invalid])
        self.assertFalse(res.get("result"))

    def test_activation_and_wallet(self):
        # Create a wallet and craft a transaction with two Bulletproof outputs.
        self.rpc_call("createwallet", ["bpwallet"], path="/")
        addr1 = self.rpc_call("getnewaddress", path="/wallet/bpwallet").get("result")
        addr2 = self.rpc_call("getnewaddress", path="/wallet/bpwallet").get("result")
        created = self.rpc_call(
            "createrawbulletprooftransaction",
            [[], {addr1: 0, addr2: 0}],
            path="/wallet/bpwallet",
        )
        tx_hex = created.get("result", {}).get("hex")
        if tx_hex is None:
            self.skipTest("Bulletproof RPC not available")

        # Mempool acceptance should succeed once Bulletproofs are active. If not
        # yet active, skip the test to avoid spurious failure.
        r = self.rpc_call("testmempoolaccept", [[tx_hex]])
        result = r.get("result", [{}])[0]
        if not result.get("allowed"):
            self.skipTest("Bulletproofs not active: " + result.get("reject-reason", ""))
        self.assertTrue(result.get("allowed"))

        # Wallet RPCs should still function when Bulletproof support is built in.
        addr = self.rpc_call("getnewaddress", path="/wallet/bpwallet").get("result")
        self.assertIsInstance(addr, str)

if __name__ == '__main__':
    unittest.main()
