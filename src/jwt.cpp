#include "jwtxx/jwt.h"

#include "keyimpl.h"
#include "hmackey.h"
#include "pemkey.h"
#include "base64url.h"
#include "utils.h"
#include "json.h"

#include <vector>
#include <mutex> // std::once_flag/std::call_once

#include <openssl/evp.h>
#include <openssl/err.h>

using JWTXX::Algorithm;
using JWTXX::Validator;
using JWTXX::Pairs;
using JWTXX::Key;
using JWTXX::JWT;

namespace Keys = JWTXX::Keys;
namespace Validate = JWTXX::Validate;

namespace
{

struct NoneKey : public Key::Impl
{
    std::string sign(const void* /*data*/, size_t /*size*/) const override { return {}; }
    bool verify(const void* /*data*/, size_t /*size*/, const std::string& /*signature*/) const override { return true; }
};

Key::Impl* createKey(Algorithm alg, const std::string& keyData)
{
    switch (alg)
    {
        case Algorithm::none: return new NoneKey;
        case Algorithm::HS256: return new Keys::HMAC(EVP_sha256(), keyData);
        case Algorithm::HS384: return new Keys::HMAC(EVP_sha384(), keyData);
        case Algorithm::HS512: return new Keys::HMAC(EVP_sha512(), keyData);
        case Algorithm::RS256: return new Keys::PEM(EVP_sha256(), keyData);
        case Algorithm::RS384: return new Keys::PEM(EVP_sha384(), keyData);
        case Algorithm::RS512: return new Keys::PEM(EVP_sha512(), keyData);
        case Algorithm::ES256: return new Keys::PEM(EVP_sha256(), keyData);
        case Algorithm::ES384: return new Keys::PEM(EVP_sha384(), keyData);
        case Algorithm::ES512: return new Keys::PEM(EVP_sha512(), keyData);
    }
    return new NoneKey; // Just in case.
}

std::tuple<std::string, std::string, std::string> split(const std::string& token)
{
    auto pos = token.find_first_of('.');
    if (pos == std::string::npos)
        throw JWT::Error("JWT should have at least 2 parts separated by a dot.");
    auto spos = token.find_first_of('.', pos + 1);
    if (spos == std::string::npos)
        return std::make_tuple(token.substr(0, pos), token.substr(pos + 1, token.length() - pos - 1), "");
    return std::make_tuple(token.substr(0, pos),
                           token.substr(pos + 1, spos - pos - 1),
                           token.substr(spos + 1, token.length() - spos - 1));
}

template <typename F>
bool validTime(const std::string& value, F&& next)
{
    try
    {
        size_t pos = 0;
        auto t = std::stoull(value, &pos);
        if (pos != value.length())
            return false;
        return next(t);
    }
    catch (const std::logic_error&)
    {
        return false;
    }
}

template <typename F>
bool validClaim(const Pairs& claims, const std::string& claim, F&& next)
{
    auto it = claims.find(claim);
    if (it == std::end(claims))
        return true;
    return next(it->second);
}

template <typename F>
bool validTimeClaim(const Pairs& claims, const std::string& claim, F&& next)
{
    return validClaim(claims, claim, [=](const std::string& value) { return validTime(value, next); });
}

Validator stringValidator(const std::string& name, const std::string& validValue)
{
    return [=](const Pairs& claims)
           {
               return validClaim(claims, name, [&](const std::string& value){ return value == validValue; });
           };
}

}

void JWTXX::enableOpenSSLErrors()
{
    static std::once_flag flag;
    std::call_once(flag, [](){ OpenSSL_add_all_algorithms(); ERR_load_crypto_strings(); });
}

std::string JWTXX::algToString(Algorithm alg)
{
    switch (alg)
    {
        case Algorithm::none: return "none";
        case Algorithm::HS256: return "HS256";
        case Algorithm::HS384: return "HS384";
        case Algorithm::HS512: return "HS512";
        case Algorithm::RS256: return "RS256";
        case Algorithm::RS384: return "RS384";
        case Algorithm::RS512: return "RS512";
        case Algorithm::ES256: return "ES256";
        case Algorithm::ES384: return "ES384";
        case Algorithm::ES512: return "ES512";
    }
    return ""; // Just in case.
}

Algorithm JWTXX::stringToAlg(const std::string& value)
{
    if (value == "none") return Algorithm::none;
    else if (value == "HS256") return Algorithm::HS256;
    else if (value == "HS384") return Algorithm::HS384;
    else if (value == "HS512") return Algorithm::HS512;
    else if (value == "RS256") return Algorithm::RS256;
    else if (value == "RS384") return Algorithm::RS384;
    else if (value == "RS512") return Algorithm::RS512;
    else if (value == "ES256") return Algorithm::ES256;
    else if (value == "ES384") return Algorithm::ES384;
    else if (value == "ES512") return Algorithm::ES512;
    else throw Error("Invalid algorithm name: '" + value + "'.");
}

Key::Key(Algorithm alg, const std::string& keyData)
    : m_alg(alg), m_impl(createKey(alg, keyData))
{
}

Key::~Key() = default;
Key::Key(Key&&) = default;
Key& Key::operator=(Key&&) = default;

std::string Key::sign(const void* data, size_t size) const
{
    return m_impl->sign(data, size);
}

bool Key::verify(const void* data, size_t size, const std::string& signature) const
{
    return m_impl->verify(data, size, signature);
}

JWT::JWT(Algorithm alg, Pairs claims, Pairs header)
    : m_alg(alg), m_header(header), m_claims(claims)
{
    m_header["typ"] = "JWT";
    m_header["alg"] = algToString(m_alg);
}

JWT::JWT(const std::string& token, Key key, Validators&& validators)
{
    auto parts = split(token);
    auto data = std::get<0>(parts) + "." + std::get<1>(parts);
    if (!key.verify(data.c_str(), data.size(), std::get<2>(parts)))
        throw Error("Signature is invalid.");
    m_alg = key.alg();
    m_header = fromJSON(Base64URL::decode(std::get<0>(parts)).toString());
    m_claims = fromJSON(Base64URL::decode(std::get<1>(parts)).toString());
    for (const auto& validator : validators)
        if (!validator(m_claims))
            throw Error("Invalid token.");
}

JWT JWT::parse(const std::string& token)
{
    auto parts = split(token);
    Pairs header = fromJSON(Base64URL::decode(std::get<0>(parts)).toString());
    Pairs claims = fromJSON(Base64URL::decode(std::get<1>(parts)).toString());
    Algorithm alg = Algorithm::none;
    if (!header["alg"].empty())
        alg = stringToAlg(header["alg"]);
    return JWT(alg, claims, header);
}

bool JWT::verify(const std::string& token, Key key, Validators&& validators)
{
    auto parts = split(token);
    auto data = std::get<0>(parts) + "." + std::get<1>(parts);
    if (!key.verify(data.c_str(), data.size(), std::get<2>(parts)))
        return false;
    Pairs claims = fromJSON(Base64URL::decode(std::get<1>(parts)).toString());
    for (const auto& validator : validators)
        if (!validator(claims))
            return false;
    return true;
}

std::string JWT::claim(const std::string& name) const
{
    auto it = m_claims.find(name);
    if (it == std::end(m_claims))
        return {};
    return it->second;
}

std::string JWT::token(const std::string& keyData) const
{
    auto data = Base64URL::encode(toJSON(m_header)) + "." + Base64URL::encode(toJSON(m_claims));
    Key key(m_alg, keyData);
    auto signature = key.sign(data.c_str(), data.size());
    if (signature.empty())
        return data;
    return data + "." + signature;
}

Validator Validate::exp(std::time_t now)
{
    return [=](const Pairs& claims)
           {
               return validTimeClaim(claims, "exp", [=](std::time_t value){ return value > now; });
           };
}

Validator Validate::nbf(std::time_t now)
{
    return [=](const Pairs& claims)
           {
               return validTimeClaim(claims, "nbf", [=](std::time_t value){ return value < now; });
           };
}

Validator Validate::iat(std::time_t now)
{
    return [=](const Pairs& claims)
           {
               return validTimeClaim(claims, "iat", [=](std::time_t value){ return value < now; });
           };
}

Validator Validate::iss(const std::string& issuer)
{
    return stringValidator("iss", issuer);
}

Validator Validate::aud(const std::string& audience)
{
    return stringValidator("aud", audience);
}

Validator Validate::sub(const std::string& subject)
{
    return stringValidator("sub", subject);
}
