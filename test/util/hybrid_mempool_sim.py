#!/usr/bin/env python3
"""Utility to generate synthetic transactions for hybrid mempool testing."""

import argparse
import logging
import random
from dataclasses import dataclass
from typing import List

@dataclass
class SyntheticTx:
    txid: str
    fee: int
    size: int

def generate_tx() -> SyntheticTx:
    """Generate a single synthetic transaction."""
    txid = ''.join(random.choices('0123456789abcdef', k=64))
    fee = random.randint(1000, 10000)
    size = random.randint(200, 500)
    return SyntheticTx(txid, fee, size)

SCENARIOS = {
    'small': 5,
    'medium': 10,
    'large': 20,
}

def run_scenario(name: str, count: int) -> List[SyntheticTx]:
    logging.info("Running scenario '%s' with %d transactions", name, count)
    txs = [generate_tx() for _ in range(count)]
    logging.info("Generated %d transactions", len(txs))
    return txs

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('scenarios', nargs='+', choices=SCENARIOS.keys(),
                        help='Scenarios to run')
    args = parser.parse_args()

    # Deterministic randomness for reproducible CI
    random.seed(0)

    logging.basicConfig(level=logging.INFO, format='%(message)s')

    for scenario in args.scenarios:
        run_scenario(scenario, SCENARIOS[scenario])

if __name__ == '__main__':
    main()
