#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

FUZZ_TARGET(script_verify)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};
    const CScript script_sig = ConsumeScript(fdp);
    const CScript script_pubkey = ConsumeScript(fdp, /*maybe_p2wsh=*/true);
    const std::optional<CMutableTransaction> mtx = ConsumeDeserializable<CMutableTransaction>(fdp, TX_WITH_WITNESS);
    if (!mtx) return;
    const CTransaction tx{*mtx};
    const unsigned int in = fdp.ConsumeIntegral<unsigned int>();
    if (in >= tx.vin.size()) return;
    const unsigned int flags = fdp.ConsumeIntegral<unsigned int>();
    const CAmount amount = ConsumeMoney(fdp);
    PrecomputedTransactionData txdata{tx};
    const CScriptWitness* witness = &tx.vin[in].scriptWitness;
    TransactionSignatureChecker checker{&tx, in, amount, txdata, MissingDataBehavior::FAIL};
    (void)VerifyScript(script_sig, script_pubkey, witness, flags, checker);
}
