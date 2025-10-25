#pragma once

#include <stdint.h>

// An extremely simple random number generator that is absolutely
// not cryptographically secure, and not very good.
class Rand {
  public:
    // Gets a 32-bits random number, except the value 0.
    uint32_t rand();

    // Gets a random number between 0 and RANGE, exclusive.
    template <uint32_t RANGE>
    uint32_t rand() {
        return rand() % RANGE;
    }

    // Initializes the random number generator. Optional, but
    // recommended to avoid the same sequence every time. Use
    // for example the `now` function of the `GPU` class to
    // pass as a seed argument. Due to the way it works,
    // the random number generator will break if seed == 0.
    void seed(uint32_t seed);

  private:
    static constexpr uint32_t INITIAL_SEED = 2891583007UL;
    uint32_t m_seed = INITIAL_SEED;
};