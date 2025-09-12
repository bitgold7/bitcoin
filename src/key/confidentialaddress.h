#ifndef BITCOIN_KEY_CONFIDENTIALADDRESS_H
#define BITCOIN_KEY_CONFIDENTIALADDRESS_H

#include <string>

struct ConfidentialAddress {
    std::string address;
    std::string blinding_key;

    ConfidentialAddress() = default;
    ConfidentialAddress(std::string addr, std::string blind)
        : address(std::move(addr)), blinding_key(std::move(blind)) {}

    std::string ToString() const
    {
        return address + ":" + blinding_key;
    }

    static ConfidentialAddress FromString(const std::string& input)
    {
        ConfidentialAddress out;
        auto pos = input.find(':');
        if (pos == std::string::npos) {
            out.address = input;
        } else {
            out.address = input.substr(0, pos);
            out.blinding_key = input.substr(pos + 1);
        }
        return out;
    }

    bool IsValid() const { return !address.empty(); }
};

#endif // BITCOIN_KEY_CONFIDENTIALADDRESS_H
