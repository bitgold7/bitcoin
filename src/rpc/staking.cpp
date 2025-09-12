// Copyright (c) 2024 The BitGold developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <common/args.h>
#include <core_io.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <stake/priority.h>
#include <univalue.h>
#include <util/translation.h>
#include <wallet/rpc/util.h>
#include <wallet/wallet.h>

using wallet::CWallet;
using wallet::GetWalletForJSONRPCRequest;

namespace wallet {

static RPCHelpMan getstakingstatus()
{
    return RPCHelpMan{
        "getstakingstatus",
        "Returns the staking status for this wallet.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "", {
                                              {RPCResult::Type::BOOL, "enabled", "true if staking is enabled via -staking/-staker"},
                                              {RPCResult::Type::BOOL, "staking", "true if the staking thread is running"},
                                          }},
        RPCExamples{HelpExampleCli("getstakingstatus", "") + HelpExampleRpc("getstakingstatus", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) return UniValue::VNULL;
            pwallet->BlockUntilSyncedToCurrentChain();

            UniValue obj(UniValue::VOBJ);
            obj.pushKV("enabled", gArgs.GetBoolArg("-staker", false) || gArgs.GetBoolArg("-staking", false));
            obj.pushKV("staking", pwallet->IsStaking());
            return obj;
        }};
}

static RPCHelpMan setreservebalance()
{
    return RPCHelpMan{
        "setreservebalance",
        "Set or get the reserve balance that will not be used for staking.",
        {
            {"reserve", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "true to reserve balance, false to disable reserve"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "amount in BGD to reserve"},
        },
        RPCResult{RPCResult::Type::OBJ, "", "", {{RPCResult::Type::NUM, "reserved", "current reserved amount"}}},
        RPCExamples{HelpExampleCli("setreservebalance", "true 100") + HelpExampleCli("setreservebalance", "") + HelpExampleRpc("setreservebalance", "false")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            std::shared_ptr<CWallet> pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) return UniValue::VNULL;
            pwallet->BlockUntilSyncedToCurrentChain();
            if (request.params.empty()) {
                UniValue ret(UniValue::VOBJ);
                ret.pushKV("reserved", ValueFromAmount(pwallet->GetReserveBalance()));
                return ret;
            }
            bool reserve = request.params[0].get_bool();
            if (!reserve) {
                pwallet->SetReserveBalance(0);
            } else {
                if (request.params.size() < 2) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "amount required");
                }
                CAmount amount = AmountFromValue(request.params[1]);
                pwallet->SetReserveBalance(amount);
            }
            UniValue ret(UniValue::VOBJ);
            ret.pushKV("reserved", ValueFromAmount(pwallet->GetReserveBalance()));
            return ret;
        }};
}

static RPCHelpMan reservebalance()
{
    return RPCHelpMan{
        "reservebalance",
        "Deprecated alias for setreservebalance.",
        {
            {"reserve", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "true to reserve balance, false to disable reserve"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "amount in BGD to reserve"},
        },
        RPCResult{RPCResult::Type::OBJ, "", "", {{RPCResult::Type::NUM, "reserved", "current reserved amount"}}},
        RPCExamples{HelpExampleCli("setreservebalance", "true 100") + HelpExampleCli("setreservebalance", "") + HelpExampleRpc("setreservebalance", "false")},
        [](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            return setreservebalance().HandleRequest(request);
        }};
}

static RPCHelpMan getstakinginfo()
{
    return RPCHelpMan{
        "getstakinginfo",
        "Returns the staking status for this wallet.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "", {
                                              {RPCResult::Type::BOOL, "enabled", "true if staking is enabled via -staking/-staker"},
                                              {RPCResult::Type::BOOL, "staking", "true if the staking thread is running"},
                                          }},
        RPCExamples{HelpExampleCli("getstakinginfo", "") + HelpExampleRpc("getstakinginfo", "")},
        [](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            return getstakingstatus().HandleRequest(request);
        }};
}

static RPCHelpMan setstakesplitthreshold()
{
    return RPCHelpMan{
        "setstakesplitthreshold",
        "Set or get the stake split threshold.",
        {{"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "threshold in BGD"}},
        RPCResult{RPCResult::Type::OBJ, "", "", {{RPCResult::Type::NUM, "threshold", "current threshold"}}},
        RPCExamples{HelpExampleCli("setstakesplitthreshold", "100") + HelpExampleRpc("setstakesplitthreshold", "50")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            std::shared_ptr<CWallet> pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) return UniValue::VNULL;
            pwallet->BlockUntilSyncedToCurrentChain();
            if (!request.params.empty()) {
                CAmount amount = AmountFromValue(request.params[0]);
                pwallet->SetStakeSplitThreshold(amount);
            }
            UniValue ret(UniValue::VOBJ);
            ret.pushKV("threshold", ValueFromAmount(pwallet->GetStakeSplitThreshold()));
            return ret;
        }
    };
}

static RPCHelpMan startstaking()
{
    return RPCHelpMan{
        "startstaking",
        "Start the staking thread for this wallet.",
        {},
        RPCResult{RPCResult::Type::BOOL, "", "true if staking is active"},
        RPCExamples{HelpExampleCli("startstaking", "") + HelpExampleRpc("startstaking", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            std::shared_ptr<CWallet> pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) return UniValue::VNULL;
            pwallet->BlockUntilSyncedToCurrentChain();
            pwallet->StartStakeMiner();
            return UniValue(pwallet->IsStaking());
        }};
}

static RPCHelpMan stopstaking()
{
    return RPCHelpMan{
        "stopstaking",
        "Stop the staking thread for this wallet.",
        {},
        RPCResult{RPCResult::Type::BOOL, "", "false if staking stopped"},
        RPCExamples{HelpExampleCli("stopstaking", "") + HelpExampleRpc("stopstaking", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            std::shared_ptr<CWallet> pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) return UniValue::VNULL;
            pwallet->BlockUntilSyncedToCurrentChain();
            pwallet->StopStakeMiner();
            return UniValue(pwallet->IsStaking());
        }};
}

static RPCHelpMan priorityengine()
{
    return RPCHelpMan{
        "priorityengine",
        "Enable or disable the staking priority engine. Returns the current state.",
        {{"enable", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "true to enable, false to disable"}},
        RPCResult{RPCResult::Type::BOOL, "", "true if priority engine is enabled"},
        RPCExamples{HelpExampleCli("priorityengine", "false") + HelpExampleRpc("priorityengine", "true")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            if (!request.params.empty()) {
                bool enable = request.params[0].get_bool();
                SetPriorityEngineEnabled(enable);
            }
            return UniValue(IsPriorityEngineEnabled());
        }};
}

static const CRPCCommand commands[] = {
    {"wallet", &getstakingstatus},
    {"wallet", &setreservebalance},
    {"wallet", &reservebalance},
    {"wallet", &getstakinginfo},
    {"wallet", &setstakesplitthreshold},
    {"wallet", &startstaking},
    {"wallet", &stopstaking},
    {"wallet", &priorityengine},
};

} // namespace wallet

void RegisterStakingRPCCommands(CRPCTable& t)
{
    for (const auto& c : wallet::commands) {
        t.appendCommand(c);
    }
}
