#include "blackjack.h"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "leitorObjeto.h"
#include "nomes.h"

RegrasBlackjack lerRegrasBlackjack(const nlohmann::json& regras) {
    LeitorObjeto r(regras, "regras do blackjack");
    RegrasBlackjack b;
    b.limiteEstouro = r.inteiro(nomes::LIMITE_DE_ESTOURO);
    LeitorObjeto dado = r.objeto(nomes::DADO);
    b.faces = dado.inteiro(nomes::FACES);
    b.quantidade = dado.inteiro(nomes::QUANTIDADE);
    b.quantidadeReduzida = dado.inteiro(nomes::QUANTIDADE_REDUZIDA);
    b.reduzAPartirDe = dado.inteiro(nomes::REDUZ_A_PARTIR_DE);
    b.rolagemObrigatoria = r.logico(nomes::ROLAGEM_OBRIGATORIA);
    const std::string empate = r.texto(nomes::EMPATE);
    dado.semChavesDesconhecidas();
    r.semChavesDesconhecidas();

    if (b.limiteEstouro < 1) r.erro("limite_de_estouro deveria ser pelo menos 1");
    if (b.faces < 2) r.erro("o dado deveria ter pelo menos 2 faces");
    if (b.quantidade < 1 || b.quantidadeReduzida < 1) r.erro("as quantidades de dados deveriam ser pelo menos 1");
    if (b.reduzAPartirDe < 0) r.erro("reduz_a_partir_de não pode ser negativo");
    if (empate != nomes::EMPATE_MEIO_PONTO)
        r.erro("empate \"" + empate + "\" não suportado (só \"" + nomes::EMPATE_MEIO_PONTO + "\")");
    return b;
}

BlackjackDados::BlackjackDados(const nlohmann::json& regras, const Vocabulario& vocab, Aleatorio& aleatorio)
    : regras_(lerRegrasBlackjack(regras)),
      aleatorio_(aleatorio),
      iMinhaSoma_(vocab.indiceDe(nomes::MINHA_SOMA)),
      iSomaAdv_(vocab.indiceDe(nomes::SOMA_ADV)),
      iSouSegundo_(vocab.indiceDe(nomes::SOU_SEGUNDO)),
      estado_(vocab.variaveis.size(), 0) {
    if (vocab.variaveis.size() != 3)
        throw std::runtime_error("blackjack: a spec deveria ter exatamente as variáveis minha_soma, soma_adv e sou_segundo");
    for (const Variavel& v : vocab.variaveis) faixas_.push_back({v.min, v.max});
}

void BlackjackDados::reiniciar(int primeiro) {
    if (primeiro != 0 && primeiro != 1) throw std::invalid_argument("BlackjackDados::reiniciar: primeiro deve ser 0 ou 1");
    soma_[0] = soma_[1] = 0;
    estourou_[0] = estourou_[1] = false;
    primeiro_ = vez_ = primeiro;
    comecou_ = true;
    terminou_ = false;
    vencedor_ = -1;
    if (regras_.rolagemObrigatoria) rolar();
}

void BlackjackDados::encerrar(int vencedor) {
    terminou_ = true;
    vencedor_ = vencedor;
}

void BlackjackDados::rolar() {
    const int dados = soma_[vez_] < regras_.reduzAPartirDe ? regras_.quantidade : regras_.quantidadeReduzida;
    for (int i = 0; i < dados; ++i) soma_[vez_] += aleatorio_.inteiro(1, regras_.faces);
    if (soma_[vez_] > regras_.limiteEstouro) {
        // Estourou: perde. Se foi o primeiro, o segundo vence sem jogar
        // (por isso nunca acontece de os dois estourarem).
        estourou_[vez_] = true;
        encerrar(1 - vez_);
    }
}

void BlackjackDados::aplicar(Acao acao) {
    if (terminou_) throw std::logic_error("BlackjackDados::aplicar: a partida já terminou");
    if (acao == Acao::CONTINUAR) {
        rolar();
        return;
    }
    if (vez_ == primeiro_) {
        // O primeiro parou: começa a mão do segundo.
        vez_ = 1 - primeiro_;
        if (regras_.rolagemObrigatoria) rolar();
        return;
    }
    // O segundo parou: compara as somas (ninguém estourou, senão já teria acabado).
    if (soma_[0] > soma_[1]) encerrar(0);
    else if (soma_[1] > soma_[0]) encerrar(1);
    else encerrar(-1);
}

const Estado& BlackjackDados::estado() {
    const bool souSegundo = vez_ != primeiro_;
    auto definir = [&](int i, int valor) {
        const Faixa& f = faixas_[static_cast<std::size_t>(i)];
        estado_[static_cast<std::size_t>(i)] = std::clamp(valor, f.min, f.max);
    };
    definir(iMinhaSoma_, soma_[vez_]);
    definir(iSomaAdv_, souSegundo ? soma_[primeiro_] : 0);  // o primeiro não sabe nada do outro
    definir(iSouSegundo_, souSegundo ? 1 : 0);
    return estado_;
}

Resultado BlackjackDados::resultado() const {
    if (!comecou_ || !terminou_) throw std::logic_error("BlackjackDados::resultado: a partida não terminou");
    if (vencedor_ == 0) return Resultado::VITORIA_J1;
    if (vencedor_ == 1) return Resultado::VITORIA_J2;
    return Resultado::EMPATE;
}
