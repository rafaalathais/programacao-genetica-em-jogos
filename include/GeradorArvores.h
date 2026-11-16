#ifndef GERADOR_ARVORES_H
#define GERADOR_ARVORES_H
#pragma once
#include <memory>
#include <random>
#include <vector>
#include <string>
#include "No.h"
#include "Individuo.h"

struct SpecVariavel {
    std::string nome;
    int min;
    int max;
};

class GeradorDeArvores {
private:
    std::mt19937 rng;
    std::vector<SpecVariavel> variaveis;
    std::vector<std::string> acoes;
    std::vector<std::string> comparacoes;

    std::shared_ptr<No> gerarNoRecursivo(int profAtual, int profMax, int profMin);
    std::shared_ptr<No> criarNoComparacao();
    std::shared_ptr<No> criarNoAcao();

public:
    GeradorDeArvores(unsigned int semente,
                     const std::vector<SpecVariavel>& vars,
                     const std::vector<std::string>& acs,
                     const std::vector<std::string>& comps);

    // Gera um indivíduo completo para a população inicial (entre profMin e profMax)
    Individuo gerarIndividuo(int profMin, int profMax);

    // Gera apenas uma subárvore (útil para os Operadores de mutação depois)
    std::shared_ptr<No> gerarSubarvore(int profMax);
};

#endif 