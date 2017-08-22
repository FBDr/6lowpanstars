/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2013-2016 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#include "data.hpp"
#include "security/key-chain.hpp"
#include "security/v1/cryptopp.hpp"
#include "encoding/buffer-stream.hpp"

#include "boost-test.hpp"
#include "identity-management-fixture.hpp"

namespace ndn {
    namespace tests {

        BOOST_AUTO_TEST_SUITE(TestData)

        const uint8_t Content1[] = {0x53, 0x55, 0x43, 0x43, 0x45, 0x53, 0x53, 0x21};

        const uint8_t Data1[] = {
            0x06, 0xc5, // NDN Data
            0x07, 0x14, // Name
            0x08, 0x05,
            0x6c, 0x6f, 0x63, 0x61, 0x6c,
            0x08, 0x03,
            0x6e, 0x64, 0x6e,
            0x08, 0x06,
            0x70, 0x72, 0x65, 0x66, 0x69, 0x78,
            0x14, 0x04, // MetaInfo
            0x19, 0x02, // FreshnessPeriod
            0x27, 0x10,
            0x15, 0x08, // Content
            0x53, 0x55, 0x43, 0x43, 0x45, 0x53, 0x53, 0x21,
            0x16, 0x1b, // SignatureInfo
            0x1b, 0x01, // SignatureType
            0x01,
            0x1c, 0x16, // KeyLocator
            0x07, 0x14, // Name
            0x08, 0x04,
            0x74, 0x65, 0x73, 0x74,
            0x08, 0x03,
            0x6b, 0x65, 0x79,
            0x08, 0x07,
            0x6c, 0x6f, 0x63, 0x61, 0x74, 0x6f, 0x72,
            0x17, 0x80, // SignatureValue
            0x2f, 0xd6, 0xf1, 0x6e, 0x80, 0x6f, 0x10, 0xbe, 0xb1, 0x6f, 0x3e, 0x31, 0xec,
            0xe3, 0xb9, 0xea, 0x83, 0x30, 0x40, 0x03, 0xfc, 0xa0, 0x13, 0xd9, 0xb3, 0xc6,
            0x25, 0x16, 0x2d, 0xa6, 0x58, 0x41, 0x69, 0x62, 0x56, 0xd8, 0xb3, 0x6a, 0x38,
            0x76, 0x56, 0xea, 0x61, 0xb2, 0x32, 0x70, 0x1c, 0xb6, 0x4d, 0x10, 0x1d, 0xdc,
            0x92, 0x8e, 0x52, 0xa5, 0x8a, 0x1d, 0xd9, 0x96, 0x5e, 0xc0, 0x62, 0x0b, 0xcf,
            0x3a, 0x9d, 0x7f, 0xca, 0xbe, 0xa1, 0x41, 0x71, 0x85, 0x7a, 0x8b, 0x5d, 0xa9,
            0x64, 0xd6, 0x66, 0xb4, 0xe9, 0x8d, 0x0c, 0x28, 0x43, 0xee, 0xa6, 0x64, 0xe8,
            0x55, 0xf6, 0x1c, 0x19, 0x0b, 0xef, 0x99, 0x25, 0x1e, 0xdc, 0x78, 0xb3, 0xa7,
            0xaa, 0x0d, 0x14, 0x58, 0x30, 0xe5, 0x37, 0x6a, 0x6d, 0xdb, 0x56, 0xac, 0xa3,
            0xfc, 0x90, 0x7a, 0xb8, 0x66, 0x9c, 0x0e, 0xf6, 0xb7, 0x64, 0xd1
        };

        const unsigned char DEFAULT_PRIVATE_KEY_DER[] = {
            0x30, 0x82, 0x02, 0x74, 0x02, 0x01, 0x00, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48,
            0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x04, 0x82, 0x02, 0x5e, 0x30, 0x82,
            0x02, 0x5a, 0x02, 0x01, 0x00, 0x02, 0x81, 0x81, 0x00, 0x9e, 0x06, 0x3e, 0x47, 0x85,
            0xb2, 0x34, 0x37, 0xaa, 0x85, 0x47, 0xac, 0x03, 0x24, 0x83, 0xb5, 0x9c, 0xa8, 0x05,
            0x3a, 0x24, 0x1e, 0xeb, 0x89, 0x01, 0xbb, 0xe9, 0x9b, 0xb2, 0xc3, 0x22, 0xac, 0x68,
            0xe3, 0xf0, 0x6c, 0x02, 0xce, 0x68, 0xa6, 0xc4, 0xd0, 0xa7, 0x06, 0x90, 0x9c, 0xaa,
            0x1b, 0x08, 0x1d, 0x8b, 0x43, 0x9a, 0x33, 0x67, 0x44, 0x6d, 0x21, 0xa3, 0x1b, 0x88,
            0x9a, 0x97, 0x5e, 0x59, 0xc4, 0x15, 0x0b, 0xd9, 0x2c, 0xbd, 0x51, 0x07, 0x61, 0x82,
            0xad, 0xc1, 0xb8, 0xd7, 0xbf, 0x9b, 0xcf, 0x7d, 0x24, 0xc2, 0x63, 0xf3, 0x97, 0x17,
            0xeb, 0xfe, 0x62, 0x25, 0xba, 0x5b, 0x4d, 0x8a, 0xc2, 0x7a, 0xbd, 0x43, 0x8a, 0x8f,
            0xb8, 0xf2, 0xf1, 0xc5, 0x6a, 0x30, 0xd3, 0x50, 0x8c, 0xc8, 0x9a, 0xdf, 0xef, 0xed,
            0x35, 0xe7, 0x7a, 0x62, 0xea, 0x76, 0x7c, 0xbb, 0x08, 0x26, 0xc7, 0x02, 0x01, 0x11,
            0x02, 0x81, 0x80, 0x04, 0xa5, 0xd4, 0xa7, 0xc0, 0x2a, 0xe3, 0x6b, 0x0c, 0x8b, 0x73,
            0x0c, 0x96, 0xae, 0x40, 0x1b, 0xee, 0x04, 0xf1, 0x18, 0x4c, 0x5b, 0x43, 0x29, 0xad,
            0x3a, 0x3b, 0x93, 0xa3, 0x60, 0x17, 0x9b, 0xa8, 0xbb, 0x68, 0xf4, 0x1e, 0x33, 0x3f,
            0x50, 0x32, 0xf7, 0x13, 0xf8, 0xa9, 0xe6, 0x7d, 0x79, 0x44, 0x00, 0xde, 0x72, 0xed,
            0xf2, 0x73, 0xfa, 0x7b, 0xae, 0x2a, 0x71, 0xc0, 0x40, 0xc8, 0x37, 0x6f, 0x38, 0xb2,
            0x69, 0x1f, 0xa8, 0x83, 0x7b, 0x42, 0x00, 0x73, 0x46, 0xe6, 0x4c, 0x91, 0x7f, 0x13,
            0x06, 0x69, 0x06, 0xd8, 0x3f, 0x22, 0x15, 0x75, 0xf6, 0xde, 0xcd, 0xb0, 0xbc, 0x66,
            0x61, 0x91, 0x08, 0x9b, 0x2b, 0xb2, 0x00, 0xa9, 0x67, 0x05, 0x39, 0x40, 0xb9, 0x37,
            0x85, 0x88, 0x4f, 0x76, 0x79, 0x63, 0xc0, 0x88, 0x3c, 0x86, 0xa8, 0x12, 0x94, 0x5f,
            0xe4, 0x36, 0x3d, 0xea, 0xb9, 0x02, 0x41, 0x00, 0xb6, 0x2e, 0xbb, 0xcd, 0x2f, 0x3a,
            0x99, 0xe0, 0xa1, 0xa5, 0x44, 0x77, 0xea, 0x0b, 0xbe, 0x16, 0x95, 0x0e, 0x64, 0xa7,
            0x68, 0xd7, 0x4b, 0x15, 0x15, 0x23, 0xe2, 0x1e, 0x4e, 0x00, 0x2c, 0x22, 0x97, 0xae,
            0xb0, 0x74, 0xa6, 0x99, 0xd0, 0x5d, 0xb7, 0x1b, 0x10, 0x34, 0x13, 0xd2, 0x5f, 0x6e,
            0x56, 0xad, 0x85, 0x4a, 0xdb, 0xf0, 0x78, 0xbd, 0xf4, 0x8c, 0xb7, 0x9a, 0x3e, 0x99,
            0xef, 0xb9, 0x02, 0x41, 0x00, 0xde, 0x0d, 0xa7, 0x48, 0x75, 0x90, 0xad, 0x11, 0xa1,
            0xac, 0xee, 0xcb, 0x41, 0x81, 0xc6, 0xc8, 0x7f, 0xe7, 0x25, 0x94, 0xa1, 0x2a, 0x21,
            0xa8, 0x57, 0xfe, 0x84, 0xf2, 0x5e, 0xb4, 0x96, 0x35, 0xaf, 0xef, 0x2e, 0x7a, 0xf8,
            0xda, 0x3f, 0xac, 0x8a, 0x3c, 0x1c, 0x9c, 0xbd, 0x44, 0xd6, 0x90, 0xb5, 0xce, 0x1b,
            0x12, 0xf9, 0x3b, 0x8c, 0x69, 0xf6, 0xa9, 0x02, 0x93, 0x48, 0x35, 0x0a, 0x7f, 0x02,
            0x40, 0x6b, 0x2a, 0x8c, 0x96, 0xd0, 0x7c, 0xd2, 0xfc, 0x9b, 0x52, 0x28, 0x46, 0x89,
            0xac, 0x8d, 0xef, 0x2a, 0x80, 0xef, 0xea, 0x01, 0x6f, 0x95, 0x93, 0xee, 0x51, 0x57,
            0xd5, 0x97, 0x4b, 0x65, 0x41, 0x86, 0x66, 0xc2, 0x26, 0x80, 0x1e, 0x3e, 0x55, 0x3e,
            0x88, 0x63, 0xe2, 0x66, 0x03, 0x47, 0x31, 0xd8, 0xa2, 0x4e, 0x68, 0x45, 0x24, 0x0a,
            0xca, 0x17, 0x61, 0xd5, 0x69, 0xca, 0x78, 0xab, 0x21, 0x02, 0x41, 0x00, 0x8f, 0xae,
            0x7b, 0x4d, 0x00, 0xc7, 0x06, 0x92, 0xf0, 0x24, 0x9a, 0x83, 0x84, 0xbd, 0x62, 0x81,
            0xbc, 0x2c, 0x27, 0x60, 0x2c, 0x0c, 0x33, 0xe5, 0x66, 0x1d, 0x28, 0xd9, 0x10, 0x1a,
            0x7f, 0x4f, 0xea, 0x4f, 0x78, 0x6d, 0xb0, 0x14, 0xbf, 0xc9, 0xff, 0x17, 0xd6, 0x47,
            0x4d, 0x4a, 0xa8, 0xf4, 0x39, 0x67, 0x3e, 0xb1, 0xec, 0x8f, 0xf1, 0x71, 0xbd, 0xb8,
            0xa7, 0x50, 0x3d, 0xc7, 0xf7, 0xbb, 0x02, 0x40, 0x0d, 0x85, 0x32, 0x73, 0x9f, 0x0a,
            0x33, 0x2f, 0x4b, 0xa2, 0xbd, 0xd1, 0xb1, 0x42, 0xf0, 0x72, 0xa8, 0x7a, 0xc8, 0x15,
            0x37, 0x1b, 0xde, 0x76, 0x70, 0xce, 0xfd, 0x69, 0x20, 0x00, 0x4d, 0xc9, 0x4f, 0x35,
            0x6f, 0xd1, 0x35, 0xa1, 0x04, 0x95, 0x30, 0xe8, 0x3b, 0xd5, 0x03, 0x5a, 0x50, 0x21,
            0x6d, 0xa0, 0x84, 0x39, 0xe9, 0x2e, 0x1e, 0xfc, 0xe4, 0x82, 0x43, 0x20, 0x46, 0x7d,
            0x0a, 0xb6
        };

        BOOST_AUTO_TEST_CASE(DataEqualityChecks) {
            using namespace time;

            Data a;
            Data b;
            BOOST_CHECK_EQUAL(a == b, true);
            BOOST_CHECK_EQUAL(a != b, false);

            a.setName("ndn:/A");
            BOOST_CHECK_EQUAL(a == b, false);
            BOOST_CHECK_EQUAL(a != b, true);

            b.setName("ndn:/B");
            BOOST_CHECK_EQUAL(a == b, false);
            BOOST_CHECK_EQUAL(a != b, true);

            b.setName("ndn:/A");
            BOOST_CHECK_EQUAL(a == b, true);
            BOOST_CHECK_EQUAL(a != b, false);

            a.setFreshnessPeriod(seconds(10));
            BOOST_CHECK_EQUAL(a == b, false);
            BOOST_CHECK_EQUAL(a != b, true);

            b.setFreshnessPeriod(seconds(10));
            BOOST_CHECK_EQUAL(a == b, true);
            BOOST_CHECK_EQUAL(a != b, false);

            static const uint8_t someData[] = "someData";
            a.setContent(someData, sizeof (someData));
            BOOST_CHECK_EQUAL(a == b, false);
            BOOST_CHECK_EQUAL(a != b, true);

            b.setContent(someData, sizeof (someData));
            BOOST_CHECK_EQUAL(a == b, true);
            BOOST_CHECK_EQUAL(a != b, false);

            a.setSignature(SignatureSha256WithRsa());
            BOOST_CHECK_EQUAL(a == b, false);
            BOOST_CHECK_EQUAL(a != b, true);

            b.setSignature(SignatureSha256WithRsa());
            BOOST_CHECK_EQUAL(a == b, true);
            BOOST_CHECK_EQUAL(a != b, false);
        }

        BOOST_AUTO_TEST_CASE(SignatureEqualityChecks) {
            Signature a;
            Signature b;

            BOOST_CHECK_EQUAL(a == b, true);
            BOOST_CHECK_EQUAL(a != b, false);

            a = SignatureSha256WithRsa();
            BOOST_CHECK_EQUAL(a == b, false);
            BOOST_CHECK_EQUAL(a != b, true);

            b = SignatureSha256WithRsa();
            static const uint8_t someData[256] = {};
            Block signatureValue = makeBinaryBlock(tlv::SignatureValue, someData, sizeof (someData));
            b.setValue(signatureValue);
            BOOST_CHECK_EQUAL(a == b, false);
            BOOST_CHECK_EQUAL(a != b, true);

            a.setValue(signatureValue);
            BOOST_CHECK_EQUAL(a == b, true);
            BOOST_CHECK_EQUAL(a != b, false);

            a = DigestSha256();
            b = SignatureSha256WithRsa();
            BOOST_CHECK_EQUAL(a == b, false);
            BOOST_CHECK_EQUAL(a != b, true);

            b = DigestSha256();
            BOOST_CHECK_EQUAL(a == b, true);
            BOOST_CHECK_EQUAL(a != b, false);
        }

        class TestDataFixture {
        public:

            TestDataFixture() {
                CryptoPP::StringSource source(DEFAULT_PRIVATE_KEY_DER, sizeof (DEFAULT_PRIVATE_KEY_DER), true);
                privateKey_.Load(source);
                publicKey_ = privateKey_;
            }

        protected:
            CryptoPP::AutoSeededRandomPool rng_;
            CryptoPP::RSA::PrivateKey privateKey_;
            CryptoPP::RSA::PublicKey publicKey_;
        };

        BOOST_FIXTURE_TEST_CASE(Decode, TestDataFixture) {
            Block dataBlock(Data1, sizeof (Data1));

            ndn::Data d;
            BOOST_REQUIRE_NO_THROW(d.wireDecode(dataBlock));

            BOOST_REQUIRE_EQUAL(d.getName().toUri(), "/local/ndn/prefix");
            BOOST_REQUIRE_EQUAL(d.getContentType(), static_cast<uint32_t> (tlv::ContentType_Blob));
            BOOST_REQUIRE_EQUAL(d.getFreshnessPeriod(), time::seconds(10));

            BOOST_REQUIRE_EQUAL(std::string(reinterpret_cast<const char*> (d.getContent().value()),
                    d.getContent().value_size()), "SUCCESS!");

            BOOST_REQUIRE_EQUAL(d.getSignature().getType(), static_cast<uint32_t> (Signature::Sha256WithRsa));
            ndn::Block block = d.getSignature().getInfo();
            block.parse();
            KeyLocator keyLocator;
            BOOST_REQUIRE_NO_THROW(keyLocator.wireDecode(block.get(tlv::KeyLocator)));

            BOOST_REQUIRE_EQUAL(keyLocator.getName().toUri(), "/test/key/locator");

            using namespace CryptoPP;
            RSASS<PKCS1v15, SHA256>::Verifier verifier(publicKey_);
            bool signatureVerified = verifier.VerifyMessage(d.wireEncode().value(),
                    d.wireEncode().value_size() -
                    d.getSignature().getValue().size(),
                    d.getSignature().getValue().value(),
                    d.getSignature().getValue().value_size());
            BOOST_REQUIRE_EQUAL(signatureVerified, true);
        }

        BOOST_FIXTURE_TEST_CASE(Encode, TestDataFixture) {
            // manual data packet creation for now

            ndn::Data d(ndn::Name("/local/ndn/prefix"));
            d.setContentType(tlv::ContentType_Blob);
            d.setFreshnessPeriod(time::seconds(10));

            d.setContent(Content1, sizeof (Content1));

            Block signatureInfo(tlv::SignatureInfo);
            // SignatureType
            {
                signatureInfo.push_back(makeNonNegativeIntegerBlock(tlv::SignatureType, Signature::Sha256WithRsa));
            }
            // KeyLocator
            {
                KeyLocator keyLocator;
                keyLocator.setName("/test/key/locator");

                signatureInfo.push_back(keyLocator.wireEncode());
            }
            signatureInfo.encode();

            // SignatureValue
            OBufferStream os;
            tlv::writeVarNumber(os, tlv::SignatureValue);

            using namespace CryptoPP;

            RSASS<PKCS1v15, SHA256>::Signer signer(privateKey_);

            PK_MessageAccumulator *hash = signer.NewSignatureAccumulator(rng_);
            hash->Update(d.getName(). wireEncode().wire(), d.getName(). wireEncode().size());
            hash->Update(d.getMetaInfo().wireEncode().wire(), d.getMetaInfo().wireEncode().size());
            hash->Update(d.getContent(). wire(), d.getContent(). size());
            hash->Update(signatureInfo. wire(), signatureInfo. size());

            size_t length = signer.MaxSignatureLength();
            SecByteBlock buf(length);
            signer.Sign(rng_, hash, buf);

            tlv::writeVarNumber(os, buf.size());
            os.write(reinterpret_cast<const char *> (buf.BytePtr()), buf.size());

            ndn::Block signatureValue(Block(os.buf()));

            Signature signature(signatureInfo, signatureValue);

            d.setSignature(signature);

            Block dataBlock;
            BOOST_REQUIRE_NO_THROW(dataBlock = d.wireEncode());

            BOOST_REQUIRE_EQUAL_COLLECTIONS(Data1, Data1 + sizeof (Data1),
                    dataBlock.begin(), dataBlock.end());

            std::ostringstream strStream;
            BOOST_CHECK_NO_THROW(strStream << d);

            BOOST_CHECK_EQUAL(strStream.str(),
                    "Name: /local/ndn/prefix\n"
                    "MetaInfo: ContentType: 0, FreshnessPeriod: 10000 milliseconds\n"
                    "Content: (size: 8)\n"
                    "Signature: (type: 1, value_length: 128)\n");
        }

        BOOST_FIXTURE_TEST_CASE(FullName, IdentityManagementFixture) {
            // Encoding pipeline

            ndn::Data d(ndn::Name("/local/ndn/prefix"));
            d.setContentType(tlv::ContentType_Blob);
            d.setFreshnessPeriod(time::seconds(10));

            d.setContent(Content1, sizeof (Content1));

            BOOST_CHECK_THROW(d.getFullName(), Data::Error);

            m_keyChain.sign(d);

            Name fullName;
            BOOST_REQUIRE_NO_THROW(fullName = d.getFullName());

            BOOST_CHECK_EQUAL(d.getName().hasWire(), true);
            BOOST_CHECK_EQUAL(fullName.hasWire(), false);

            // check if name was properly cached
            BOOST_CHECK_EQUAL(fullName.get(-1).value(), d.getFullName().get(-1).value());

            // check FullName content
            BOOST_REQUIRE_EQUAL(d.getName().size() + 1, fullName.size());
            BOOST_CHECK_EQUAL_COLLECTIONS(d.getName().begin(), d.getName().end(),
                    fullName.begin(), fullName.end() - 1);
            BOOST_CHECK_EQUAL(fullName.get(-1).value_size(), 32);

            // FullName should be reset after the next line
            d.setFreshnessPeriod(time::seconds(100));
            BOOST_CHECK_THROW(d.getFullName(), Data::Error);

            // Decoding pipeline
            d.wireDecode(Block(Data1, sizeof (Data1)));
            BOOST_REQUIRE_NO_THROW(fullName = d.getFullName());

            BOOST_CHECK_EQUAL(fullName.toUri(),
                    "/local/ndn/prefix/"
                    "sha256digest=28bad4b5275bd392dbb670c75cf0b66f13f7942b21e80f55c0e86b374753a548");
        }

        BOOST_AUTO_TEST_SUITE_END()

    } // namespace tests
} // namespace ndn
