// Testes do Aleatorio: reprodutibilidade, limites e uniformidade dos sorteios.
#include <climits>
#include <cmath>
#include <random>
#include <vector>

#include "aleatorio.h"
#include "teste.h"

int main() {
    // 1. O mt19937 é padronizado: com a semente padrão, o 10000º número é 4123659995
    //    em qualquer compilador (exigência do próprio padrão C++).
    {
        std::mt19937 m;
        m.discard(9999);  // pula 9999 números
        VERIFICAR(m() == 4123659995u);
    }

    // 2. inteiro(0, 99) segue exatamente a regra escrita (rejeição + resto),
    //    conferida contra uma conta independente feita direto no mt19937.
    {
        Aleatorio a(42);
        std::mt19937 m(42);
        const std::uint64_t limite = 4294967296ull - (4294967296ull % 100);
        bool igual = true;
        for (int i = 0; i < 100000; ++i) {
            std::uint64_t x;
            do { x = m(); } while (x >= limite);
            if (a.inteiro(0, 99) != static_cast<int>(x % 100)) igual = false;
        }
        VERIFICAR(igual);
    }

    // 3. Mesma semente -> mesma sequência; semente diferente -> sequência diferente.
    for (std::uint32_t semente : {0u, 1u, 42u, 123456u, 4294967295u}) {
        Aleatorio a(semente), b(semente), c(semente + 1);
        bool iguais = true, diferentes = false;
        for (int i = 0; i < 10000; ++i) {
            const int x = a.inteiro(-5, 37), y = b.inteiro(-5, 37), z = c.inteiro(-5, 37);
            if (x != y) iguais = false;
            if (x != z) diferentes = true;
            if (a.chance(0.3) != b.chance(0.3)) iguais = false;
            c.chance(0.3);
        }
        VERIFICAR(iguais);
        VERIFICAR(diferentes);
    }

    // 4. Limites.
    {
        Aleatorio a(7);
        bool dentro = true, viuMin = false, viuMax = false;
        for (int i = 0; i < 100000; ++i) {
            const int x = a.inteiro(-3, 3);
            if (x < -3 || x > 3) dentro = false;
            viuMin |= (x == -3);
            viuMax |= (x == 3);
        }
        VERIFICAR(dentro && viuMin && viuMax);
        VERIFICAR(a.inteiro(5, 5) == 5);
        // faixa inteira do int: não pode ter estouro (o sanitizer acusaria)
        for (int i = 0; i < 1000; ++i) a.inteiro(INT_MIN, INT_MAX);
        VERIFICAR_LANCA(std::invalid_argument, a.inteiro(5, 4));
        VERIFICAR_LANCA(std::invalid_argument, a.indice(0));
        bool indiceOk = true;
        for (int i = 0; i < 1000; ++i) indiceOk &= a.indice(1) == 0;
        VERIFICAR(indiceOk);
    }

    // 5. Uniformidade: 600000 sorteios de um dado; cada face perto de 100000.
    //    Desvio padrão ~289; aceitamos ±1500 (mais de 5 desvios).
    for (std::uint32_t semente : {1u, 2u, 3u}) {
        Aleatorio a(semente);
        std::vector<int> faces(7, 0);
        for (int i = 0; i < 600000; ++i) faces[static_cast<std::size_t>(a.inteiro(1, 6))]++;
        for (int f = 1; f <= 6; ++f) VERIFICAR_MSG(std::abs(faces[static_cast<std::size_t>(f)] - 100000) < 1500,
                                                   "face " << f << ": " << faces[static_cast<std::size_t>(f)]);
    }

    // 6. chance(p).
    {
        Aleatorio a(99);
        bool nunca = true, sempre = true;
        int acertos = 0;
        for (int i = 0; i < 400000; ++i) {
            nunca &= !a.chance(0.0);
            sempre &= a.chance(1.0);
            acertos += a.chance(0.25);
        }
        VERIFICAR(nunca);
        VERIFICAR(sempre);
        VERIFICAR_MSG(std::abs(acertos / 400000.0 - 0.25) < 0.005, acertos);
    }

    return teste::fim("aleatorio");
}
