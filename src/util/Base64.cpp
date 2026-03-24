#include "util/Base64.hpp"
#include <algorithm>
#include <stdexcept>

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>

std::vector<unsigned char> Base64::decode(const std::string& input)
{
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* bmem = BIO_new_mem_buf(input.data(), static_cast<int>(input.size()));
    bmem = BIO_push(b64, bmem);

    std::vector<unsigned char> output(input.size());
    int decoded = BIO_read(bmem, output.data(), static_cast<int>(output.size()));
    BIO_free_all(bmem);

    if (decoded < 0)
        throw std::runtime_error("Base64::decode failed");

    output.resize(static_cast<size_t>(decoded));
    return output;
}

std::string Base64::encode(const unsigned char* data, size_t len)
{
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);

    BIO_write(b64, data, static_cast<int>(len));
    BIO_flush(b64);

    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);

    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);
    return result;
}

std::vector<unsigned char> Base64::urlDecode(const std::string& input)
{
    std::string standard = input;
    std::replace(standard.begin(), standard.end(), '-', '+');
    std::replace(standard.begin(), standard.end(), '_', '/');

    // Add padding if needed
    while (standard.size() % 4 != 0)
        standard.push_back('=');

    return decode(standard);
}
