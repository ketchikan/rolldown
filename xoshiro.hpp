#include <cstdint>
#include <array>

class xoshiro256pp
{
private:
    std::array<uint64_t, 4> s;

    static inline uint64_t rotl(const uint64_t x, int k)
    {
        return (x << k) | (x >> (64 - k));
    }

    // SplitMix64 to seed the 256-bit state properly
    static uint64_t splitmix64(uint64_t &state)
    {
        uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

public:
    using result_type = uint64_t;

    static constexpr uint64_t min() { return 0; }
    static constexpr uint64_t max() { return UINT64_MAX; }

    // Constructor with 64-bit seed
    explicit xoshiro256pp(uint64_t seed)
    {
        s[0] = splitmix64(seed);
        s[1] = splitmix64(seed);
        s[2] = splitmix64(seed);
        s[3] = splitmix64(seed);
    }

    // Constructor with array state
    explicit xoshiro256pp(const std::array<uint64_t, 4> &state) : s(state) {}

    uint64_t operator()()
    {
        const uint64_t result = rotl(s[0] + s[3], 23) + s[0];

        const uint64_t t = s[1] << 17;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];

        s[2] ^= t;
        s[3] = rotl(s[3], 45);

        return result;
    }

    // Jump function (advances by 2^128 steps)
    void jump()
    {
        static const uint64_t JUMP[] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c};
        uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
        for (size_t i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
            for (int b = 0; b < 64; b++)
            {
                if (JUMP[i] & UINT64_C(1) << b)
                {
                    s0 ^= s[0];
                    s1 ^= s[1];
                    s2 ^= s[2];
                    s3 ^= s[3];
                }
                operator()();
            }
        s[0] = s0;
        s[1] = s1;
        s[2] = s2;
        s[3] = s3;
    }
};
