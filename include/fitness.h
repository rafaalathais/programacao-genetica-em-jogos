#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_FITNESS_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_FITNESS_H

// Fitness "minimo_com_desempate_pela_media".
// Para cada adversário i: wᵢ = (vitórias + 0.5 * empates) / partidas.
//   minimo = o menor wᵢ (o adversário contra quem a árvore vai pior)
//   media  = a média dos wᵢ
// Compara-se como numa tabela de campeonato: primeiro o mínimo; se empatar, a média.
struct Fitness {
    double minimo = 0.0;
    double media = 0.0;
};

// a < b  significa "a é pior que b".
// Comparar doubles com != é seguro aqui: os winrates são calculados sempre pela
// mesma conta a partir de contagens inteiras, então valores iguais saem idênticos.
inline bool operator<(const Fitness& a, const Fitness& b) {
    if (a.minimo != b.minimo) return a.minimo < b.minimo;
    return a.media < b.media;
}

inline bool operator==(const Fitness& a, const Fitness& b) { return a.minimo == b.minimo && a.media == b.media; }

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_FITNESS_H
