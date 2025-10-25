


#include "rand.hh"

uint32_t Rand::rand() {
    unsigned int a = m_seed;
    a *= 3148259783UL;
    m_seed = a;
    return a;
}

void Rand::seed(uint32_t seed) { m_seed = INITIAL_SEED * seed; }