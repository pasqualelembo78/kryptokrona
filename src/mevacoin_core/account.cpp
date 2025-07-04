// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "account.h"
#include "mevacoin_serialization.h"
#include "crypto/keccak.c"

namespace mevacoin
{
    //-----------------------------------------------------------------
    AccountBase::AccountBase()
    {
        setNull();
    }
    //-----------------------------------------------------------------
    void AccountBase::setNull()
    {
        m_keys = AccountKeys();
    }
    //-----------------------------------------------------------------
    void AccountBase::generate()
    {

        crypto::generate_keys(m_keys.address.spendPublicKey, m_keys.spendSecretKey);

        /* We derive the view secret key by taking our spend secret key, hashing
           with keccak-256, and then using this as the seed to generate a new set
           of keys - the public and private view keys. See generate_deterministic_keys */

        generateViewFromSpend(m_keys.spendSecretKey, m_keys.viewSecretKey, m_keys.address.viewPublicKey);
        m_creation_timestamp = time(NULL);
    }
    void AccountBase::generateViewFromSpend(const crypto::SecretKey &spend, crypto::SecretKey &viewSecret, crypto::PublicKey &viewPublic)
    {
        crypto::SecretKey viewKeySeed;

        keccak((uint8_t *)&spend, sizeof(spend), (uint8_t *)&viewKeySeed, sizeof(viewKeySeed));

        crypto::generate_deterministic_keys(viewPublic, viewSecret, viewKeySeed);
    }

    void AccountBase::generateViewFromSpend(const crypto::SecretKey &spend, crypto::SecretKey &viewSecret)
    {
        /* If we don't need the pub key */
        crypto::PublicKey unused_dummy_variable;
        generateViewFromSpend(spend, viewSecret, unused_dummy_variable);
    }
    //-----------------------------------------------------------------
    const AccountKeys &AccountBase::getAccountKeys() const
    {
        return m_keys;
    }
    //-----------------------------------------------------------------

    void AccountBase::serialize(ISerializer &s)
    {
        s(m_keys, "m_keys");
        s(m_creation_timestamp, "m_creation_timestamp");
    }
}
