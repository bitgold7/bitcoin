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
    def rpc_call(self, method, params=None):
        if params is None:
            params = []
        headers = {
            "Authorization": "Basic " + b64encode(f"{RPC_USER}:{RPC_PASSWORD}".encode()).decode(),
            "Content-Type": "application/json",
        }
        payload = json.dumps({"jsonrpc": "1.0", "id": "test", "method": method, "params": params})
        conn = http.client.HTTPConnection("127.0.0.1", RPC_PORT)
        try:
            conn.request("POST", "/", payload, headers)
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

if __name__ == '__main__':
    unittest.main()
