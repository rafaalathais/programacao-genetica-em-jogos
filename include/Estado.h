#ifndef ESTADO_H
#define ESTADO_H

#include <vector>

struct Estado {
    std::vector<int> variaveis; // Ex: [0] = pontos_turno, [1] = meu_placar, [2] = placar_adv
};
#endif