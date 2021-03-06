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

#include <opentxs/core/stdafx.hpp>

#include <opentxs/client/OTAPI.hpp>
#include <opentxs/client/OpenTransactions.hpp>
#include <opentxs/client/OT_ME.hpp>
#include <opentxs/client/OTClient.hpp>
#include <opentxs/client/OTWallet.hpp>
#include <opentxs/client/ot_otapi_ot.hpp>

#include <opentxs/cash/Purse.hpp>

#include <opentxs/ext/Helpers.hpp>
#include <opentxs/core/Account.hpp>
#include <opentxs/core/AssetContract.hpp>
#include <opentxs/core/crypto/OTEnvelope.hpp>
#include <opentxs/core/Log.hpp>
#include <opentxs/core/Message.hpp>
#include <opentxs/core/util/OTPaths.hpp>
#include <opentxs/core/Nym.hpp>
#include <opentxs/core/OTServerContract.hpp>
#include <opentxs/core/script/OTVariable.hpp>

#include <anyoption/anyoption.hpp>

#include <sstream>
#include <cstring>

#ifndef WIN32
#include <iterator>
#include <memory>
#endif

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

using namespace opentxs;

#define OT_OPTIONS_FILE_DEFAULT "command-line-ot.opt"

void HandleCommandLineArguments(int32_t argc, char* argv[], AnyOption* opt);
bool SetupPointersForWalletMyNymAndServerContract(
    std::string& str_NotaryID, std::string& str_MyNym, Nym*& pMyNym,
    OTWallet*& pWallet, OTServerContract*& pServerContract);
void CollectDefaultedCLValues(AnyOption* opt, std::string& str_NotaryID,
                              std::string& str_MyAcct, std::string& str_MyNym,
                              std::string& str_MyPurse,
                              std::string& str_HisAcct, std::string& str_HisNym,
                              std::string& str_HisPurse);

/*

--server     (NOTARY_ID)

USAGE:  ot -COMMAND [AMOUNT] [--from ACCT/NYM/ASSET] [--to ACCT or NYM]

ot -w 100    (WITHDRAW 100 FROM DEFAULT ACCOUNT.)
ot -d 100    (DEPOSIT FROM DEFAULT PURSE TO DEFAULT ACCOUNT.)
ot -t 100 --to j43k  (TRANSFER 100 FROM DEFAULT ACCT TO ACCT STARTING WITH j43k)
ot -t 100 --from qwer --to j43k  (TRANSFER 100 from ACCT STARTING WITH qwer TO
ACCT starting j43k)

*/

// If false, error happened, usually based on what user just attemped.
//
bool SetupPointersForWalletMyNymAndServerContract(
    std::string& str_NotaryID, std::string& str_MyNym, Nym*& pMyNym,
    OTWallet*& pWallet, OTServerContract*& pServerContract)
{
    // If we got down here, that means there were no commands on the command
    // line
    // (That's why we dropped into the OT prompt.)
    // However, there still may have been OPTIONS -- and if so, we'll go ahead
    // and
    // load the wallet. (If there were NOT ANY OPTIONS, then we do NOT load the
    // wallet,
    // although there is a COMMAND for doing that.)
    //

    OTAPI_Wrap::OTAPI()->LoadWallet();

    //
    pWallet = OTAPI_Wrap::OTAPI()->GetWallet();

    if (nullptr == pWallet) {
        otOut
            << "The wallet object is still nullptr, somehow. Please load it.\n";
        return false;
    }

    // Below this point, pWallet is available :-)

    if (str_NotaryID.size() > 0) {
        const Identifier NOTARY_ID(str_NotaryID.c_str());

        pServerContract = pWallet->GetServerContract(NOTARY_ID);
        // If failure, then we try PARTIAL match.
        if (nullptr == pServerContract)
            pServerContract =
                pWallet->GetServerContractPartialMatch(str_NotaryID);

        if (nullptr != pServerContract) {
            String strTemp;
            pServerContract->GetIdentifier(strTemp);

            str_NotaryID = strTemp.Get();
            otOut << "Using as server: " << str_NotaryID << "\n";
        }
        else {
            otOut
                << "Unable to find a server contract. Please use the option:  "
                   "--server NOTARY_ID\n"
                   "(Where NOTARY_ID is the server ID. Partial matches are "
                   "accepted "
                   "if the contract is already in the wallet.)\n"
                   "Also, see default values located in "
                   "~/.ot/comand-line-ot.opt \n";
            //          return false;
        }
    }
    // Below this point, pServerContract MAY be available, but also may be
    // nullptr.
    //

    if (str_MyNym.size() > 0) {
        const Identifier MY_NYM_ID(str_MyNym.c_str());

        pMyNym = pWallet->GetNymByID(MY_NYM_ID);

        // If failure, then we try PARTIAL match.
        if (nullptr == pMyNym)
            pMyNym = pWallet->GetNymByIDPartialMatch(str_MyNym);

        if (nullptr != pMyNym) {
            String strTemp;
            pMyNym->GetIdentifier(strTemp);

            str_MyNym = strTemp.Get();
            otOut << "Using as mynym: " << str_MyNym << "\n";
        }
        else {
            otOut << "==> Unable to find My Nym. Please use the option:   "
                     "--mynym NYM_ID\n"
                     "(Where NYM_ID is the Nym's ID. Partial matches are "
                     "accepted "
                     "if the nym is already in the wallet.)\n"
                     "Also, see default values located in "
                     "~/.ot/comand-line-ot.opt\n";
            //          return false;
        }
    } // Below this point, pMyNym MIGHT be a valid pointer, or MIGHT be nullptr.

    // Below THIS point, there's no guarantee of pWallet, though it MIGHT be
    // there.
    // Same with pServerContract. (MIGHT be there.)

    return true;
}

void HandleCommandLineArguments(int32_t argc, char* argv[], AnyOption* opt)
{
    if (nullptr == opt) return;

    String strConfigPath(OTPaths::AppDataFolder());
    {
        bool GetConfigPathSuccess =
            strConfigPath.Exists() && 3 < strConfigPath.GetLength();
        OT_ASSERT_MSG(
            GetConfigPathSuccess,
            "HandleCommandLineArguments:  Must Set Conifg Path First!");
    }

    /* 1. CREATE AN OBJECT */
    //    AnyOption *opt = new AnyOption();
    //    OT_ASSERT(nullptr != opt);
    //    std::unique_ptr<AnyOption> theOptionAngel(opt);

    /* 2. SET PREFERENCES  */
    // opt->noPOSIX(); /* do not check for POSIX style character options */
    // opt->setVerbose(); /* print warnings about unknown options */
    // opt->autoUsagePrint(true); /* print usage for bad options */

    /* 3. SET THE USAGE/HELP   */
    opt->addUsage("");
    opt->addUsage(" **** NOTE: DO NOT USE 'ot' !! Use 'opentxs help' instead! "
                  "*** OT CLI Usage:  ");
    opt->addUsage("");
    opt->addUsage(
        "ot  --stat (Prints the wallet contents)    ot --prompt (Enter "
        "the OT prompt)");
    opt->addUsage("ot  [-h|-?|--help]    (Prints this help)   ot --script "
                  "<filename> [--args \"key value ...\"]");
    opt->addUsage(
        "The '|' symbol means use --balance or -b, use --withdraw or -w, etc.");
    opt->addUsage(
        "The brackets '[]' show required arguments, where default values are");
    opt->addUsage(
        "normally expected to be found in:   ~/.ot/command-line-ot.opt");
    opt->addUsage(
        "ot  --balance  | -b          [--myacct <acct_id>]   (Display "
        "account balance)");
    opt->addUsage("ot  --withdraw | -w <amount> [--myacct <acct_id>]   "
                  "(Withdraw as CASH)");
    opt->addUsage(
        "ot  --transfer | -t <amount> [--myacct <acct_id>] [--hisacct "
        "<acct_id>]");
    opt->addUsage(
        "ot  --cheque   | -c <amount> [--myacct <acct_id>] [--hisnym  "
        "<nym_id> ]");
    opt->addUsage(
        "ot  --voucher  | -v <amount> [--myacct <acct_id>] [--hisnym  "
        "<nym_id> ]");
    opt->addUsage(
        "ot  --depositcheque  [--myacct <acct_id>]   (Deposit a cheque.)");
    opt->addUsage(
        "ot  --depositpurse   [--myacct <acct_id>]   (Deposit a cash purse.)");
    opt->addUsage("ot  --deposittokens  [--myacct <acct_id>]   (Deposit "
                  "individual cash tokens.)");
    opt->addUsage(
        "ot  --inbox    | -i  [--myacct <acct_id>]   (Display the inbox.)");
    opt->addUsage(
        "ot  --sign     | -s  [--mynym  <nym_id> ]   (Sign a contract.)");
    opt->addUsage(
        "ot  --verify         [--mynym  <nym_id> ]   (Verify a signature.)");
    opt->addUsage(
        "ot  --purse    | -p   <arguments>           (Display a purse.)");
    opt->addUsage("  Arguments:     [--mynym  <nym_id> ] [--mypurse "
                  "<instrument_definition_id>]");
    opt->addUsage("ot  --refresh  | -r  [--myacct <acct_id>]    (Download "
                  "account files from server.)");
    opt->addUsage("ot  --refreshnym     [--mynym  <nym_id> ]    (Download nym "
                  "files from server.)");
    opt->addUsage(
        "ot  --marketoffer    [--mynym  <nym_id> ]    (Place an offer "
        "on a market.)");
    opt->addUsage(
        "Also, [--server <notary_id>] will work with all of the above.");
    opt->addUsage("");
    opt->addUsage("Recurring payments:");
    opt->addUsage("ot --proposeplan  <arguments>   (Merchant)");
    opt->addUsage("  Arguments: [--mynym  <nym_id> ] [--myacct  <acct_id>]  "
                  "(continued.)");
    opt->addUsage("  Continued: [--hisnym <nym_id> ] [--hisacct <acct_id> ]");
    opt->addUsage("ot --confirmplan  <arguments>   (Customer)");
    opt->addUsage("ot --activateplan <arguments>   (Customer again)");
    opt->addUsage("  Arguments: [--mynym  <nym_id> ] [--myacct  <acct_id>]");
    opt->addUsage(
        " **** NOTE: DO NOT USE 'ot' !! Use 'opentxs help' instead! ***");

    /* 4. SET THE OPTION STRINGS/CHARACTERS */
    //
    // COMMAND LINE *AND* RESOURCE FILE

    //  opt->setOption(  "server" );      /* an option (takes an argument),
    // supporting only int64_t form */

    /* 4. SET THE OPTION STRINGS/CHARACTERS */
    //
    // COMMAND LINE *AND* RESOURCE FILE

    //  opt->setOption(  "server" );      /* an option (takes an argument),
    // supporting only int64_t form */

    // COMMAND LINE ONLY
    /* for options that will be checked only on the command and line not in
     * option/resource file */
    //    opt->setCommandFlag(  "zip" , 'z'); /* a flag (takes no argument),
    // supporting int64_t and short form */
    opt->setCommandOption("withdraw",
                          'w'); // withdraw from acct to purse, myacct, topurse
    opt->setCommandOption("transfer",
                          't'); // transfer acct-to-acct, myacct, toacct
    opt->setCommandOption("cheque", 'c');  // write a cheque   myacct, tonym
    opt->setCommandOption("voucher", 'v'); // withdraw voucher myacct, tonym
    opt->setCommandFlag("marketoffer");    // add an offer to the market.
    opt->setCommandFlag("balance", 'b');   // Display account balance
    opt->setCommandFlag("depositcheque");  // deposit a cheque to myacct
    opt->setCommandFlag("depositpurse");   // deposit cash purse to myacct
    opt->setCommandFlag(
        "deposittokens"); // deposit individual cash tokens to myacct
    opt->setCommandFlag("proposeplan");  // Merchant proposes a payment plan.
    opt->setCommandFlag("confirmplan");  // Customer confirms a payment plan.
    opt->setCommandFlag("activateplan"); // Customer activates a payment plan.
    opt->setCommandFlag("inbox", 'i');   // displays inbox (for ACCT_ID...)
    opt->setCommandFlag("sign", 's');    // sign a contract  mynym
    opt->setCommandFlag("verify");       // verify a signature
    opt->setCommandFlag("purse", 'p');   // display purse contents.
    opt->setCommandFlag("refresh", 'r'); // refresh intermediary files from
    // server + verify against last receipt.
    opt->setCommandFlag("refreshnym"); // refresh intermediary files from server
                                       // + verify against last receipt.
    opt->setCommandFlag("stat");       // print out the wallet contents.
    opt->setCommandFlag("prompt");     // Enter the OT prompt.
    opt->setCommandOption(
        "script"); // Process a script from out of a scriptfile
    opt->setCommandOption(
        "args"); // Pass custom arguments from command line: --args "key1 value1
                 // key2 \"here is value2\" key3 value3"

    opt->setCommandFlag("help", 'h'); // the Help screen.
    opt->setCommandFlag('?');         // the Help screen.

    /*
          --myacct   (ACCT ID)
          --mynym    (NYM ID)
          --mypurse  (ASSET TYPE ID)

          --toacct   (ACCT ID)
          --tonym    (NYM ID)
          --topurse  (ASSET TYPE ID)
          */
    opt->setCommandOption("server");

    opt->setCommandOption("myacct");
    opt->setCommandOption("mynym");
    opt->setCommandOption("mypurse");
    opt->setCommandOption("hisacct");
    opt->setCommandOption("hisnym");
    opt->setCommandOption("hispurse");

    // NOTE: Above and Below me are IDs. This interface should allow PARTIAL
    // IDs.

    // RESOURCE FILE ONLY
    /* for options that will be checked only from the option/resource file */
    opt->setFileOption("defaultserver");
    /* an option (takes an argument), supporting only int64_t form */

    opt->setFileOption("defaultmyacct");
    /* an option (takes an argument), supporting only int64_t form */
    opt->setFileOption("defaultmynym");
    /* an option (takes an argument), supporting only int64_t form */
    opt->setFileOption("defaultmypurse");
    /* an option (takes an argument), supporting only int64_t form */
    opt->setFileOption("defaulthisacct");
    /* an option (takes an argument), supporting only int64_t form */
    opt->setFileOption("defaulthisnym");
    /* an option (takes an argument), supporting only int64_t form */
    opt->setFileOption("defaulthispurse");
    /* an option (takes an argument), supporting only int64_t form */
    /*
          --defaultmyacct   (ACCT ID)
          --defaultmynym    (NYM ID)
          --defaultmypurse  (ASSET TYPE ID)

          --defaulttoacct   (ACCT ID)
          --defaulttonym    (NYM ID)
          --defaulttopurse  (ASSET TYPE ID)
          */

    /* 5. PROCESS THE COMMANDLINE AND RESOURCE FILE */

    /* read options from a option/resource file with ':' separated options or
     * flags, one per line */

    String strOptionsFile(OT_OPTIONS_FILE_DEFAULT), strIniFileExact;
    {
        bool bBuildFullPathSuccess = OTPaths::RelativeToCanonical(
            strIniFileExact, strConfigPath, strOptionsFile);
        OT_ASSERT_MSG(bBuildFullPathSuccess, "Unalbe to set Full Path");
    }

    opt->processFile(strIniFileExact.Get());
    opt->processCommandArgs(argc, argv);
}

/*
I'm starting to need this in possibly multiple places below, so I
made a function to avoid duplicating code. These are values such
as "my account ID" and "his NymID" that are provided on the command
line, and which also can be defaulted in a config file in ~/.ot
*/
void CollectDefaultedCLValues(AnyOption* opt, std::string& str_NotaryID,
                              std::string& str_MyAcct, std::string& str_MyNym,
                              std::string& str_MyPurse,
                              std::string& str_HisAcct, std::string& str_HisNym,
                              std::string& str_HisPurse)
{
    OT_ASSERT(nullptr != opt);

    OTAPI_Wrap::Output(1, "\n");

    // First we pre-set all the values based on the defaults from the options
    // file.
    //
    if (opt->getValue("defaultserver") != nullptr) {
        //      cerr << "Server default: " << (str_NotaryID = opt->getValue(
        // "defaultserver" )) << endl;
        str_NotaryID = opt->getValue("defaultserver");
        otWarn << "Server default: " << str_NotaryID << " \n";
    }

    if (opt->getValue("defaultmyacct") != nullptr) {
        //      cerr << "MyAcct default: " << (str_MyAcct = opt->getValue(
        // "defaultmyacct" )) << endl;
        str_MyAcct = opt->getValue("defaultmyacct");
        otWarn << "MyAcct default: " << str_MyAcct << " \n";
    }
    if (opt->getValue("defaultmynym") != nullptr) {
        //      cerr << "MyNym default: " << (str_MyNym = opt->getValue(
        // "defaultmynym" )) << endl;
        str_MyNym = opt->getValue("defaultmynym");
        otWarn << "MyNym default: " << str_MyNym << " \n";
    }
    if (opt->getValue("defaultmypurse") != nullptr) {
        //      cerr << "MyPurse default: " << (str_MyPurse = opt->getValue(
        // "defaultmypurse" )) << endl;
        str_MyPurse = opt->getValue("defaultmypurse");
        otWarn << "MyPurse default: " << str_MyPurse << " \n";
    }

    if (opt->getValue("defaulthisacct") != nullptr) {
        //      cerr << "HisAcct default: " << (str_HisAcct = opt->getValue(
        // "defaulthisacct" )) << endl;
        str_HisAcct = opt->getValue("defaulthisacct");
        otWarn << "HisAcct default: " << str_HisAcct << " \n";
    }
    if (opt->getValue("defaulthisnym") != nullptr) {
        //      cerr << "HisNym default: " << (str_HisNym = opt->getValue(
        // "defaulthisnym" )) << endl;
        str_HisNym = opt->getValue("defaulthisnym");
        otWarn << "HisNym default: " << str_HisNym << " \n";
    }
    if (opt->getValue("defaulthispurse") != nullptr) {
        //      cerr << "HisPurse default: " << (str_HisPurse = opt->getValue(
        // "defaulthispurse" )) << endl;
        str_HisPurse = opt->getValue("defaulthispurse");
        otWarn << "HisPurse default: " << str_HisPurse << " \n";
    }

    // Next, we overwrite those with any that were passed in on the command
    // line.

    if (opt->getValue("server") != nullptr) {
        //      cerr << "Server from command-line: " << (str_NotaryID =
        // opt->getValue( "server" )) << endl;
        str_NotaryID = opt->getValue("server");
        otWarn << "Server from command-line: " << str_NotaryID << " \n";
    }

    if (opt->getValue("myacct") != nullptr) {
        //      cerr << "MyAcct from command-line: " << (str_MyAcct =
        // opt->getValue(
        // "myacct" )) << endl;
        str_MyAcct = opt->getValue("myacct");
        otWarn << "MyAcct from command-line: " << str_MyAcct << " \n";
    }
    if (opt->getValue("mynym") != nullptr) {
        //      cerr << "MyNym from command-line: " << (str_MyNym =
        // opt->getValue(
        // "mynym" )) << endl;
        str_MyNym = opt->getValue("mynym");
        otWarn << "MyNym from command-line: " << str_MyNym << " \n";
    }
    if (opt->getValue("mypurse") != nullptr) {
        //      cerr << "MyPurse from command-line: " << (str_MyPurse =
        // opt->getValue( "mypurse" )) << endl;
        str_MyPurse = opt->getValue("mypurse");
        otWarn << "MyPurse from command-line: " << str_MyPurse << " \n";
    }

    if (opt->getValue("hisacct") != nullptr) {
        //      cerr << "HisAcct from command-line: " << (str_HisAcct =
        // opt->getValue( "hisacct" )) << endl;
        str_HisAcct = opt->getValue("hisacct");
        otWarn << "HisAcct from command-line: " << str_HisAcct << " \n";
    }
    if (opt->getValue("hisnym") != nullptr) {
        //      cerr << "HisNym from command-line: " << (str_HisNym =
        // opt->getValue(
        // "hisnym" )) << endl;
        str_HisNym = opt->getValue("hisnym");
        otWarn << "HisNym from command-line: " << str_HisNym << " \n";
    }
    if (opt->getValue("hispurse") != nullptr) {
        //      cerr << "HisPurse from command-line: " << (str_HisPurse =
        // opt->getValue( "hispurse" )) << endl;
        str_HisPurse = opt->getValue("hispurse");
        otWarn << "HisPurse from command-line: " << str_HisPurse << " \n";
    }
}

// *************************************   MAIN FUNCTION

using std::cerr;
using std::endl;

int32_t main(int32_t argc, char* argv[])
{
    class __OTclient_RAII
    {
    public:
        __OTclient_RAII()
        {
            // OT_API class exists only on the client side.

            OTAPI_Wrap::AppInit(); // SSL gets initialized in here, before any
                                   // keys
                                   // are loaded.
        }
        ~__OTclient_RAII()
        {
            OTAPI_Wrap::AppCleanup();
        }
    };
    //
    // This makes SURE that AppCleanup() gets called before main() exits
    // (without
    // any
    // twisted logic being necessary below, for that to happen.)
    //
    __OTclient_RAII the_client_cleanup;
    //

    if (nullptr == OTAPI_Wrap::OTAPI())
        return -1; // error out if we don't have the API.

    String strConfigPath(OTPaths::AppDataFolder());
    bool bConfigPathFound =
        strConfigPath.Exists() && 3 < strConfigPath.GetLength();

    OT_ASSERT_MSG(bConfigPathFound,
                  "RegisterAPIWithScript: Must set Config Path first!\n");

    otWarn << "Using configuration path:  " << strConfigPath << "\n";

    //    otOut << "Prefix Path:  " << OTPaths::PrefixFolder() << "\n";
    //    otOut << "Scripts Path:  " << OTPaths::ScriptsFolder() << "\n";
    //
    //    OTString out_strHomeFolder;
    //    OTPaths::GetHomeFromSystem(out_strHomeFolder);
    //    otOut << "Home from System:  " << out_strHomeFolder << "\n";

    // COMMAND-LINE OPTIONS (and default values from files.)
    //
    AnyOption* opt = new AnyOption();
    OT_ASSERT(nullptr != opt);
    std::unique_ptr<AnyOption> theOptionAngel(opt);

    // Process the command line args
    //
    HandleCommandLineArguments(argc, argv, opt);

    // command line values such as account ID, Nym ID, etc.
    // Also available as defaults in a config file in the ~/.ot folder
    //
    std::string str_NotaryID;

    std::string str_MyAcct;
    std::string str_MyNym;
    std::string str_MyPurse;

    std::string str_HisAcct;
    std::string str_HisNym;
    std::string str_HisPurse;

    CollectDefaultedCLValues(opt, str_NotaryID, str_MyAcct, str_MyNym,
                             str_MyPurse, str_HisAcct, str_HisNym,
                             str_HisPurse);
    // Users can put --args "key value key value key value etc"
    // Then they can access those values from within their scripts.

    std::string str_Args;

    if (opt->getValue("args") != nullptr)
        cerr << "User-defined arguments aka:  --args "
             << (str_Args = opt->getValue("args")) << endl;

    /*  USAGE SCREEN (HELP) */
    //
    if (opt->getFlag("help") || opt->getFlag('h') || opt->getFlag('?')) {
        opt->printUsage();

        return 0;
    }

    bool bIsCommandProvided = false;

    // See if there's a COMMAND chosen at command line.
    //
    if (opt->hasOptions()) {
        // Below are COMMANDS (only one of them can be true...)
        //

        if (opt->getValue('w') != nullptr ||
            opt->getValue("withdraw") != nullptr) {
            bIsCommandProvided = true;
            cerr << "withdraw amount = " << opt->getValue('w') << endl;
        }
        else if (opt->getValue('t') != nullptr ||
                   opt->getValue("transfer") != nullptr) {
            bIsCommandProvided = true;
            cerr << "transfer amount = " << opt->getValue('t') << endl;
        }
        else if (opt->getValue('c') != nullptr ||
                   opt->getValue("cheque") != nullptr) {
            bIsCommandProvided = true;
            cerr << "cheque amount = " << opt->getValue('c') << endl;
        }
        else if (opt->getFlag("marketoffer") == true) {
            bIsCommandProvided = true;
            cerr << "marketoffer flag set " << endl;
        }
        else if (opt->getValue('v') != nullptr ||
                   opt->getValue("voucher") != nullptr) {
            bIsCommandProvided = true;
            cerr << "voucher amount = " << opt->getValue('v') << endl;
        }
        else if (opt->getFlag("depositcheque")) {
            bIsCommandProvided = true;
            cerr << "deposit cheque flag set " << endl;
        }
        else if (opt->getFlag("depositpurse")) {
            bIsCommandProvided = true;
            cerr << "deposit purse flag set " << endl;
        }
        else if (opt->getFlag("deposittokens")) {
            bIsCommandProvided = true;
            cerr << "deposit tokens flag set " << endl;
        }
        else if (opt->getFlag("proposepaymentplan")) {
            bIsCommandProvided = true;
            cerr << "proposepaymentplan flag set " << endl;
        }
        else if (opt->getFlag("confirmpaymentplan")) {
            bIsCommandProvided = true;
            cerr << "confirm payment plan flag set " << endl;
        }
        else if (opt->getFlag("activatepaymentplan")) {
            bIsCommandProvided = true;
            cerr << "activate payment plan flag set " << endl;
        }
        else if (opt->getFlag('b') || opt->getFlag("balance")) {
            bIsCommandProvided = true;
            cerr << "balance flag set " << endl;
        }
        else if (opt->getFlag('i') || opt->getFlag("inbox")) {
            bIsCommandProvided = true;
            cerr << "inbox flag set " << endl;
        }
        else if (opt->getFlag('p') || opt->getFlag("purse")) {
            bIsCommandProvided = true;
            cerr << "purse flag set " << endl;
        }
        else if (opt->getFlag('s') || opt->getFlag("sign")) {
            bIsCommandProvided = true;
            cerr << "sign flag set " << endl;
        }
        else if (opt->getFlag("verify")) {
            bIsCommandProvided = true;
            cerr << "verify flag set " << endl;
        }
        else if (opt->getFlag("stat")) {
            bIsCommandProvided = true;
            cerr << "stat flag set " << endl;
        }
        else if (opt->getFlag("prompt")) {
            bIsCommandProvided = true;
            cerr << "prompt flag set " << endl;
        }
        else if (opt->getValue("script") != nullptr) {
            bIsCommandProvided = true;
            cerr << "script filename: " << opt->getValue("script") << endl;
        }
        else if (opt->getFlag('r') || opt->getFlag("refresh")) {
            bIsCommandProvided = true;
            cerr << "refresh flag set " << endl;
        }
        else if (opt->getFlag("refreshnym")) {
            bIsCommandProvided = true;
            cerr << "refreshnym flag set " << endl;
        }

        cerr << endl;
    }
    else
        bIsCommandProvided = false;

    //
    if (!(opt->getArgc() > 0) && (false == bIsCommandProvided)) // If no command
                                                                // was provided
                                                                // (though other
    // command-line options may have been...)
    { // then we expect a script to come in through stdin, and we run it
        // through the script interpreter!
        otOut << "\n\nYou probably don't want to do this... Use CTRL-C, "
                 "and try \"ot --help\" for instructions.\n\n "
                 "==> Expecting ot script from standard input. (Terminate "
                 "with CTRL-D):\n\n";

        // don't skip the whitespace while reading
        std::cin >> std::noskipws;

        // use stream iterators to copy the stream to a string
        std::istream_iterator<char> it(std::cin);
        std::istream_iterator<char> end;
        std::string results(it, end);

        OT_ME madeEasy;
        OTAPI_Func::CopyVariables();
        madeEasy.ExecuteScript_ReturnVoid(results, ("stdin"));

        return 0;
    }

    // Otherwise a command WAS provided at the command line, so we execute a
    // single time, once just for that command.
    {
        OTWallet* pWallet = nullptr;
        OTServerContract* pServerContract = nullptr;
        Nym* pMyNym = nullptr;

        // This does LoadWallet, andif Nym or Notary IDs were provided, loads
        // those
        // up as well.
        // (They may still be nullptr after this call, however.)
        //
        bool bMainPointersSetupSuccessful =
            SetupPointersForWalletMyNymAndServerContract(
                str_NotaryID, str_MyNym, pMyNym, pWallet, pServerContract);

        OT_ASSERT_MSG(
            bMainPointersSetupSuccessful,
            "main: SetupPointersForWalletMyNymAndServerContract failed "
            "to return true");

        // Below this point, pWallet is available :-)
        // Later I can split the below commands into "those that need a server
        // contract"
        // and "those that don't need a server contract", and put this code
        // between
        // them.
        // That's what the OT Prompt loop does. For now I'm making things easy
        // here
        // by just
        // making it a blanket requirement.
        //
        if (nullptr == pServerContract) {
            otOut << "Unable to find a server contract to use. Please use "
                     "the option: --server NOTARY_ID\n"
                     "(Where NOTARY_ID is the Server's ID. Partial matches "
                     "ARE accepted.)\n";
            //          return 0;
        }

        Identifier theNotaryID;
        String strNotaryID;

        if (nullptr != pServerContract) {
            pServerContract->GetIdentifier(theNotaryID);
            theNotaryID.GetString(strNotaryID);
        }
        //      int32_t       nServerPort = 0;
        //      OTString  strServerHostname;
        // You can't just connect to any hostname and port.
        // Instead, you give me the Server Contract, and *I'll* look up all that
        // stuff FOR you...
        // (We verify this up here, but use it at the bottom of the function
        // once
        // the message is set up.)
        //

        //      if (!pServerContract->GetConnectInfo(strServerHostname,
        // nServerPort))
        //      {
        //          otErr << "Failed retrieving connection info from server "
        //                   "contract: " << strNotaryID << "\n";
        //          return 0;
        //      }

        // Below this point, pWallet and pServerContract are both available.
        // UPDATE: Not necessarily... (pServerContract may be nullptr...)
        //

        Account* pMyAccount = nullptr;
        Account* pHisAccount = nullptr;

        if (str_MyAcct.size() > 0) {
            const Identifier MY_ACCOUNT_ID(str_MyAcct.c_str());

            pMyAccount = pWallet->GetAccount(MY_ACCOUNT_ID);

            // If failure, then we try PARTIAL match.
            if (nullptr == pMyAccount)
                pMyAccount = pWallet->GetAccountPartialMatch(str_MyAcct);

            if (nullptr != pMyAccount) {
                String strTemp;
                pMyAccount->GetPurportedAccountID().GetString(strTemp);

                str_MyAcct = strTemp.Get();
                otOut << "Using as myacct: " << str_MyAcct << "\n";
            }
            else // Execution aborts if we cannot find MyAcct when one was
                   // provided.
            {
                otOut << "Aborting: Unable to find specified myacct: "
                      << str_MyAcct << "\n";
                return 0;
            }
        }
        // TODO: I wouldn't have HIS account in MY wallet -- I'd only have his
        // account ID.
        // Therefore need to be able to retrieve this info from the ADDRESS BOOK
        // (in
        // order
        // to be able to do PARTIAL MATCHES...)
        //
        if (str_HisAcct.size() > 0) {
            const Identifier HIS_ACCOUNT_ID(str_HisAcct.c_str());

            pHisAccount = pWallet->GetAccount(HIS_ACCOUNT_ID);

            // If failure, then we try PARTIAL match.
            if (nullptr == pHisAccount)
                pHisAccount = pWallet->GetAccountPartialMatch(str_HisAcct);
            if (nullptr != pHisAccount) {
                String strTemp;
                pHisAccount->GetPurportedAccountID().GetString(strTemp);

                str_HisAcct = strTemp.Get();
                otOut << "Using as hisacct: " << str_HisAcct << "\n";
            }

            // Execution continues, even if we fail to find his account.
            // (Only my accounts will be in my wallet. Anyone else's account
            // will exist on the server, even if it's not in my wallet.
            // Therefore
            // we still allow users to use HisAcctID since their server messages
            // will usually actually work.)
            //
            // Again: Just because account lkjsf09234lkjafkljasd098q345lkjasdf
            // doesn't
            // appear in my wallet, doesn't mean the account doesn't exist on
            // the
            // server
            // and in reality. Therefore I must assume, if I didn't find it by
            // abbreviation,
            // that it exists exactly as entered. The server message will just
            // fail,
            // if it
            // doesn't exist. (But then that's the user's fault...)
            //
            // We can still keep account IDs in the address book, even if they
            // aren't
            // in the wallet (since they're owned by someone else...)
            //
        }

        // I put this here too since I think it's required in all cases.
        // Update: commented out the return in order to allow for empty wallets.
        //
        if (nullptr ==
            pMyNym) // Todo maybe move this check to the commands below
                    // (ONLY the ones that use a nym.)
        {
            otOut << "Unable to find My Nym. Please use the option:   --mynym "
                     "NYM_ID\n"
                     "(Where NYM_ID is the Nym's ID. Partial matches and "
                     "names are "
                     "accepted.)\n";
            //          return 0;
        }

        Identifier MY_NYM_ID;

        if (nullptr != pMyNym) pMyNym->GetIdentifier(MY_NYM_ID);
        Nym* pHisNym = nullptr;

        if (str_HisNym.size() > 0) {
            const Identifier HIS_NYM_ID(str_HisNym.c_str());

            pHisNym = pWallet->GetNymByID(HIS_NYM_ID);
            // If failure, then we try PARTIAL match.
            if (nullptr == pHisNym)
                pHisNym = pWallet->GetNymByIDPartialMatch(str_HisNym);
            if (nullptr != pHisNym) {
                String strTemp;
                pHisNym->GetIdentifier(strTemp);

                str_HisNym = strTemp.Get();
                otOut << "Using as hisnym: " << str_HisNym << "\n";
            }
        }

        // Below this point, if Nyms or Accounts were specified, they are now
        // available.
        // (Pointers might be null, though currently My Nym is required to be
        // there.)
        //
        // Execution continues, even if we fail to find his nym.
        // This is so the script has the opportunity to "check nym" (Download
        // it)
        // based on the ID that the user has entered here.

        Identifier thePurseInstrumentDefinitionID;
        AssetContract* pMyAssetContract = nullptr;

        if (str_MyPurse.size() > 0) {
            const Identifier MY_INSTRUMENT_DEFINITION_ID(str_MyPurse.c_str());
            pMyAssetContract =
                pWallet->GetAssetContract(MY_INSTRUMENT_DEFINITION_ID);

            // If failure, then we try PARTIAL match.
            if (nullptr == pMyAssetContract)
                pMyAssetContract =
                    pWallet->GetAssetContractPartialMatch(str_MyPurse);

            if (nullptr != pMyAssetContract) {
                String strTemp;
                pMyAssetContract->GetIdentifier(strTemp);

                str_MyPurse = strTemp.Get();
                otOut << "Using as mypurse: " << str_MyPurse << "\n";

                pMyAssetContract->GetIdentifier(thePurseInstrumentDefinitionID);
            }
            // Execution continues here, so the script has the option to
            // download
            // any asset contract, if it can't find it in the wallet.
        }
        // if no purse (instrument definition) ID was provided, but MyAccount
        // WAS provided,
        // then
        // use the instrument definition for the account instead.
        else if (nullptr != pMyAccount)
            thePurseInstrumentDefinitionID =
                pMyAccount->GetInstrumentDefinitionID();
        if (!thePurseInstrumentDefinitionID.IsEmpty()) {
            String strTempAssetType(thePurseInstrumentDefinitionID);
            str_MyPurse = strTempAssetType.Get();
        }
        // BELOW THIS POINT, pMyAssetContract MIGHT be nullptr, or MIGHT be an
        // asset
        // type specified by the user.
        // There's no guarantee that it's available, but if it IS, then it WILL
        // be
        // available below this point.
        Identifier hisPurseInstrumentDefinitionID;

        if (str_HisPurse.size() > 0) {
            const Identifier HIS_INSTRUMENT_DEFINITION_ID(str_HisPurse.c_str());
            AssetContract* pHisAssetContract =
                pWallet->GetAssetContract(HIS_INSTRUMENT_DEFINITION_ID);

            // If failure, then we try PARTIAL match.
            if (nullptr == pHisAssetContract)
                pHisAssetContract =
                    pWallet->GetAssetContractPartialMatch(str_HisPurse);

            if (nullptr != pHisAssetContract) {
                String strTemp;
                pHisAssetContract->GetIdentifier(strTemp);

                str_HisPurse = strTemp.Get();
                otOut << "Using as hispurse: " << str_HisPurse << "\n";

                pHisAssetContract->GetIdentifier(
                    hisPurseInstrumentDefinitionID);
            }
        }
        // If no "HisPurse" was provided, but HisAcct WAS, then we use the
        // instrument definition of HisAcct as HisPurse.
        else if (nullptr != pHisAccount)
            hisPurseInstrumentDefinitionID =
                pHisAccount->GetInstrumentDefinitionID();
        if (!hisPurseInstrumentDefinitionID.IsEmpty()) {
            String strTempAssetType(hisPurseInstrumentDefinitionID);
            str_HisPurse = strTempAssetType.Get();
        }

        otOut << "\n";

        // Also, pAccount and pMyAssetContract have not be validated AGAINST
        // EACH
        // OTHER (yet)...
        // Also, pHisAccount and pHisAssetContract have not be validated AGAINST
        // EACH OTHER (yet)...

        /*  GET THE ACTUAL ARGUMENTS AFTER THE OPTIONS */
        //
        //      for( int32_t i = 0 ; i < opt->getArgc() ; i++ )
        //      {
        //         cerr << "arg = " <<  opt->getArgv( i ) << endl ;
        //      }

        bool bSendCommand = false; // Determines whether to actually send a
                                   // message to the server.

        Message theMessage;

        // If we can match the user's request to a client command,
        // AND theClient object is able to process that request into
        // theMessage, then we send it down the pipe.

        // COMMANDS

        if ((opt->getValue("script") != nullptr) || (opt->getArgc() > 0)) {
            std::string strFilename;

            // If a filename is provided as a normal argument (like this: ot
            // <filename>)
            // then it will work...
            //
            if (opt->getArgc() > 0) {
                strFilename = opt->getArgv(0);
            }

            // the --script option will ALSO work for the filename, and will
            // override
            // the above.
            // so:   ot --script <filename>
            // also: ot --script <actual_filename> <ignored_filename>
            //
            // In this above example, ignored_filename WOULD have been used, but
            // then
            // it got
            // overridden by the --script actual_filename.

            if (nullptr != opt->getValue("script")) {
                strFilename = opt->getValue("script");
            }

            std::ifstream t(strFilename.c_str(),
                            std::ios::in | std::ios::binary);
            std::stringstream buffer;
            buffer << t.rdbuf();
            std::string results = buffer.str();
            OT_ME madeEasy;

            std::unique_ptr<OTVariable> angelArgs;

            std::unique_ptr<OTVariable> angelMyNymVar;
            std::unique_ptr<OTVariable> angelHisNymVar;
            std::unique_ptr<OTVariable> angelServer;
            std::unique_ptr<OTVariable> angelMyAcct;
            std::unique_ptr<OTVariable> angelHisAcct;
            std::unique_ptr<OTVariable> angelMyPurse;
            std::unique_ptr<OTVariable> angelHisPurse;

            if ((str_Args.size() > 0) || (opt->getArgc() > 1)) {
                const std::string str_var_name("Args");
                std::string str_var_value, str_command;

                if (str_Args.size() > 0) str_var_value += str_Args;

                if (opt->getArgc() > 1) {
                    if (str_Args.size() > 0) str_var_value += " ";

                    str_command = opt->getArgv(1);
                    str_var_value += "ot_cli_command ";
                    str_var_value += str_command;
                }

                otWarn << "Adding user-defined command line arguments as '"
                       << str_var_name << "' "
                                          "containing value: " << str_var_value
                       << "\n";

                OTVariable* pVar =
                    new OTVariable(str_var_name,  // "Args"
                                   str_var_value, // "key1 value1 key2 value2
                                                  // key3 value3 key4 value4"
                                   OTVariable::Var_Constant); // constant,
                                                              // persistent, or
                                                              // important.
                angelArgs.reset(pVar);
                OT_ASSERT(nullptr != pVar);
                madeEasy.AddVariable(str_var_name, *pVar);
            }
            else {
                otInfo << "Args variable (optional user-defined "
                          "arguments) isn't set...\n";
            }

            if (str_NotaryID.size() > 0) {
                const std::string str_var_name("Server");
                const std::string str_var_value(str_NotaryID);

                otWarn << "Adding constant with name " << str_var_name
                       << " and value: " << str_var_value << " ...\n";

                OTVariable* pVar = new OTVariable(
                    str_var_name,  // "Server"
                    str_var_value, // "lkjsdf09834lk5j34lidf09" (Whatever)
                    OTVariable::Var_Constant); // constant, persistent, or
                                               // important.
                angelServer.reset(pVar);
                OT_ASSERT(nullptr != pVar);
                madeEasy.AddVariable(str_var_name, *pVar);
            }
            else {
                otInfo << "Server variable isn't set...\n";
            }

            if (nullptr != pMyNym) {
                const std::string str_party_name("MyNym");

                otWarn << "Adding constant with name " << str_party_name
                       << " and value: " << str_MyNym << " ...\n";

                OTVariable* pVar = new OTVariable(
                    str_party_name, // "MyNym"
                    str_MyNym,      // "lkjsdf09834lk5j34lidf09" (Whatever)
                    OTVariable::Var_Constant); // constant, persistent, or
                                               // important.
                angelMyNymVar.reset(pVar);
                OT_ASSERT(nullptr != pVar);
                madeEasy.AddVariable(str_party_name, *pVar);
            }
            else {
                otInfo << "MyNym variable isn't set...\n";
            }

            if ((nullptr != pHisNym) || (str_HisNym.size() > 0)) // Even if we
                                                                 // didn't find
            // him, we still
            // let
            // the ID through, if there is one.
            {
                const std::string str_party_name("HisNym");

                otWarn << "Adding constant with name " << str_party_name
                       << " and value: " << str_HisNym << " ...\n";

                OTVariable* pVar = new OTVariable(
                    str_party_name, // "HisNym"
                    str_HisNym,     // "lkjsdf09834lk5j34lidf09" (Whatever)
                    OTVariable::Var_Constant); // constant, persistent, or
                                               // important.
                angelHisNymVar.reset(pVar);
                OT_ASSERT(nullptr != pVar);
                madeEasy.AddVariable(str_party_name, *pVar);
            }
            else {
                otInfo << "HisNym variable isn't set...\n";
            }
            // WE NO LONGER PASS THE PARTY DIRECTLY TO THE SCRIPT,
            // BUT INSTEAD, ONLY THE PARTY'S NAME.
            //
            // (Because often, "HisNym" isn't in my wallet and wouldn't be found
            // anyway,
            //  even though it ends up to contain a perfectly legitimate Nym
            // ID.)
            /*
                  if (nullptr != pMyNym)
                  {
                      const std::string str_party_name("MyNym"),
            str_agent_name("mynym"), str_acct_name("myacct");

                      pPartyMyNym = new OTParty (str_party_name, *pMyNym,
            str_agent_name, pMyAccount, &str_acct_name);
                      angelMyNym.reset(pPartyMyNym);
                      OT_ASSERT(nullptr != pPartyMyNym);
                      pScript-> AddParty("MyNym", *pPartyMyNym);
                  }
                  else
                  {
                      otErr << "MyNym variable isn't set...\n";
                  }
                  if (nullptr != pHisNym)
                  {
                      const std::string str_party_name("HisNym"),
            str_agent_name("hisnym"), str_acct_name("hisacct");

                      pPartyHisNym = new OTParty (str_party_name, *pHisNym,
            str_agent_name, pHisAccount, &str_acct_name);
                      angelHisNym.reset(pPartyHisNym);
                      OT_ASSERT(nullptr != pPartyHisNym);
                      pScript-> AddParty("HisNym", *pPartyHisNym);
                  }
                  else
                  {
                      otErr << "HisNym variable isn't set...\n";
                  }
                  */

            if (str_MyAcct.size() > 0) {
                const std::string str_var_name("MyAcct");
                const std::string str_var_value(str_MyAcct);

                otWarn << "Adding variable with name " << str_var_name
                       << " and value: " << str_var_value << " ...\n";

                OTVariable* pVar = new OTVariable(
                    str_var_name,  // "MyAcct"
                    str_var_value, // "lkjsdf09834lk5j34lidf09" (Whatever)
                    OTVariable::Var_Constant); // constant, persistent, or
                                               // important.
                angelMyAcct.reset(pVar);
                OT_ASSERT(nullptr != pVar);
                madeEasy.AddVariable(str_var_name, *pVar);
            }
            else {
                otInfo << "MyAcct variable isn't set...\n";
            }

            if (str_MyPurse.size() > 0) {
                const std::string str_var_name("MyPurse");
                const std::string str_var_value(str_MyPurse);

                otWarn << "Adding variable with name " << str_var_name
                       << " and value: " << str_var_value << " ...\n";

                OTVariable* pVar = new OTVariable(
                    str_var_name,  // "MyPurse"
                    str_var_value, // "lkjsdf09834lk5j34lidf09" (Whatever)
                    OTVariable::Var_Constant); // constant, persistent, or
                                               // important.
                angelMyPurse.reset(pVar);
                OT_ASSERT(nullptr != pVar);
                madeEasy.AddVariable(str_var_name, *pVar);
            }
            else {
                otInfo << "MyPurse variable isn't set...\n";
            }

            if (str_HisAcct.size() > 0) {
                const std::string str_var_name("HisAcct");
                const std::string str_var_value(str_HisAcct);

                otWarn << "Adding variable with name " << str_var_name
                       << " and value: " << str_var_value << " ...\n";

                OTVariable* pVar = new OTVariable(
                    str_var_name,  // "HisAcct"
                    str_var_value, // "lkjsdf09834lk5j34lidf09" (Whatever)
                    OTVariable::Var_Constant); // constant, persistent, or
                                               // important.
                angelHisAcct.reset(pVar);
                OT_ASSERT(nullptr != pVar);
                madeEasy.AddVariable(str_var_name, *pVar);
            }
            else {
                otInfo << "HisAcct variable isn't set...\n";
            }

            if (str_HisPurse.size() > 0) {
                const std::string str_var_name("HisPurse");
                const std::string str_var_value(str_HisPurse);

                otWarn << "Adding variable with name " << str_var_name
                       << " and value: " << str_var_value << " ...\n";

                OTVariable* pVar = new OTVariable(
                    str_var_name,  // "HisPurse"
                    str_var_value, // "lkjsdf09834lk5j34lidf09" (Whatever)
                    OTVariable::Var_Constant); // constant, persistent, or
                                               // important.
                angelHisPurse.reset(pVar);
                OT_ASSERT(nullptr != pVar);
                madeEasy.AddVariable(str_var_name, *pVar);
            }
            else {
                otInfo << "MyPurse variable isn't set...\n";
            }

            OTAPI_Func::CopyVariables();

            otWarn << "Script output:\n\n";

            return madeEasy.ExecuteScript_ReturnInt(results, strFilename);
        }
        // OT SCRIPT ABOVE.

        if ((nullptr == pServerContract) || (nullptr == pMyNym)) {
            otErr << "Unexpected nullptr: "
                  << ((nullptr == pServerContract) ? "pServerContract" : "")
                  << " " << ((nullptr == pMyNym) ? "pMyNym" : "") << "\n";
        }
        else if (opt->getValue('c') != nullptr ||
                   opt->getValue("cheque") != nullptr) {
            otOut << "(User has instructed to write a cheque...)\n";

            const int64_t lAmount = String::StringToLong(opt->getValue('c'));

            Identifier HIS_NYM_ID((str_HisNym.size() > 0)
                                      ? str_HisNym.c_str()
                                      : "aaaaaaaa"); // todo hardcoding

            OTAPI_Wrap::OTAPI()->GetClient()->ProcessUserCommand(
                OTClient::writeCheque, theMessage, *pMyNym, *pServerContract,
                pMyAccount, lAmount, nullptr, // asset contract
                (str_HisNym.size() > 0) ? &HIS_NYM_ID : nullptr);
        }
        else if (opt->getFlag("activatepaymentplan")) {
            otOut << "(User has instructed to activate a payment plan...)\n";

            if (0 < OTAPI_Wrap::OTAPI()->GetClient()->ProcessUserCommand(
                        OTClient::paymentPlan, theMessage, *pMyNym,
                        *pServerContract, pMyAccount)) // if user DOES specify
                                                       // an account
                                                       // (unnecessary)
            { // then OT will verify that they match, and error otherwise.
                bSendCommand = true;
            }
            else
                otErr << "Error processing activate payment plan command in "
                         "ProcessMessage.\n";
        }
        else if (opt->getFlag("depositcheque")) {
            otOut << "(User has instructed to deposit a cheque...)\n";

            // if successful setting up the command payload...

            if (0 < OTAPI_Wrap::OTAPI()->GetClient()->ProcessUserCommand(
                        OTClient::notarizeCheque, theMessage, *pMyNym,
                        *pServerContract, pMyAccount)) {
                bSendCommand = true;
            }
            else
                otErr << "Error processing deposit cheque command in "
                         "ProcessMessage.\n";
        }
        else if (opt->getFlag("depositpurse")) {
            otOut << "(User has instructed to deposit a cash purse...)\n";

            // if successful setting up the command payload...

            if (0 < OTAPI_Wrap::OTAPI()->GetClient()->ProcessUserCommand(
                        OTClient::notarizePurse, theMessage, *pMyNym,
                        *pServerContract, pMyAccount, 0, // amount (unused here)
                        pMyAssetContract)) {
                bSendCommand = true;
            }
            else
                otErr << "Error processing deposit purse command in "
                         "ProcessMessage.\n";
        }
        else if (opt->getFlag('i') || opt->getFlag("inbox")) {
            cerr << "DISPLAY INBOX CONTENTS HERE... (When I code this. What "
                    "can I "
                    "say? Use the GUI.)" << endl;
        }
        else if (opt->getFlag('p') || opt->getFlag("purse")) {
            cerr << "User wants to display purse contents (not coded yet here.)"
                 << endl;
        }
        else if (opt->getFlag("verify")) {
            cerr << "User wants to verify a signature on a contract (not coded "
                    "yet "
                    "here) " << endl;
        }
        else if (opt->getFlag("stat")) {
            otOut << "User has instructed to display wallet contents...\n";

            String strStat;
            pWallet->DisplayStatistics(strStat);
            otOut << strStat << "\n";
        }
        else if (opt->getFlag("prompt")) {
            otOut << "User has instructed to enter the OT prompt...\n";
        }
        else if (opt->getFlag('b') || opt->getFlag("balance")) {
            otOut << "\n ACCT BALANCE (server-side): "
                  << pMyAccount->GetBalance() << "\n\n";

            Purse* pPurse = OTAPI_Wrap::OTAPI()->LoadPurse(
                theNotaryID, thePurseInstrumentDefinitionID, MY_NYM_ID);
            std::unique_ptr<Purse> thePurseAngel(pPurse);
            if (nullptr != pPurse)
                otOut << " CASH PURSE (client-side): "
                      << pPurse->GetTotalValue() << "\n";
        }

        //
        const Nym* pServerNym = pServerContract->GetContractPublicNym();

        if ((nullptr == pServerNym) ||
            (false == pServerNym->VerifyPseudonym())) {
            otOut << "The server Nym was nullptr or failed to verify on server "
                     "contract: " << strNotaryID << "\n";
            return 0;
        }
        //

        if (bSendCommand && pServerNym->VerifyPseudonym()) {
            OTAPI_Wrap::OTAPI()->SendMessage(pServerContract, pMyNym,
                                             theMessage);

        } // if bSendCommand

        if (!opt->getFlag("prompt")) // If the user selected to enter the OT
                                     // prompt, then we
                                     // drop down below... (otherwise return.)
        {
            return 0;
        }
    } // Command line interface (versus below, which is the PROMPT interface.)

    otOut << "\nLOOKING FOR INSTRUCTIONS for the OT COMMAND LINE?\n"
             "Try:   quit\n"
             "Followed by:  ot -?\n"
             "or:    ot -h\n"
             "or:    ot --help\n"
             "\n"
             "(NOW ENTERING OT PROMPT) \n"
             "See docs/CLIENT-COMMANDS.txt\n\n";

    //
    //  THE OPEN-TRANSACTIONS PROMPT
    //
    // OT>
    //
    // Basically, loop:
    //
    // 1) Present a prompt, and get a user string of input. Wait for that.
    //
    // 2) Process it out as an OTMessage to the server. It goes down the pipe.
    //
    // 3) Sleep for 1 second.
    //
    // 4) Awake and check for messages to be read in response from the server.
    //    Loop. As long as there are any responses there, then process and
    // handle
    //    them all.
    //    Then continue back up to the prompt at step (1).

    Nym* pMyNym = nullptr;
    OTWallet* pWallet = nullptr;
    OTServerContract* pServerContract = nullptr;

    // If we got down here, that means there were no commands on the command
    // line
    // (That's why we dropped into the OT prompt.)
    // However, there still may have been OPTIONS -- and if so, we'll go ahead
    // and
    // load the wallet. (If there were NOT ANY OPTIONS, then we do NOT load the
    // wallet,
    // although there is a COMMAND for doing that.)
    //
    if ((str_NotaryID.size() > 0) || (str_MyNym.size() > 0)) {
        if (false ==
            SetupPointersForWalletMyNymAndServerContract(
                str_NotaryID, str_MyNym, pMyNym, pWallet, pServerContract)) {
            return 0;
        }
    }
    else
        otOut << "\nYou may wish to 'load' then 'stat'.\n"
                 "(FYI, --server NOTARY_ID  and  --mynym NYM_ID  were both "
                 "valid options.)\n"
                 "Also, see:  ~/.ot/command-line-ot.opt for defaults.\n";

    // Below this point, pWallet is available and loaded, IF opt->HasOptions().
    // Otherwise, pWallet is NOT loaded, and we're waiting for the Load command.

    // Below this point, pMyNym MIGHT be a valid pointer (if it was specified),
    // or MIGHT be nullptr. Same with pServerContract. (MIGHT be there.)
    //

    char buf[200] = "";

    otLog4 << "Starting client loop.\n";

// Set the logging level for the network transport code.
#ifndef _WIN32
// XmlRpc::setVerbosity(1);
#endif

    for (;;) {
        buf[0] = 0; // Making it fresh again.

        // 1) Present a prompt, and get a user string of input. Wait for that.
        otOut << "\nOT -- WARNING: This prompt is too low-level for you.\nType "
                 "'quit', and then try 'opentxs help' and 'opentxs "
                 "list'.\n\nOT> ";

        if (nullptr ==
            fgets(buf, 190, stdin)) // Leaving myself 10 extra bytes at the
                                    // end for safety's sake.
            break;

        otOut << ".\n..\n...\n....\n.....\n......\n.......\n........\n........."
                 "\n..........\n...........\n............\n.............\n";

        // so we can process the user input
        std::string strLine = buf;

        // Load wallet.xml
        if (strLine.compare(0, 4, "load") == 0) {
            otOut << "User has instructed to load wallet.xml...\n";

            if (!SetupPointersForWalletMyNymAndServerContract(
                    str_NotaryID, str_MyNym, pMyNym, pWallet,
                    pServerContract)) {
                return 0;
            }

            continue;
        }
        else if ('\0' == buf[0]) {
            continue;
        }
        else if (strLine.compare(0, 4, "test") == 0) {
            std::string strScript = "print(\"Hello, world\")";
            OT_ME madeEasy;
            OTAPI_Func::CopyVariables();
            madeEasy.ExecuteScript_ReturnVoid(strScript, "hardcoded");

            /*
                              // TODO: Make sure there's no issues with a known
         plaintext attack.
                              // (Not here, but I am doing a similar thing in
         OTASCIIArmor to maintain a
         minimum size,
                              // due to a bug in some other library that I can't
         recall at this time.)
                              //
                              const char * szBlah = "Transaction processor
         featuring Untraceable Digital
         Cash, "
                              "Anonymous Numbered Accounts, Triple-Signed
         Receipts, Basket Currencies,
         and Signed "
                              "XML Contracts. Also supports cheques, invoices,
         payment plans, markets
         with trades, "
                              "and other instruments... it's like PGP for
         Money.... Uses OpenSSL and
         Lucre blinded tokens.\n";

                        OTASCIIArmor theArmoredText(szBlah);
            otOut << "Armored text:\n" << theArmoredText << "\n";

            OTString theFixedText(theArmoredText);
            otOut << "Uncompressed, etc text:\n" << theFixedText << "\n";
         */

            continue;
        }
        else if (strLine.compare(0, 8, "clearreq") ==
                   0) // clear request numbers
        {
            if (nullptr == pMyNym) {
                otOut << "No Nym yet available. Try 'load'.\n";
                continue;
            }

            String strNotaryID;
            pServerContract->GetIdentifier(strNotaryID);

            otOut << "You are trying to mess around with your (clear your) "
                     "request numbers.\n"
                     "Enter the relevant server ID [" << strNotaryID << "]: ";

            std::string str_NotaryID = OT_CLI_ReadLine();

            const String strReqNumNotaryID((str_NotaryID.size() > 0)
                                               ? str_NotaryID.c_str()
                                               : strNotaryID.Get());

            pMyNym->RemoveReqNumbers(&strReqNumNotaryID);

            pMyNym->SaveSignedNymfile(*pMyNym);

            otOut << "Successfully removed request number for server "
                  << strReqNumNotaryID << ". Saving nym...\n";
            continue;
        }
        else if (strLine.compare(0, 5, "clear") == 0) {
            if (nullptr == pMyNym) {
                otOut << "No Nym yet available. Try 'load'.\n";
                continue;
            }

            String strNotaryID;
            pServerContract->GetIdentifier(strNotaryID);

            otOut << "You are trying to mess around with your (clear your) "
                     "transaction numbers.\n"
                     "Enter the relevant server ID [" << strNotaryID << "]: ";

            std::string str_NotaryID = OT_CLI_ReadLine();

            const String strTransNumNotaryID((str_NotaryID.size() > 0)
                                                 ? str_NotaryID.c_str()
                                                 : strNotaryID.Get());

            pMyNym->RemoveAllNumbers(&strTransNumNotaryID,
                                     true); // bRemoveHighestNum = true.
            pMyNym->SaveSignedNymfile(*pMyNym);

            otOut << "Successfully removed all issued and transaction "
                     "numbers for server " << strTransNumNotaryID
                  << ". Saving nym...\n";
            continue;
        }
        else if (strLine.compare(0, 7, "decrypt") == 0) {
            if (nullptr == pMyNym) {
                otOut << "No Nym yet available to decrypt with.\n";
                continue;
            }

            otOut << "Enter text to be decrypted:\n> ";

            OTASCIIArmor theArmoredText;
            char decode_buffer[200]; // Safe since we only read sizeof - 1

            do {
                decode_buffer[0] = 0;
                if (nullptr !=
                    fgets(decode_buffer, sizeof(decode_buffer) - 1, stdin)) {
                    theArmoredText.Concatenate("%s\n", decode_buffer);
                    otOut << "> ";
                }
                else {
                    break;
                }
            } while (strlen(decode_buffer) > 1);

            OTEnvelope theEnvelope(theArmoredText);
            String strDecodedText;

            theEnvelope.Open(*pMyNym, strDecodedText);

            otOut << "\n\nDECRYPTED TEXT:\n\n" << strDecodedText << "\n\n";

            continue;
        }
        else if (strLine.compare(0, 6, "decode") == 0) {
            otOut << "Enter text to be decoded:\n> ";

            OTASCIIArmor theArmoredText;
            char decode_buffer[200]; // Safe since we only read sizeof - 1.

            do {
                decode_buffer[0] = 0;
                if (nullptr !=
                    fgets(decode_buffer, sizeof(decode_buffer) - 1, stdin)) {
                    theArmoredText.Concatenate("%s\n", decode_buffer);
                    otOut << "> ";
                }
                else {
                    break;
                }

            } while (strlen(decode_buffer) > 1);

            String strDecodedText(theArmoredText);

            otOut << "\n\nDECODED TEXT:\n\n" << strDecodedText << "\n\n";

            continue;
        }
        else if (strLine.compare(0, 6, "encode") == 0) {
            otOut << "Enter text to be ascii-encoded (terminate with ~ on a "
                     "new line):\n> ";

            String strDecodedText;
            char decode_buffer[200]; // Safe since we only read sizeof - 1.

            do {
                decode_buffer[0] = 0;

                if ((nullptr !=
                     fgets(decode_buffer, sizeof(decode_buffer) - 1, stdin)) &&
                    (decode_buffer[0] != '~')) {
                    strDecodedText.Concatenate("%s", decode_buffer);
                    otOut << "> ";
                }
                else {
                    break;
                }

            } while (decode_buffer[0] != '~');

            OTASCIIArmor theArmoredText(strDecodedText);

            otOut << "\n\nENCODED TEXT:\n\n" << theArmoredText << "\n\n";

            continue;
        }
        else if (strLine.compare(0, 4, "hash") == 0) {
            otOut << "Enter text to be hashed (terminate with ~ on a "
                     "new line):\n> ";

            String strDecodedText;
            char decode_buffer[200]; // Safe since we only read sizeof - 1.

            do {
                decode_buffer[0] = 0;

                if ((nullptr !=
                     fgets(decode_buffer, sizeof(decode_buffer) - 1, stdin)) &&
                    (decode_buffer[0] != '~')) {
                    strDecodedText.Concatenate("%s\n", decode_buffer);
                    otOut << "> ";
                }
                else {
                    break;
                }

            } while (decode_buffer[0] != '~');

            std::string str_Trim(strDecodedText.Get());
            std::string str_Trim2 = String::trim(str_Trim);
            strDecodedText.Set(str_Trim2.c_str());

            Identifier theIdentifier;
            theIdentifier.CalculateDigest(strDecodedText);

            String strHash(theIdentifier);

            otOut << "\n\nMESSAGE DIGEST:\n\n" << strHash << "\n\n";

            continue;
        }
        else if (strLine.compare(0, 4, "stat") == 0) {
            otOut << "User has instructed to display wallet contents...\n";

            if (pWallet) {
                String strStat;
                pWallet->DisplayStatistics(strStat);
                otOut << strStat << "\n";
            }
            else
                otOut << "No wallet is loaded...\n";

            continue;
        }
        else if (strLine.compare(0, 4, "help") == 0) {
            otOut << "User has instructed to display the help file...\nPlease "
                     "see this file: docs/CLIENT_COMMANDS.txt\n";

            continue;
        }
        else if (strLine.compare(0, 4, "quit") == 0) {
            otOut << "User has instructed to exit the wallet...\n";

            break;
        }

        /*
                    --myacct   (ACCT ID)
                    --mynym    (NYM ID)
                    --mypurse  (ASSET TYPE ID)

                    --toacct   (ACCT ID)
                    --tonym    (NYM ID)
                    --topurse  (ASSET TYPE ID)

                    OTPseudonym *  GetNymByIDPartialMatch(const
      std::string PARTIAL_ID);
                    OTServerContract * GetServerContractPartialMatch(const
      std::string
      PARTIAL_ID);
                    OTAssetContract * GetAssetContractPartialMatch(const
      std::string
      PARTIAL_ID);
                    OTAccount *         GetAccountPartialMatch(const std::string
      PARTIAL_ID);
                    */

        if (nullptr == pServerContract) {
            otOut << "Unable to find a server contract. Please restart using "
                     "the option:  --server NOTARY_ID\n"
                     "(Where NOTARY_ID is the server ID. Partial matches ARE "
                     "accepted.)\n";
            continue;
        }

        // You can't just connect to any hostname and port.
        // Instead, you give me the Server Contract, and *I'll* look up all that
        // stuff FOR you...
        // (We verify this up here, but use it at the bottom of the function
        // once
        // the message is set up.)
        //
        // int32_t nServerPort = 0;
        // OTString strServerHostname;
        // if (!pServerContract->GetConnectInfo(strServerHostname,
        //     nServerPort))
        // {
        //     otErr << "Failed retrieving connection info from"
        //         "server contract.\n";
        //     continue;
        // }

        // I put this here too since I think it's required in all cases below.
        //
        if (nullptr ==
            pMyNym) // Todo maybe move this check to the commands below
                    // (ONLY the ones that use a nym.)
        {
            otOut
                << "Unable to find My Nym. Please restart and use the option:\n"
                   "   --mynym NYM_ID\n"
                   "(Where NYM_ID is the Nym's ID. Partial matches ARE "
                   "accepted.)\n";
            continue;
        }

        bool bSendCommand = false; // Determines whether to actually send a
                                   // message to the server.

        Message theMessage;

        // 'check server ID' command
        if (buf[0] == 'c') {
            otOut << "(User has instructed to send a pingNotary command "
                     "to the server...)\n";

            // if successful setting up the command payload...

            if (0 < OTAPI_Wrap::OTAPI()->GetClient()->ProcessUserCommand(
                        OTClient::pingNotary, theMessage, *pMyNym,
                        *pServerContract,
                        nullptr)) // nullptr pAccount on this command (so far).
            {
                bSendCommand = true;
            }
            else
                otErr << "Error processing pingNotary command in "
                         "ProcessMessage: " << buf[0] << "\n";

        }

        // register new user account
        else if (buf[0] == 'r') {
            otOut << "(User has instructed to send a registerNym "
                     "command to the server...)\n";

            // if successful setting up the command payload...

            if (0 < OTAPI_Wrap::OTAPI()->GetClient()->ProcessUserCommand(
                        OTClient::registerNym, theMessage, *pMyNym,
                        *pServerContract,
                        nullptr)) // nullptr pAccount on this command.
            {
                bSendCommand = true;
            }
            else
                otErr << "Error processing registerNym command in "
                         "ProcessMessage: " << buf[0] << "\n";

        }

        // ALL MESSAGES BELOW THIS POINT SHOULD ATTACH A REQUEST NUMBER IF THEY
        // EXPECT THE SERVER TO PROCESS THEM.
        // (Handled inside ProcessUserCommand)

        // Nym, Account, Notary ID, Server Contract

        // deposit cheque
        else if (buf[0] == 'q') {
            otOut << "User has instructed to deposit a cheque...\n";

            // if successful setting up the command payload...

            if (0 < OTAPI_Wrap::OTAPI()->GetClient()->ProcessUserCommand(
                        OTClient::notarizeCheque, theMessage, *pMyNym,
                        *pServerContract, nullptr)) {
                bSendCommand = true;
            }
            else
                otErr << "Error processing deposit cheque command in "
                         "ProcessMessage: " << buf[0] << "\n";

        }

        // deposit purse
        else if (buf[0] == 'p') {
            otOut << "(User has instructed to deposit a purse "
                     "containing cash...)\n";

            // if successful setting up the command payload...

            if (0 < OTAPI_Wrap::OTAPI()->GetClient()->ProcessUserCommand(
                        OTClient::notarizePurse, theMessage, *pMyNym,
                        *pServerContract, nullptr)) {
                bSendCommand = true;
            }
            else
                otErr << "Error processing deposit command in ProcessMessage: "
                      << buf[0] << "\n";

        }

        // activate payment plan
        else if (!strcmp(buf, "activate\n")) {
            otOut << "User has instructed to activate a payment plan...\n";

            // if successful setting up the command payload...

            if (0 < OTAPI_Wrap::OTAPI()->GetClient()->ProcessUserCommand(
                        OTClient::paymentPlan, theMessage, *pMyNym,
                        *pServerContract,
                        nullptr)) // nullptr pAccount on this command.
            {
                bSendCommand = true;
            }
            else
                otErr << "Error processing payment plan command in "
                         "ProcessMessage: " << buf[0] << "\n";

        }
        else if (!strcmp(buf, "cheque\n")) {
            otOut << "(User has instructed to write a cheque...)\n";

            OTAPI_Wrap::OTAPI()->GetClient()->ProcessUserCommand(
                OTClient::writeCheque, theMessage, *pMyNym, *pServerContract,
                nullptr); // It will ascertain the account inside the call.
            continue;
        }

        // getRequestNumber
        else if (buf[0] == 'g') {
            otOut
                << "(User has instructed to send a getRequestNumber command to "
                   "the server...)\n";

            // if successful setting up the command payload...

            if (0 < OTAPI_Wrap::OTAPI()->GetClient()->ProcessUserCommand(
                        OTClient::getRequestNumber, theMessage, *pMyNym,
                        *pServerContract,
                        nullptr)) // nullptr pAccount on this command.
            {
                bSendCommand = true;
            }
            else
                otErr << "Error processing getRequestNumber command in "
                         "ProcessMessage: " << buf[0] << "\n";

        }

        // getTransactionNumbers
        else if (buf[0] == 'n') {
            // I just coded (here) for myself a secret option (for testing)...
            // Optionally instead of JUST 'n', I can put n <number>, (without
            // brackets) and
            // this code will add that number to my list of issued and
            // transaction
            // numbers.
            // I already have the ability to clear the list, so now I can add
            // numbers
            // to it as well.
            // (Which adds to both lists.)
            // I can also remove a number from the transaction list but LEAVE it
            // on
            // the issued list,
            // for example by writing a cheque and throwing it away.
            //
            // This code is for testing and allows me to find and patch any
            // problems
            // without
            // having to re-create my data each time -- speeds up debugging.
            //
            int64_t lTransactionNumber =
                ((strlen(buf) > 2) ? String::StringToLong(&(buf[2])) : 0);

            if (lTransactionNumber > 0) {
                String strNotaryID;
                pServerContract->GetIdentifier(strNotaryID);

                otOut << "You are trying to mess around with your (add to "
                         "your) transaction numbers.\n"
                         "Enter the relevant server ID [" << strNotaryID
                      << "]: ";

                std::string str_NotaryID = OT_CLI_ReadLine();

                const String strTransNumNotaryID((str_NotaryID.size() > 0)
                                                     ? str_NotaryID.c_str()
                                                     : strNotaryID.Get());

                pMyNym->AddTransactionNum(*pMyNym, strTransNumNotaryID,
                                          lTransactionNumber,
                                          true); // bool bSave=true

                otOut << "Transaction number " << lTransactionNumber
                      << " added to both lists "
                         "(on client side.)\n";
            }
            else {
                otOut << "(User has instructed to send a getTransactionNumbers "
                         "command to the server...)\n";

                // if successful setting up the command payload...

                if (0 < OTAPI_Wrap::OTAPI()->GetClient()->ProcessUserCommand(
                            OTClient::getTransactionNumbers, theMessage,
                            *pMyNym, *pServerContract,
                            nullptr)) // nullptr pAccount on this command.
                {
                    bSendCommand = true;
                }
                else
                    otErr
                        << "Error processing getTransactionNumbers command in "
                           "ProcessMessage: " << buf[0] << "\n";
            }

        }
        else {
            {
                // gDebugLog.Write("unknown user command in ProcessMessage in
                // main.cpp");
                otOut << "\n";
                // otErr << "unknown user command in ProcessMessage in main.cpp:
                // " << buf[0] << "\n";
            }
            continue;
        }

        const Nym* pServerNym = pServerContract->GetContractPublicNym();

        if (bSendCommand && (nullptr != pServerNym) &&
            pServerNym->VerifyPseudonym()) {
            OTAPI_Wrap::OTAPI()->SendMessage(pServerContract, pMyNym,
                                             theMessage);

        } // if bSendCommand
    }     // for

    otOut << "Exiting OT prompt.\n";

    // NOTE: Cleanup is handled via a nested class at the top of this main
    // function.

    return 0;
}
