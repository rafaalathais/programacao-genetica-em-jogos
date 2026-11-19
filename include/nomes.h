#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_NOMES_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_NOMES_H

// Todos os textos fixos que aparecem nos arquivos json, definidos uma única vez.
// Se um nome mudar no json, muda só aqui. `inline constexpr` (C++17) permite
// incluir este arquivo em vários .cpp sem erro de "definição múltipla".
namespace nomes {

// ações e comparações (spec)
inline constexpr const char* CONTINUAR = "CONTINUAR";
inline constexpr const char* PARAR = "PARAR";
inline constexpr const char* MAIOR_IGUAL = ">=";
inline constexpr const char* MAIOR = ">";

// chaves do vocabulário (spec)
inline constexpr const char* VARIAVEIS = "variaveis";
inline constexpr const char* NOME = "nome";
inline constexpr const char* MIN = "min";
inline constexpr const char* MAX = "max";
inline constexpr const char* ACOES = "acoes";
inline constexpr const char* COMPARACOES = "comparacoes";
inline constexpr const char* REGRAS = "regras";

// avaliador: tipos de adversário e fitness
inline constexpr const char* ALEATORIO = "aleatorio";
inline constexpr const char* LIMIAR = "limiar";
inline constexpr const char* SUPERAR = "superar";
inline constexpr const char* FITNESS_MINIMO_DESEMPATE_MEDIA = "minimo_com_desempate_pela_media";

// variáveis que os simuladores e os adversários procuram pelo nome (nunca por posição)
inline constexpr const char* PONTOS_TURNO = "pontos_turno";
inline constexpr const char* MEU_PLACAR = "meu_placar";
inline constexpr const char* PLACAR_ADV = "placar_adv";
inline constexpr const char* MINHA_SOMA = "minha_soma";
inline constexpr const char* SOMA_ADV = "soma_adv";
inline constexpr const char* SOU_SEGUNDO = "sou_segundo";

// chaves do bloco "regras" (comuns)
inline constexpr const char* DADO = "dado";
inline constexpr const char* FACES = "faces";
inline constexpr const char* QUANTIDADE = "quantidade";

// regras do pig
inline constexpr const char* PONTOS_PARA_VENCER = "pontos_para_vencer";
inline constexpr const char* FACE_QUE_ZERA_TURNO = "face_q_zera_turno";
inline constexpr const char* VITORIA_IMEDIATA = "vitoria_imediata";

// regras do blackjack
inline constexpr const char* LIMITE_DE_ESTOURO = "limite_de_estouro";
inline constexpr const char* QUANTIDADE_REDUZIDA = "quantidade_reduzida";
inline constexpr const char* REDUZ_A_PARTIR_DE = "reduz_a_partir_de";
inline constexpr const char* ROLAGEM_OBRIGATORIA = "rolagem_obrigatoria";
inline constexpr const char* EMPATE = "empate";
inline constexpr const char* EMPATE_MEIO_PONTO = "meio_ponto";

}  // namespace nomes

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_NOMES_H
