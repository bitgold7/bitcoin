# DNS Seeding Operations

This document describes how to operate DNS seeders for the BitGold network.
Operating several independent seeders provides robustness for new nodes
searching for peers. At least three seeders should be deployed and maintained
by different operators.

## Deploying seeders

The reference network currently uses the following public seeders:

- `dnsseed.bitgold.org`
- `dnsseed.bitgold.com`
- `dnsseed.bitgold.io`

Additional seeders may be added as the network grows. Seeders should run a
crawler that collects reachable peers and exports them via DNS records.

## Peer crawler and liveness checks

The `contrib/dnsseed/peer_crawler.py` script automates crawling and basic
health checks. It resolves peers from the configured seeders, connects to the
peers to verify they are reachable and keeps one peer per network group to
promote diversity. If the number of reachable peers drops below a configured
minimum, an optional alert command is executed.

Example usage:

```sh
python3 contrib/dnsseed/peer_crawler.py --min-peers 12 --alert-cmd 'echo alert'
```

The script exits with a non-zero status when the threshold is not met, making
it suitable for integration in cron jobs or other monitoring systems.

## Alerts

Operators should configure `--alert-cmd` to trigger notifications (e.g. send an
email or push message) whenever the script detects insufficient reachable peers.

Keeping the seeders responsive and populated with diverse peers ensures new
nodes can quickly join the network.
