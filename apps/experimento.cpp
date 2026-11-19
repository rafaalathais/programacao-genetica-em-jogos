// Executável de experimentos: uma rodada (jogo, semente, configuração) por chamada.
// Compilar: make experimento        Ajuda: ./experimento --ajuda
// Para rodar uma bateria inteira em paralelo, use scripts/bateria.sh.
#include <iostream>
#include <string>
#include <vector>

#include "experimento.h"

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty() || args[0] == "--ajuda" || args[0] == "-h") {
        std::cout << usoExperimento();
        return args.empty() ? 1 : 0;
    }
    try {
        rodarExperimento(lerArgumentos(args), std::cout);
    } catch (const std::exception& e) {
        std::cerr << "erro: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
