#include "thc.h"
#include <bits/stdc++.h>

// {{{ MAKRA
#define pb push_back
#define pf push_front
#define turbo                                                                  \
  cin.tie(0);                                                                  \
  cout.tie(0);                                                                 \
  ios_base::sync_with_stdio(false)
#define fi first
#define sc second
#define ll long long
#define un unsigned
#define vi vector<int>
#define vb vector<bool>
#define vll vector<ll>
#define pii pair<int, int>
#define pll pair<ll, ll>
// }}}

#define debug 0
#define VERBOSE 1
#define HEUR 0
#define HIT_CONFIRMATION 0

using namespace std;
// {{{ TEMPLATE
// -----------------------------------------------------
// {{{ SVECTOR
// vector alokujacy dokladnie tyle pamieci ile potrzebuje
// UWAGA nie wola destruktorów
template <class T> class svector {
  T *tab;
  size_t sz;

public:
  svector() {
    tab = nullptr;
    sz = 0;
  }
  svector(size_t s) {
    sz = s;
    tab = calloc(s, sizeof(T));
  }
  ~svector() { free(tab); }
  size_t size() const { return sz; }
  void push_back(T x) {
    tab = (T *)realloc(tab, (sz + 1) * sizeof(T));
    tab[sz] = x;
    sz++;
  }
  void resize(size_t nsize) {
    tab = (T *)realloc(tab, nsize * sizeof(T));
    sz = nsize;
  }
  T &operator[](size_t i) { return tab[i]; }
};
// }}}
// {{{ BINARY_HEAP
// kopiec minimowy z operacją decrease_priority w log(n)
// używa 2*więcej pamięci niż zwykły kopiec
// unordered_map loc mozna zmienić na tablice gdy T to int
template <class key, class T, class Compare = less<T>> class binary_heap {
  vector<T> V;
  unordered_map<key, size_t> loc;
  /* boost::unordered_map<key, size_t> loc; */
  Compare comp;

public:
  binary_heap() { V = vector<T>(1); }
  ~binary_heap() = default;
  bool empty() const { return V.size() == 1; }
  T top() const {
    if (empty())
      throw runtime_error("no top in empty");
    return V[1];
  }
  void insert(const T &x) {
    V.pb(x);
    size_t i = V.size() - 1;
    loc[x.key()] = i;
    while (i != 1 and comp(V[i], V[i / 2])) {
      loc[V[i].key()] = i / 2;
      loc[V[i / 2].key()] = i;
      swap(V[i], V[i / 2]);
      i = i / 2;
    }
  }
  void pop() {
    if (empty()) {
      throw runtime_error("trying to pop empty");
    }
    loc.erase(V[1].key());
    loc[V[V.size() - 1].key()] = 1;
    swap(V[1], V[V.size() - 1]);
    V.pop_back();
    size_t i = 1;
    while (true) {
      size_t s = i;
      size_t l = 2 * i;
      size_t r = 2 * i + 1;
      if (l < V.size() and comp(V[l], V[s])) {
        s = l;
      }
      if (r < V.size() and comp(V[r], V[s])) {
        s = r;
      }
      if (s != i) {
        loc[V[i].key()] = s;
        loc[V[s].key()] = i;
        swap(V[i], V[s]);
        i = s;
      } else {
        break;
      }
    }
  }
  void decrease_priority(const T &x) {
    if (!loc.contains(x.key())) {
      insert(x);
      return;
    }
    V[loc[x.key()]] = x;
    size_t i = loc[x.key()];
    while (i != 1 and comp(V[i], V[i / 2])) {
      loc[V[i].key()] = i / 2;
      loc[V[i / 2].key()] = i;
      swap(V[i], V[i / 2]);
      i = i / 2;
    }
  }
};
// }}}
// TEMPLATE_END
// ----------------------------------------------
// }}}

int DEPTH = 6;
#define SORTED DEPTH / 2 + 1

const double mate_score = 1e10;

chrono::time_point<chrono::system_clock> start;
chrono::milliseconds per_move;

typedef optional<thc::Move> Move;
typedef vector<thc::Move> Moves;

enum Node_type { Exact, Upper, Lower };

struct TT_node {
  Move best_move;
  double value;
  int depth;
  Node_type type;
  int history = 0;
};
unordered_map<uint64_t, TT_node> TT;

class Game {
public:
  thc::ChessRules board;
  Game(const Game &other) { board = other.board; }
  const Game &operator=(const Game &other) {
    board = other.board;
    return other;
  }
  Game() { board = thc::ChessRules(); }
  void print() {
    string pos = board.ToDebugStr();
    cout << pos;
  }
  void gen_moves(vector<thc::Move> &moves, vb &check, vb &mate, vb &stalemate) {
    board.GenLegalMoveList(moves, check, mate, stalemate);
  }
  void apply_move_no_undo(Move m) {
    if (m) {
      board.PlayMove(m.value());
    } else {
      throw runtime_error("null move in chess???");
    }
  }
  uint64_t Hash_roll(Move m, uint64_t oldhash) {
    return board.Hash64Update(oldhash, m.value());
  }
  uint64_t Hash() { return board.Hash64Calculate(); }
  void apply_move(Move m) {
    if (m) {
      board.PushMove(m.value());
    } else {
      throw runtime_error("null move in chess???");
    }
  }
  thc::TERMINAL terminal() {
    thc::TERMINAL res;
    board.Evaluate(res);
    return res;
  }
  void undo_move(Move m) {
    if (m)
      board.PopMove(m.value());
  }
  double heuristic() {
    // material count
    map<char, double> piece_value = {
        {'p', -1},  {'P', 1},  {'n', -3.2}, {'N', 3.2}, {'b', -3.3},
        {'B', 3.3}, {'r', -5}, {'R', 5},    {'q', -9},  {'Q', 9},
        {'k', 0},   {'K', 0},  {' ', 0}};
    double material = 0;
    thc::Square King;
    thc::Square king;
    bool queen = false;
    int minor_count = 0;
    for (int i = 0; i <= 63; i++) {
      char piece = board.squares[i];
      if (tolower(piece) == 'q') {
        queen = true;
      }
      if (tolower(piece) == 'n' or tolower(piece) == 'b') {
        minor_count++;
      }
      material += piece_value[piece];
      if (piece == 'K') {
        King = static_cast<thc::Square>(i);
      }
      if (piece == 'k') {
        king = static_cast<thc::Square>(i);
      }
    }
    // mobility
    Moves m[2];
    vb checks;
    vb ma, st;
    board.GenLegalMoveList(m[0], checks, ma, st);
    double W_mate =
        accumulate(ma.begin(), ma.end(), 0) * (board.WhiteToPlay() ? 1e6 : 0);
    double W_checks = accumulate(checks.begin(), checks.end(), 0) + W_mate;
    double B_checks;
    checks.clear();
    ma.clear();
    st.clear();
    {
      thc::ChessRules other = board;
      other.Toggle();
      other.GenLegalMoveList(m[1], checks, ma, st);
      double B_mate = accumulate(ma.begin(), ma.end(), 0) *
                      (!board.WhiteToPlay() ? 1e3 : 0);
      B_checks = accumulate(checks.begin(), checks.end(), 0) + B_mate;
      checks.clear();
      ma.clear();
      st.clear();
    }
    double mobility = m[0].size() * (board.WhiteToPlay() ? 1.0 : -1.0) +
                      m[1].size() * (board.WhiteToPlay() ? -1.0 : 1.0);
    // activity (preset values)
    double Pawn_ac[64] = {
        0,  0,  0,  0,   0,   0,  0,  0,  50, 50, 50,  50, 50, 50,  50, 50,
        10, 10, 20, 30,  30,  20, 10, 10, 5,  5,  10,  25, 25, 10,  5,  5,
        0,  0,  0,  20,  20,  0,  0,  0,  5,  -5, -10, 0,  0,  -10, -5, 5,
        5,  10, 10, -20, -20, 10, 10, 5,  0,  0,  0,   0,  0,  0,   0,  0};
    double pawn_ac[64] = {
        0,  0,  0,   0,  0,  0,   0,  0,  5,  10, 10, -20, -20, 10, 10, 5,
        5,  -5, -10, 0,  0,  -10, -5, 5,  0,  0,  0,  20,  20,  0,  0,  0,
        5,  5,  10,  25, 25, 10,  5,  5,  10, 10, 20, 30,  30,  20, 10, 10,
        50, 50, 50,  50, 50, 50,  50, 50, 0,  0,  0,  0,   0,   0,  0,  0,
    };
    double Knight_ac[64] = {
        -50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   0,   0,
        0,   -20, -40, -30, 0,   10,  15,  15,  10,  0,   -30, -30, 5,
        15,  20,  20,  15,  5,   -30, -30, 0,   15,  20,  20,  15,  0,
        -30, -30, 5,   10,  15,  15,  10,  5,   -30, -40, -20, 0,   5,
        5,   0,   -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};
    double knight_ac[64] = {
        -50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   5,   5,
        0,   -20, -40, -30, 5,   10,  15,  15,  10,  5,   -30, -30, 0,
        15,  20,  20,  15,  0,   -30, -30, 5,   15,  20,  20,  15,  5,
        -30, -30, 0,   10,  15,  15,  10,  0,   -30, -40, -20, 0,   0,
        0,   0,   -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};
    double Bishop_ac[64] = {
        -20, -10, -10, -10, -10, -10, -10, -20, -10, 0,   0,   0,   0,
        0,   0,   -10, -10, 0,   5,   10,  10,  5,   0,   -10, -10, 5,
        5,   10,  10,  5,   5,   -10, -10, 0,   10,  10,  10,  10,  0,
        -10, -10, 10,  10,  10,  10,  10,  10,  -10, -10, 5,   0,   0,
        0,   0,   5,   -10, -20, -10, -10, -10, -10, -10, -10, -20};
    double bishop_ac[64] = {
        -20, -10, -10, -10, -10, -10, -10, -20, -10, 5,   0,   0,   0,
        0,   5,   -10, -10, 10,  10,  10,  10,  10,  10,  -10, -10, 0,
        10,  10,  10,  10,  0,   -10, -10, 5,   5,   10,  10,  5,   5,
        -10, -10, 0,   5,   10,  10,  5,   0,   -10, -10, 0,   0,   0,
        0,   0,   0,   -10, -20, -10, -10, -10, -10, -10, -10, -20};
    double Rook_ac[64] = {0,  0,  0, 0,  0, 0,  0,  0, 5,  10, 10, 10, 10,
                          10, 10, 5, -5, 0, 0,  0,  0, 0,  0,  -5, -5, 0,
                          0,  0,  0, 0,  0, -5, -5, 0, 0,  0,  0,  0,  0,
                          -5, -5, 0, 0,  0, 0,  0,  0, -5, -5, 0,  0,  0,
                          0,  0,  0, -5, 0, 0,  0,  5, 5,  0,  0,  0};
    double rook_ac[64] = {
        0,  0,  0,  5,  5,  0,  0,  0,  -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0,  0,  0,  0,  0,  0,  -5, -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0,  0,  0,  0,  0,  0,  -5, -5, 0, 0, 0, 0, 0, 0, -5,
        5,  10, 10, 10, 10, 10, 10, 5,  0,  0, 0, 0, 0, 0, 0, 0,
    };
    double Queen_ac[64] = {-20, -10, -10, -5,  -5,  -10, -10, -20, -10, 0,  0,
                           0,   0,   0,   0,   -10, -10, 0,   5,   5,   5,  5,
                           0,   -10, -5,  0,   5,   5,   5,   5,   0,   -5, 0,
                           0,   5,   5,   5,   5,   0,   -5,  -10, 5,   5,  5,
                           5,   5,   0,   -10, -10, 0,   5,   0,   0,   0,  0,
                           -10, -20, -10, -10, -5,  -5,  -10, -10, -20};
    double queen_ac[64] = {-20, -10, -10, -5,  -5,  -10, -10, -20, -10, 0,  5,
                           0,   0,   0,   0,   -10, -10, 5,   5,   5,   5,  5,
                           0,   -10, 0,   0,   5,   5,   5,   5,   0,   -5, -5,
                           0,   5,   5,   5,   5,   0,   -5,  -10, 0,   5,  5,
                           5,   5,   0,   -10, -10, 0,   0,   0,   0,   0,  0,
                           -10, -20, -10, -10, -5,  -5,  -10, -10, -20};
    double King_ac[64] = {-30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40,
                          -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40,
                          -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -20,
                          -30, -30, -40, -40, -30, -30, -20, -10, -20, -20, -20,
                          -20, -20, -20, -10, 20,  20,  0,   0,   0,   0,   20,
                          20,  20,  30,  10,  0,   0,   10,  30,  20};
    double king_ac[64] = {20,  30,  10,  0,   0,   10,  30,  20,  20,  20,  0,
                          0,   0,   0,   20,  20,  -10, -20, -20, -20, -20, -20,
                          -20, -10, -20, -30, -30, -40, -40, -30, -30, -20, -30,
                          -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50,
                          -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40,
                          -30, -30, -40, -40, -50, -50, -40, -40, -30};
    double King_endgame[64] = {
        -50, -40, -30, -20, -20, -30, -40, -50, -30, -20, -10, 0,   0,
        -10, -20, -30, -30, -10, 20,  30,  30,  20,  -10, -30, -30, -10,
        30,  40,  40,  30,  -10, -30, -30, -10, 30,  40,  40,  30,  -10,
        -30, -30, -10, 20,  30,  30,  20,  -10, -30, -30, -30, 0,   0,
        0,   0,   -30, -30, -50, -30, -30, -30, -30, -30, -30, -50};
    double king_endgame[64] = {
        -50, -30, -30, -30, -30, -30, -30, -50 - 30, -30, 0,   0,   0,   0,
        -30, -30, -30, -10, 20,  30,  30,  20,       -10, -30, -30, -10, 30,
        40,  40,  30,  -10, -30, -30, -10, 30,       40,  40,  30,  -10, -30,
        -30, -10, 20,  30,  30,  20,  -10, -30,      -30, -20, -10, 0,   0,
        -10, -20, -30, -50, -40, -30, -20, -20,      -30, -40, -50,
    };
    double activity = 0.0;
    for (int s = 0; s <= 63; s++) {
      char piece = board.squares[s];
      switch (piece) {
      case 'p':
        activity += -pawn_ac[s];
        break;
      case 'P':
        activity += Pawn_ac[s];
        break;
      case 'n':
        activity += -knight_ac[s];
        break;
      case 'N':
        activity += Knight_ac[s];
        break;
      case 'b':
        activity += -bishop_ac[s];
        break;
      case 'B':
        activity += Bishop_ac[s];
        break;
      case 'r':
        activity += -rook_ac[s];
        break;
      case 'R':
        activity += Rook_ac[s];
        break;
      case 'q':
        activity += -queen_ac[s];
        break;
      case 'Q':
        activity += Queen_ac[s];
        break;
      case 'k':
        if (not queen or minor_count <= 2) { // endgame
          activity += -king_endgame[s];
        } else {
          activity += -king_ac[s];
        }
        break;
      case 'K':
        if (not queen or minor_count <= 2) {
          activity += King_endgame[s];
        } else {
          activity += King_ac[s];
        }
        break;
      }
    }
    activity *= 0.1;
    // king safety
    char Kr = thc::get_rank(King);
    char Kf = thc::get_file(King);
    double W_attacked = 0;
    for (int dx = -1; dx <= 1; dx++) {
      for (int dy = -1; dy <= 1; dy++) {
        char new_Kr = Kr + dy;
        if (not(new_Kr >= '1' and new_Kr <= '8')) {
          continue;
        }
        char new_Kf = Kf + dx;
        if (not(new_Kf >= 'a' and new_Kf <= 'h')) {
          goto next;
        }
        if (board.AttackedSquare(thc::make_square(new_Kf, new_Kr), false)) {
          W_attacked += 1.0;
        }
      }
    next:;
    }
    char kr = thc::get_rank(king);
    char kf = thc::get_file(king);
    double B_attacked = 0;
    for (int dx = -1; dx <= 1; dx++) {
      for (int dy = -1; dy <= 1; dy++) {
        char new_kr = kr + dy;
        if (not(new_kr >= '1' and new_kr <= '8')) {
          continue;
        }
        char new_kf = kf + dx;
        if (not(new_kf >= 'a' and new_kf <= 'h')) {
          goto next1;
        }
        if (board.AttackedSquare(thc::make_square(new_kf, new_kr), true)) {
          B_attacked += 1.0;
        }
      }
    next1:;
    }
    double w_safety = W_attacked + 3 * B_checks;
    double b_safety = B_attacked + 3 * W_checks;
    double king_value = -w_safety + b_safety;
    // pawn structure
    double W_isolated = 0;
    double B_isolated = 0;
    double W_doubled = 0;
    double B_doubled = 0;
    map<char, int> W_f_pawn_count;
    map<char, int> B_f_pawn_count;
    for (int i = 0; i <= 63; i++) {
      if (board.squares[i] == 'p') {
        B_f_pawn_count[thc::get_file(static_cast<thc::Square>(i))]++;
      } else if (board.squares[i] == 'P') {
        W_f_pawn_count[thc::get_file(static_cast<thc::Square>(i))]++;
      }
    }
    for (char file = 'a'; file <= 'h'; file++) {
      if (W_f_pawn_count[file] and
          not(W_f_pawn_count[file - 1] or W_f_pawn_count[file + 1])) {
        W_isolated += 1.0;
        if (W_f_pawn_count[file] > 1) {
          W_doubled += 1.0;
        }
      }
      if (B_f_pawn_count[file] and
          not(B_f_pawn_count[file - 1] or B_f_pawn_count[file + 1])) {
        B_isolated += 1.0;
        if (B_f_pawn_count[file] > 1) {
          B_doubled += 1.0;
        }
      }
    }
    double pawn_value =
        -W_isolated - 3 * W_doubled + B_isolated + 3 * B_doubled;
    // space
    double W_space = 0;
    for (int s = 0; s <= static_cast<int>(thc::h5); s++) {
      W_space += board.AttackedSquare(static_cast<thc::Square>(s), true);
    }
    double B_space = 0;
    for (int s = static_cast<int>(thc::a4); s <= static_cast<int>(thc::h1);
         s++) {
      B_space += board.AttackedSquare(static_cast<thc::Square>(s), false);
    }
    double space = W_space - B_space;
    double sum = 5.6 * material + 0.1 * king_value + 0.1 * mobility +
                 0.5 * pawn_value + 0.5 * space + 1.0 * activity;
#if HEUR
    print();
    cout << "material: " << material << "\nking_value: " << king_value
         << "\nmobility: " << mobility << "\npawn_value: " << pawn_value
         << "\nspace: " << space << "\nactivity: " << activity << "\n"
         << "------\nsum: " << sum;
#endif
    return sum;
  }
};

inline double heuristic_terminal(const thc::TERMINAL &a) {
  switch (a) {
  case thc::NOT_TERMINAL:
    throw runtime_error("should only be called when pos terminal");
    break;
  case thc::TERMINAL_WCHECKMATE:
    return -mate_score;
    break;
  case thc::TERMINAL_BCHECKMATE:
    return mate_score;
    break;
  default:
    return 0;
  }
}

double heur_wrapper(Game &state, thc::Move m, bool check, bool mate,
                    bool stalemate, uint64_t Hash) {
  if (mate) {
    return mate_score * (state.board.WhiteToPlay() ? 1.0 : -1.0);
  }
  if (stalemate) {
    return 0;
  }
  uint64_t new_hash = state.Hash_roll(m, Hash);
  auto it = TT.find(new_hash);
  if (it != TT.end()) {
    return it->sc.value * (check ? 1000 : 1);
  }
  state.apply_move(m);
  double h = state.heuristic() * (check ? 1000 : 1);
  state.undo_move(m);
  return h;
}

pair<Move, double> alpha_beta(Game &state, Move a, int depth, double alpha,
                              double beta, bool maxi, uint64_t Hash) {
  thc::TERMINAL result = state.terminal();
  if (result != thc::NOT_TERMINAL) {
    return {{}, (depth + 1) * heuristic_terminal(result)};
  }
  auto memory = TT.find(Hash);
  if (memory != TT.end() and memory->sc.depth >= depth) {
#if HIT_CONFIRMATION
    cout << "Hit! Best Move: " << memory->sc.best_move->NaturalOut(&state.board)
         << " " << memory->sc.best_move->TerseOut()
         << "\nValue: " << memory->sc.value << "\nPosition: \n";
    state.print();
#endif
    return {memory->sc.best_move, memory->sc.value};
  }
  if (depth == 0) {
    return {a, state.heuristic()};
  }
  auto T = chrono::system_clock::now();
  if (chrono::duration_cast<chrono::milliseconds>(T - start) > per_move) {
    throw runtime_error("out of time");
  }
  if (maxi) {
    double value = -INFINITY;
    Moves moves;
    vb check;
    vb mate;
    vb stalemate;
    state.gen_moves(moves, check, mate, stalemate);
    Move ans = {moves[0]};
    vector<pair<Move, double>> moves_sorted(moves.size());
    if (DEPTH - depth < SORTED) { // sort by heuristic if close to root
      for (size_t i = 0; i < moves.size(); i++) {
        moves_sorted[i] = {moves[i], heur_wrapper(state, moves[i], check[i],
                                                  mate[i], stalemate[i], Hash)};
      }
    } else { // else just check forcing first (mates really should go first)
      for (size_t i = 0; i < moves.size(); i++) {
        moves_sorted[i] = {moves[i], (mate[i] ? INFINITY : 0.0) +
                                         (check[i] ? 1.0 : 0.0) +
                                         (stalemate[i] ? 1.0 : 0.0)};
      }
    }
    sort(moves_sorted.begin(), moves_sorted.end(),
         [state](const pair<Move, double> &a, const pair<Move, double> &b) {
           return a.sc > b.sc;
         });
    bool cutoff = false;
    for (auto &[m, h] : moves_sorted) {
      uint64_t new_hash = state.Hash_roll(m, Hash);
      state.apply_move(m);
      pair<Move, double> new_m =
          alpha_beta(state, m, depth - 1, alpha, beta, false, new_hash);
      state.undo_move(m);
      if (new_m.sc > value) {
        value = new_m.sc;
        ans = m;
      }
      alpha = max(alpha, value);
      if (beta <= alpha) {
        // beta cutoff
        cutoff = true;
        break;
      }
    }
#if debug
    cout << "Choosen: " << ans.value().NaturalOut(&state.board)
         << " as: " << (maxi ? "max " : "min ") << "at depth: " << depth
         << " with heuristic v of: " << value << "\n";
#endif
    if (depth != DEPTH and value < 0) {
      thc::DRAWTYPE res;
      if (state.board.IsDraw(true, res)) {
        return {{}, 0};
      }
    }
    if (memory != TT.end() and
        (memory->sc.value < value or memory->sc.depth < depth)) {
      memory->second = {ans, value, depth, cutoff ? Lower : Exact};
    } else {
      TT[Hash] = {ans, value, depth, cutoff ? Lower : Exact};
    }
    return {ans, value};
  } else {
    double value = INFINITY;
    Moves moves;
    vb check;
    vb mate;
    vb stalemate;
    state.gen_moves(moves, check, mate, stalemate);
    Move ans = {moves[0]};
    vector<pair<Move, double>> moves_sorted(moves.size());
    if (DEPTH - depth < SORTED) { // sort by heuristic if close to root
      for (size_t i = 0; i < moves.size(); i++) {
        moves_sorted[i] = {moves[i], heur_wrapper(state, moves[i], check[i],
                                                  mate[i], stalemate[i], Hash)};
      }
    } else { // else just check forcing first (mates really should go first)
      for (size_t i = 0; i < moves.size(); i++) {
        moves_sorted[i] = {moves[i], -(mate[i] ? INFINITY : 0.0) +
                                         (check[i] ? 1.0 : 0.0) +
                                         (stalemate[i] ? 1.0 : 0.0)};
      }
    }
    sort(moves_sorted.begin(), moves_sorted.end(),
         [state](const pair<Move, double> &a, const pair<Move, double> &b) {
           return a.sc < b.sc;
         });
    bool cutoff = false;
    for (auto &[m, h] : moves_sorted) {
      uint64_t new_hash = state.Hash_roll(m, Hash);
      state.apply_move(m);
      pair<Move, double> new_m =
          alpha_beta(state, m, depth - 1, alpha, beta, true, new_hash);
      state.undo_move(m);
      if (new_m.sc < value) {
        value = new_m.sc;
        ans = m;
      }
      beta = min(beta, value);
      if (beta <= alpha) {
        // alpha cutoff
        cutoff = true;
        break;
      }
    }
#if debug
    cout << "Choosen: " << ans.value().NaturalOut(&state.board)
         << " as: " << (maxi ? "max " : "min ") << "at depth: " << depth
         << " with heuristic v of: " << value << "\n";
#endif
    if (depth != DEPTH and value > 0) {
      thc::DRAWTYPE res;
      if (state.board.IsDraw(false, res)) {
        return {{}, 0};
      }
    }
    if (memory != TT.end() and
        (memory->sc.value > value or memory->sc.depth < depth)) {
      memory->second = {ans, value, depth, cutoff ? Lower : Exact};
    } else {
      TT[Hash] = {ans, value, depth, cutoff ? Lower : Exact};
    }
    return {ans, value};
  }
}

Move run_alpha_beta(Game state) {
  for (auto it = TT.begin(); it != TT.end();) {
    if (it->sc.history >= 8) {
      it = TT.erase(it);
    } else {
      (it++)->sc.history++;
    }
  }
  start = chrono::system_clock::now();
  uint64_t Hash = state.Hash();
  auto ans = alpha_beta(state, {}, 2, -INFINITY, INFINITY,
                        state.board.WhiteToPlay(), Hash);
  auto T = chrono::system_clock::now();
  for (int depth = 3;
       chrono::duration_cast<chrono::milliseconds>(T - start) < per_move;
       depth++, T = chrono::system_clock::now()) {
    DEPTH = depth;
    try {
      ans = alpha_beta(state, {}, depth, -INFINITY, INFINITY,
                       state.board.WhiteToPlay(), Hash);
    } catch (runtime_error A) {
      break;
    }
#if VERBOSE
    cout << "choosen " << ans.fi->NaturalOut(&state.board)
                        << " at depth: " << depth
                        << " with value of: " << ans.sc << endl;
#endif
  }
  return ans.fi;
}

void print_result(Game &state) {
  thc::TERMINAL result = state.terminal();
  switch (result) {
  case thc::TERMINAL_WCHECKMATE:
    cout << "Black wins!\n";
    break;
  case thc::TERMINAL_BCHECKMATE:
    cout << "White wins!\n";
    break;
  default:
    cout << "Stalemate!\n";
    break;
  }
}

int main() {
  Game state;
  cout << "From fen? (1/0) ";
  int fen;
  cin >> fen;
  if (fen) {
    cout << "fen:\n";
    string fen_in;
    cin >> ws;
    getline(cin, fen_in);
    state.board.Forsyth(fen_in.c_str());
  }
  cout << "Moves only? (1/0) ";
  int mo;
  cin >> mo;
  cout << "White or Black? (W/B) ";
  string choice;
  cin >> choice;
  transform(choice.begin(), choice.end(), choice.begin(), ::tolower);
  float seconds_per_move;
  cout << "Seconds per move? ";
  cin >> seconds_per_move;
  per_move =
      round<chrono::milliseconds>(chrono::duration<float>{seconds_per_move});
  while (state.terminal() == thc::NOT_TERMINAL) {
    string m;
    bool ok;
    thc::Move mv;
    if ((choice == "b" and state.board.WhiteToPlay()) or
        (choice == "w" and !state.board.WhiteToPlay())) {
      choice.clear();
      goto black;
    }
    do {
      cout << "Your move: ";
      cin >> m;
      ok = mv.NaturalIn(&state.board, m.c_str());
    } while (not ok);
    state.apply_move_no_undo({mv});
  black:
    if (not mo)
      state.print();
    if (state.terminal() != thc::NOT_TERMINAL) {
      break;
    }
    Move b = run_alpha_beta(state);
    if (b) {
      cout << b.value().NaturalOut(&state.board) << "\n";
      state.apply_move_no_undo(b);
    } else {
      cerr << "Fin?\n";
    }
    if (not mo)
      state.print();
  }
  print_result(state);
}
