#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#define MATE 32000
#define INF 32001
#define MAX_PLY 64
#define U8 unsigned __int8
#define S16 signed __int16
#define U16 unsigned __int16
#define S64 signed __int64
#define U64 unsigned __int64
#define FALSE 0
#define TRUE 1
#define NAME "Alien"
#define VERSION "2026-04-08"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define FLIP(sq) ((sq)^56)
#define RANK_1_BB       (U64)0x00000000000000FF
#define RANK_2_BB       (U64)0x000000000000FF00
#define RANK_3_BB       (U64)0x0000000000FF0000
#define RANK_4_BB       (U64)0x00000000FF000000
#define RANK_5_BB       (U64)0x000000FF00000000
#define RANK_6_BB       (U64)0x0000FF0000000000
#define RANK_7_BB       (U64)0x00FF000000000000
#define RANK_8_BB       (U64)0xFF00000000000000
#define FILE_A_BB       (U64)0x0101010101010101
#define FILE_B_BB       (U64)0x0202020202020202
#define FILE_C_BB       (U64)0x0404040404040404
#define FILE_D_BB       (U64)0x0808080808080808
#define FILE_E_BB       (U64)0x1010101010101010
#define FILE_F_BB       (U64)0x2020202020202020
#define FILE_G_BB       (U64)0x4040404040404040
#define FILE_H_BB       (U64)0x8080808080808080

#define DIAG_A1H8_BB    (U64)0x8040201008040201
#define DIAG_A8H1_BB    (U64)0x0102040810204080
#define DIAG_B8H2_BB    (U64)0x0204081020408000

#define File(x)         ((x) & 7)
#define Rank(x)         ((x) >> 3)
#define Sq(x, y)        (((y) << 3) | (x))

#define SqBb(x)         ((U64)1 << (x))

#define Map0x88(x)      (((x) & 7) | (((x) & ~7) << 1))
#define Unmap0x88(x)    (((x) & 7) | (((x) & ~7) >> 1))
#define Sq0x88Off(x)    ((unsigned)(x) & 0x88)

#define RankIndex(o, x) (((o) >> ((070 & (x)) + 1)) & 63)
#define FileIndex(o, x) (((FILE_A_BB & ((o) >> File(x))) * DIAG_B8H2_BB) >> 58)
#define DiagIndex(o, x) ((((o) & line_mask[2][x]) * FILE_B_BB) >> 58)
#define AntiIndex(o, x) ((((o) & line_mask[3][x]) * FILE_B_BB) >> 58)

#define L1Attacks(o, x) attacks[0][x][RankIndex(o, x)]
#define L2Attacks(o, x) attacks[1][x][FileIndex(o, x)]
#define L3Attacks(o, x) attacks[2][x][DiagIndex(o, x)]
#define L4Attacks(o, x) attacks[3][x][AntiIndex(o, x)]
#define RAttacks(o, x)  (L1Attacks(o, x) | L2Attacks(o, x))
#define BAttacks(o, x)  (L3Attacks(o, x) | L4Attacks(o, x))
#define QAttacks(o, x)  (RAttacks(o, x) | BAttacks(o, x))

enum Color { WHITE, BLACK, COLOR_NB };
enum PieceType { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, PT_NB };
enum Bound { LOWER, UPPER, EXACT };
enum File { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
enum Rank { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };

typedef struct {
	U8 flipped;
	U8 move50;
	U64 castling[4];
	U64 color[2];
	U64 pieces[6];
	U64 ep;
}Position;

typedef struct {
	U8 from;
	U8 to;
	U8 promo;
}Move;

typedef struct {
	S16 score;
	Move move;
	Move killer1;
	Move killer2;
} Stack;

typedef struct {
	U8 depthLimit;
	U8 post;
	U8 stop;
	U64 timeStart;
	U64 timeLimit;
	U64 nodes;
	U64 nodesLimit;
}SearchInfo;

typedef struct {
	U8 depth;
	U8 flag;
	S16 score;
	U64 hash;
	Move move;
}TTEntry;

U64 bbSquare[64];
U64 line_mask[4][64];
U64 attacks[4][64][64];
U64 bbKnightAttack[64];
U64 bbKingAttack[64];
int insufVal[PT_NB] = { 5,2,3,5,5,0 };
int phaseVal[PT_NB] = { 0,1,1,2,4,0 };
int mg_material[PT_NB] = { 82, 337, 365, 477, 1025, 0 };
int eg_material[PT_NB] = { 94, 281, 297, 512,  936, 0 };
int mx_material[PT_NB] = { 94, 337, 365, 477, 1025, 0 };

int mg_pawn_table[64] = {
	  0,   0,   0,   0,   0,   0,  0,   0,
	 98, 134,  61,  95,  68, 126, 34, -11,
	 -6,   7,  26,  31,  65,  56, 25, -20,
	-14,  13,   6,  21,  23,  12, 17, -23,
	-27,  -2,  -5,  12,  17,   6, 10, -25,
	-26,  -4,  -4, -10,   3,   3, 33, -12,
	-35,  -1, -20, -23, -15,  24, 38, -22,
	  0,   0,   0,   0,   0,   0,  0,   0,
};

int eg_pawn_table[64] = {
	  0,   0,   0,   0,   0,   0,   0,   0,
	178, 173, 158, 134, 147, 132, 165, 187,
	 94, 100,  85,  67,  56,  53,  82,  84,
	 32,  24,  13,   5,  -2,   4,  17,  17,
	 13,   9,  -3,  -7,  -7,  -8,   3,  -1,
	  4,   7,  -6,   1,   0,  -5,  -1,  -8,
	 13,   8,   8,  10,  13,   0,   2,  -7,
	  0,   0,   0,   0,   0,   0,   0,   0,
};

int mg_knight_table[64] = {
	-167, -89, -34, -49,  61, -97, -15, -107,
	 -73, -41,  72,  36,  23,  62,   7,  -17,
	 -47,  60,  37,  65,  84, 129,  73,   44,
	  -9,  17,  19,  53,  37,  69,  18,   22,
	 -13,   4,  16,  13,  28,  19,  21,   -8,
	 -23,  -9,  12,  10,  19,  17,  25,  -16,
	 -29, -53, -12,  -3,  -1,  18, -14,  -19,
	-105, -21, -58, -33, -17, -28, -19,  -23,
};

int eg_knight_table[64] = {
	-58, -38, -13, -28, -31, -27, -63, -99,
	-25,  -8, -25,  -2,  -9, -25, -24, -52,
	-24, -20,  10,   9,  -1,  -9, -19, -41,
	-17,   3,  22,  22,  22,  11,   8, -18,
	-18,  -6,  16,  25,  16,  17,   4, -18,
	-23,  -3,  -1,  15,  10,  -3, -20, -22,
	-42, -20, -10,  -5,  -2, -20, -23, -44,
	-29, -51, -23, -15, -22, -18, -50, -64,
};

int mg_bishop_table[64] = {
	-29,   4, -82, -37, -25, -42,   7,  -8,
	-26,  16, -18, -13,  30,  59,  18, -47,
	-16,  37,  43,  40,  35,  50,  37,  -2,
	 -4,   5,  19,  50,  37,  37,   7,  -2,
	 -6,  13,  13,  26,  34,  12,  10,   4,
	  0,  15,  15,  15,  14,  27,  18,  10,
	  4,  15,  16,   0,   7,  21,  33,   1,
	-33,  -3, -14, -21, -13, -12, -39, -21,
};

int eg_bishop_table[64] = {
	-14, -21, -11,  -8, -7,  -9, -17, -24,
	 -8,  -4,   7, -12, -3, -13,  -4, -14,
	  2,  -8,   0,  -1, -2,   6,   0,   4,
	 -3,   9,  12,   9, 14,  10,   3,   2,
	 -6,   3,  13,  19,  7,  10,  -3,  -9,
	-12,  -3,   8,  10, 13,   3,  -7, -15,
	-14, -18,  -7,  -1,  4,  -9, -15, -27,
	-23,  -9, -23,  -5, -9, -16,  -5, -17,
};

int mg_rook_table[64] = {
	 32,  42,  32,  51, 63,  9,  31,  43,
	 27,  32,  58,  62, 80, 67,  26,  44,
	 -5,  19,  26,  36, 17, 45,  61,  16,
	-24, -11,   7,  26, 24, 35,  -8, -20,
	-36, -26, -12,  -1,  9, -7,   6, -23,
	-45, -25, -16, -17,  3,  0,  -5, -33,
	-44, -16, -20,  -9, -1, 11,  -6, -71,
	-19, -13,   1,  17, 16,  7, -37, -26,
};

int eg_rook_table[64] = {
	13, 10, 18, 15, 12,  12,   8,   5,
	11, 13, 13, 11, -3,   3,   8,   3,
	 7,  7,  7,  5,  4,  -3,  -5,  -3,
	 4,  3, 13,  1,  2,   1,  -1,   2,
	 3,  5,  8,  4, -5,  -6,  -8, -11,
	-4,  0, -5, -1, -7, -12,  -8, -16,
	-6, -6,  0,  2, -9,  -9, -11,  -3,
	-9,  2,  3, -1, -5, -13,   4, -20,
};

int mg_queen_table[64] = {
	-28,   0,  29,  12,  59,  44,  43,  45,
	-24, -39,  -5,   1, -16,  57,  28,  54,
	-13, -17,   7,   8,  29,  56,  47,  57,
	-27, -27, -16, -16,  -1,  17,  -2,   1,
	 -9, -26,  -9, -10,  -2,  -4,   3,  -3,
	-14,   2, -11,  -2,  -5,   2,  14,   5,
	-35,  -8,  11,   2,   8,  15,  -3,   1,
	 -1, -18,  -9,  10, -15, -25, -31, -50,
};

int eg_queen_table[64] = {
	 -9,  22,  22,  27,  27,  19,  10,  20,
	-17,  20,  32,  41,  58,  25,  30,   0,
	-20,   6,   9,  49,  47,  35,  19,   9,
	  3,  22,  24,  45,  57,  40,  57,  36,
	-18,  28,  19,  47,  31,  34,  39,  23,
	-16, -27,  15,   6,   9,  17,  10,   5,
	-22, -23, -30, -16, -16, -23, -36, -32,
	-33, -28, -22, -43,  -5, -32, -20, -41,
};

int mg_king_table[64] = {
	-65,  23,  16, -15, -56, -34,   2,  13,
	 29,  -1, -20,  -7,  -8,  -4, -38, -29,
	 -9,  24,   2, -16, -20,   6,  22, -22,
	-17, -20, -12, -27, -30, -25, -14, -36,
	-49,  -1, -27, -39, -46, -44, -33, -51,
	-14, -14, -22, -46, -44, -30, -15, -27,
	  1,   7,  -8, -64, -43, -16,   9,   8,
	-15,  36,  12, -54,   8, -28,  24,  14,
};

int eg_king_table[64] = {
	-74, -35, -18, -18, -11,  15,   4, -17,
	-12,  17,  14,  17,  17,  38,  23,  11,
	 10,  17,  23,  15,  20,  45,  44,  13,
	 -8,  22,  24,  27,  26,  33,  26,   3,
	-18,  -4,  21,  24,  27,  23,   9, -11,
	-19,  -3,  11,  21,  23,  16,   7,  -9,
	-27, -11,   4,  13,  14,   4,  -5, -17,
	-53, -34, -21, -11, -28, -14, -24, -43
};

int* mg_table[6] = {
	mg_pawn_table,
	mg_knight_table,
	mg_bishop_table,
	mg_rook_table,
	mg_queen_table,
	mg_king_table
};

int* eg_table[6] = {
	eg_pawn_table,
	eg_knight_table,
	eg_bishop_table,
	eg_rook_table,
	eg_queen_table,
	eg_king_table
};

int mg_pst[PT_NB][64];
int eg_pst[PT_NB][64];
U64 keys[848];
int historyCount = 0;
U64 historyHash[1024];

Stack stack[MAX_PLY];
SearchInfo info;
const U64 tt_count = 64ULL << 15;
TTEntry tt[64ULL << 15];

void UciCommand(Position* pos, char* line);

static inline int Equal(const Move lhs, const Move rhs) { return !memcmp(&rhs, &lhs, sizeof(Move)); }
static inline int FileOf(int sq) { return sq & 7; }
static inline int RankOf(int sq) { return sq >> 3; }
static inline U64 GetTimeMs() { return GetTickCount64(); }
static inline U64 FlipBitboard(const U64 bb) { return _byteswap_uint64(bb); }
static inline U64 LSB(const U64 bb) { return _tzcnt_u64(bb); }
static inline U64 Count(const U64 bb) { return _mm_popcnt_u64(bb); }
static inline U64 East(const U64 bb) { return (bb << 1) & ~FILE_A_BB; }
static inline U64 West(const U64 bb) { return (bb >> 1) & ~FILE_H_BB; }
static inline U64 North(const U64 bb) { return bb << 8; }
static inline U64 South(const U64 bb) { return bb >> 8; }
static inline U64 NW(const U64 bb) { return (bb << 7) & ~FILE_H_BB; }
static inline U64 NE(const U64 bb) { return (bb << 9) & ~FILE_A_BB; }
static inline U64 SW(const U64 bb) { return (bb >> 9) & ~FILE_H_BB; }
static inline U64 SE(const U64 bb) { return (bb >> 7) & ~FILE_A_BB; }

static void Swap(U64* a, U64* b) {
	U64 temp = *a;
	*a = *b;
	*b = temp;
}

static int PieceTypeOnSquare(Position* pos, int sq) {
	const U64 bb = 1ULL << sq;
	for (int i = PAWN; i < PT_NB; ++i)
		if (pos->pieces[i] & bb)
			return i;
	return PT_NB;
}

static U64 Rand64() {
	static U64 next = 1234;
	U64 z = (next += 0x9E3779B97F4A7C15ULL);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return (unsigned long long)(z ^ (z >> 31));
}

static U64 Ray(const U64 bb, const U64 blockers, U64(*f)(U64)) {
	U64 mask = f(bb);
	mask |= f(mask & ~blockers);
	mask |= f(mask & ~blockers);
	mask |= f(mask & ~blockers);
	mask |= f(mask & ~blockers);
	mask |= f(mask & ~blockers);
	mask |= f(mask & ~blockers);
	mask |= f(mask & ~blockers);
	return mask;
}

static U64 KnightAttackBB(const U64 bb) {
	return (((bb << 15) | (bb >> 17)) & 0x7F7F7F7F7F7F7F7FULL) | (((bb << 17) | (bb >> 15)) & 0xFEFEFEFEFEFEFEFEULL) |
		(((bb << 10) | (bb >> 6)) & 0xFCFCFCFCFCFCFCFCULL) | (((bb << 6) | (bb >> 10)) & 0x3F3F3F3F3F3F3F3FULL);
}

static U64 KnightAttack(const int sq) {
	return bbKnightAttack[sq];
}

static U64 BishopAttackBB(U64 bb, U64 blockers) {
	return Ray(bb, blockers, NW) | Ray(bb, blockers, NE) | Ray(bb, blockers, SW) | Ray(bb, blockers, SE);
}

static U64 BishopAttack(const int sq, const U64 blockers) {
	return BAttacks(blockers, sq);
}

static U64 RookAttackBB(U64 bb, U64 blockers) {
	return Ray(bb, blockers, North) | Ray(bb, blockers, East) | Ray(bb, blockers, South) | Ray(bb, blockers, West);
}

static U64 RookAttack(const int sq, const U64 blockers) {
	return RAttacks(blockers, sq);
}

static U64 KingAttackBB(const U64 bb) {
	return (bb << 8) | (bb >> 8) | (((bb >> 1) | (bb >> 9) | (bb << 7)) & 0x7F7F7F7F7F7F7F7FULL) |
		(((bb << 1) | (bb << 9) | (bb >> 7)) & 0xFEFEFEFEFEFEFEFEULL);
}

static U64 QueenAttack(const int sq, const U64 blockers) {
	return QAttacks(blockers, sq);
}

static U64 KingAttack(const int sq) {
	return bbKingAttack[sq];
}

static void Init() {
	static const int dirs[4][2] = { {1, -1}, {16, -16}, {17, -17}, {15, -15} };
	for (int i = 0; i < 848; ++i)
		keys[i] = Rand64();
	for (int pt = PAWN; pt <= KING; pt++) {
		for (int sq = 0; sq < 64; sq++) {
			mg_pst[pt][sq] = mg_material[pt] + mg_table[pt][FLIP(sq)];
			eg_pst[pt][sq] = eg_material[pt] + eg_table[pt][FLIP(sq)];
		}
	}
	for (int sq = 0; sq < 64; ++sq) {
		U64 bb = 1ULL << sq;
		bbSquare[sq] = bb;
		bbKnightAttack[sq] = KnightAttackBB(bb);
		bbKingAttack[sq] = KingAttackBB(bb);
	}
	for (int i = 0; i < 64; i++) {
		line_mask[0][i] = RANK_1_BB << (i & 070);
		line_mask[1][i] = FILE_A_BB << (i & 007);
		int j = File(i) - Rank(i);
		if (j > 0)
			line_mask[2][i] = DIAG_A1H8_BB >> (j * 8);
		else
			line_mask[2][i] = DIAG_A1H8_BB << (-j * 8);
		j = File(i) - (RANK_8 - Rank(i));
		if (j > 0)
			line_mask[3][i] = DIAG_A8H1_BB << (j * 8);
		else
			line_mask[3][i] = DIAG_A8H1_BB >> (-j * 8);
	}
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 64; j++)
			for (int k = 0; k < 64; k++) {
				attacks[i][j][k] = 0;
				for (int l = 0; l < 2; l++) {
					int x = Map0x88(j) + dirs[i][l];
					while (!Sq0x88Off(x)) {
						int y = Unmap0x88(x);
						attacks[i][j][k] |= SqBb(y);
						if ((k << 1) & (1 << (i != 1 ? File(y) : Rank(y))))
							break;
						x += dirs[i][l];
					}
				}
			}
}

static void FlipPosition(Position* pos) {
	pos->color[0] = FlipBitboard(pos->color[0]);
	pos->color[1] = FlipBitboard(pos->color[1]);
	for (int i = PAWN; i < PT_NB; ++i)
		pos->pieces[i] = FlipBitboard(pos->pieces[i]);
	pos->ep = FlipBitboard(pos->ep);
	Swap(&pos->color[0], &pos->color[1]);
	Swap(&pos->castling[0], &pos->castling[2]);
	Swap(&pos->castling[1], &pos->castling[3]);
	pos->flipped = !pos->flipped;
}

static int CheckUp(Position* pos) {
	if ((++info.nodes & 0xffff) == 0) {
		if (info.timeLimit && GetTimeMs() - info.timeStart > info.timeLimit)
			info.stop = TRUE;
		if (info.nodesLimit && info.nodes > info.nodesLimit)
			info.stop = TRUE;
		if (InputAvailable()) {
			char line[4000];
			fgets(line, sizeof(line), stdin);
			UciCommand(pos, line);
		}
	}
	return info.stop;
}

static U64 Attacked(Position* pos, int sq, int them) {
	const U64 bb = 1ULL << sq;
	const U64 kt = pos->color[them] & pos->pieces[KNIGHT];
	const U64 BQ = pos->pieces[BISHOP] | pos->pieces[QUEEN];
	const U64 RQ = pos->pieces[ROOK] | pos->pieces[QUEEN];
	const U64 pawns = pos->color[them] & pos->pieces[PAWN];
	const U64 pawn_attacks = them ? SW(pawns) | SE(pawns) : NW(pawns) | NE(pawns);
	return (pawn_attacks & bb) | (kt & KnightAttack(sq)) |
		(BishopAttack(sq, pos->color[0] | pos->color[1]) & pos->color[them] & BQ) |
		(RookAttack(sq, pos->color[0] | pos->color[1]) & pos->color[them] & RQ) |
		(KingAttack(sq) & pos->color[them] & pos->pieces[KING]);
}

static void AddMove(Move* const movelist, int* num_moves, const int from, const int to, const int promo) {
	Move* m = &movelist[(*num_moves)++];
	m->from = from;
	m->to = to;
	m->promo = promo;
}

static void GeneratePawnMoves(Move* const movelist, int* num_moves, U64 to_mask, const int offset) {
	while (to_mask) {
		const int to = (int)LSB(to_mask);
		to_mask &= to_mask - 1;
		if (to >= 56) {
			AddMove(movelist, num_moves, to + offset, to, QUEEN);
			AddMove(movelist, num_moves, to + offset, to, ROOK);
			AddMove(movelist, num_moves, to + offset, to, BISHOP);
			AddMove(movelist, num_moves, to + offset, to, KNIGHT);
		}
		else
			AddMove(movelist, num_moves, to + offset, to, PT_NB);
	}
}

static void GeneratePieceMoves(Move* const movelist, int* num_moves, const Position* pos, const int piece, const U64 to_mask, U64(*func)(int, U64)) {
	U64 copy = pos->color[0] & pos->pieces[piece];
	while (copy) {
		const int fr = LSB(copy);
		copy &= copy - 1;
		U64 moves = func(fr, pos->color[0] | pos->color[1]) & to_mask;
		while (moves) {
			const int to = LSB(moves);
			moves &= moves - 1;
			AddMove(movelist, num_moves, fr, to, PT_NB);
		}
	}
}

static int MoveGen(const Position* pos, Move* const movelist, int only_captures) {
	int num_moves = 0;
	const U64 all = pos->color[0] | pos->color[1];
	const U64 to_mask = only_captures ? pos->color[1] : ~pos->color[0];
	const U64 pawns = pos->color[0] & pos->pieces[PAWN];
	U64 maskTo = North(pawns) & ~all & (only_captures ? 0xFF00000000000000ULL : 0xFFFFFFFFFFFF0000ULL);
	GeneratePawnMoves(movelist, &num_moves, maskTo, -8);
	if (!only_captures)
		GeneratePawnMoves(movelist, &num_moves, North(North(pawns & 0xFF00ULL) & ~all) & ~all, -16);
	GeneratePawnMoves(movelist, &num_moves, NW(pawns) & (pos->color[1] | pos->ep), -7);
	GeneratePawnMoves(movelist, &num_moves, NE(pawns) & (pos->color[1] | pos->ep), -9);
	GeneratePieceMoves(movelist, &num_moves, pos, KNIGHT, to_mask, KnightAttack);
	GeneratePieceMoves(movelist, &num_moves, pos, BISHOP, to_mask, BishopAttack);
	GeneratePieceMoves(movelist, &num_moves, pos, ROOK, to_mask, RookAttack);
	GeneratePieceMoves(movelist, &num_moves, pos, QUEEN, to_mask, QueenAttack);
	GeneratePieceMoves(movelist, &num_moves, pos, KING, to_mask, KingAttack);
	if (!only_captures && pos->castling[0] && !(all & 0x60ULL) && !Attacked(pos, 4, 1) && !Attacked(pos, 5, 1)) {
		AddMove(movelist, &num_moves, 4, 6, PT_NB);
	}
	if (!only_captures && pos->castling[1] && !(all & 0xEULL) && !Attacked(pos, 4, 1) && !Attacked(pos, 3, 1)) {
		AddMove(movelist, &num_moves, 4, 2, PT_NB);
	}
	return num_moves;
}

static int IsRepetition(Position* pos, U64 hash) {
	int limit = max(0, historyCount - pos->move50);
	for (int n = historyCount - 4; n >= limit; n -= 2)
		if (historyHash[n] == hash)
			return TRUE;
	return FALSE;
}

static U64 GetHash(const Position* pos) {
	U64 hash = pos->flipped;
	for (int p = PAWN; p < PT_NB; ++p) {
		U64 copy = pos->pieces[p] & pos->color[0];
		while (copy) {
			const int sq = LSB(copy);
			copy &= copy - 1;
			hash ^= keys[p * 64 + sq];
		}
		copy = pos->pieces[p] & pos->color[1];
		while (copy) {
			const int sq = LSB(copy);
			copy &= copy - 1;
			hash ^= keys[p * 64 + sq + 6 * 64];
		}
	}
	if (pos->ep)
		hash ^= keys[12 * 64 + LSB(pos->ep)];
	hash ^= keys[13 * 64 + pos->castling[0] + pos->castling[1] * 2 + pos->castling[2] * 4 + pos->castling[3] * 8];
	return hash;
}

static void SetFen(Position* pos, char* fen) {
	memset(pos, 0, sizeof(Position));
	int sq = 56;
	while (*fen && *fen != ' ') {
		U64 bb = 1ull << sq;
		switch (*fen) {
		case '1': sq += 1; break;
		case '2': sq += 2; break;
		case '3': sq += 3; break;
		case '4': sq += 4; break;
		case '5': sq += 5; break;
		case '6': sq += 6; break;
		case '7': sq += 7; break;
		case '8': sq += 8; break;
		case 'P': pos->color[0] |= bb; pos->pieces[PAWN] |= bb; ++sq; break;
		case 'N': pos->color[0] |= bb; pos->pieces[KNIGHT] |= bb; ++sq; break;
		case 'B': pos->color[0] |= bb; pos->pieces[BISHOP] |= bb; ++sq; break;
		case 'R': pos->color[0] |= bb; pos->pieces[ROOK] |= bb; ++sq; break;
		case 'Q': pos->color[0] |= bb; pos->pieces[QUEEN] |= bb; ++sq; break;
		case 'K': pos->color[0] |= bb; pos->pieces[KING] |= bb; ++sq; break;
		case 'p': pos->color[1] |= bb; pos->pieces[PAWN] |= bb; ++sq; break;
		case 'n': pos->color[1] |= bb; pos->pieces[KNIGHT] |= bb; ++sq; break;
		case 'b': pos->color[1] |= bb; pos->pieces[BISHOP] |= bb; ++sq; break;
		case 'r': pos->color[1] |= bb; pos->pieces[ROOK] |= bb; ++sq; break;
		case 'q': pos->color[1] |= bb; pos->pieces[QUEEN] |= bb; ++sq; break;
		case 'k': pos->color[1] |= bb; pos->pieces[KING] |= bb; ++sq; break;
		case '/': sq -= 16; break;
		}
		fen++;
	}
	fen++;
	int flipped = *fen == 'b';
	while (*fen && *fen != ' ') fen++; fen++;
	while (*fen && *fen != ' ') {
		switch (*fen) {
		case 'K': pos->castling[0] = 1; break;
		case 'Q': pos->castling[1] = 1; break;
		case 'k': pos->castling[2] = 1; break;
		case 'q': pos->castling[3] = 1; break;
		case '-': break;
		}
		fen++;
	}
	fen++;
	if (*fen != '-')
		pos->ep = fen[0] - 'a' + 8 * (7 - (fen[1] - '1'));
	while (*fen && *fen != ' ') fen++; fen++;
	pos->move50 = atoi(fen);
	if (flipped)
		FlipPosition(pos);
}

static char* ParseToken(char* string, char* token) {
	while (*string == ' ')
		string++;
	while (*string != ' ' && *string != '\0')
		*token++ = *string++;
	*token = '\0';
	return string;
}

static char* MoveToUci(Move move, int flip) {
	static char str[6] = { 0 };
	str[0] = 'a' + (move.from % 8);
	str[1] = '1' + (flip ? (7 - move.from / 8) : (move.from / 8));
	str[2] = 'a' + (move.to % 8);
	str[3] = '1' + (flip ? (7 - move.to / 8) : (move.to / 8));
	str[4] = "\0nbrq\0\0"[move.promo];
	return str;
}

static int MakeMove(Position* pos, const Move* move) {
	const int piece = PieceTypeOnSquare(pos, move->from);
	const int captured = PieceTypeOnSquare(pos, move->to);
	const U64 to = 1ULL << move->to;
	const U64 from = 1ULL << move->from;
	pos->move50++;
	if (captured != PT_NB || piece == PAWN)
		pos->move50 = 0;
	pos->color[0] ^= from | to;
	pos->pieces[piece] ^= from | to;
	if (piece == PAWN && to == pos->ep) {
		pos->color[1] ^= to >> 8;
		pos->pieces[PAWN] ^= to >> 8;
	}
	pos->ep = 0x0ULL;
	if (piece == PAWN && move->to - move->from == 16)
		pos->ep = to >> 8;
	if (captured != PT_NB) {
		pos->color[1] ^= to;
		pos->pieces[captured] ^= to;
	}
	if (piece == KING) {
		const U64 bb = move->to - move->from == 2 ? 0xa0ULL : move->to - move->from == -2 ? 0x9ULL : 0x0ULL;
		pos->color[0] ^= bb;
		pos->pieces[ROOK] ^= bb;
	}
	if (piece == PAWN && move->to >= 56) {
		pos->pieces[PAWN] ^= to;
		pos->pieces[move->promo] ^= to;
	}
	pos->castling[0] &= ((from | to) & 0x90ULL) == 0;
	pos->castling[1] &= ((from | to) & 0x11ULL) == 0;
	pos->castling[2] &= ((from | to) & 0x9000000000000000ULL) == 0;
	pos->castling[3] &= ((from | to) & 0x1100000000000000ULL) == 0;
	FlipPosition(pos);

	return !Attacked(pos, LSB(pos->color[1] & pos->pieces[KING]), 0);
}

static Move UciToMove(char* s, int flip) {
	Move m;
	m.from = (s[0] - 'a');
	int f = (s[1] - '1');
	m.from += 8 * (flip ? 7 - f : f);
	m.to = (s[2] - 'a');
	f = (s[3] - '1');
	m.to += 8 * (flip ? 7 - f : f);
	m.promo = PT_NB;
	switch (s[4]) {
	case 'N':
	case 'n':
		m.promo = KNIGHT;
		break;
	case 'B':
	case 'b':
		m.promo = BISHOP;
		break;
	case 'R':
	case 'r':
		m.promo = ROOK;
		break;
	case 'Q':
	case 'q':
		m.promo = QUEEN;
		break;
	}
	return m;
}

static int Center(int rank, int file) {
	return -abs(rank * 2 - 7) / 2 - abs(file * 2 - 7) / 2;
}

static int CenterSq(int sq) {
	int rank = sq / 8;
	int file = sq % 8;
	return Center(rank, file);
}

static int EvalMove(Position* pos, Move* bst, Move* m) {
	int ptSou = PieceTypeOnSquare(pos, m->from);
	int ptDes = PieceTypeOnSquare(pos, m->to);
	int score = CenterSq(m->to) - CenterSq(m->from);
	//int score = mx_pst[ptSou][m->from] - mx_pst[ptSou][m->to];
	if ((m->from == bst->from) && (m->to == bst->to))
		score += 10000;
	if (ptDes != PT_NB)
		score += mx_material[ptDes] - mx_material[ptSou] / 10;
	if (m->promo < PT_NB)
		score += mx_material[m->promo] - mx_material[PAWN];
	return score;
}

static U64 Attacks(int pt, int sq, U64 blockers) {
	switch (pt) {
	case KNIGHT:
		return KnightAttack(sq);
	case BISHOP:
		return  BAttacks(blockers, sq);
	case ROOK:
		return  RAttacks(blockers, sq);
	case QUEEN:
		return  QAttacks(blockers, sq);
	case KING:
		return KingAttack(sq);
	default:
		return 0;
	}
}

static int EvalPosition(Position* pos) {
	int phase = 0;
	int score = 0;
	int scoreMg = 0;
	int scoreEg = 0;
	int insufficient[2] = { 0 };
	U64 bbBlockers = pos->color[0] | pos->color[1];
	for (int c = WHITE; c < COLOR_NB; ++c) {
		U64 bbStart1 = pos->color[1] & pos->pieces[PAWN];
		U64 bbControl1 = SW(bbStart1) | SE(bbStart1);
		for (int pt = PAWN; pt < PT_NB; ++pt) {
			U64 copy = pos->color[0] & pos->pieces[pt];
			while (copy) {
				const int fr = (int)LSB(copy);
				copy &= copy - 1;
				scoreMg += mg_pst[pt][fr];
				scoreEg += eg_pst[pt][fr];
				phase += phaseVal[pt];
				insufficient[c] += insufVal[pt];
				if (pt > PAWN && pt < KING) {
					U64 bbAttack = Attacks(pt, fr, bbBlockers);
					score += Count(bbAttack & ~bbControl1);
				}
			}
		}
		U64 bbStart0 = pos->color[0] & pos->pieces[KING];
		U64 file0 = FILE_A_BB << FileOf(LSB(bbStart0));
		file0 |= East(file0) | West(file0);
		U64 bbAttack0 = file0 & ~(FILE_D_BB | FILE_E_BB);
		bbAttack0 &= (pos->color[0] & pos->pieces[PAWN]);
		scoreMg += Count(bbAttack0 & RANK_2_BB) * 16;
		scoreMg += Count(bbAttack0 & RANK_3_BB) * 8;
		FlipPosition(pos);
		score = -score;
		scoreMg = -scoreMg;
		scoreEg = -scoreEg;
	}
	if (max(insufficient[0], insufficient[1]) < 5)return 0;
	if (insufficient[score < 0] < 4)return 0;
	if (phase > 24) phase = 24;
	score += (scoreMg * phase + scoreEg * (24 - phase)) / 24;
	return (100 - pos->move50) * score / 100;
}

static void PrintBitboard(U64 bb) {
	const char* s = "   +---+---+---+---+---+---+---+---+\n";
	const char* t = "     A   B   C   D   E   F   G   H\n";
	printf(t);
	for (int r = 7; r >= 0; r--) {
		printf(s);
		printf(" %d |", r + 1);
		for (int f = 0; f < 8; f++) {
			int sq = r * 8 + f;
			printf(" %c |", bb & 1ull << sq ? 'x' : ' ');
		}
		printf(" %d \n", r + 1);
	}
	printf(s);
	printf(t);
}

static void PrintBoard(Position* pos) {
	Position npos = *pos;
	if (npos.flipped)
		FlipPosition(&npos);
	const char* s = "   +---+---+---+---+---+---+---+---+\n";
	const char* t = "     A   B   C   D   E   F   G   H\n";
	printf(t);
	for (int r = 7; r >= 0; r--) {
		printf(s);
		printf(" %d |", r + 1);
		for (int f = 0; f < 8; f++) {
			int sq = r * 8 + f;
			int piece = PieceTypeOnSquare(&npos, sq);
			if (npos.color[0] & (1ull << sq))
				printf(" %c |", "ANBRQK "[piece]);
			else
				printf(" %c |", "anbrqk "[piece]);
		}
		printf(" %d \n", r + 1);
	}
	printf(s);
	printf(t);
	char castling[5] = "KQkq";
	for (int n = 0; n < 4; n++)
		if (!npos.castling[n])
			castling[n] = '-';
	printf("side     : %16s\n", pos->flipped ? "black" : "white");
	printf("castling : %16s\n", castling);
	printf("hash     : %16llx\n", GetHash(pos));
}

static int InputAvailable(void) {
	static int init = 0, pipe;
	static HANDLE inh;
	DWORD dw;
	if (!init) {
		init = 1;
		inh = GetStdHandle(STD_INPUT_HANDLE);
		pipe = !GetConsoleMode(inh, &dw);
		if (!pipe) {
			SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
			FlushConsoleInputBuffer(inh);
		}
	}
	if (pipe) {
		if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL))
			return 1;
		return dw > 0;
	}
	else {
		GetNumberOfConsoleInputEvents(inh, &dw);
		return dw > 1;
	}
}

static int Permill() {
	int pm = 0;
	for (int n = 0; n < 1000; n++)
		if (tt[n].hash)
			pm++;
	return pm;
}

static int IsPseudolegalMove(const Position* pos, const Move move) {
	Move moves[256];
	const int num_moves = MoveGen(pos, moves, 0);
	for (int i = 0; i < num_moves; ++i)
		if (moves[i].from == move.from && moves[i].to == move.to)
			return 1;
	return 0;
}

static void PrintPv(const Position* pos, const Move move) {
	if (!IsPseudolegalMove(pos, move))
		return;
	const Position npos = *pos;
	if (!MakeMove(&npos, &move))
		return;
	printf(" %s", MoveToUci(move, pos->flipped));
	const U64 hash = GetHash(&npos);
	TTEntry* tt_entry = tt + (hash % tt_count);
	if (tt_entry->hash != hash || tt_entry->flag != EXACT || IsRepetition(&npos, hash))
		return;
	historyHash[historyCount++] = hash;
	PrintPv(&npos, tt_entry->move);
	historyCount--;
}

static void PrintInfo(Position* pos, int depth, int score) {
	printf("info depth %d score ", depth);
	if (abs(score) < MATE - MAX_PLY)
		printf("cp %d", score);
	else
		printf("mate %d", (score > 0 ? (MATE - score + 1) >> 1 : -(MATE + score) >> 1));
	printf(" time %lld", GetTimeMs() - info.timeStart);
	printf(" nodes %lld", info.nodes);
	printf(" hashfull %d pv", Permill());
	PrintPv(pos, stack[0].move);
	printf("\n");
}

static void ResetInfo() {
	info.timeStart = GetTimeMs();
	info.timeLimit = 0;
	info.depthLimit = MAX_PLY;
	info.nodesLimit = 0;
	info.nodes = 0;
	info.stop = FALSE;
	info.post = TRUE;
}

static inline void PerftDriver(Position* pos, int depth) {
	Move moves[256];
	const int num_moves = MoveGen(pos, moves, 0);
	for (int n = 0; n < num_moves; n++) {
		Position npos = *pos;
		if (!MakeMove(&npos, &moves[n]))
			continue;
		if (depth)
			PerftDriver(&npos, depth - 1);
		else
			info.nodes++;
	}
}

static int ShrinkNumber(U64 n) {
	if (n < 10000)
		return 0;
	if (n < 10000000)
		return 1;
	if (n < 10000000000)
		return 2;
	return 3;
}

static void PrintSummary(U64 time, U64 nodes) {
	U64 nps = (nodes * 1000) / max(time, 1);
	const char* units[] = { "", "k", "m", "g" };
	int sn = ShrinkNumber(nps);
	int p = pow(10, sn * 3);
	int b = pow(10, 3);
	printf("-----------------------------\n");
	printf("Time        : %llu\n", time);
	printf("Nodes       : %llu\n", nodes);
	printf("Nps         : %llu (%llu%s/s)\n", nps, nps / p, units[sn]);
	printf("-----------------------------\n");
}

void PrintPerformanceHeader() {
	printf("-----------------------------\n");
	printf("ply      time        nodes\n");
	printf("-----------------------------\n");
}

static int SearchAlpha(Position* pos, int alpha, int beta, int depth, int ply, Stack* stack, int doNull) {
	if (CheckUp(pos))
		return 0;
	int  mate_value = MATE - ply;
	if (alpha < -mate_value) alpha = -mate_value;
	if (beta > mate_value - 1) beta = mate_value - 1;
	if (alpha >= beta) return alpha;
	int staticEval = EvalPosition(pos);
	if (ply >= MAX_PLY)
		return staticEval;
	stack[ply].score = staticEval;
	const U64 inCheck = Attacked(pos, (int)LSB(pos->color[0] & pos->pieces[KING]), 1);
	if (inCheck)
		depth = max(1, depth + 1);
	int inQuiescence = depth < 1;
	if (inQuiescence && alpha < staticEval) {
		alpha = staticEval;
		if (alpha >= beta)
			return beta;
	}
	const U64 hash = GetHash(pos);
	if (ply && !inQuiescence)
		if (pos->move50 > 99 + inCheck || IsRepetition(pos, hash))
			return 0;
	TTEntry* ttEntry = tt + (hash % tt_count);
	Move tt_move = { 0 };
	if (ttEntry->hash == hash) {
		tt_move = ttEntry->move;
		if (alpha == beta - 1 && ttEntry->depth >= depth) {
			if (ttEntry->flag == EXACT)return ttEntry->score;
			if (ttEntry->flag == LOWER && ttEntry->score <= alpha)return ttEntry->score;
			if (ttEntry->flag == UPPER && ttEntry->score >= beta)return ttEntry->score;
		}
	}
	else
		depth -= depth > 3;
	const int improving = ply > 1 && staticEval > stack[ply - 2].score;
	if (ttEntry->hash == hash && ttEntry->flag != staticEval > ttEntry->score)
		staticEval = ttEntry->score;
	const int inPv = beta - alpha > 1;
	if (ply && !inQuiescence && !inCheck && !inPv) {

		// Reverse futility pruning
		if (depth < 8) {
			if (staticEval - 71 * (depth - improving) >= beta)
				return staticEval;
			inQuiescence = staticEval + 238 * depth < alpha;
		}

		// Null move pruning
		if (depth > 2 && staticEval >= beta && staticEval >= stack[ply].score && doNull && pos->color[0] & ~pos->pieces[PAWN] & ~pos->pieces[KING]) {
			Position npos = *pos;
			FlipPosition(&npos);
			npos.ep = 0;
			if (-SearchAlpha(&npos, -beta, -alpha, depth - 4 - depth / 5 - min((staticEval - beta) / 196, 3), ply + 1, stack, 0) >= beta)
				return beta;
		}
	}
	int score;
	int legalMoves = 0;
	U8 tt_flag = LOWER;
	Move moves[256];
	const int num_moves = MoveGen(pos, moves, inQuiescence);
	S64 scoreList[256];
	historyHash[historyCount++] = hash;
	for (int j = 0; j < num_moves; ++j) {
		Move m = moves[j];
		const int ptDes = PieceTypeOnSquare(pos, m.to);
		if (Equal(m, tt_move))
			scoreList[j] = 1LL << 62;
		else if (ptDes != PT_NB)
			scoreList[j] = ((ptDes + 1) * (1LL << 54)) - PieceTypeOnSquare(pos, m.from);
		else if (Equal(m, stack[ply].killer1))
			scoreList[j] = 1LL << 50;
		else if (Equal(m, stack[ply].killer2))
			scoreList[j] = 1LL << 48;
		else
			scoreList[j] = CenterSq(m.to) - CenterSq(m.from);
	}
	for (int i = 0; i < num_moves; ++i) {
		int bstIdx = i;
		for (int j = i + 1; j < num_moves; ++j)
			if (scoreList[j] > scoreList[bstIdx])
				bstIdx = j;
		Move move = moves[bstIdx];
		scoreList[bstIdx] = scoreList[i];
		moves[bstIdx] = moves[i];

		// Material gain
		const int gain = mx_material[move.promo] + mx_material[PieceTypeOnSquare(pos, move.to)];

		// Delta pruning
		if (inQuiescence && !inCheck && staticEval + 50 + gain < alpha)
			break;

		// Forward futility pruning
		if (ply > 0 && depth < 8 && !inQuiescence && !inCheck && legalMoves && staticEval + 105 * depth + gain < alpha)
			break;

		Position npos = *pos;
		if (!MakeMove(&npos, &move))
			continue;
		if (!legalMoves || depth < 4)
			score = -SearchAlpha(&npos, -beta, -alpha, depth - 1, ply + 1, stack, 1);
		else {
			int r = !inPv;
			score = -SearchAlpha(&npos, -alpha - 1, -alpha, depth - 1 - r, ply + 1, stack, 1);
			if (r && score > alpha)
				score = -SearchAlpha(&npos, -alpha - 1, -alpha, depth - 1, ply + 1, stack, 1);
			if (score > alpha && score < beta)
				score = -SearchAlpha(&npos, -beta, -alpha, depth - 1, ply + 1, stack, 1);
		}
		if (info.stop)
			break;
		legalMoves++;
		if (alpha < score) {
			alpha = score;
			stack[ply].move = move;
			tt_flag = EXACT;
			if (!ply && info.post)
				PrintInfo(pos, depth, score);
			if (alpha >= beta) {
				tt_flag = UPPER;
				if (move.promo == PT_NB && PieceTypeOnSquare(pos, move.to) == PT_NB && !Equal(move, stack[ply].killer1)) {
					stack[ply].killer2 = stack[ply].killer1;
					stack[ply].killer1 = move;
				}
				break;
			}
		}
	}
	historyCount--;
	if (info.stop)
		return 0;
	if (!legalMoves && !inQuiescence)
		return inCheck ? ply - MATE : 0;
	ttEntry->hash = hash;
	ttEntry->move = stack[ply].move;
	ttEntry->depth = max(0, depth);
	ttEntry->score = alpha;
	ttEntry->flag = tt_flag;
	return alpha;
}

static void SearchIteratively(Position* pos) {
	memset(tt, 0, sizeof(tt));
	int sc = 0, prev_sc = 0;
	for (int depth = 1; depth <= info.depthLimit; ++depth) {
		if (depth < 5)
			sc = SearchAlpha(pos, -INF, INF, depth, 0, stack, 1);
		else {
			int delta = 15 + prev_sc * prev_sc / 16384;
			int alpha = max(prev_sc - delta, -INF);
			int beta = min(prev_sc + delta, INF);
			do {
				sc = SearchAlpha(pos, alpha, beta, depth, 0, stack, 1);
				if (sc <= alpha) {
					beta = (alpha + beta) / 2;
					alpha = max(alpha - delta, -INF);
				}
				else if (sc >= beta)
					beta = min(beta + delta, INF);
				else
					break;
				delta += delta / 2;
			} while (!info.stop);
		}
		prev_sc = sc;
		if (info.stop)
			break;
		if (info.timeLimit && GetTimeMs() - info.timeStart > info.timeLimit / 2)
			break;
	}
	char* uci = MoveToUci(stack[0].move, pos->flipped);
	if (info.post)
		printf("bestmove %s\n", uci);
	fflush(stdout);
}

//performance test
static inline void UciPerformance(Position* pos) {
	ResetInfo();
	PrintPerformanceHeader();
	info.depthLimit = 0;
	U64 elapsed = 0;
	while (elapsed < 3000) {
		PerftDriver(pos, info.depthLimit++);
		elapsed = GetTimeMs() - info.timeStart;
		printf(" %2d. %8llu %12llu\n", info.depthLimit, elapsed, info.nodes);
	}
	PrintSummary(elapsed, info.nodes);
}

//start benchmark
static void UciBench(Position* pos) {
	ResetInfo();
	PrintPerformanceHeader();
	info.depthLimit = 0;
	info.post = FALSE;
	U64 elapsed = 0;
	while (elapsed < 3000) {
		++info.depthLimit;
		SearchIteratively(pos);
		elapsed = GetTimeMs() - info.timeStart;
		printf(" %2d. %8llu %12llu\n", info.depthLimit, elapsed, info.nodes);
	}
	PrintSummary(elapsed, info.nodes);
}

static void UciNewGame() {
	memset(tt, 0, sizeof(tt));
}

static void ParsePosition(Position* pos, char* ptr) {
	char token[80], fen[80];
	ptr = ParseToken(ptr, token);
	if (strcmp(token, "fen") == 0) {
		fen[0] = '\0';
		while (1) {
			ptr = ParseToken(ptr, token);
			if (*token == '\0' || strcmp(token, "moves") == 0)
				break;
			strcat(fen, token);
			strcat(fen, " ");
		}
		SetFen(pos, fen);
	}
	else {
		ptr = ParseToken(ptr, token);
		SetFen(pos, START_FEN);
	}
	historyCount = 0;
	if (strcmp(token, "moves") == 0)
		while (1) {
			ptr = ParseToken(ptr, token);
			if (*token == '\0')
				break;
			Move m = UciToMove(token, pos->flipped);
			if (PieceTypeOnSquare(pos, m.to) != PT_NB || PieceTypeOnSquare(pos, m.from) == PAWN)
				historyCount = 0;
			historyHash[historyCount++] = GetHash(pos);
			MakeMove(pos, &m);
		}
}

static void ParseGo(Position* pos, char* command) {
	ResetInfo();
	int wtime = 0;
	int btime = 0;
	int winc = 0;
	int binc = 0;
	int movestogo = 32;
	char* argument = NULL;
	if (argument = strstr(command, "wtime"))
		wtime = max(1, atoi(argument + 6));
	if (argument = strstr(command, "btime"))
		btime = max(1, atoi(argument + 6));
	if (argument = strstr(command, "binc"))
		binc = atoi(argument + 5);
	if (argument = strstr(command, "winc"))
		winc = atoi(argument + 5);
	if ((argument = strstr(command, "movestogo")))
		movestogo = atoi(argument + 10);
	if ((argument = strstr(command, "movetime")))
		info.timeLimit = atoi(argument + 9);
	if ((argument = strstr(command, "depth")))
		info.depthLimit = atoi(argument + 6);
	if (argument = strstr(command, "nodes"))
		info.nodesLimit = atoi(argument + 5);
	int time = pos->flipped ? btime : wtime;
	int inc = pos->flipped ? binc : winc;
	if (time)
		info.timeLimit = max(1, min(time / movestogo + inc, time / 2));
	SearchIteratively(pos);
}

static void UciCommand(Position* pos, char* line) {
	if (strncmp(line, "ucinewgame", 10) == 0)
		UciNewGame();
	else if (!strncmp(line, "uci", 3)) {
		printf("id name %s\nuciok\n", NAME);
		fflush(stdout);
	}
	if (!strncmp(line, "isready", 7)) {
		printf("readyok\n");
		fflush(stdout);
	}
	else if (!strncmp(line, "go", 2))ParseGo(pos, line + 2);
	else if (!strncmp(line, "position", 8))ParsePosition(pos, line + 8);
	else if (!strncmp(line, "perft", 5))UciPerformance(pos);
	else if (!strncmp(line, "bench", 5))UciBench(pos);
	else if (!strncmp(line, "print", 5))PrintBoard(pos);
	else if (!strncmp(line, "stop", 4))info.stop = TRUE;
	else if (!strncmp(line, "quit", 4))exit(0);
}

static void UciLoop(Position* pos) {
	//UciCommand(pos, "position fen 8/8/8/8/2R5/4r1k1/8/7K b - - 99 96");
	//UciCommand(pos, "go depth 1");
	char line[4000];
	while (fgets(line, sizeof(line), stdin))
		UciCommand(pos, line);
}

int main(const int argc, const char** argv) {
	Position pos;
	Init();
	printf("%s %s\n", NAME, VERSION);
	SetFen(&pos, START_FEN);
	UciLoop(&pos);
}
