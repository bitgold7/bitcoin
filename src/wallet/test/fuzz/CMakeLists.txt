target_sources(fuzz
  PRIVATE
    coincontrol.cpp
    coinselection.cpp
    crypter.cpp
    fees.cpp
    scriptpubkeyman.cpp
    spend.cpp
    wallet_bdb_parser.cpp
)
target_link_libraries(fuzz bitcoin_wallet)
