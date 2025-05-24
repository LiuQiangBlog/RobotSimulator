//
// Created by qiang on 2022/8/22.
//

#ifndef ROBOTMATH_RANDOM_H
#define ROBOTMATH_RANDOM_H

#include <chrono>
#include <ctime>
#include <random>
#include <cassert>

class Random
{
public:
    static void Seed(unsigned int seed);

    static unsigned Seed();

    static double Rand();

    static double Rand(double from, double to);

    static int RandInt(int from, int to);

    static double RandNormal(double mean, double sigma);
};

static std::default_random_engine generator; // NOLINT

static std::uniform_real_distribution<double> distribution(0.0, 1.0); // NOLINT

void Random::Seed(unsigned int seed)
{
    generator.seed(seed);
}

unsigned Random::Seed()
{
    auto seed = static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    generator.seed(seed);
    Seed(seed);
    return seed;
}

double Random::Rand()
{
    return distribution(generator);
}

double Random::Rand(double from, double to)
{
    assert(from <= to);
    if (from < to)
    {
        double res;
        do
        {
            res = from + (to - from) * Rand();
        } while (res > to);
        return res;
    }
    return from;
}

int Random::RandInt(int from, int to)
{
    return static_cast<int>(floor(Rand(from, to)));
}

double Random::RandNormal(double mean, double sigma)
{
    std::normal_distribution<double> dist(0.0, 1.0);
    return mean + sigma * dist(generator);
}


#endif // ROBOTMATH_RANDOM_H
