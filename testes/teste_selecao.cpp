// Testes do Fitness e da seleção (torneio e elitismo).
#include <cmath>
#include <vector>

#include "selecao.h"
#include "teste.h"

int main() {
    // ---- ordem do Fitness: mínimo primeiro, média desempata ----
    VERIFICAR((Fitness{0.50, 0.90} < Fitness{0.60, 0.10}));
    VERIFICAR(!(Fitness{0.60, 0.10} < Fitness{0.50, 0.90}));
    VERIFICAR((Fitness{0.55, 0.60} < Fitness{0.55, 0.70}));
    VERIFICAR(!(Fitness{0.55, 0.70} < Fitness{0.55, 0.60}));
    VERIFICAR(!(Fitness{0.55, 0.70} < Fitness{0.55, 0.70}));
    VERIFICAR((Fitness{0.55, 0.70} == Fitness{0.55, 0.70}));
    // winrates calculados pela mesma conta são idênticos (o operador != entre doubles é seguro)
    VERIFICAR((Fitness{(250 + 0.5 * 10) / 500.0, 0} == Fitness{(250 + 0.5 * 10) / 500.0, 0}));

    // ---- elitismo ----
    const std::vector<Fitness> f = {{0.3, 0.5}, {0.7, 0.8}, {0.7, 0.9}, {0.1, 0.9}, {0.7, 0.8}, {0.5, 0.5}};
    {
        const auto m = melhores(f, 3);
        VERIFICAR(m.size() == 3);
        VERIFICAR(m[0] == 2);                // (0.7, 0.9)
        VERIFICAR(m[1] == 1 && m[2] == 4);   // empate (0.7, 0.8): menor índice primeiro
        VERIFICAR(melhores(f, 0).empty());
        VERIFICAR(melhores(f, 6).size() == 6 && melhores(f, 6).back() == 3);
        VERIFICAR_LANCA(std::invalid_argument, melhores(f, 7));
        VERIFICAR_LANCA(std::invalid_argument, melhores(f, -1));
    }

    // ---- torneio: frequência de vitória bate com a conta ----
    // 10 indivíduos com fitness distintos (0 é o pior, 9 o melhor), torneio de 3 com reposição.
    // P(o indivíduo de posição r vence) = ((r+1)^3 - r^3) / 10^3.
    {
        std::vector<Fitness> dez;
        for (int i = 0; i < 10; ++i) dez.push_back({i / 10.0, 0.0});
        for (std::uint32_t semente : {1u, 2u, 3u}) {
            Aleatorio a(semente);
            std::vector<int> vitorias(10, 0);
            const int N = 300000;
            for (int i = 0; i < N; ++i) vitorias[torneio(dez, 3, a)]++;
            for (int r = 0; r < 10; ++r) {
                const double esperado = (std::pow(r + 1, 3) - std::pow(r, 3)) / 1000.0;
                const double visto = vitorias[static_cast<std::size_t>(r)] / static_cast<double>(N);
                VERIFICAR_MSG(std::abs(visto - esperado) < 0.004, "r=" << r << " visto=" << visto << " esperado=" << esperado);
            }
        }
    }

    // ---- torneio de 1 = escolha uniforme ----
    {
        std::vector<Fitness> quatro = {{0.1, 0}, {0.9, 0}, {0.5, 0}, {0.3, 0}};
        Aleatorio a(9);
        std::vector<int> c(4, 0);
        for (int i = 0; i < 200000; ++i) c[torneio(quatro, 1, a)]++;
        for (int i = 0; i < 4; ++i) VERIFICAR_MSG(std::abs(c[static_cast<std::size_t>(i)] - 50000) < 1200, c[static_cast<std::size_t>(i)]);
    }

    // ---- todos empatados: o torneio fica com o primeiro sorteado (uniforme) ----
    {
        std::vector<Fitness> iguais(5, Fitness{0.5, 0.5});
        Aleatorio a(4);
        std::vector<int> c(5, 0);
        for (int i = 0; i < 100000; ++i) c[torneio(iguais, 3, a)]++;
        for (int i = 0; i < 5; ++i) VERIFICAR_MSG(std::abs(c[static_cast<std::size_t>(i)] - 20000) < 900, c[static_cast<std::size_t>(i)]);
    }

    // ---- reprodutibilidade ----
    {
        Aleatorio a(42), b(42);
        bool iguais = true;
        for (int i = 0; i < 10000; ++i) iguais &= torneio(f, 3, a) == torneio(f, 3, b);
        VERIFICAR(iguais);
    }

    // ---- entradas inválidas ----
    {
        Aleatorio a(1);
        VERIFICAR_LANCA(std::invalid_argument, torneio({}, 3, a));
        VERIFICAR_LANCA(std::invalid_argument, torneio(f, 0, a));
    }

    return teste::fim("selecao");
}
