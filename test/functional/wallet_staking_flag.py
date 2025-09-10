from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class WalletStakingFlagTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-staking=1"], []]

    def run_test(self):
        info_enabled = self.nodes[0].getstakinginfo()
        assert_equal(info_enabled["enabled"], True)
        info_disabled = self.nodes[1].getstakinginfo()
        assert_equal(info_disabled["enabled"], False)

if __name__ == '__main__':
    WalletStakingFlagTest().main()
