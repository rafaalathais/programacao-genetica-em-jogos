#include <iostream>
#include "GeradorArvores.h"

int main() {
    std::vector<SpecVariavel> varsPig = {
        {"pontos_turno", 0, 100},
        {"meu_placar", 0, 100},
        {"placar_adv", 0, 100}
    };
    std::vector<std::string> acoes = {"CONTINUAR", "PARAR"};
    std::vector<std::string> comps = {">=", ">"};

    GeradorDeArvores gerador(42, varsPig, acoes, comps);
    Individuo ind = gerador.gerarIndividuo(2, 5);

    Estado estadoTeste{{22, 40, 35}}; // pontos_turno=22, meu_placar=40, placar_adv=35
    std::string decisao = ind.raiz->avaliar(estadoTeste);

    std::cout << "Profundidade: " << ind.profundidade << "\n";
    std::cout << "Total de Nos: " << ind.numNos << "\n";
    std::cout << "Decisao tomada: " << decisao << "\n";

    return 0;
}