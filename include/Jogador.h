#ifndef JOGADOR_H
#define JOGADOR_H
#include <string>
#include "Estado.h"

class Jogador {
public:
    virtual ~Jogador() = default;
    virtual std::string decidirJogada(const Estado& estado) = 0;
};

#endif 