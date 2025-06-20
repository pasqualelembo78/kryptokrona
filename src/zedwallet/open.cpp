// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

///////////////////////////
#include <zedwallet/open.h>
///////////////////////////

#include <mevacoin_core/account.h>
#include <mevacoin_core/mevacoin_basic_impl.h>

#include <mnemonics/mnemonics.h>

#include <wallet/wallet_errors.h>

#include <utilities/coloured_msg.h>
#include <zedwallet/command_implementations.h>
#include <zedwallet/tools.h>
#include <zedwallet/transfer.h>
#include <zedwallet/types.h>
#include <zedwallet/password_container.h>
#include <config/wallet_config.h>

std::shared_ptr<WalletInfo> createViewWallet(mevacoin::WalletGreen &wallet)
{
    std::cout << WarningMsg("View wallets are only for viewing incoming ")
              << WarningMsg("transactions, and cannot make transfers.")
              << std::endl;

    bool create = confirm("Is this OK?");

    std::cout << std::endl;

    if (!create)
    {
        return nullptr;
    }

    crypto::SecretKey privateViewKey = getPrivateKey("Private View Key: ");

    std::string address;

    while (true)
    {
        std::cout << InformationMsg("Enter your public ")
                  << InformationMsg(wallet_config::ticker)
                  << InformationMsg(" address: ");

        std::getline(std::cin, address);
        trim(address);

        if (parseStandardAddress(address, true))
        {
            break;
        }
    }

    const std::string walletFileName = getNewWalletFileName();

    const std::string msg = "Give your new wallet a password: ";

    const std::string walletPass = getWalletPassword(true, msg);

    const uint64_t scanHeight = getScanHeight();

    wallet.createViewWallet(walletFileName, walletPass, address,
                            privateViewKey, scanHeight, false);

    std::cout << std::endl
              << InformationMsg("Your view wallet ")
              << InformationMsg(address)
              << InformationMsg(" has been successfully imported!")
              << std::endl
              << std::endl;

    viewWalletMsg();

    return std::make_shared<WalletInfo>(walletFileName, walletPass,
                                        address, true, wallet);
}

std::shared_ptr<WalletInfo> importWallet(mevacoin::WalletGreen &wallet)
{
    const crypto::SecretKey privateSpendKey = getPrivateKey("Enter your private spend key: ");

    const crypto::SecretKey privateViewKey = getPrivateKey("Enter your private view key: ");

    return importFromKeys(wallet, privateSpendKey, privateViewKey);
}

std::shared_ptr<WalletInfo> mnemonicImportWallet(mevacoin::WalletGreen
                                                     &wallet)
{
    while (true)
    {
        std::cout << InformationMsg("Enter your mnemonic phrase (25 words): ");

        std::string mnemonicPhrase;

        std::getline(std::cin, mnemonicPhrase);

        trim(mnemonicPhrase);

        auto [error, privateSpendKey] = Mnemonics::MnemonicToPrivateKey(mnemonicPhrase);

        if (error)
        {
            std::cout << std::endl
                      << WarningMsg(error.getErrorMessage())
                      << std::endl
                      << std::endl;
        }
        else
        {
            crypto::SecretKey privateViewKey;

            mevacoin::AccountBase::generateViewFromSpend(
                privateSpendKey, privateViewKey);

            return importFromKeys(wallet, privateSpendKey, privateViewKey);
        }
    }
}

std::shared_ptr<WalletInfo> importFromKeys(mevacoin::WalletGreen &wallet,
                                           crypto::SecretKey privateSpendKey,
                                           crypto::SecretKey privateViewKey)
{
    const std::string walletFileName = getNewWalletFileName();

    const std::string msg = "Give your new wallet a password: ";

    const std::string walletPass = getWalletPassword(true, msg);

    const uint64_t scanHeight = getScanHeight();

    connectingMsg();

    wallet.initializeWithViewKey(
        walletFileName, walletPass, privateViewKey, scanHeight, false);

    const std::string walletAddress = wallet.createAddress(
        privateSpendKey, scanHeight, false);

    std::cout << std::endl
              << InformationMsg("Your wallet ")
              << InformationMsg(walletAddress)
              << InformationMsg(" has been successfully imported!")
              << std::endl
              << std::endl;

    return std::make_shared<WalletInfo>(walletFileName, walletPass,
                                        walletAddress, false, wallet);
}

std::shared_ptr<WalletInfo> generateWallet(mevacoin::WalletGreen &wallet)
{
    const std::string walletFileName = getNewWalletFileName();

    const std::string msg = "Give your new wallet a password: ";

    const std::string walletPass = getWalletPassword(true, msg);

    mevacoin::KeyPair spendKey;
    crypto::SecretKey privateViewKey;

    crypto::generate_keys(spendKey.publicKey, spendKey.secretKey);

    mevacoin::AccountBase::generateViewFromSpend(spendKey.secretKey,
                                                   privateViewKey);

    wallet.initializeWithViewKey(
        walletFileName, walletPass, privateViewKey, 0, true);

    const std::string walletAddress = wallet.createAddress(
        spendKey.secretKey, 0, true);

    promptSaveKeys(wallet);

    std::cout << WarningMsg("If you lose these your wallet cannot be ")
              << WarningMsg("recreated!")
              << std::endl
              << std::endl;

    return std::make_shared<WalletInfo>(walletFileName, walletPass,
                                        walletAddress, false, wallet);
}

std::shared_ptr<WalletInfo> openWallet(mevacoin::WalletGreen &wallet,
                                       Config &config)
{
    const std::string walletFileName = getExistingWalletFileName(config);

    bool initial = true;

    while (true)
    {
        std::string walletPass;

        /* Only use the command line pass once, otherwise we will infinite
           loop if it is incorrect */
        if (initial && config.passGiven)
        {
            walletPass = config.walletPass;
        }
        else
        {
            walletPass = getWalletPassword(false, "Enter password: ");
        }

        initial = false;

        connectingMsg();

        try
        {
            wallet.load(walletFileName, walletPass);

            const std::string walletAddress = wallet.getAddress(0);

            const crypto::SecretKey privateSpendKey = wallet.getAddressSpendKey(0).secretKey;

            bool viewWallet = false;

            if (privateSpendKey == mevacoin::NULL_SECRET_KEY)
            {
                std::cout << std::endl
                          << InformationMsg("Your view only wallet ")
                          << InformationMsg(walletAddress)
                          << InformationMsg(" has been successfully opened!")
                          << std::endl
                          << std::endl;

                viewWalletMsg();

                viewWallet = true;
            }
            else
            {
                std::cout << std::endl
                          << InformationMsg("Your wallet ")
                          << InformationMsg(walletAddress)
                          << InformationMsg(" has been successfully opened!")
                          << std::endl
                          << std::endl;
            }

            return std::make_shared<WalletInfo>(
                walletFileName, walletPass, walletAddress, viewWallet, wallet);
        }
        catch (const std::system_error &e)
        {
            bool handled = false;

            switch (e.code().value())
            {
            case mevacoin::error::WRONG_PASSWORD:
            {
                std::cout << std::endl
                          << WarningMsg("Incorrect password! Try again.")
                          << std::endl
                          << std::endl;

                handled = true;

                break;
            }
            case mevacoin::error::WRONG_VERSION:
            {
                std::stringstream msg;

                msg << "Could not open wallet file! It doesn't appear "
                    << "to be a valid wallet!" << std::endl
                    << "Ensure you are opening a wallet file, and the "
                    << "file has not gotten corrupted." << std::endl
                    << "Try reimporting via keys, and always close "
                    << wallet_config::walletName << " with the exit "
                    << "command to prevent corruption." << std::endl;

                std::cout << WarningMsg(msg.str()) << std::endl;

                return nullptr;
            }
            }

            if (handled)
            {
                continue;
            }

            const std::string alreadyOpenMsg =
                "MemoryMappedFile::open: The process cannot access the file "
                "because it is being used by another process.";

            const std::string errorMsg = e.what();

            /* The message actually has a \r\n on the end but I'd prefer to
               keep just the raw string in the source so check if it starts
               with the message instead */
            if (startsWith(errorMsg, alreadyOpenMsg))
            {
                std::cout << WarningMsg("Could not open wallet! It is already "
                                        "open in another process.")
                          << std::endl
                          << WarningMsg("Check with a task manager that you "
                                        "don't have ")
                          << wallet_config::walletName
                          << WarningMsg(" open twice.")
                          << std::endl
                          << WarningMsg("Also check you don't have another "
                                        "wallet program open, such as a GUI "
                                        "wallet or ")
                          << WarningMsg(wallet_config::walletdName)
                          << WarningMsg(".")
                          << std::endl
                          << std::endl;

                return nullptr;
            }
            else
            {
                std::cout << "Unexpected error: " << errorMsg << std::endl;
                std::cout << "Please report this error message and what "
                          << "you did to cause it." << std::endl
                          << std::endl;

                wallet.shutdown();
                return nullptr;
            }
        }
    }
}

crypto::SecretKey getPrivateKey(std::string msg)
{
    const uint64_t privateKeyLen = 64;
    uint64_t size;

    std::string privateKeyString;
    crypto::Hash privateKeyHash;
    crypto::SecretKey privateKey;
    crypto::PublicKey publicKey;

    while (true)
    {
        std::cout << InformationMsg(msg);

        std::getline(std::cin, privateKeyString);
        trim(privateKeyString);

        if (privateKeyString.length() != privateKeyLen)
        {
            std::cout << std::endl
                      << WarningMsg("Invalid private key, should be 64 ")
                      << WarningMsg("characters! Try again.") << std::endl
                      << std::endl;

            continue;
        }
        else if (!common::fromHex(privateKeyString, &privateKeyHash,
                                  sizeof(privateKeyHash), size) ||
                 size != sizeof(privateKeyHash))
        {
            std::cout << WarningMsg("Invalid private key, it is not a valid ")
                      << WarningMsg("hex string! Try again.")
                      << std::endl
                      << std::endl;

            continue;
        }

        privateKey = *(struct crypto::SecretKey *)&privateKeyHash;

        /* Just used for verification purposes before we pass it to
           walletgreen */
        if (!crypto::secret_key_to_public_key(privateKey, publicKey))
        {
            std::cout << std::endl
                      << WarningMsg("Invalid private key, is not on the ")
                      << WarningMsg("ed25519 curve!") << std::endl
                      << WarningMsg("Probably a typo - ensure you entered ")
                      << WarningMsg("it correctly.")
                      << std::endl
                      << std::endl;

            continue;
        }

        return privateKey;
    }
}

std::string getExistingWalletFileName(Config &config)
{
    bool initial = true;

    std::string walletName;

    while (true)
    {
        /* Only use wallet file once in case it is incorrect */
        if (config.walletGiven && initial)
        {
            walletName = config.walletFile;
        }
        else
        {
            std::cout << InformationMsg("What is the name of the wallet ")
                      << InformationMsg("you want to open?: ");

            std::getline(std::cin, walletName);
        }

        initial = false;

        const std::string walletFileName = walletName + ".wallet";

        if (walletName == "")
        {
            std::cout << std::endl
                      << WarningMsg("Wallet name can't be blank! Try again.")
                      << std::endl
                      << std::endl;
        }
        /* Allow people to enter wallet name with or without file extension */
        else if (fileExists(walletName))
        {
            return walletName;
        }
        else if (fileExists(walletFileName))
        {
            return walletFileName;
        }
        else
        {
            std::cout << std::endl
                      << WarningMsg("A wallet with the filename ")
                      << InformationMsg(walletName)
                      << WarningMsg(" or ")
                      << InformationMsg(walletFileName)
                      << WarningMsg(" doesn't exist!")
                      << std::endl
                      << "Ensure you entered your wallet name correctly."
                      << std::endl
                      << std::endl;
        }
    }
}

std::string getNewWalletFileName()
{
    std::string walletName;

    while (true)
    {
        std::cout << InformationMsg("What would you like to call your ")
                  << InformationMsg("new wallet?: ");

        std::getline(std::cin, walletName);

        const std::string walletFileName = walletName + ".wallet";

        if (fileExists(walletFileName))
        {
            std::cout << std::endl
                      << WarningMsg("A wallet with the filename ")
                      << InformationMsg(walletFileName)
                      << WarningMsg(" already exists!")
                      << std::endl
                      << "Try another name." << std::endl
                      << std::endl;
        }
        else if (walletName == "")
        {
            std::cout << std::endl
                      << WarningMsg("Wallet name can't be blank! Try again.")
                      << std::endl
                      << std::endl;
        }
        else
        {
            return walletFileName;
        }
    }
}

std::string getWalletPassword(bool verifyPwd, std::string msg)
{
    tools::PasswordContainer pwdContainer;
    pwdContainer.read_password(verifyPwd, msg);
    return pwdContainer.password();
}

void viewWalletMsg()
{
    std::cout << InformationMsg("Please remember that when using a view wallet "
                                "you can only view incoming transactions!")
              << std::endl
              << InformationMsg("Therefore, if you have recieved transactions ")
              << InformationMsg("which you then spent, your balance will ")
              << InformationMsg("appear inflated.") << std::endl;
}

void connectingMsg()
{
    std::cout << std::endl
              << "Making initial contact with "
              << wallet_config::daemonName
              << "."
              << std::endl
              << "Please wait, this sometimes can take a long time..."
              << std::endl
              << std::endl;
}

void promptSaveKeys(mevacoin::WalletGreen &wallet)
{
    std::cout << "Welcome to your new wallet, here is your payment address:"
              << std::endl
              << InformationMsg(wallet.getAddress(0))
              << std::endl
              << std::endl
              << "Please copy your secret keys and mnemonic seed and store "
              << "them in a secure location: " << std::endl;

    printPrivateKeys(wallet, false);

    std::cout << std::endl;
}
