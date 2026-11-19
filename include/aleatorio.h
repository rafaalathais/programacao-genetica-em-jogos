#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_ALEATORIO_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_ALEATORIO_H

#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>

// Único gerador de números aleatórios do programa.
//
// Por dentro é um std::mt19937, criado uma vez com a `semente` do parametros.json.
// Todo mundo que precisa sortear recebe uma referência (Aleatorio&) para ESTE objeto.
//
// Por que não usar std::uniform_int_distribution e std::bernoulli_distribution?
// O padrão C++ fixa a sequência do mt19937, mas NÃO fixa o algoritmo das
// distribuições: g++ (libstdc++), clang (libc++) e MSVC podem dar números
// diferentes para a mesma semente. Como o trabalho exige "mesma semente repete a
// saída", os dois sorteios abaixo são feitos à mão e dão o mesmo resultado em
// qualquer compilador.
class Aleatorio {
public:
    explicit Aleatorio(std::uint32_t semente) : motor_(semente) {}

    // Copiar é proibido: uma cópia repetiria a mesma sequência do original
    // sem ninguém perceber (e quebraria a regra de um gerador só).
    Aleatorio(const Aleatorio&) = delete;
    Aleatorio& operator=(const Aleatorio&) = delete;

    // Inteiro uniforme em [a, b] (inclusive), com a <= b.
    int inteiro(int a, int b) {
        if (a > b) throw std::invalid_argument("Aleatorio::inteiro: a > b");
        // Quantos valores existem em [a, b]. Contas em 64 bits para não estourar.
        const std::uint64_t tamanho =
            static_cast<std::uint64_t>(static_cast<std::int64_t>(b) - static_cast<std::int64_t>(a)) + 1;
        // O mt19937 devolve valores em [0, 2^32). Fazer só "x % tamanho" favoreceria
        // os primeiros valores quando 2^32 não é múltiplo de `tamanho`. Por isso
        // descartamos o pedaço do topo que sobra (método da rejeição).
        const std::uint64_t total = std::uint64_t(1) << 32;
        const std::uint64_t limite = total - (total % tamanho);
        std::uint64_t x;
        do {
            x = static_cast<std::uint64_t>(motor_());
        } while (x >= limite);
        return static_cast<int>(static_cast<std::int64_t>(a) + static_cast<std::int64_t>(x % tamanho));
    }

    // Índice uniforme em [0, n-1], para escolher um elemento de um vetor.
    std::size_t indice(std::size_t n) {
        if (n == 0) throw std::invalid_argument("Aleatorio::indice: n == 0");
        if (n - 1 > static_cast<std::size_t>(INT32_MAX)) throw std::invalid_argument("Aleatorio::indice: n grande demais");
        return static_cast<std::size_t>(inteiro(0, static_cast<int>(n - 1)));
    }

    // Real uniforme em [0, 1).
    double real01() { return static_cast<double>(motor_()) * (1.0 / 4294967296.0); }

    // Verdadeiro com probabilidade p. chance(0) nunca acontece; chance(1) sempre.
    bool chance(double p) { return real01() < p; }

private:
    std::mt19937 motor_;
};

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_ALEATORIO_H
