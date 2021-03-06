// Copyright (C) 2011-2013 Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <config.h>

#include <dhcp/dhcp6.h>
#include <dhcp/option.h>
#include <dhcp/option6_iaprefix.h>
#include <util/buffer.h>

#include <boost/scoped_ptr.hpp>
#include <gtest/gtest.h>

#include <iostream>
#include <sstream>

#include <arpa/inet.h>

using namespace std;
using namespace isc;
using namespace isc::dhcp;
using namespace isc::util;
using namespace isc::asiolink;

namespace {
class Option6IAPrefixTest : public ::testing::Test {
public:
    Option6IAPrefixTest() : buf_(255), outBuf_(255) {
        for (int i = 0; i < 255; i++) {
            buf_[i] = 255 - i;
        }
    }

    /// @brief creates on-wire representation of IAPREFIX option
    ///
    /// buf_ field is set up to have IAPREFIX with preferred=1000,
    /// valid=3000000000 and prefix beign 2001:db8:1::dead:beef/77
    void setExampleBuffer() {
        for (int i = 0; i < 255; i++) {
            buf_[i] = 0;
        }

        buf_[ 0] = 0x00;
        buf_[ 1] = 0x00;
        buf_[ 2] = 0x03;
        buf_[ 3] = 0xe8; // preferred lifetime = 1000

        buf_[ 4]  = 0xb2;
        buf_[ 5] = 0xd0;
        buf_[ 6] = 0x5e;
        buf_[ 7] = 0x00; // valid lifetime = 3,000,000,000

        buf_[ 8] = 77; // Prefix length = 77

        buf_[ 9] = 0x20;
        buf_[10] = 0x01;
        buf_[11] = 0x0d;
        buf_[12] = 0xb8;
        buf_[13] = 0x00;
        buf_[14] = 0x01;
        buf_[21] = 0xde;
        buf_[22] = 0xad;
        buf_[23] = 0xbe;
        buf_[24] = 0xef; // 2001:db8:1::dead:beef
    }


    /// @brief Checks whether specified IAPREFIX option meets expected values
    ///
    /// To be used with option generated by setExampleBuffer
    ///
    /// @param opt IAPREFIX option being tested
    /// @param expected_type expected option type
    void checkOption(Option6IAPrefix& opt, uint16_t expected_type) {

        // Check if all fields have expected values
        EXPECT_EQ(Option::V6, opt.getUniverse());
        EXPECT_EQ(expected_type, opt.getType());
        EXPECT_EQ("2001:db8:1::dead:beef", opt.getAddress().toText());
        EXPECT_EQ(1000, opt.getPreferred());
        EXPECT_EQ(3000000000U, opt.getValid());
        EXPECT_EQ(77, opt.getLength());

        // 4 bytes header + 25 bytes content
        EXPECT_EQ(Option::OPTION6_HDR_LEN + Option6IAPrefix::OPTION6_IAPREFIX_LEN,
                  opt.len());
    }

    /// @brief Checks whether content of output buffer is correct
    ///
    /// Output buffer is expected to be filled with an option matchin
    /// buf_ content as defined in setExampleBuffer().
    ///
    /// @param expected_type expected option type
    void checkOutputBuffer(uint16_t expected_type) {
        // Check if pack worked properly:
        const uint8_t* out = (const uint8_t*)outBuf_.getData();

        // - if option type is correct
        EXPECT_EQ(expected_type, out[0]*256 + out[1]);

        // - if option length is correct
        EXPECT_EQ(25, out[2]*256 + out[3]);

        // - if option content is correct
        EXPECT_EQ(0, memcmp(out + 4, &buf_[0], 25));
    }

    OptionBuffer buf_;
    OutputBuffer outBuf_;
};

// Tests if receiving option can be parsed correctly
TEST_F(Option6IAPrefixTest, basic) {

    setExampleBuffer();

    // Create an option (unpack content)
    boost::scoped_ptr<Option6IAPrefix> opt;
    ASSERT_NO_THROW(opt.reset(new Option6IAPrefix(D6O_IAPREFIX, buf_.begin(),
                                                  buf_.begin() + 25)));
    ASSERT_TRUE(opt);

    // Pack this option
    opt->pack(outBuf_);
    EXPECT_EQ(29, outBuf_.getLength());

    checkOption(*opt, D6O_IAPREFIX);

    checkOutputBuffer(D6O_IAPREFIX);

    // Check that option can be disposed safely
    EXPECT_NO_THROW(opt.reset());
}

// Checks whether a new option can be built correctly
TEST_F(Option6IAPrefixTest, build) {

    boost::scoped_ptr<Option6IAPrefix> opt;
    setExampleBuffer();

    ASSERT_NO_THROW(opt.reset(new Option6IAPrefix(12345,
                    IOAddress("2001:db8:1::dead:beef"), 77, 1000, 3000000000u)));
    ASSERT_TRUE(opt);

    checkOption(*opt, 12345);

    // Check if we can build it properly
    EXPECT_NO_THROW(opt->pack(outBuf_));
    EXPECT_EQ(29, outBuf_.getLength());
    checkOutputBuffer(12345);

    // Check that option can be disposed safely
    EXPECT_NO_THROW(opt.reset());
}

// Checks negative cases
TEST_F(Option6IAPrefixTest, negative) {

    // Truncated option (at least 25 bytes is needed)
    EXPECT_THROW(Option6IAPrefix(D6O_IAPREFIX, buf_.begin(), buf_.begin() + 24),
                 OutOfRange);

    // Empty option
    EXPECT_THROW(Option6IAPrefix(D6O_IAPREFIX, buf_.begin(), buf_.begin()),
                 OutOfRange);

    // This is for IPv6 prefixes only
    EXPECT_THROW(Option6IAPrefix(12345, IOAddress("192.0.2.1"), 77, 1000, 2000),
                 BadValue);

    // Prefix length can't be larger than 128
    EXPECT_THROW(Option6IAPrefix(12345, IOAddress("192.0.2.1"), 255, 1000, 2000),
                 BadValue);
}

}
