#ifndef CPP_BASE_LIBRARY_CLIENTIPRESOLVER_H
#define CPP_BASE_LIBRARY_CLIENTIPRESOLVER_H

#include <string>
#include <vector>

/**
 * Resolves the client IP address of an HTTP request.
 *
 * The X-Forwarded-For header is only honored when the direct peer (the
 * socket remote address) is a configured trusted proxy. Untrusted peers
 * cannot spoof their IP, so the header is ignored by default. Trusted
 * proxies are matched as exact IPs or CIDR ranges (e.g. "10.0.0.0/8").
 */
class ClientIpResolver {
public:
    ClientIpResolver() = default;

    /** @param trustedProxies IPs or CIDR ranges allowed to forward X-Forwarded-For */
    explicit ClientIpResolver(std::vector<std::string> trustedProxies);

    /** Replace the list of trusted proxies (exact IPs or CIDR ranges). */
    void setTrustedProxies(std::vector<std::string> trustedProxies);

    /**
     * Determine the client IP of a request.
     * @param remoteAddress the direct peer address (socket remote address)
     * @param forwardedFor  the raw X-Forwarded-For header value (may be empty)
     * @return the client IP; falls back to the remote address unless a trusted
     *         proxy provided a valid, non-empty X-Forwarded-For entry
     */
    std::string resolveClientIp(const std::string &remoteAddress,
                                const std::string &forwardedFor) const;

private:
    /** @return true if the direct peer is a configured trusted proxy. */
    [[nodiscard]] bool isTrustedPeer(const std::string &remoteAddress) const;

    std::vector<std::string> m_trustedProxies;
};

#endif  // CPP_BASE_LIBRARY_CLIENTIPRESOLVER_H
