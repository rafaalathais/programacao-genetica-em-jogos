#include "Jogador.h"


using namespace std;
class JogadorLimiarBlackjack : public Jogador {
    public:

    std::string decidirJogada(const Estado &estado) {
        int minhaPontuacao = estado.variaveis[1];
        int pontuacaoOponente = estado.variaveis[2];

        if (minhaPontuacao > 16 && pontuacaoOponente > minhaPontuacao || pontuacaoOponente >= 18) {
            return "CONTINUAR";
        }else {
            return "PARAR";
        }
    }

};