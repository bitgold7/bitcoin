#include <pos/slashing.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <univalue.h>

namespace {

static RPCHelpMan getslashings()
{
    return RPCHelpMan{
        "getslashings",
        "Return known slashing events.",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "", {
                    {RPCResult::Type::STR, "validator", "Validator identifier"},
                    {RPCResult::Type::STR, "reason", "Reason for slashing"},
                    {RPCResult::Type::NUM, "time", "Event time"},
                }}
            }
        },
        RPCExamples{
            HelpExampleCli("getslashings", "") +
            HelpExampleRpc("getslashings", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            UniValue res(UniValue::VARR);
            for (const auto& ev : pos::g_slashing_tracker.GetEvents()) {
                UniValue obj(UniValue::VOBJ);
                obj.pushKV("validator", ev.validator_id);
                obj.pushKV("reason", pos::ToString(ev.reason));
                obj.pushKV("time", ev.time);
                res.push_back(obj);
            }
            return res;
        }
    };
}

static const CRPCCommand commands[] = {
    {"pos", &getslashings, nullptr, {}},
};

} // namespace

void RegisterSlashingRPCCommands(CRPCTable& table)
{
    for (const auto& c : commands) {
        table.appendCommand(c.name, &c);
    }
}
