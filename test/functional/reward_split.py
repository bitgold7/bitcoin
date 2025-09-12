from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import CTxOut
from test_framework.script import CScript, OP_TRUE
from test_framework.util import assert_equal


class RewardSplitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        best_hash = node.getbestblockhash()
        prev_block = node.getblock(best_hash)
        height = prev_block["height"] + 1

        # Block with missing dividend output must be rejected
        coinbase = create_coinbase(height)
        subsidy = coinbase.vout[0].nValue
        coinbase.vout = [CTxOut(subsidy, CScript([OP_TRUE]))]
        block = create_block(int(best_hash, 16), coinbase, prev_block["time"] + 1,
                             tmpl={"bits": prev_block["bits"], "height": height})
        block.solve()
        assert node.submitblock(block.serialize().hex()) is not None
        assert_equal(node.getblockcount(), height - 1)

        # Correctly split block is accepted
        coinbase = create_coinbase(height)
        validator_reward = subsidy * 9 // 10
        dividend_reward = subsidy - validator_reward
        coinbase.vout = [
            CTxOut(validator_reward, CScript([OP_TRUE])),
            CTxOut(dividend_reward, CScript([OP_TRUE])),
        ]
        block = create_block(int(best_hash, 16), coinbase, prev_block["time"] + 1,
                             tmpl={"bits": prev_block["bits"], "height": height})
        block.solve()
        assert node.submitblock(block.serialize().hex()) is None
        assert_equal(node.getblockcount(), height)


if __name__ == '__main__':
    RewardSplitTest(__file__).main()

