// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/util.h>
#include <univalue.h>

#include <core_io.h>
#include <primitives/transaction.h>
#include <rpc/rawtransaction_util.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#ifdef ENABLE_BULLETPROOFS
#include <bulletproofs.h>
#include <random.h>
#include <script/script.h>
#ifdef ENABLE_WALLET
#include <wallet/rpc/util.h>
#include <wallet/wallet.h>
#endif
#endif

static RPCHelpMan createrawbulletprooftransaction()
{
    return RPCHelpMan{"createrawbulletprooftransaction",
        "Create a raw transaction with Bulletproof confidential outputs.",
        {
            {"inputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "JSON array of transaction inputs."},
            {"outputs", RPCArg::Type::OBJ, RPCArg::Optional::NO, "JSON object of transaction outputs."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "hex", "The serialized transaction in hex."},
                {RPCResult::Type::ARR, "proofs", "Bulletproof range proofs for each output.",
                    {
                        {RPCResult::Type::STR_HEX, "proof", "Bulletproof range proof"},
                    }
                },
            }
        },
        RPCExamples{
            HelpExampleCli("createrawbulletprooftransaction", "\"[]\" \"{}\"") +
            HelpExampleRpc("createrawbulletprooftransaction", "[], {}")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
#ifndef ENABLE_BULLETPROOFS
            throw JSONRPCError(RPC_MISC_ERROR, "Bulletproofs not enabled");
#else
#ifdef ENABLE_WALLET
            std::shared_ptr<wallet::CWallet> wallet = wallet::GetWalletForJSONRPCRequest(request);
            if (!wallet) return UniValue::VNULL;
            wallet->BlockUntilSyncedToCurrentChain();

            const UniValue& inputs = request.params[0];
            const UniValue& outputs = request.params[1];

            if (!inputs.isArray()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Inputs must be an array");
            }
            if (!outputs.isObject()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Outputs must be an object");
            }

            // Build the transaction using existing raw transaction helpers
            CMutableTransaction mtx;
            try {
                mtx = ConstructTransaction(inputs, outputs, NullUniValue, std::nullopt);
            } catch (const std::exception& e) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Transaction construction failed: ") + e.what());
            }

            if (mtx.vout.empty()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Transaction must have at least one output");
            }

            std::vector<CBulletproof> proofs_vec;
            if (!wallet::CreateBulletproofProof(*wallet, mtx, proofs_vec)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to create Bulletproofs");
            }
            UniValue proofs(UniValue::VARR);
            for (size_t i = 0; i < proofs_vec.size(); ++i) {
                const CBulletproof& bp = proofs_vec[i];
                mtx.vout[i].scriptPubKey << OP_BULLETPROOF
                    << std::vector<unsigned char>(bp.commitment.data, bp.commitment.data + sizeof(bp.commitment.data))
                    << bp.proof;
                CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
                ss << bp;
                proofs.push_back(HexStr(ss));
            }

            UniValue result(UniValue::VOBJ);
            result.pushKV("hex", EncodeHexTx(CTransaction(mtx)));
            result.pushKV("proofs", proofs);
            return result;
#else
            throw JSONRPCError(RPC_WALLET_ERROR, "Wallet functionality not enabled");
#endif
#endif
        }
    };
}

static RPCHelpMan verifybulletproof()
{
    return RPCHelpMan{"verifybulletproof",
        "Verify a Bulletproof range proof.",
        {
            {"proof", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Hex-encoded Bulletproof to verify."},
        },
        RPCResult{RPCResult::Type::BOOL, "", "Whether the proof is valid."},
        RPCExamples{
            HelpExampleCli("verifybulletproof", "\"proofhex\"") +
            HelpExampleRpc("verifybulletproof", "\"proofhex\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
#ifndef ENABLE_BULLETPROOFS
            throw JSONRPCError(RPC_MISC_ERROR, "Bulletproofs not enabled");
#else
            const std::string proof_hex = request.params[0].get_str();
            if (!IsHex(proof_hex)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Proof must be hex encoded");
            }
            const std::vector<unsigned char> proof_bytes = ParseHex(proof_hex);
            CBulletproof bp;
            try {
                CDataStream ss(proof_bytes, SER_NETWORK, PROTOCOL_VERSION);
                ss >> bp;
            } catch (const std::exception&) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to decode Bulletproof");
            }

            return VerifyBulletproof(bp);
#endif
        }
    };
}

void RegisterBulletproofRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] {
        {"rawtransactions", &createrawbulletprooftransaction},
        {"rawtransactions", &verifybulletproof},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}

