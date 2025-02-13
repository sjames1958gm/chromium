// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/http2/hpack/varint/hpack_varint_encoder.h"

#include "net/third_party/http2/platform/api/http2_string_utils.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace http2 {
namespace test {
namespace {

// Freshly constructed encoder is not in the process of encoding.
TEST(HpackVarintEncoderTest, Done) {
  HpackVarintEncoder varint_encoder;
  EXPECT_FALSE(varint_encoder.IsEncodingInProgress());
}

struct {
  uint8_t high_bits;
  uint8_t prefix_length;
  uint64_t value;
  uint8_t expected_encoding;
} kShortTestData[] = {{0b10110010, 1, 0, 0b10110010},
                      {0b10101100, 2, 2, 0b10101110},
                      {0b10100000, 3, 6, 0b10100110},
                      {0b10110000, 4, 13, 0b10111101},
                      {0b10100000, 5, 8, 0b10101000},
                      {0b11000000, 6, 48, 0b11110000},
                      {0b10000000, 7, 99, 0b11100011},
                      // Example from RFC7541 C.1.
                      {0b00000000, 5, 10, 0b00001010}};

// Encode integers that fit in the prefix.
TEST(HpackVarintEncoderTest, Short) {
  HpackVarintEncoder varint_encoder;

  for (size_t i = 0; i < arraysize(kShortTestData); ++i) {
    EXPECT_EQ(kShortTestData[i].expected_encoding,
              varint_encoder.StartEncoding(kShortTestData[i].high_bits,
                                           kShortTestData[i].prefix_length,
                                           kShortTestData[i].value));
    EXPECT_FALSE(varint_encoder.IsEncodingInProgress());
  }
}

struct {
  uint8_t high_bits;
  uint8_t prefix_length;
  uint64_t value;
  const char* expected_encoding;
} kLongTestData[] = {
    // One extension byte.
    {0b10011000, 3, 103, "9f60"},
    {0b10010000, 4, 57, "9f2a"},
    {0b11000000, 5, 158, "df7f"},
    {0b01000000, 6, 65, "7f02"},
    {0b00000000, 7, 200, "7f49"},
    // Two extension bytes.
    {0b10011000, 3, 12345, "9fb260"},
    {0b10010000, 4, 5401, "9f8a2a"},
    {0b11000000, 5, 16327, "dfa87f"},
    {0b01000000, 6, 399, "7fd002"},
    {0b00000000, 7, 9598, "7fff49"},
    // Three extension bytes.
    {0b10011000, 3, 1579281, "9f8ab260"},
    {0b10010000, 4, 689488, "9fc18a2a"},
    {0b11000000, 5, 2085964, "dfada87f"},
    {0b01000000, 6, 43103, "7fa0d002"},
    {0b00000000, 7, 1212541, "7ffeff49"},
    // Four extension bytes.
    {0b10011000, 3, 202147110, "9f9f8ab260"},
    {0b10010000, 4, 88252593, "9fa2c18a2a"},
    {0b11000000, 5, 266999535, "dfd0ada87f"},
    {0b01000000, 6, 5509304, "7ff9a0d002"},
    {0b00000000, 7, 155189149, "7f9efeff49"},
    // Six extension bytes.
    {0b10011000, 3, 3311978140938, "9f83aa9f8ab260"},
    {0b10010000, 4, 1445930244223, "9ff0b0a2c18a2a"},
    {0b11000000, 5, 4374519874169, "dfda84d0ada87f"},
    {0b01000000, 6, 90263420404, "7fb5fbf9a0d002"},
    {0b00000000, 7, 2542616951118, "7fcff19efeff49"},
    // Eight extension bytes.
    {0b10011000, 3, 54263449861016696, "9ff19883aa9f8ab260"},
    {0b10010000, 4, 23690121121119891, "9f84fdf0b0a2c18a2a"},
    {0b11000000, 5, 71672133617889215, "dfa0dfda84d0ada87f"},
    {0b01000000, 6, 1478875878881374, "7f9ff0b5fbf9a0d002"},
    {0b00000000, 7, 41658236125045114, "7ffbc1cff19efeff49"},
    // Ten extension bytes.
    {0b10011000, 3, 12832019021693745307u, "9f94f1f19883aa9f8ab201"},
    {0b10010000, 4, 9980690937382242223u, "9fa08f84fdf0b0a2c18a01"},
    {0b11000000, 5, 12131360551794650846u, "dfbfdda0dfda84d0ada801"},
    {0b01000000, 6, 15006530362736632796u, "7f9dc79ff0b5fbf9a0d001"},
    {0b00000000, 7, 18445754019193211014u, "7f8790fbc1cff19efeff01"},
    // Maximum value: 2^64-1.
    {0b10011000, 3, 18446744073709551615u, "9ff8ffffffffffffffff01"},
    {0b10010000, 4, 18446744073709551615u, "9ff0ffffffffffffffff01"},
    {0b11000000, 5, 18446744073709551615u, "dfe0ffffffffffffffff01"},
    {0b01000000, 6, 18446744073709551615u, "7fc0ffffffffffffffff01"},
    {0b00000000, 7, 18446744073709551615u, "7f80ffffffffffffffff01"},
    // Example from RFC7541 C.1.
    {0b00000000, 5, 1337, "1f9a0a"},
};

// Encode integers that do not fit in the prefix.
TEST(HpackVarintEncoderTest, Long) {
  HpackVarintEncoder varint_encoder;

  // Test encoding byte by byte, also test encoding in
  // a single ResumeEncoding() call.
  for (bool byte_by_byte : {true, false}) {
    for (size_t i = 0; i < arraysize(kLongTestData); ++i) {
      Http2String expected_encoding =
          Http2HexDecode(kLongTestData[i].expected_encoding);
      ASSERT_FALSE(expected_encoding.empty());

      EXPECT_EQ(static_cast<unsigned char>(expected_encoding[0]),
                varint_encoder.StartEncoding(kLongTestData[i].high_bits,
                                             kLongTestData[i].prefix_length,
                                             kLongTestData[i].value));
      EXPECT_TRUE(varint_encoder.IsEncodingInProgress());

      Http2String output;
      if (byte_by_byte) {
        while (varint_encoder.IsEncodingInProgress()) {
          EXPECT_EQ(1u, varint_encoder.ResumeEncoding(1, &output));
        }
      } else {
        // TODO(bnc): Factor out maximum number of extension bytes into a
        // constant in HpackVarintEncoder.
        EXPECT_EQ(expected_encoding.size() - 1,
                  varint_encoder.ResumeEncoding(10, &output));
        EXPECT_FALSE(varint_encoder.IsEncodingInProgress());
      }
      EXPECT_EQ(expected_encoding.size() - 1, output.size());
      EXPECT_EQ(expected_encoding.substr(1), output);
    }
  }
}

struct {
  uint8_t high_bits;
  uint8_t prefix_length;
  uint64_t value;
  uint8_t expected_encoding_first_byte;
} kLastByteIsZeroTestData[] = {
    {0b10110010, 1, 1, 0b10110011},  {0b10101100, 2, 3, 0b10101111},
    {0b10101000, 3, 7, 0b10101111},  {0b10110000, 4, 15, 0b10111111},
    {0b10100000, 5, 31, 0b10111111}, {0b11000000, 6, 63, 0b11111111},
    {0b10000000, 7, 127, 0b11111111}};

// Make sure that the encoder outputs the last byte even when it is zero.  This
// happens exactly when encoding  the value 2^prefix_length - 1.
TEST(HpackVarintEncoderTest, LastByteIsZero) {
  HpackVarintEncoder varint_encoder;

  for (size_t i = 0; i < arraysize(kLastByteIsZeroTestData); ++i) {
    EXPECT_EQ(
        kLastByteIsZeroTestData[i].expected_encoding_first_byte,
        varint_encoder.StartEncoding(kLastByteIsZeroTestData[i].high_bits,
                                     kLastByteIsZeroTestData[i].prefix_length,
                                     kLastByteIsZeroTestData[i].value));
    EXPECT_TRUE(varint_encoder.IsEncodingInProgress());

    Http2String output;
    EXPECT_EQ(1u, varint_encoder.ResumeEncoding(1, &output));
    ASSERT_EQ(1u, output.size());
    EXPECT_EQ(0b00000000, output[0]);
    EXPECT_FALSE(varint_encoder.IsEncodingInProgress());
  }
}

}  // namespace
}  // namespace test
}  // namespace http2
