// THIS IS AN AUTOMATICALLY GENERATED FILE.
// DO NOT MODIFY BY HAND!!
//
// Generated by zcm-gen

#include <zcm/zcm_coretypes.h>

#ifndef __all_timed_value_hpp__
#define __all_timed_value_hpp__

#include <vector>
#include "timed_value.hpp"


class all_timed_value
{
    public:
        int32_t    cnt;

        std::vector< timed_value > channels;

        int8_t     channel_updated;

    public:
        /**
         * Destructs a message properly if anything inherits from it
        */
        virtual ~all_timed_value() {}

        /**
         * Encode a message into binary form.
         *
         * @param buf The output buffer.
         * @param offset Encoding starts at thie byte offset into @p buf.
         * @param maxlen Maximum number of bytes to write.  This should generally be
         *  equal to getEncodedSize().
         * @return The number of bytes encoded, or <0 on error.
         */
        inline int encode(void* buf, uint32_t offset, uint32_t maxlen) const;

        /**
         * Check how many bytes are required to encode this message.
         */
        inline uint32_t getEncodedSize() const;

        /**
         * Decode a message from binary form into this instance.
         *
         * @param buf The buffer containing the encoded message.
         * @param offset The byte offset into @p buf where the encoded message starts.
         * @param maxlen The maximum number of bytes to read while decoding.
         * @return The number of bytes decoded, or <0 if an error occured.
         */
        inline int decode(const void* buf, uint32_t offset, uint32_t maxlen);

        /**
         * Retrieve the 64-bit fingerprint identifying the structure of the message.
         * Note that the fingerprint is the same for all instances of the same
         * message type, and is a fingerprint on the message type definition, not on
         * the message contents.
         */
        inline static int64_t getHash();

        /**
         * Returns "all_timed_value"
         */
        inline static const char* getTypeName();

        // ZCM support functions. Users should not call these
        inline int      _encodeNoHash(void* buf, uint32_t offset, uint32_t maxlen) const;
        inline uint32_t _getEncodedSizeNoHash() const;
        inline int      _decodeNoHash(const void* buf, uint32_t offset, uint32_t maxlen);
        inline static uint64_t _computeHash(const __zcm_hash_ptr* p);
};

int all_timed_value::encode(void* buf, uint32_t offset, uint32_t maxlen) const
{
    uint32_t pos = 0;
    int thislen;
    int64_t hash = (int64_t)getHash();

    thislen = __int64_t_encode_array(buf, offset + pos, maxlen - pos, &hash, 1);
    if(thislen < 0) return thislen; else pos += thislen;

    thislen = this->_encodeNoHash(buf, offset + pos, maxlen - pos);
    if (thislen < 0) return thislen; else pos += thislen;

    return pos;
}

int all_timed_value::decode(const void* buf, uint32_t offset, uint32_t maxlen)
{
    uint32_t pos = 0;
    int thislen;

    int64_t msg_hash;
    thislen = __int64_t_decode_array(buf, offset + pos, maxlen - pos, &msg_hash, 1);
    if (thislen < 0) return thislen; else pos += thislen;
    if (msg_hash != getHash()) return -1;

    thislen = this->_decodeNoHash(buf, offset + pos, maxlen - pos);
    if (thislen < 0) return thislen; else pos += thislen;

    return pos;
}

uint32_t all_timed_value::getEncodedSize() const
{
    return 8 + _getEncodedSizeNoHash();
}

int64_t all_timed_value::getHash()
{
    static int64_t hash = (int64_t)_computeHash(NULL);
    return hash;
}

const char* all_timed_value::getTypeName()
{
    return "all_timed_value";
}

int all_timed_value::_encodeNoHash(void* buf, uint32_t offset, uint32_t maxlen) const
{
    uint32_t pos_byte = 0;
    int thislen;

    thislen = __int32_t_encode_array(buf, offset + pos_byte, maxlen - pos_byte, &this->cnt, 1);
    if(thislen < 0) return thislen; else pos_byte += thislen;

    for (int a0 = 0; a0 < this->cnt; ++a0) {
        thislen = this->channels[a0]._encodeNoHash(buf, offset + pos_byte, maxlen - pos_byte);
        if(thislen < 0) return thislen; else pos_byte += thislen;
    }

    thislen = __boolean_encode_array(buf, offset + pos_byte, maxlen - pos_byte, &this->channel_updated, 1);
    if(thislen < 0) return thislen; else pos_byte += thislen;

    return pos_byte;
}

int all_timed_value::_decodeNoHash(const void* buf, uint32_t offset, uint32_t maxlen)
{
    uint32_t pos_byte = 0;
    int thislen;

    thislen = __int32_t_decode_array(buf, offset + pos_byte, maxlen - pos_byte, &this->cnt, 1);
    if(thislen < 0) return thislen; else pos_byte += thislen;

    this->channels.resize(this->cnt);
    for (int a0 = 0; a0 < this->cnt; ++a0) {
        thislen = this->channels[a0]._decodeNoHash(buf, offset + pos_byte, maxlen - pos_byte);
        if(thislen < 0) return thislen; else pos_byte += thislen;
    }

    thislen = __boolean_decode_array(buf, offset + pos_byte, maxlen - pos_byte, &this->channel_updated, 1);
    if(thislen < 0) return thislen; else pos_byte += thislen;

    return pos_byte;
}

uint32_t all_timed_value::_getEncodedSizeNoHash() const
{
    uint32_t enc_size = 0;
    enc_size += __int32_t_encoded_array_size(NULL, 1);
    for (int a0 = 0; a0 < this->cnt; ++a0) {
        enc_size += this->channels[a0]._getEncodedSizeNoHash();
    }
    enc_size += __boolean_encoded_array_size(NULL, 1);
    return enc_size;
}

#if defined(__clang__)
__attribute__((no_sanitize("integer")))
#endif
uint64_t all_timed_value::_computeHash(const __zcm_hash_ptr* p)
{
    const __zcm_hash_ptr* fp;
    for(fp = p; fp != NULL; fp = fp->parent)
        if(fp->v == all_timed_value::getHash)
            return 0;
    const __zcm_hash_ptr cp = { p, (void*)all_timed_value::getHash };

    uint64_t hash = (uint64_t)0x8116b392bee5da9eLL +
         timed_value::_computeHash(&cp);

    return (hash<<1) + ((hash>>63)&1);
}

#endif
