#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <dividend/dividend.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <univalue.h>
#include <util/translation.h>

static CAmount g_dividend_pool = 100 * COIN;

static RPCHelpMan getdividendpool()
{
    return RPCHelpMan{
        "getdividendpool",
        "Return the current dividend pool amount.",
        {},
        RPCResult{RPCResult::Type::OBJ, "", "", {{RPCResult::Type::AMOUNT, "amount", "total dividend pool"}}},
        RPCExamples{HelpExampleCli("getdividendpool", "") + HelpExampleRpc("getdividendpool", "")},
        [](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            UniValue obj(UniValue::VOBJ);
            obj.pushKV("amount", ValueFromAmount(g_dividend_pool));
            return obj;
        }
    };
}

static RPCHelpMan claimdividends()
{
    return RPCHelpMan{
        "claimdividends",
        "Claim dividends from the dividend pool.",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "address claiming dividends"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "amount to claim"},
        },
        RPCResult{RPCResult::Type::OBJ, "", "", {{RPCResult::Type::AMOUNT, "claimed", "amount claimed"}, {RPCResult::Type::AMOUNT, "remaining", "remaining pool"}}},
        RPCExamples{HelpExampleCli("claimdividends", "\"addr\" 1") + HelpExampleRpc("claimdividends", "[\"addr\",1]")},
        [](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            if (request.params.size() < 2) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "address and amount required");
            }
            CAmount amount = AmountFromValue(request.params[1]);
            if (amount <= 0) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "amount must be positive");
            }
            if (amount > g_dividend_pool) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "insufficient dividend pool");
            }
            g_dividend_pool -= amount;
            UniValue obj(UniValue::VOBJ);
            obj.pushKV("claimed", ValueFromAmount(amount));
            obj.pushKV("remaining", ValueFromAmount(g_dividend_pool));
            return obj;
        }
    };
}

static RPCHelpMan getdividendschedule()
{
    return RPCHelpMan{
        "getdividendschedule",
        "Calculate dividend payouts for a set of stakes.",
        {
            {"stakes", RPCArg::Type::ARR, RPCArg::Optional::NO, "array of stake objects",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "stake object",
                        {
                            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "stake address"},
                            {"weight", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "stake weight"},
                            {"last_payout_height", RPCArg::Type::NUM, RPCArg::Optional::NO, "last payout height"},
                        }
                    }
                }
            },
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
        }
    };
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
