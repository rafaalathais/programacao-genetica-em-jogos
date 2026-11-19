#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_BLACKJACK_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_BLACKJACK_H

#include "aleatorio.h"
#include "jogo.h"
#include "nlohmann/json.hpp"
#include "vocabulario.h"

// Regras do blackjack de dados, lidas do bloco "regras" de specs_regras_blackjack.json.
struct RegrasBlackjack {
    int limiteEstouro = 21;      // passar disso perde
    int faces = 6;
    int quantidade = 2;          // dados por rolagem enquanto a soma < reduzAPartirDe
    int quantidadeReduzida = 1;  // dados por rolagem quando a soma >= reduzAPartirDe
    int reduzAPartirDe = 16;
    bool rolagemObrigatoria = true;  // a primeira rolagem da mão é automática
};

// Lê e confere o bloco "regras". Só existe a regra de empate "meio_ponto".
RegrasBlackjack lerRegrasBlackjack(const nlohmann::json& regras);

// Blackjack de dados: cada jogador joga a MÃO INTEIRA antes do outro.
//   - O primeiro joga sem saber nada do outro: vê soma_adv = 0 e sou_segundo = 0.
//     Se ele estourar, a partida acaba ali e o segundo vence sem jogar.
//   - O segundo joga sabendo a soma final do primeiro (soma_adv) e com sou_segundo = 1.
//     Se estourar, perde.
//   - Ninguém estourou: maior soma vence; somas iguais dão EMPATE (meio ponto para cada).
// Cada rolagem usa `quantidade` dados enquanto a soma < reduzAPartirDe, e
// `quantidadeReduzida` dados a partir daí (com o spec atual: 2 dados até 15, 1 dado de 16 em diante).
class BlackjackDados : public Jogo {
public:
    BlackjackDados(const nlohmann::json& regras, const Vocabulario& vocab, Aleatorio& aleatorio);

    void reiniciar(int primeiro) override;
    bool terminou() const override { return terminou_; }
    int daVez() const override { return vez_; }
    const Estado& estado() override;
    void aplicar(Acao acao) override;
    Resultado resultado() const override;

    // Para testes e estatísticas.
    int soma(int jogador) const { return soma_[jogador]; }
    bool estourou(int jogador) const { return estourou_[jogador]; }
    int primeiro() const { return primeiro_; }
    const RegrasBlackjack& regras() const { return regras_; }

private:
    void rolar();
    void encerrar(int vencedor);  // -1 = empate

    RegrasBlackjack regras_;
    Aleatorio& aleatorio_;
    int iMinhaSoma_, iSomaAdv_, iSouSegundo_;
    std::vector<Faixa> faixas_;
    Estado estado_;

    int soma_[2] = {0, 0};
    bool estourou_[2] = {false, false};
    int primeiro_ = 0;
    int vez_ = 0;
    bool comecou_ = false;  // houve algum reiniciar()? (antes disso resultado() é erro)
    bool terminou_ = true;
    int vencedor_ = -1;
};

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_BLACKJACK_H
