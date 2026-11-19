#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_JOGADOR_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_JOGADOR_H

#include <memory>

#include "aleatorio.h"
#include "arvore.h"
#include "criterioAvaliador.h"
#include "vocabulario.h"

// Todo jogador é um programa: recebe o estado (visto do lado dele) e decide.
// `decidir` não é const porque o jogador aleatório consome o gerador.
class Jogador {
public:
    virtual ~Jogador() = default;
    virtual Acao decidir(const Estado& estado) = 0;
};

// O indivíduo da evolução: decide avaliando a árvore.
// Guarda um PONTEIRO para a árvore (não copia): a árvore precisa continuar viva
// enquanto o jogador for usado.
class JogadorArvore : public Jogador {
public:
    explicit JogadorArvore(const No& arvore) : arvore_(&arvore) {}
    Acao decidir(const Estado& estado) override { return avaliar(*arvore_, estado); }

private:
    const No* arvore_;
};

// ---- adversários de referência (avaliador_*.json) ----

// "aleatorio": CONTINUAR ou PARAR com 50% cada, usando o Aleatorio único
// (nada de random_device: a mesma semente tem de repetir a partida).
class JogadorAleatorio : public Jogador {
public:
    explicit JogadorAleatorio(Aleatorio& aleatorio) : aleatorio_(aleatorio) {}
    Acao decidir(const Estado&) override { return aleatorio_.chance(0.5) ? Acao::CONTINUAR : Acao::PARAR; }

private:
    Aleatorio& aleatorio_;
};

// "limiar": continua enquanto a variável estiver ABAIXO do valor; ao chegar nele, para.
//   Pig:       limiar(pontos_turno, 20) -> "para quando juntar 20 pontos no turno"
//   blackjack: limiar(minha_soma, 17)   -> "para com 17 ou mais"
// A mesma classe serve aos dois jogos: o índice da variável vem do nome, pelo vocabulário.
class JogadorLimiar : public Jogador {
public:
    JogadorLimiar(int indice, int valor) : indice_(static_cast<std::size_t>(indice)), valor_(valor) {}
    Acao decidir(const Estado& estado) override {
        return estado.at(indice_) < valor_ ? Acao::CONTINUAR : Acao::PARAR;
    }

private:
    std::size_t indice_;
    int valor_;
};

// "superar" (blackjack):
//   jogando primeiro (sou_segundo = 0): age como limiar(minha_soma, limiar_se_primeiro);
//   jogando em segundo (sou_segundo = 1): já sabe a soma do adversário, então continua
//   enquanto minha_soma < soma_adv e para assim que IGUALAR ou passar.
// Parar ao igualar é escolha nossa: com o empate valendo meio ponto, arriscar o estouro
// para passar da soma do adversário costuma ser pior que garantir o empate.
class JogadorSuperar : public Jogador {
public:
    JogadorSuperar(int minhaSoma, int somaAdv, int souSegundo, int limiarSePrimeiro)
        : minhaSoma_(static_cast<std::size_t>(minhaSoma)),
          somaAdv_(static_cast<std::size_t>(somaAdv)),
          souSegundo_(static_cast<std::size_t>(souSegundo)),
          limiarSePrimeiro_(limiarSePrimeiro) {}
    Acao decidir(const Estado& estado) override {
        const int alvo = estado.at(souSegundo_) == 0 ? limiarSePrimeiro_ : estado.at(somaAdv_);
        return estado.at(minhaSoma_) < alvo ? Acao::CONTINUAR : Acao::PARAR;
    }

private:
    std::size_t minhaSoma_, somaAdv_, souSegundo_;
    int limiarSePrimeiro_;
};

// Fábrica: monta o adversário descrito no avaliador_*.json.
// Confere os campos: tipo conhecido, campos obrigatórios presentes, nenhum campo que
// não se aplica ao tipo, variável existente no vocabulário e valor dentro da faixa.
// Lança std::runtime_error explicando o problema.
std::unique_ptr<Jogador> criarAdversario(const Adversario& adversario, const Vocabulario& vocab, Aleatorio& aleatorio);

// Frase curta dizendo como o adversário joga, para relatórios.
// Ex.: "limiar: continua enquanto pontos_turno < 20, depois para".
std::string descrever(const Adversario& adversario);

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_JOGADOR_H
