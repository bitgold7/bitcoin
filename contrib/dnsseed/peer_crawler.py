#!/usr/bin/env python3
"""DNS seeder peer crawler with liveness checks.

Resolves peers from configured DNS seeders, tests connectivity and ensures
peers originate from diverse network groups. If the number of reachable
peers falls below a configured threshold, an optional alert command is
executed.
"""

import argparse
import asyncio
import ipaddress
import socket
import subprocess
import sys
from typing import Dict, List

DEFAULT_SEEDS = [
    "dnsseed.bitgold.org",
    "dnsseed.bitgold.com",
    "dnsseed.bitgold.io",
]
DEFAULT_PORT = 8888


async def check_peer(addr: str, port: int, timeout: float) -> bool:
    """Attempt to open a connection to ``addr`` and return ``True`` if successful."""
    try:
        reader, writer = await asyncio.wait_for(
            asyncio.open_connection(addr, port), timeout
        )
        writer.close()
        await writer.wait_closed()
        return True
    except Exception:
        return False


def network_group(addr: str) -> str:
    """Return the network group identifier for ``addr``.

    IPv4 addresses are grouped by /16, IPv6 by /32.
    """
    ip = ipaddress.ip_address(addr)
    if isinstance(ip, ipaddress.IPv4Address):
        network = ipaddress.ip_network(f"{addr}/16", strict=False)
    else:
        network = ipaddress.ip_network(f"{addr}/32", strict=False)
    return str(network)


async def crawl(seeds: List[str], port: int, timeout: float, limit: int) -> List[str]:
    """Resolve peers from ``seeds`` and return list of reachable peer addresses."""
    unique: Dict[str, str] = {}
    tasks: List[asyncio.Task] = []

    for seed in seeds:
        try:
            infos = socket.getaddrinfo(seed, port)
        except socket.gaierror:
            continue
        for info in infos:
            addr = info[4][0]
            group = network_group(addr)
            if group in unique:
                continue
            unique[group] = addr
            tasks.append(asyncio.create_task(check_peer(addr, port, timeout)))
            if limit and len(unique) >= limit:
                break

    results = await asyncio.gather(*tasks)
    reachable = [addr for addr, ok in zip(unique.values(), results) if ok]
    return reachable


def main() -> int:
    parser = argparse.ArgumentParser(
        description="DNS seeder peer crawler with liveness checks",
    )
    parser.add_argument(
        "--seeds",
        nargs="+",
        default=DEFAULT_SEEDS,
        help="DNS seeder hostnames to query",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help="peer port to test",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=3.0,
        help="connection timeout in seconds",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=50,
        help="maximum unique peers to test",
    )
    parser.add_argument(
        "--min-peers",
        type=int,
        default=8,
        help="minimum reachable peers before alerting",
    )
    parser.add_argument(
        "--alert-cmd",
        help="shell command to execute when reachable peers drop below --min-peers",
    )

    args = parser.parse_args()
    reachable = asyncio.run(crawl(args.seeds, args.port, args.timeout, args.limit))
    print(f"reachable peers: {len(reachable)}")

    if len(reachable) < args.min_peers:
        print(
            f"ALERT: reachable peers below threshold ({len(reachable)} < {args.min_peers})",
            file=sys.stderr,
        )
        if args.alert_cmd:
            subprocess.call(args.alert_cmd, shell=True)
        return 1
    return 0


if __name__ == "__main__":  # pragma: no cover - script entry point
    raise SystemExit(main())
