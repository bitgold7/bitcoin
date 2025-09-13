#include <bitgold-build-config.h> // IWYU pragma: keep

#include <consensus/dividends/dividend.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <validation.h>
#include <wallet/rpc/util.h>
#include <wallet/receive.h>
#include <key_io.h>

namespace wallet {

RPCHelpMan getwalletdividends()
{
    return RPCHelpMan{
        "getwalletdividends",
        "Return pending dividend amounts for wallet addresses.",
        {},
        RPCResult{RPCResult::Type::OBJ, "", "", {{RPCResult::Type::AMOUNT, "<address>", "pending dividend"}}},
        RPCExamples{HelpExampleCli("getwalletdividends", "") + HelpExampleRpc("getwalletdividends", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) return UniValue::VNULL;
            LOCK(cs_main);
            Chainstate& chainstate = EnsureAnyChainman(request.context).ActiveChainstate();
            UniValue ret(UniValue::VOBJ);
            auto balances = GetAddressBalances(*pwallet);
            for (const auto& [dest, bal] : balances) {
                std::string addr = EncodeDestination(dest);
                auto it = chainstate.GetPendingDividends().find(addr);
                if (it != chainstate.GetPendingDividends().end()) {
                    ret.pushKV(addr, ValueFromAmount(it->second));
                }
            }
            return ret;
        }
    };
}

RPCHelpMan claimwalletdividends()
{
    return RPCHelpMan{
        "claimwalletdividends",
        "Claim pending dividends for addresses in the wallet.",
        {{"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "specific address or * for all"}},
        RPCResult{RPCResult::Type::OBJ, "", "", {{RPCResult::Type::AMOUNT, "<address>", "amount claimed"}}},
        RPCExamples{HelpExampleCli("claimwalletdividends", "") + HelpExampleRpc("claimwalletdividends", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) return UniValue::VNULL;
            LOCK(cs_main);
            Chainstate& chainstate = EnsureAnyChainman(request.context).ActiveChainstate();
            UniValue ret(UniValue::VOBJ);
            if (!request.params[0].isNull() && request.params[0].get_str() != "*") {
                std::string addr = request.params[0].get_str();
                CAmount amt = chainstate.ClaimDividend(addr);
                if (amt > 0) ret.pushKV(addr, ValueFromAmount(amt));
                return ret;
            }
            auto balances = GetAddressBalances(*pwallet);
            for (const auto& [dest, bal] : balances) {
                std::string addr = EncodeDestination(dest);
                CAmount amt = chainstate.ClaimDividend(addr);
                if (amt > 0) ret.pushKV(addr, ValueFromAmount(amt));
            }
            return ret;
        }
    };
}

RPCHelpMan getwalletdividendhistory()
{
    return RPCHelpMan{
        "getwalletdividendhistory",
        "Return historical dividend payouts for wallet addresses.",
        {},
        RPCResult{RPCResult::Type::OBJ, "", "", {{RPCResult::Type::OBJ, "<height>", "payouts", {{RPCResult::Type::AMOUNT, "<address>", "payout"}}}}},
        RPCExamples{HelpExampleCli("getwalletdividendhistory", "") + HelpExampleRpc("getwalletdividendhistory", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) return UniValue::VNULL;
            Chainstate& chainstate = EnsureAnyChainman(request.context).ActiveChainstate();
            LOCK(cs_main);
            pwallet->UpdateDividendHistory(chainstate);
            UniValue ret(UniValue::VOBJ);
            for (const auto& [height, payouts] : pwallet->GetDividendHistory()) {
                UniValue inner(UniValue::VOBJ);
                for (const auto& [addr, amt] : payouts) {
                    inner.pushKV(addr, ValueFromAmount(amt));
                }
                ret.pushKV(std::to_string(height), inner);
            }
            return ret;
        }
    };
}

RPCHelpMan getwalletnextdividend()
{
    return RPCHelpMan{
        "getwalletnextdividend",
        "Estimate next dividend payout for wallet addresses.",
        {},
        RPCResult{RPCResult::Type::OBJ, "", "", {
            {RPCResult::Type::NUM, "height", "next payout height"},
            {RPCResult::Type::OBJ, "payouts", "payouts", {{RPCResult::Type::AMOUNT, "<address>", "payout"}}}
        }},
        RPCExamples{HelpExampleCli("getwalletnextdividend", "") + HelpExampleRpc("getwalletnextdividend", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) return UniValue::VNULL;
            auto res = pwallet->GetNextDividend();
            UniValue ret(UniValue::VOBJ);
            ret.pushKV("height", res.first);
            UniValue pobj(UniValue::VOBJ);
            for (const auto& [addr, amt] : res.second) {
                pobj.pushKV(addr, ValueFromAmount(amt));
            }
            ret.pushKV("payouts", pobj);
            return ret;
        }
    };
}

} // namespace wallet
