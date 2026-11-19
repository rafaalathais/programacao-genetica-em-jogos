#include "tipos.h"

#include "nomes.h"

std::optional<Acao> acaoDeTexto(const std::string& texto) {
    if (texto == nomes::CONTINUAR) return Acao::CONTINUAR;
    if (texto == nomes::PARAR) return Acao::PARAR;
    return std::nullopt;
}

std::optional<Comparacao> comparacaoDeTexto(const std::string& texto) {
    if (texto == nomes::MAIOR_IGUAL) return Comparacao::MAIOR_IGUAL;
    if (texto == nomes::MAIOR) return Comparacao::MAIOR;
    return std::nullopt;
}

std::string textoDe(Acao acao) {
    return acao == Acao::CONTINUAR ? nomes::CONTINUAR : nomes::PARAR;
}

std::string textoDe(Comparacao comparacao) {
    return comparacao == Comparacao::MAIOR_IGUAL ? nomes::MAIOR_IGUAL : nomes::MAIOR;
}

bool comparar(Comparacao comparacao, int a, int b) {
    return comparacao == Comparacao::MAIOR_IGUAL ? a >= b : a > b;
}
