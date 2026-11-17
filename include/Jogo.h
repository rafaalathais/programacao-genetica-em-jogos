#ifndef JOGO_H
#define JOGO_H
#include "Estado.h"
#include "Jogador.h"

class Jogo {
public:
    virtual ~Jogo() = default;
    virtual Estado getEstadoAtual(int jogadorId) const = 0;
    virtual bool isFimDeJogo() const = 0;
    virtual int getVencedor() const = 0;
    virtual void executarAcao(int jogadorId, const std::string& acao) = 0;
    virtual void resetar() = 0;
};

#endif