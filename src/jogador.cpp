#include "jogador.h"

#include <stdexcept>
#include <string>

#include "nomes.h"

namespace {

[[noreturn]] void erro(const Adversario& a, const std::string& mensagem) {
    throw std::runtime_error("adversário \"" + a.tipo + "\": " + mensagem);
}

// Índice da variável, com a mensagem de erro dizendo de quem é a culpa.
int indice(const Adversario& a, const Vocabulario& vocab, const std::string& nome) {
    try {
        return vocab.indiceDe(nome);
    } catch (const std::runtime_error&) {
        erro(a, "a spec não tem a variável \"" + nome + "\"");
    }
}

}  // namespace

std::unique_ptr<Jogador> criarAdversario(const Adversario& a, const Vocabulario& vocab, Aleatorio& aleatorio) {
    if (a.tipo == nomes::ALEATORIO) {
        if (a.variavel || a.valor || a.limiar_se_primeiro) erro(a, "não usa variavel, valor nem limiar_se_primeiro");
        return std::make_unique<JogadorAleatorio>(aleatorio);
    }

    if (a.tipo == nomes::LIMIAR) {
        if (!a.variavel || !a.valor) erro(a, "precisa de \"variavel\" e \"valor\"");
        if (a.limiar_se_primeiro) erro(a, "não usa limiar_se_primeiro");
        const int i = indice(a, vocab, *a.variavel);
        const Variavel& v = vocab.variaveis[static_cast<std::size_t>(i)];
        if (*a.valor < v.min || *a.valor > v.max)
            erro(a, "valor " + std::to_string(*a.valor) + " fora da faixa [" + std::to_string(v.min) + ", " +
                        std::to_string(v.max) + "] de " + v.nome);
        return std::make_unique<JogadorLimiar>(i, *a.valor);
    }

    if (a.tipo == nomes::SUPERAR) {
        if (!a.limiar_se_primeiro) erro(a, "precisa de \"limiar_se_primeiro\"");
        if (a.variavel || a.valor) erro(a, "não usa variavel nem valor");
        const int minha = indice(a, vocab, nomes::MINHA_SOMA);
        const int adv = indice(a, vocab, nomes::SOMA_ADV);
        const int segundo = indice(a, vocab, nomes::SOU_SEGUNDO);
        const Variavel& v = vocab.variaveis[static_cast<std::size_t>(minha)];
        if (*a.limiar_se_primeiro < v.min || *a.limiar_se_primeiro > v.max)
            erro(a, "limiar_se_primeiro fora da faixa de " + v.nome);
        return std::make_unique<JogadorSuperar>(minha, adv, segundo, *a.limiar_se_primeiro);
    }

    erro(a, std::string("tipo desconhecido (esperado \"") + nomes::ALEATORIO + "\", \"" + nomes::LIMIAR + "\" ou \"" +
                nomes::SUPERAR + "\")");
}

std::string descrever(const Adversario& a) {
    if (a.tipo == nomes::ALEATORIO) return a.tipo + ": decide CONTINUAR ou PARAR no cara ou coroa";
    if (a.tipo == nomes::LIMIAR && a.variavel && a.valor)
        return a.tipo + ": continua enquanto " + *a.variavel + " < " + std::to_string(*a.valor) + ", depois para";
    if (a.tipo == nomes::SUPERAR && a.limiar_se_primeiro)
        return a.tipo + ": jogando primeiro, para com " + std::to_string(*a.limiar_se_primeiro) +
               " ou mais; jogando em segundo, continua até igualar a soma do outro";
    return a.tipo;
}
