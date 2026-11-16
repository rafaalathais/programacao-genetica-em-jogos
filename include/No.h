#ifndef NO_H
#define NO_H
#include <vector>
#include <memory>
#include <string>
#include "Estado.h"

enum class TipoNo { SE, COMPARADOR, VARIAVEL, CONSTANTE, ACAO };

struct No {
    TipoNo tipo;
    std::string valorStr; // Ex: ">=", "CONTINUAR", "PARAR"
    int valorNum;         // Índice da variável no Estado ou o valor da constante
    std::vector<std::shared_ptr<No>> filhos;

    // Método recursivo para avaliar a árvore e retornar a ação ("CONTINUAR" ou "PARAR")
    std::string avaliar(const Estado& estado) const;
};

#endif