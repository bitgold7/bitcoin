#include <bitgold-build-config.h> // IWYU pragma: keep

#include <dividend/dividend.h>
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

} // namespace wallet
