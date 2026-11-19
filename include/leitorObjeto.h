#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_LEITOROBJETO_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_LEITOROBJETO_H

#include <set>
#include <string>

#include "nlohmann/json.hpp"

// Lê os campos de um objeto json sem conversão silenciosa (1.5 num campo inteiro,
// "true" como texto num campo lógico etc. são recusados) e, no fim, acusa as chaves
// que ninguém leu — quase sempre um erro de digitação ("face_que_zera_turno").
//
// Usado pelos simuladores para ler o bloco "regras" da spec, que cada jogo
// interpreta do seu jeito. Todos os erros são std::runtime_error e começam por
// `onde` (ex.: "regras.dado: falta a chave \"faces\"").
//
// Guarda uma REFERÊNCIA ao json: o objeto lido precisa continuar vivo enquanto
// o LeitorObjeto for usado.
class LeitorObjeto {
public:
    LeitorObjeto(const nlohmann::json& objeto, std::string onde);

    int inteiro(const char* chave);
    bool logico(const char* chave);
    std::string texto(const char* chave);
    LeitorObjeto objeto(const char* chave);  // sub-objeto, ex.: "dado"

    // Lança se o objeto tiver alguma chave que não foi lida.
    void semChavesDesconhecidas() const;

    // Monta um erro com o prefixo `onde`.
    [[noreturn]] void erro(const std::string& mensagem) const;

private:
    const nlohmann::json& campo(const char* chave);

    const nlohmann::json& objeto_;
    std::string onde_;
    std::set<std::string> lidas_;
};

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_LEITOROBJETO_H
