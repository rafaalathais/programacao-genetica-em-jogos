#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_JOGO_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_JOGO_H

#include "arvore.h"  // Estado
#include "tipos.h"   // Acao

// Resultado de uma partida, do ponto de vista de quem chamou: J1 é o jogador 0, J2 o jogador 1.
// No blackjack, somas iguais dão EMPATE (meio ponto para cada). No Pig, só há empate
// se a partida bater o limite de turnos.
enum class Resultado { VITORIA_J1, VITORIA_J2, EMPATE };

// Interface que todo simulador implementa. O núcleo (Partida, Avaliador, motor)
// só conhece esta interface; quem sabe as regras de cada jogo é a classe concreta
// (Pig, BlackjackDados).
//
// Ciclo de uso (é o que jogarPartida faz):
//   jogo.reiniciar(quemComeca);
//   while (!jogo.terminou()) {
//       Acao a = jogadores[jogo.daVez()]->decidir(jogo.estado());
//       jogo.aplicar(a);
//   }
//   jogo.resultado();
//
// Os jogadores são numerados 0 e 1. Os dados são rolados pelo próprio jogo, com o
// Aleatorio único do programa (recebido no construtor).
class Jogo {
public:
    virtual ~Jogo() = default;

    // Começa uma partida nova. `primeiro` (0 ou 1) é quem joga primeiro.
    // Pode já rolar dados (rolagem obrigatória) e, em tese, até terminar a partida.
    virtual void reiniciar(int primeiro) = 0;

    virtual bool terminou() const = 0;

    // Quem decide agora (0 ou 1). Só faz sentido com a partida em andamento.
    virtual int daVez() const = 0;

    // Estado visto por quem está na vez: um valor por variável, na ordem do
    // vocabulário. A referência vale até a próxima chamada de aplicar/reiniciar.
    virtual const Estado& estado() = 0;

    // Executa a decisão de quem está na vez (rola dado, passa a vez etc.).
    virtual void aplicar(Acao acao) = 0;

    // Só depois de terminou().
    virtual Resultado resultado() const = 0;
};

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_JOGO_H
