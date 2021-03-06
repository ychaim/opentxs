/************************************************************
 *
 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  EMAIL:
 *  fellowtraveler@opentransactions.org
 *
 *  WEBSITE:
 *  http://www.opentransactions.org/
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This Source Code Form is subject to the terms of the
 *   Mozilla Public License, v. 2.0. If a copy of the MPL
 *   was not distributed with this file, You can obtain one
 *   at http://mozilla.org/MPL/2.0/.
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will
 *   be useful, but WITHOUT ANY WARRANTY; without even the
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A
 *   PARTICULAR PURPOSE.  See the Mozilla Public License
 *   for more details.
 *
 ************************************************************/

#ifndef OPENTXS_CORE_CRYPTO_OTASCIIARMOR_HPP
#define OPENTXS_CORE_CRYPTO_OTASCIIARMOR_HPP

#include <opentxs/core/String.hpp>

#include <memory>

namespace opentxs
{

class OTASCIIArmor;
class OTData;
class OTEnvelope;
class OTData;

namespace OTDB
{
class OTPacker;
}

typedef std::map<int64_t, OTASCIIArmor*> mapOfArmor;

extern const char* OT_BEGIN_ARMORED;
extern const char* OT_END_ARMORED;

extern const char* OT_BEGIN_ARMORED_escaped;
extern const char* OT_END_ARMORED_escaped;

extern const char* OT_BEGIN_SIGNED;
extern const char* OT_BEGIN_SIGNED_escaped;

// The natural state of OTASCIIArmor is in compressed and base64-encoded, string
// form.
// It is derived from OTString. The Get() method returns a base64-encoded
// string.
// The Set() method assumes that you are PASSING IN a base64-encoded string.
// The constructors assume that you are passing in a base64-encoded string.
class OTASCIIArmor : public String
{
public:
    static OTDB::OTPacker* GetPacker();

    EXPORT OTASCIIArmor();
    EXPORT OTASCIIArmor(const char* szValue);
    EXPORT OTASCIIArmor(const OTData& theValue);
    EXPORT OTASCIIArmor(const String& strValue);
    EXPORT OTASCIIArmor(const OTASCIIArmor& strValue);
    EXPORT OTASCIIArmor(const OTEnvelope& theEnvelope);
    EXPORT virtual ~OTASCIIArmor();

    using String::swap;

    EXPORT OTASCIIArmor& operator=(const char* szValue);
    EXPORT OTASCIIArmor& operator=(const OTData& theValue);
    EXPORT OTASCIIArmor& operator=(const String& strValue);
    EXPORT OTASCIIArmor& operator=(const OTASCIIArmor& strValue);

    EXPORT bool LoadFromFile(const String& foldername, const String& filename);
    EXPORT bool LoadFrom_ifstream(std::ifstream& fin);
    EXPORT bool SaveTo_ofstream(std::ofstream& fout);
    EXPORT bool LoadFromExactPath(const std::string& filename);
    EXPORT bool SaveToExactPath(const std::string& filename);
    // Let's say you don't know if the input string is raw base64, or if it has
    // bookends
    // on it like -----BEGIN BLAH BLAH ...
    // And if it DOES have Bookends, you don't know if they are escaped:  -
    // -----BEGIN ...
    // Let's say you just want an easy function that will figure that crap out,
    // and load the
    // contents up properly into an OTASCIIArmor object. (That's what this
    // function will do.)
    //
    EXPORT static bool LoadFromString(
        OTASCIIArmor& ascArmor, const String& strInput,
        std::string str_bookend = "-----BEGIN"); // todo hardcoding.
                                                 // str_bookend is a
                                                 // default. So you
                                                 // could make it more
                                                 // specific like,
                                                 // -----BEGIN ENCRYPTED
                                                 // KEY (or whatever.)

    EXPORT bool LoadFromString(String& theStr, bool bEscaped = false,
                               const // This sub-string determines where the
                                     // content starts, when loading.
                               std::string str_override =
                                   "-----BEGIN"); // "-----BEGIN" is the default
                                                  // "content start" substr.
                                                  // Todo: hardcoding.

    EXPORT bool WriteArmoredString(
        String& strOutput, const // for "-----BEGIN OT LEDGER-----", str_type
                                 // would contain ==> "LEDGER" <==
        std::string str_type, // There's no default, to force you to enter the
                              // right string.
        bool bEscaped = false) const;

    EXPORT bool WriteArmoredFile(const String& foldername,
                                 const String& filename,
                                 const // for "-----BEGIN OT LEDGER-----",
                                       // str_type would contain ==> "LEDGER"
                                       // <==
                                 std::string str_type, // There's no default, to
                                                       // force you to enter the
                                                       // right string.
                                 bool bEscaped = false) const;

    // Base64-decode
    EXPORT bool GetData(OTData& theData, bool bLineBreaks = true) const;
    // Base64-encode
    EXPORT bool SetData(const OTData& theData, bool bLineBreaks = true);

    // Base64-decode and decompress
    EXPORT bool GetString(String& theData, bool bLineBreaks = true) const;
    // compress and Base64-encode
    EXPORT bool SetString(const String& theData, bool bLineBreaks = true);

private:
    std::string compress_string(const std::string& str,
                                int32_t compressionlevel) const;
    std::string decompress_string(const std::string& str) const;

    static std::unique_ptr<OTDB::OTPacker> s_pPacker;
};

} // namespace opentxs

#endif // OPENTXS_CORE_CRYPTO_OTASCIIARMOR_HPP
