#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <common/args.h>
#include <dividend/dividend.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <univalue.h>
#include <util/translation.h>
#include <validation.h>

static RPCHelpMan getdividendpool()
{
    return RPCHelpMan{
        "getdividendpool",
        "Return the current dividend pool amount.",
        {},
        RPCResult{RPCResult::Type::OBJ, "", "", {{RPCResult::Type::AMOUNT, "amount", "total dividend pool"}}},
        RPCExamples{HelpExampleCli("getdividendpool", "") + HelpExampleRpc("getdividendpool", "")},
        [](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            ChainstateManager& chainman = EnsureAnyChainman(request.context);
            LOCK(cs_main);
            CAmount pool = chainman.ActiveChainstate().GetDividendPool();
            UniValue obj(UniValue::VOBJ);
            obj.pushKV("amount", ValueFromAmount(pool));
            return obj;
        }};
}

static RPCHelpMan claimdividends()
{
    return RPCHelpMan{
        "claimdividends",
        "Claim pending dividends for an address. Requires -dividendpayouts to be enabled.",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "address claiming dividends"},
        },
        RPCResult{RPCResult::Type::OBJ, "", "", {{RPCResult::Type::STR_AMOUNT, "claimed", "amount claimed"}, {RPCResult::Type::STR_AMOUNT, "remaining", "remaining pool"}}},
        RPCExamples{HelpExampleCli("claimdividends", "\"addr\"") + HelpExampleRpc("claimdividends", "[\"addr\"]")},
        [](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            if (!gArgs.GetBoolArg("-dividendpayouts", false)) {
                throw JSONRPCError(RPC_MISC_ERROR, "dividend payouts are disabled");
            }
            ChainstateManager& chainman = EnsureAnyChainman(request.context);
            LOCK(cs_main);
            Chainstate& chainstate = chainman.ActiveChainstate();
            std::string addr = request.params[0].get_str();
            CAmount amount = chainstate.ClaimDividend(addr);
            UniValue obj(UniValue::VOBJ);
            obj.pushKV("claimed", ValueFromAmount(amount));
            obj.pushKV("remaining", ValueFromAmount(chainstate.GetDividendPool()));
            return obj;
        }};
}

static RPCHelpMan getdividendschedule()
{
    return RPCHelpMan{
        "getdividendschedule",
        "Calculate dividend payouts for a set of stakes.",
        {
            {"stakes", RPCArg::Type::ARR, RPCArg::Optional::NO, "array of stake objects", {{"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "stake object", {
                                                                                                                                                                  {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "stake address"},
                                                                                                                                                                  {"weight", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "stake weight"},
                                                                                                                                                                  {"last_payout_height", RPCArg::Type::NUM, RPCArg::Optional::NO, "last payout height"},
                                                                                                                                                              }}}},
            {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "current block height"},
            {"pool", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "dividend pool to distribute"},
        },
        RPCResult{RPCResult::Type::OBJ, "", "payouts", {{RPCResult::Type::AMOUNT, "<address>", "payout amount"}}},
        RPCExamples{HelpExampleCli("getdividendschedule", "\"[]\" 0 0") + HelpExampleRpc("getdividendschedule", "[[],0,0]")},
        [](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            if (request.params.size() < 3) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "stakes, height, pool required");
            }
            std::map<std::string, StakeInfo> stakes;
            const UniValue& arr = request.params[0].get_array();
            for (unsigned int idx = 0; idx < arr.size(); ++idx) {
                const UniValue& obj = arr[idx].get_obj();
                const std::string& addr = obj["address"].get_str();
                CAmount weight = AmountFromValue(obj["weight"]);
                int last = obj["last_payout_height"].get_int();
                stakes.emplace(addr, StakeInfo{weight, last});
            }
            int height = request.params[1].get_int();
            CAmount pool = AmountFromValue(request.params[2]);
            dividend::Payouts payouts = dividend::CalculatePayouts(stakes, height, pool);
            UniValue ret(UniValue::VOBJ);
            for (const auto& [addr, amt] : payouts) {
                ret.pushKV(addr, ValueFromAmount(amt));
            }
            return ret;
        }};
}

static const CRPCCommand commands[] = {
    {"dividend", &getdividendpool},
    {"dividend", &claimdividends},
    {"dividend", &getdividendschedule},
};

void RegisterDividendRPCCommands(CRPCTable& t)
{
    for (const auto& c : commands) {
        t.appendCommand(c);
    }
}
