// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2016-2018, The Karbowanec developers
// Copyright (c) 2018-2019, The TurtleCoin Developers
//
// Please see the included LICENSE file for more information.

#include <alloca.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "common/varint.h"
#include "crypto.h"
#include "hash.h"
#include "random.h"

namespace crypto
{

    extern "C"
    {
#include "crypto-ops.h"
    }

    static inline void random_scalar(EllipticCurveScalar &res)
    {
        unsigned char tmp[64];
        rnd::randomBytes(64, tmp);
        sc_reduce(tmp);
        memcpy(&res, tmp, 32);
    }

    static inline void hash_to_scalar(const void *data, size_t length, EllipticCurveScalar &res)
    {
        cn_fast_hash(data, length, reinterpret_cast<Hash &>(res));
        sc_reduce32(reinterpret_cast<unsigned char *>(&res));
    }

    void crypto_ops::generate_keys(PublicKey &pub, SecretKey &sec)
    {
        ge_p3 point;
        random_scalar(reinterpret_cast<EllipticCurveScalar &>(sec));
        ge_scalarmult_base(&point, reinterpret_cast<unsigned char *>(&sec));
        ge_p3_tobytes(reinterpret_cast<unsigned char *>(&pub), &point);
    }

    void crypto_ops::generate_deterministic_keys(PublicKey &pub, SecretKey &sec, SecretKey &second)
    {
        ge_p3 point;
        sec = second;
        sc_reduce32(reinterpret_cast<unsigned char *>(&sec)); // reduce in case second round of keys (sendkeys)
        ge_scalarmult_base(&point, reinterpret_cast<unsigned char *>(&sec));
        ge_p3_tobytes(reinterpret_cast<unsigned char *>(&pub), &point);
    }

    SecretKey crypto_ops::generate_m_keys(PublicKey &pub, SecretKey &sec, const SecretKey &recovery_key, bool recover)
    {
        ge_p3 point;
        SecretKey rng;
        if (recover)
        {
            rng = recovery_key;
        }
        else
        {
            random_scalar(reinterpret_cast<EllipticCurveScalar &>(rng));
        }
        sec = rng;
        sc_reduce32(reinterpret_cast<unsigned char *>(&sec)); // reduce in case second round of keys (sendkeys)
        ge_scalarmult_base(&point, reinterpret_cast<unsigned char *>(&sec));
        ge_p3_tobytes(reinterpret_cast<unsigned char *>(&pub), &point);

        return rng;
    }

    bool crypto_ops::check_key(const PublicKey &key)
    {
        ge_p3 point;
        return ge_frombytes_vartime(&point, reinterpret_cast<const unsigned char *>(&key)) == 0;
    }

    bool crypto_ops::secret_key_to_public_key(const SecretKey &sec, PublicKey &pub)
    {
        ge_p3 point;
        if (sc_check(reinterpret_cast<const unsigned char *>(&sec)) != 0)
        {
            return false;
        }
        ge_scalarmult_base(&point, reinterpret_cast<const unsigned char *>(&sec));
        ge_p3_tobytes(reinterpret_cast<unsigned char *>(&pub), &point);
        return true;
    }

    bool crypto_ops::generate_key_derivation(const PublicKey &key1, const SecretKey &key2, KeyDerivation &derivation)
    {
        ge_p3 point;
        ge_p2 point2;
        ge_p1p1 point3;
        assert(sc_check(reinterpret_cast<const unsigned char *>(&key2)) == 0);
        if (ge_frombytes_vartime(&point, reinterpret_cast<const unsigned char *>(&key1)) != 0)
        {
            return false;
        }
        ge_scalarmult(&point2, reinterpret_cast<const unsigned char *>(&key2), &point);
        ge_mul8(&point3, &point2);
        ge_p1p1_to_p2(&point2, &point3);
        ge_tobytes(reinterpret_cast<unsigned char *>(&derivation), &point2);
        return true;
    }

    static void derivation_to_scalar(const KeyDerivation &derivation, size_t output_index, EllipticCurveScalar &res)
    {
        struct
        {
            KeyDerivation derivation;
            char output_index[(sizeof(size_t) * 8 + 6) / 7];
        } buf;
        char *end = buf.output_index;
        buf.derivation = derivation;
        tools::write_varint(end, output_index);
        assert(end <= buf.output_index + sizeof buf.output_index);
        hash_to_scalar(&buf, end - reinterpret_cast<char *>(&buf), res);
    }

    static void derivation_to_scalar(const KeyDerivation &derivation, size_t output_index, const uint8_t *suffix, size_t suffixLength, EllipticCurveScalar &res)
    {
        assert(suffixLength <= 32);
        struct
        {
            KeyDerivation derivation;
            char output_index[(sizeof(size_t) * 8 + 6) / 7 + 32];
        } buf;
        char *end = buf.output_index;
        buf.derivation = derivation;
        tools::write_varint(end, output_index);
        assert(end <= buf.output_index + sizeof buf.output_index);
        size_t bufSize = end - reinterpret_cast<char *>(&buf);
        memcpy(end, suffix, suffixLength);
        hash_to_scalar(&buf, bufSize + suffixLength, res);
    }

    bool crypto_ops::derive_public_key(const KeyDerivation &derivation, size_t output_index,
                                       const PublicKey &base, PublicKey &derived_key)
    {
        EllipticCurveScalar scalar;
        ge_p3 point1;
        ge_p3 point2;
        ge_cached point3;
        ge_p1p1 point4;
        ge_p2 point5;
        if (ge_frombytes_vartime(&point1, reinterpret_cast<const unsigned char *>(&base)) != 0)
        {
            return false;
        }
        derivation_to_scalar(derivation, output_index, scalar);
        ge_scalarmult_base(&point2, reinterpret_cast<unsigned char *>(&scalar));
        ge_p3_to_cached(&point3, &point2);
        ge_add(&point4, &point1, &point3);
        ge_p1p1_to_p2(&point5, &point4);
        ge_tobytes(reinterpret_cast<unsigned char *>(&derived_key), &point5);
        return true;
    }

    bool crypto_ops::derive_public_key(const KeyDerivation &derivation, size_t output_index,
                                       const PublicKey &base, const uint8_t *suffix, size_t suffixLength, PublicKey &derived_key)
    {
        EllipticCurveScalar scalar;
        ge_p3 point1;
        ge_p3 point2;
        ge_cached point3;
        ge_p1p1 point4;
        ge_p2 point5;
        if (ge_frombytes_vartime(&point1, reinterpret_cast<const unsigned char *>(&base)) != 0)
        {
            return false;
        }
        derivation_to_scalar(derivation, output_index, suffix, suffixLength, scalar);
        ge_scalarmult_base(&point2, reinterpret_cast<unsigned char *>(&scalar));
        ge_p3_to_cached(&point3, &point2);
        ge_add(&point4, &point1, &point3);
        ge_p1p1_to_p2(&point5, &point4);
        ge_tobytes(reinterpret_cast<unsigned char *>(&derived_key), &point5);
        return true;
    }

    bool crypto_ops::underive_public_key_and_get_scalar(const KeyDerivation &derivation, size_t output_index,
                                                        const PublicKey &derived_key, PublicKey &base, EllipticCurveScalar &hashed_derivation)
    {
        ge_p3 point1;
        ge_p3 point2;
        ge_cached point3;
        ge_p1p1 point4;
        ge_p2 point5;
        if (ge_frombytes_vartime(&point1, reinterpret_cast<const unsigned char *>(&derived_key)) != 0)
        {
            return false;
        }
        derivation_to_scalar(derivation, output_index, hashed_derivation);
        ge_scalarmult_base(&point2, reinterpret_cast<unsigned char *>(&hashed_derivation));
        ge_p3_to_cached(&point3, &point2);
        ge_sub(&point4, &point1, &point3);
        ge_p1p1_to_p2(&point5, &point4);
        ge_tobytes(reinterpret_cast<unsigned char *>(&base), &point5);
        return true;
    }

    void crypto_ops::derive_secret_key(const KeyDerivation &derivation, size_t output_index,
                                       const SecretKey &base, SecretKey &derived_key)
    {
        EllipticCurveScalar scalar;
        assert(sc_check(reinterpret_cast<const unsigned char *>(&base)) == 0);
        derivation_to_scalar(derivation, output_index, scalar);
        sc_add(reinterpret_cast<unsigned char *>(&derived_key), reinterpret_cast<const unsigned char *>(&base), reinterpret_cast<unsigned char *>(&scalar));
    }

    void crypto_ops::derive_secret_key(const KeyDerivation &derivation, size_t output_index,
                                       const SecretKey &base, const uint8_t *suffix, size_t suffixLength, SecretKey &derived_key)
    {
        EllipticCurveScalar scalar;
        assert(sc_check(reinterpret_cast<const unsigned char *>(&base)) == 0);
        derivation_to_scalar(derivation, output_index, suffix, suffixLength, scalar);
        sc_add(reinterpret_cast<unsigned char *>(&derived_key), reinterpret_cast<const unsigned char *>(&base), reinterpret_cast<unsigned char *>(&scalar));
    }

    bool crypto_ops::underive_public_key(const KeyDerivation &derivation, size_t output_index,
                                         const PublicKey &derived_key, PublicKey &base)
    {
        EllipticCurveScalar scalar;
        ge_p3 point1;
        ge_p3 point2;
        ge_cached point3;
        ge_p1p1 point4;
        ge_p2 point5;
        if (ge_frombytes_vartime(&point1, reinterpret_cast<const unsigned char *>(&derived_key)) != 0)
        {
            return false;
        }
        derivation_to_scalar(derivation, output_index, scalar);
        ge_scalarmult_base(&point2, reinterpret_cast<unsigned char *>(&scalar));
        ge_p3_to_cached(&point3, &point2);
        ge_sub(&point4, &point1, &point3);
        ge_p1p1_to_p2(&point5, &point4);
        ge_tobytes(reinterpret_cast<unsigned char *>(&base), &point5);
        return true;
    }

    bool crypto_ops::underive_public_key(const KeyDerivation &derivation, size_t output_index,
                                         const PublicKey &derived_key, const uint8_t *suffix, size_t suffixLength, PublicKey &base)
    {
        EllipticCurveScalar scalar;
        ge_p3 point1;
        ge_p3 point2;
        ge_cached point3;
        ge_p1p1 point4;
        ge_p2 point5;
        if (ge_frombytes_vartime(&point1, reinterpret_cast<const unsigned char *>(&derived_key)) != 0)
        {
            return false;
        }

        derivation_to_scalar(derivation, output_index, suffix, suffixLength, scalar);
        ge_scalarmult_base(&point2, reinterpret_cast<unsigned char *>(&scalar));
        ge_p3_to_cached(&point3, &point2);
        ge_sub(&point4, &point1, &point3);
        ge_p1p1_to_p2(&point5, &point4);
        ge_tobytes(reinterpret_cast<unsigned char *>(&base), &point5);
        return true;
    }

    struct s_comm
    {
        Hash h;
        EllipticCurvePoint key;
        EllipticCurvePoint comm;
    };

    void crypto_ops::generate_signature(const Hash &prefix_hash, const PublicKey &pub, const SecretKey &sec, Signature &sig)
    {
        ge_p3 tmp3;
        EllipticCurveScalar k;
        s_comm buf;
#if !defined(NDEBUG)
        {
            ge_p3 t;
            PublicKey t2;
            assert(sc_check(reinterpret_cast<const unsigned char *>(&sec)) == 0);
            ge_scalarmult_base(&t, reinterpret_cast<const unsigned char *>(&sec));
            ge_p3_tobytes(reinterpret_cast<unsigned char *>(&t2), &t);
            assert(pub == t2);
        }
#endif
        buf.h = prefix_hash;
        buf.key = reinterpret_cast<const EllipticCurvePoint &>(pub);
        random_scalar(k);
        ge_scalarmult_base(&tmp3, reinterpret_cast<unsigned char *>(&k));
        ge_p3_tobytes(reinterpret_cast<unsigned char *>(&buf.comm), &tmp3);
        hash_to_scalar(&buf, sizeof(s_comm), reinterpret_cast<EllipticCurveScalar &>(sig));
        sc_mulsub(reinterpret_cast<unsigned char *>(&sig) + 32, reinterpret_cast<unsigned char *>(&sig), reinterpret_cast<const unsigned char *>(&sec), reinterpret_cast<unsigned char *>(&k));
    }

    bool crypto_ops::check_signature(const Hash &prefix_hash, const PublicKey &pub, const Signature &sig)
    {
        ge_p2 tmp2;
        ge_p3 tmp3;
        EllipticCurveScalar c;
        s_comm buf;
        assert(check_key(pub));
        buf.h = prefix_hash;
        buf.key = reinterpret_cast<const EllipticCurvePoint &>(pub);
        if (ge_frombytes_vartime(&tmp3, reinterpret_cast<const unsigned char *>(&pub)) != 0)
        {
            return false;
        }
        if (sc_check(reinterpret_cast<const unsigned char *>(&sig)) != 0 || sc_check(reinterpret_cast<const unsigned char *>(&sig) + 32) != 0)
        {
            return false;
        }
        ge_double_scalarmult_base_vartime(&tmp2, reinterpret_cast<const unsigned char *>(&sig), &tmp3, reinterpret_cast<const unsigned char *>(&sig) + 32);
        ge_tobytes(reinterpret_cast<unsigned char *>(&buf.comm), &tmp2);
        hash_to_scalar(&buf, sizeof(s_comm), c);
        sc_sub(reinterpret_cast<unsigned char *>(&c), reinterpret_cast<unsigned char *>(&c), reinterpret_cast<const unsigned char *>(&sig));
        return sc_isnonzero(reinterpret_cast<unsigned char *>(&c)) == 0;
    }

    static void hash_to_ec(const PublicKey &key, ge_p3 &res)
    {
        Hash h;
        ge_p2 point;
        ge_p1p1 point2;
        cn_fast_hash(std::addressof(key), sizeof(PublicKey), h);
        ge_fromfe_frombytes_vartime(&point, reinterpret_cast<const unsigned char *>(&h));
        ge_mul8(&point2, &point);
        ge_p1p1_to_p3(&res, &point2);
    }

    KeyImage crypto_ops::scalarmultKey(const KeyImage &P, const KeyImage &a)
    {
        ge_p3 A;
        ge_p2 R;
        // maybe use assert instead?
        ge_frombytes_vartime(&A, reinterpret_cast<const unsigned char *>(&P));
        ge_scalarmult(&R, reinterpret_cast<const unsigned char *>(&a), &A);
        KeyImage aP;
        ge_tobytes(reinterpret_cast<unsigned char *>(&aP), &R);
        return aP;
    }

    void crypto_ops::hash_data_to_ec(const uint8_t *data, std::size_t len, PublicKey &key)
    {
        Hash h;
        ge_p2 point;
        ge_p1p1 point2;
        cn_fast_hash(data, len, h);
        ge_fromfe_frombytes_vartime(&point, reinterpret_cast<const unsigned char *>(&h));
        ge_mul8(&point2, &point);
        ge_p1p1_to_p2(&point, &point2);
        ge_tobytes(reinterpret_cast<unsigned char *>(&key), &point);
    }

    void crypto_ops::generate_key_image(const PublicKey &pub, const SecretKey &sec, KeyImage &image)
    {
        ge_p3 point;
        ge_p2 point2;
        assert(sc_check(reinterpret_cast<const unsigned char *>(&sec)) == 0);
        hash_to_ec(pub, point);
        ge_scalarmult(&point2, reinterpret_cast<const unsigned char *>(&sec), &point);
        ge_tobytes(reinterpret_cast<unsigned char *>(&image), &point2);
    }

#ifdef _MSC_VER
#pragma warning(disable : 4200)
#endif

    struct rs_comm
    {
        Hash h;
        struct
        {
            EllipticCurvePoint a, b;
        } ab[];
    };

    static inline size_t rs_comm_size(size_t pubs_count)
    {
        return sizeof(rs_comm) + pubs_count * sizeof(((rs_comm *)0)->ab[0]);
    }

    std::tuple<bool, std::vector<Signature>> crypto_ops::generateRingSignatures(
        const Hash prefixHash,
        const KeyImage keyImage,
        const std::vector<PublicKey> publicKeys,
        const crypto::SecretKey transactionSecretKey,
        uint64_t realOutput)
    {
        std::vector<Signature> signatures(publicKeys.size());

        ge_p3 image_unp;
        ge_dsmp image_pre;
        EllipticCurveScalar sum, k, h;

        rs_comm *const buf = reinterpret_cast<rs_comm *>(alloca(rs_comm_size(publicKeys.size())));

        if (ge_frombytes_vartime(&image_unp, reinterpret_cast<const unsigned char *>(&keyImage)) != 0)
        {
            return {false, signatures};
        }

        ge_dsm_precomp(image_pre, &image_unp);

        sc_0(reinterpret_cast<unsigned char *>(&sum));

        buf->h = prefixHash;

        for (size_t i = 0; i < publicKeys.size(); i++)
        {
            ge_p2 tmp2;
            ge_p3 tmp3;

            if (i == realOutput)
            {
                random_scalar(k);
                ge_scalarmult_base(&tmp3, reinterpret_cast<unsigned char *>(&k));
                ge_p3_tobytes(reinterpret_cast<unsigned char *>(&buf->ab[i].a), &tmp3);
                hash_to_ec(publicKeys[i], tmp3);
                ge_scalarmult(&tmp2, reinterpret_cast<unsigned char *>(&k), &tmp3);
                ge_tobytes(reinterpret_cast<unsigned char *>(&buf->ab[i].b), &tmp2);
            }
            else
            {
                random_scalar(reinterpret_cast<EllipticCurveScalar &>(signatures[i]));
                random_scalar(*reinterpret_cast<EllipticCurveScalar *>(reinterpret_cast<unsigned char *>(&signatures[i]) + 32));

                if (ge_frombytes_vartime(&tmp3, reinterpret_cast<const unsigned char *>(&publicKeys[i])) != 0)
                {
                    return {false, signatures};
                }

                ge_double_scalarmult_base_vartime(
                    &tmp2,
                    reinterpret_cast<unsigned char *>(&signatures[i]),
                    &tmp3,
                    reinterpret_cast<unsigned char *>(&signatures[i]) + 32);

                ge_tobytes(reinterpret_cast<unsigned char *>(&buf->ab[i].a), &tmp2);

                hash_to_ec(publicKeys[i], tmp3);

                ge_double_scalarmult_precomp_vartime(
                    &tmp2,
                    reinterpret_cast<unsigned char *>(&signatures[i]) + 32,
                    &tmp3,
                    reinterpret_cast<unsigned char *>(&signatures[i]),
                    image_pre);

                ge_tobytes(reinterpret_cast<unsigned char *>(&buf->ab[i].b), &tmp2);

                sc_add(
                    reinterpret_cast<unsigned char *>(&sum),
                    reinterpret_cast<unsigned char *>(&sum),
                    reinterpret_cast<unsigned char *>(&signatures[i]));
            }
        }

        hash_to_scalar(buf, rs_comm_size(publicKeys.size()), h);

        sc_sub(
            reinterpret_cast<unsigned char *>(&signatures[realOutput]),
            reinterpret_cast<unsigned char *>(&h),
            reinterpret_cast<unsigned char *>(&sum));

        sc_mulsub(
            reinterpret_cast<unsigned char *>(&signatures[realOutput]) + 32,
            reinterpret_cast<unsigned char *>(&signatures[realOutput]),
            reinterpret_cast<const unsigned char *>(&transactionSecretKey),
            reinterpret_cast<unsigned char *>(&k));

        return {true, signatures};
    }

    bool crypto_ops::checkRingSignature(
        const Hash &prefix_hash,
        const KeyImage &image,
        const std::vector<PublicKey> pubs,
        const std::vector<Signature> signatures)
    {

        ge_p3 image_unp;

        ge_dsmp image_pre;

        EllipticCurveScalar sum, h;

        rs_comm *const buf = reinterpret_cast<rs_comm *>(alloca(rs_comm_size(pubs.size())));

        if (ge_frombytes_vartime(&image_unp, reinterpret_cast<const unsigned char *>(&image)) != 0)
        {
            return false;
        }

        ge_dsm_precomp(image_pre, &image_unp);

        if (ge_check_subgroup_precomp_vartime(image_pre) != 0)
        {
            return false;
        }

        sc_0(reinterpret_cast<unsigned char *>(&sum));

        buf->h = prefix_hash;

        for (size_t i = 0; i < pubs.size(); i++)
        {
            ge_p2 tmp2;
            ge_p3 tmp3;

            if (sc_check(reinterpret_cast<const unsigned char *>(&signatures[i])) != 0 || sc_check(reinterpret_cast<const unsigned char *>(&signatures[i]) + 32) != 0)
            {
                return false;
            }

            if (ge_frombytes_vartime(&tmp3, reinterpret_cast<const unsigned char *>(&pubs[i])) != 0)
            {
                return false;
            }

            ge_double_scalarmult_base_vartime(
                &tmp2,
                reinterpret_cast<const unsigned char *>(&signatures[i]),
                &tmp3,
                reinterpret_cast<const unsigned char *>(&signatures[i]) + 32);

            ge_tobytes(reinterpret_cast<unsigned char *>(&buf->ab[i].a), &tmp2);

            hash_to_ec(pubs[i], tmp3);

            ge_double_scalarmult_precomp_vartime(
                &tmp2,
                reinterpret_cast<const unsigned char *>(&signatures[i]) + 32,
                &tmp3,
                reinterpret_cast<const unsigned char *>(&signatures[i]),
                image_pre);

            ge_tobytes(reinterpret_cast<unsigned char *>(&buf->ab[i].b), &tmp2);

            sc_add(
                reinterpret_cast<unsigned char *>(&sum),
                reinterpret_cast<unsigned char *>(&sum),
                reinterpret_cast<const unsigned char *>(&signatures[i]));
        }

        hash_to_scalar(buf, rs_comm_size(pubs.size()), h);

        sc_sub(
            reinterpret_cast<unsigned char *>(&h),
            reinterpret_cast<unsigned char *>(&h),
            reinterpret_cast<unsigned char *>(&sum));

        return sc_isnonzero(reinterpret_cast<unsigned char *>(&h)) == 0;
    }
}
