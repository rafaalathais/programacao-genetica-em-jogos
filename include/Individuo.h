#ifndef INDIVIDUO_H
#define INDIVIDUO_H
#include <memory>
#include "No.h"

struct Individuo {
    std::shared_ptr<No> raiz;
    double fitness = 0.0;
    int profundidade = 0;
    int numNos = 0;
};

#endif