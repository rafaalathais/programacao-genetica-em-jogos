#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_PIG_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_PIG_H

#include "aleatorio.h"
#include "jogo.h"
#include "nlohmann/json.hpp"
#include "vocabulario.h"

// Regras do Pig, lidas do bloco "regras" de specs_regras_pig.json.
struct RegrasPig {
    int pontosParaVencer = 100;
    int faces = 6;
    int faceQueZera = 1;          // tirar esta face zera os pontos do turno e passa a vez
    bool vitoriaImediata = true;  // vence assim que placar + pontos do turno alcançar o alvo, sem precisar parar
};

// Lê e confere o bloco "regras". Lança std::runtime_error com o problema.
// Só existe a variante de 1 dado ("quantidade": 1).
RegrasPig lerRegrasPig(const nlohmann::json& regras);

// Pig: na sua vez, o jogador rola o dado quantas vezes quiser, somando os pontos
// do turno. CONTINUAR rola de novo; PARAR guarda os pontos do turno no placar e
// passa a vez. Tirar a face que zera perde os pontos do turno e passa a vez.
//
// Duas proteções contra partidas que não acabam (decididas pelo grupo):
//   1. A PRIMEIRA rolagem de cada turno é obrigatória: o jogador só decide depois
//      dela. Sem isso, "PARAR com 0 pontos" passaria a vez sem marcar nada, e duas
//      árvores assim jogariam para sempre (22% da população inicial faz isso).
//      Consequência: na hora de decidir, pontos_turno nunca é 0.
//   2. LIMITE_TURNOS: se a partida passar desse número de turnos (somando os dois
//      jogadores), termina em EMPATE. Com a regra 1 isso só acontece com
//      vitoria_imediata = false e um jogador que nunca para.
//
// Estado (achado pelo nome no vocabulário): pontos_turno, meu_placar, placar_adv,
// sempre do ponto de vista de quem está na vez. Valores fora da faixa da variável
// são cortados para a faixa (só acontece com vitoria_imediata = false).
class Pig : public Jogo {
public:
    static constexpr int LIMITE_TURNOS = 1000;

    Pig(const nlohmann::json& regras, const Vocabulario& vocab, Aleatorio& aleatorio);

    void reiniciar(int primeiro) override;
    bool terminou() const override { return terminou_; }
    int daVez() const override { return vez_; }
    const Estado& estado() override;
    void aplicar(Acao acao) override;
    Resultado resultado() const override;

    // Para testes e estatísticas.
    int placar(int jogador) const { return placar_[jogador]; }
    int pontosTurno() const { return pontosTurno_; }
    int turnos() const { return turnos_; }
    const RegrasPig& regras() const { return regras_; }

private:
    void rolar();       // uma rolagem (e, se zerar, a rolagem obrigatória do próximo turno)
    bool passarVez();   // devolve false se bateu o limite de turnos (partida encerrada)
    void encerrar(int vencedor);  // vencedor -1 = empate

    RegrasPig regras_;
    Aleatorio& aleatorio_;
    int iPontosTurno_, iMeuPlacar_, iPlacarAdv_;
    std::vector<Faixa> faixas_;
    Estado estado_;

    int placar_[2] = {0, 0};
    int pontosTurno_ = 0;
    int vez_ = 0;
    int turnos_ = 0;
    bool comecou_ = false;  // houve algum reiniciar()? (antes disso resultado() é erro)
    bool terminou_ = true;  // até o primeiro reiniciar() não há partida
    int vencedor_ = -1;
};

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_PIG_H
