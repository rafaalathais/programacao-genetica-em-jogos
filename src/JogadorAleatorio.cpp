#include "Jogador.h"
#include <string>
#include <random>

using namespace std;

class JogadorAleatorio : public Jogador {
public:
    string decidirJogada(const Estado &estado) {
        string opcoes[2] = {"CONTINUAR", "PARAR"};
        static random_device rd;
        static mt19937 gen(rd());

        uniform_int_distribution<> dist(0, 1);

        return opcoes[dist(gen)];
    }
};
