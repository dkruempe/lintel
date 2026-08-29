#ifndef CPP_BASE_LIBRARY_HTTPSTATUSCODES_H
#define CPP_BASE_LIBRARY_HTTPSTATUSCODES_H

#include <map>
#include <string>

/** Standard HTTP status codes with int-to-enum conversion */
class HttpStatusCodes {
public:
    enum Value : int {
        Continue = 100,
        SwitchingProtocols = 101,
        Processing = 102,
        EarlyHints = 103,
        OK = 200,
        Created = 201,
        Accepted = 202,
        NonAuthoritativeInformation = 203,
        NoContent = 204,
        ResetContent = 205,
        PartialContent = 206,
        MultiStatus = 207,
        AlreadyReported = 208,
        IAmUsed = 226,
        MultipleChoices = 300,
        MovedPermanently = 301,
        Found = 302,
        SeeOther = 303,
        NotModified = 304,
        UseProxy = 305,
        Reserved = 306,
        TemporaryRedirect = 307,
        PermanentRedirect = 308,
        BadRequest = 400,
        Unauthorized = 401,
        PaymentRequired = 402,
        Forbidden = 403,
        NotFound = 404,
        MethodNotAllowed = 405,
        NotAcceptable = 406,
        ProxyAuthenticationRequired = 407,
        RequestTimeout = 408,
        Conflict = 409,
        Gone = 410,
        LengthRequired = 411,
        PreconditionFailed = 412,
        PayloadTooLarge = 413,
        URITooLong = 414,
        UnsupportedMediaType = 415,
        RangeNotSatisfiable = 416,
        ExpectationFailed = 417,
        MisdirectedRequest = 421,
        UnprocessableEntity = 422,
        Locked = 423,
        FailedDependency = 424,
        TooEarly = 425,
        UpgradeRequired = 426,
        PreconditionRequired = 428,
        TooManyRequests = 429,
        RequestHeaderFieldsTooLarge = 431,
        UnavailableForLegalReasons = 451,
        IAmATeaPot = 418,
        PolicyNotFulfilled = 420,
        NoResponse = 444,
        TheRequestShouldBeRetriedAfterDoingTheAppropriateAction = 449,
        ClientClosedRequest = 499,
        InternalServerError = 500,
        NotImplemented = 501,
        BadGateway = 502,
        ServiceUnavailable = 503,
        GatewayTimeout = 504,
        HTTPVersionNotSupported = 505,
        VariantAlsoNegotiates = 506,
        InsufficientStorage = 507,
        LoopDetected = 508,
        BandwidthLimitExceeded = 509,
        NotExtended = 510,
        NetworkAuthenticationRequired = 511
    };

    /** @param value the status code enum value */
    constexpr HttpStatusCodes(Value value) : m_value(value) {}

    /** @param statusCode numeric HTTP status code */
    constexpr explicit HttpStatusCodes(int statusCode)
        : m_value(static_cast<Value>(statusCode)) {}

    constexpr operator Value() const { return m_value; }

    explicit operator bool() = delete;

    /** @return the underlying enum value */
    [[nodiscard]] constexpr Value getValue() const { return m_value; }

    /** @return the numeric status code */
    [[nodiscard]] constexpr int getCode() const { return static_cast<int>(m_value); }

private:
    Value m_value;
};

#endif  // CPP_BASE_LIBRARY_HTTPSTATUSCODES_H
