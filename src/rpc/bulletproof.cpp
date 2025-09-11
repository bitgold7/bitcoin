// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/util.h>
#include <univalue.h>

#include <core_io.h>
#include <primitives/transaction.h>
#include <rpc/rawtransaction_util.h>
#include <util/strencodings.h>
#ifdef ENABLE_BULLETPROOFS
#include <bulletproofs.h>
#include <random.h>
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
                {RPCResult::Type::STR_HEX, "proof", "Bulletproof range proof for the first output."},
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

            UniValue result(UniValue::VOBJ);
            result.pushKV("hex", EncodeHexTx(CTransaction(mtx)));

            // Generate a Bulletproof for the first output amount
            if (mtx.vout.empty()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Transaction must have at least one output");
            }

            CBulletproof bp;
            CAmount amount = mtx.vout[0].nValue;

            unsigned char blind[32];
            GetRandBytes({blind, 32});

            static secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);

            if (secp256k1_pedersen_commit(ctx, &bp.commitment, blind, amount, &secp256k1_generator_h) != 1) {
                throw JSONRPCError(RPC_MISC_ERROR, "Failed to create commitment");
            }

            bp.proof.resize(SECP256K1_RANGE_PROOF_MAX_LENGTH);
            size_t proof_len = bp.proof.size();
            if (secp256k1_rangeproof_sign(ctx, bp.proof.data(), &proof_len, 0, &bp.commitment, blind, nullptr, 0, 0, amount, &secp256k1_generator_h) != 1) {
                throw JSONRPCError(RPC_MISC_ERROR, "Failed to generate Bulletproof");
            }
            bp.proof.resize(proof_len);
            bp.extra.clear();

            CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
            ss << bp;
            result.pushKV("proof", HexStr(ss));

            return result;
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

