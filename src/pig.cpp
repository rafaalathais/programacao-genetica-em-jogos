#include "pig.h"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "leitorObjeto.h"
#include "nomes.h"

RegrasPig lerRegrasPig(const nlohmann::json& regras) {
    LeitorObjeto r(regras, "regras do pig");
    RegrasPig p;
    p.pontosParaVencer = r.inteiro(nomes::PONTOS_PARA_VENCER);
    LeitorObjeto dado = r.objeto(nomes::DADO);
    p.faces = dado.inteiro(nomes::FACES);
    const int quantidade = dado.inteiro(nomes::QUANTIDADE);
    p.faceQueZera = r.inteiro(nomes::FACE_QUE_ZERA_TURNO);
    p.vitoriaImediata = r.logico(nomes::VITORIA_IMEDIATA);
    dado.semChavesDesconhecidas();
    r.semChavesDesconhecidas();

    if (p.pontosParaVencer < 1) r.erro("pontos_para_vencer deveria ser pelo menos 1");
    if (p.faces < 2) r.erro("o dado deveria ter pelo menos 2 faces");
    if (quantidade != 1) r.erro("só a variante com 1 dado é suportada (quantidade = 1)");
    if (p.faceQueZera < 1 || p.faceQueZera > p.faces) r.erro("face_q_zera_turno deveria ser uma face do dado");
    return p;
}

Pig::Pig(const nlohmann::json& regras, const Vocabulario& vocab, Aleatorio& aleatorio)
    : regras_(lerRegrasPig(regras)),
      aleatorio_(aleatorio),
      iPontosTurno_(vocab.indiceDe(nomes::PONTOS_TURNO)),
      iMeuPlacar_(vocab.indiceDe(nomes::MEU_PLACAR)),
      iPlacarAdv_(vocab.indiceDe(nomes::PLACAR_ADV)),
      estado_(vocab.variaveis.size(), 0) {
    // Toda variável da spec precisa ser preenchida pelo simulador; uma variável
    // a mais ficaria sempre 0 e a árvore decidiria olhando um valor inventado.
    if (vocab.variaveis.size() != 3)
        throw std::runtime_error("pig: a spec deveria ter exatamente as variáveis pontos_turno, meu_placar e placar_adv");
    for (const Variavel& v : vocab.variaveis) faixas_.push_back({v.min, v.max});
}

void Pig::reiniciar(int primeiro) {
    if (primeiro != 0 && primeiro != 1) throw std::invalid_argument("Pig::reiniciar: primeiro deve ser 0 ou 1");
    placar_[0] = placar_[1] = 0;
    pontosTurno_ = 0;
    vez_ = primeiro;
    turnos_ = 1;
    comecou_ = true;
    terminou_ = false;
    vencedor_ = -1;
    rolar();  // rolagem obrigatória do primeiro turno
}

void Pig::encerrar(int vencedor) {
    terminou_ = true;
    vencedor_ = vencedor;
}

bool Pig::passarVez() {
    vez_ = 1 - vez_;
    pontosTurno_ = 0;
    ++turnos_;
    if (turnos_ > LIMITE_TURNOS) {
        encerrar(-1);
        return false;
    }
    return true;
}

void Pig::rolar() {
    // Laço em vez de recursão: tirar a face que zera passa a vez, e o próximo turno
    // começa com outra rolagem obrigatória, que também pode zerar, e assim por diante.
    while (true) {
        const int face = aleatorio_.inteiro(1, regras_.faces);
        if (face == regras_.faceQueZera) {
            if (!passarVez()) return;
            continue;
        }
        pontosTurno_ += face;
        if (regras_.vitoriaImediata && placar_[vez_] + pontosTurno_ >= regras_.pontosParaVencer) {
            placar_[vez_] += pontosTurno_;
            pontosTurno_ = 0;
            encerrar(vez_);
        }
        return;
    }
}

void Pig::aplicar(Acao acao) {
    if (terminou_) throw std::logic_error("Pig::aplicar: a partida já terminou");
    if (acao == Acao::CONTINUAR) {
        rolar();
        return;
    }
    // PARAR: guarda os pontos do turno.
    placar_[vez_] += pontosTurno_;
    if (placar_[vez_] >= regras_.pontosParaVencer) {
        pontosTurno_ = 0;
        encerrar(vez_);
        return;
    }
    if (passarVez()) rolar();  // rolagem obrigatória do turno do outro
}

const Estado& Pig::estado() {
    auto definir = [&](int i, int valor) {
        const Faixa& f = faixas_[static_cast<std::size_t>(i)];
        estado_[static_cast<std::size_t>(i)] = std::clamp(valor, f.min, f.max);
    };
    definir(iPontosTurno_, pontosTurno_);
    definir(iMeuPlacar_, placar_[vez_]);
    definir(iPlacarAdv_, placar_[1 - vez_]);
    return estado_;
}

Resultado Pig::resultado() const {
    if (!comecou_ || !terminou_) throw std::logic_error("Pig::resultado: a partida não terminou");
    if (vencedor_ == 0) return Resultado::VITORIA_J1;
    if (vencedor_ == 1) return Resultado::VITORIA_J2;
    return Resultado::EMPATE;
}
