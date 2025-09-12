# Configuration validation

At startup the node performs basic sanity checks on configuration values to help catch unsafe setups.

* **Port checks** – The P2P and RPC ports are compared against the known defaults of other networks. If a configured port matches another network's default, a warning is printed.
* **Prefix validation** – Each value passed via `rpcallowip` is parsed and validated. Invalid or overly permissive prefixes such as `0.0.0.0/0` trigger a startup warning.
* **Network mismatches** – Warnings are emitted when the chosen network does not match user supplied ports.

These checks do not stop start-up but they highlight potentially dangerous combinations so they can be corrected in the configuration file or command line options.
