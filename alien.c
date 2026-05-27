
#include <math.h>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#define HASH_MAX 1024
#define HASH_DEF 64
#define HASH_MIN 1
#define NAME "Alien"
#define VERSION "2026-05-25"
#define FALSE 0
#define TRUE 1
#define U64 unsigned __int64
#define BRD_SQ_NUM 120
#define MAXGAMEMOVES 2048
#define MAXPOSITIONMOVES 256
#define MAX_PLY 64
#define START_FEN  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define INFINITE 32000
#define ISMATE (INFINITE - MAX_PLY)
#define FROMSQ(m) ((m) & 0x7F)
#define TOSQ(m) (((m)>>7) & 0x7F)
#define CAPTURED(m) (((m)>>14) & 0xF)
#define PROMOTED(m) (((m)>>20) & 0xF)
#define MFLAGEP 0x40000
#define MFLAGPS 0x80000
#define MFLAGCA 0x1000000
#define MFLAGCAP 0x7C000
#define MFLAGPROM 0xF00000
#define NOMOVE 0
#define FR2SQ(f,r) ( (21 + (f) ) + ( (r) * 10 ) )
#define SQ64(sq120) (Sq120ToSq64[(sq120)])
#define SQ120(sq64) (Sq64ToSq120[(sq64)])
#define POP(b) PopBit(b)
#define CNT(b) CountBits(b)
#define CLRBIT(bb,sq) ((bb) &= ClearMask[(sq)])
#define SETBIT(bb,sq) ((bb) |= SetMask[(sq)])
#define IsBQ(p) (PieceBishopQueen[(p)])
#define IsRQ(p) (PieceRookQueen[(p)])
#define IsKn(p) (PieceKnight[(p)])
#define IsKi(p) (PieceKing[(p)])
#define MIRROR64(sq) (Mirror64[(sq)])
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)
#define FLIP(sq) ((sq)^56)
#define MOVE(f,t,ca,pro,fl) ( (f) | ((t) << 7) | ( (ca) << 14 ) | ( (pro) << 20 ) | (fl))
#define SQOFFBOARD(sq) (FilesBrd[(sq)]==OFFBOARD)

enum { EMPTY, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK };
enum { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE };
enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE };
enum { WHITE, BLACK, BOTH };
enum {
	A1 = 21, B1, C1, D1, E1, F1, G1, H1,
	A2 = 31, B2, C2, D2, E2, F2, G2, H2,
	A3 = 41, B3, C3, D3, E3, F3, G3, H3,
	A4 = 51, B4, C4, D4, E4, F4, G4, H4,
	A5 = 61, B5, C5, D5, E5, F5, G5, H5,
	A6 = 71, B6, C6, D6, E6, F6, G6, H6,
	A7 = 81, B7, C7, D7, E7, F7, G7, H7,
	A8 = 91, B8, C8, D8, E8, F8, G8, H8, NO_SQ, OFFBOARD
};
enum { WKCA = 1, WQCA = 2, BKCA = 4, BQCA = 8 };
enum { HFNONE, HFALPHA, HFBETA, HFEXACT };

typedef struct {
	int move;
	int score;
} S_MOVE;

typedef struct {
	S_MOVE moves[MAXPOSITIONMOVES];
	int count;
} S_MOVELIST;

typedef struct {
	U64 posKey;
	int move;
	int score;
	int depth;
	int flags;
} S_HASHENTRY;

typedef struct {
	S_HASHENTRY* pTable;
	int numEntries;
	int newWrite;
	int overWrite;
	int hit;
	int cut;
} S_HASHTABLE;

typedef struct {

	int move;
	int castlePerm;
	int enPas;
	int fiftyMove;
	U64 posKey;

} S_UNDO;

typedef struct {
	int pieces[BRD_SQ_NUM];
	U64 pawns[3];
	int KingSq[2];
	int side;
	int enPas;
	int fiftyMove;
	int ply;
	int hisPly;
	int castlePerm;
	U64 posKey;
	int pceNum[13];
	int bigPce[2];
	int majPce[2];
	int minPce[2];

	S_UNDO history[MAXGAMEMOVES];

	// piece list
	int pList[13][10];

	S_HASHTABLE hashTable;
	int PvArray[MAX_PLY];

	int searchHistory[13][BRD_SQ_NUM];
	int searchKillers[2][MAX_PLY];

} Position;

typedef struct {
	int stop;
	int post;
	int depthLimit;
	U64 nodes;
	U64 timeStart;
	U64 nodesLimit;
	U64 timeLimit;
} SearchInfo;

/* GAME MOVE */

/*
0000 0000 0000 0000 0000 0111 1111 -> From 0x7F
0000 0000 0000 0011 1111 1000 0000 -> To >> 7, 0x7F
0000 0000 0011 1100 0000 0000 0000 -> Captured >> 14, 0xF
0000 0000 0100 0000 0000 0000 0000 -> EP 0x40000
0000 0000 1000 0000 0000 0000 0000 -> Pawn Start 0x80000
0000 1111 0000 0000 0000 0000 0000 -> Promoted Piece >> 20, 0xF
0001 0000 0000 0000 0000 0000 0000 -> Castle 0x1000000
*/

/* GLOBALS */

extern int Sq120ToSq64[BRD_SQ_NUM];
extern int Sq64ToSq120[64];
extern U64 SetMask[64];
extern U64 ClearMask[64];
extern U64 PieceKeys[13][120];
extern U64 SideKey;
extern U64 CastleKeys[16];
extern char PceChar[];
extern char SideChar[];
extern char RankChar[];
extern char FileChar[];

extern int PieceBig[13];
extern int PieceMaj[13];
extern int PieceMin[13];
extern int pieceVal[13];
extern int PieceCol[13];
extern int PiecePawn[13];

extern int FilesBrd[BRD_SQ_NUM];
extern int RanksBrd[BRD_SQ_NUM];

extern int PieceKnight[13];
extern int PieceKing[13];
extern int PieceRookQueen[13];
extern int PieceBishopQueen[13];
extern int PieceSlides[13];

extern int Mirror64[64];

extern U64 FileBBMask[8];
extern U64 RankBBMask[8];

extern U64 BlackPassedMask[64];
extern U64 WhitePassedMask[64];
extern U64 IsolatedMask[64];

extern void AllInit();
extern void PrintBitBoard(U64 bb);
extern int PopBit(U64* bb);
extern int CountBits(U64 b);
extern U64 GeneratePosKey(const Position* pos);
void InitSq120To64();
void InitBitMasks();
void InitHashKeys();
void InitFilesRanksBrd();
void InitEvalMasks();
extern void ResetBoard(Position* pos);
extern int SetFen(char* fen, Position* pos);
extern void PrintBoard(const Position* pos);
extern void UpdateListsMaterial(Position* pos);
extern int SqAttacked(const int sq, const int side, const Position* pos);
extern char* MoveToUci(const int move);
extern char* PrSq(const int sq);
extern void PrintMoveList(const S_MOVELIST* list);
extern int ParseMove(char* ptrChar, Position* pos);
extern void GenerateAllMoves(const Position* pos, S_MOVELIST* list);
extern void GenerateAllCaps(const Position* pos, S_MOVELIST* list);
extern int MoveExists(Position* pos, const int move);
extern void InitMvvLva();
extern int MakeMove(Position* pos, int move);
extern void TakeMove(Position* pos);
extern void MakeNullMove(Position* pos);
extern void TakeNullMove(Position* pos);
extern void SearchIteratively(Position* pos, SearchInfo* info);
extern void InitSearch();
extern U64 GetTimeMs();
extern void ReadInput(Position* pos, SearchInfo* info);
extern void InitHashTable(S_HASHTABLE* table, const int MB);
extern void StoreHashEntry(Position* pos, const int move, int score, const int flags, const int depth);
extern int ProbeHashEntry(Position* pos, int* move, int* score, int alpha, int beta, int depth);
extern int ProbePvMove(const Position* pos);
extern int GetPvLine(const int depth, Position* pos);
extern void UciCommand(Position* pos, SearchInfo* info, char* line);
extern void UciLoop(Position* pos, SearchInfo* info);

#define RAND_64 ((U64)rand() | (U64)rand() << 15 | (U64)rand() << 30 | (U64)rand() << 45 |((U64)rand() & 0xf) << 60 )


Position pos;
SearchInfo info;

char PceChar[] = " ANBRQKanbrqk";
char SideChar[] = "wb-";
char RankChar[] = "12345678";
char FileChar[] = "abcdefgh";

int PieceBig[13] = { FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE };
int PieceMaj[13] = { FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE };
int PieceMin[13] = { FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE };
int insufficientMaterial[13] = { 0,3,1,2,3,3,0,3,1,2,3,3,0 };
int materialMg[13] = { 0, 82, 337, 365, 477, 1025, 0, 94, 281, 297, 512, 936, 0 };
int materialEg[13] = { 0, 94, 281, 297, 512, 936, 0, 82, 337, 365, 477, 1025, 0 };
int PieceCol[13] = { BOTH, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,BLACK, BLACK, BLACK, BLACK, BLACK, BLACK };
int PiecePawn[13] = { FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE };
int PieceKnight[13] = { FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE };
int PieceKing[13] = { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE };
int PieceRookQueen[13] = { FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE };
int PieceBishopQueen[13] = { FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE };
int PieceSlides[13] = { FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE };

int Mirror64[64] = {
56	,	57	,	58	,	59	,	60	,	61	,	62	,	63	,
48	,	49	,	50	,	51	,	52	,	53	,	54	,	55	,
40	,	41	,	42	,	43	,	44	,	45	,	46	,	47	,
32	,	33	,	34	,	35	,	36	,	37	,	38	,	39	,
24	,	25	,	26	,	27	,	28	,	29	,	30	,	31	,
16	,	17	,	18	,	19	,	20	,	21	,	22	,	23	,
8	,	9	,	10	,	11	,	12	,	13	,	14	,	15	,
0	,	1	,	2	,	3	,	4	,	5	,	6	,	7
};

const int BitTable[64] = {
  63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
  51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
  26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
  58, 20, 37, 17, 36, 8
};

int Sq120ToSq64[BRD_SQ_NUM];
int Sq64ToSq120[64];

U64 SetMask[64];
U64 ClearMask[64];

U64 PieceKeys[13][120];
U64 SideKey;
U64 CastleKeys[16];

int FilesBrd[BRD_SQ_NUM];
int RanksBrd[BRD_SQ_NUM];

U64 FileBBMask[8];
U64 RankBBMask[8];

U64 BlackPassedMask[64];
U64 WhitePassedMask[64];
U64 IsolatedMask[64];

const int KnDir[8] = { -8, -19,	-21, -12, 8, 19, 21, 12 };
const int RkDir[4] = { -1, -10,	1, 10 };
const int BiDir[4] = { -9, -11, 11, 9 };
const int KiDir[8] = { -1, -10,	1, 10, -9, -11, 11, 9 };

#define HASH_PCE(pce,sq) (pos->posKey ^= (PieceKeys[(pce)][(sq)]))
#define HASH_CA (pos->posKey ^= (CastleKeys[(pos->castlePerm)]))
#define HASH_SIDE (pos->posKey ^= (SideKey))
#define HASH_EP (pos->posKey ^= (PieceKeys[EMPTY][(pos->enPas)]))

const int CastlePerm[120] = {
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 13, 15, 15, 15, 12, 15, 15, 14, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15,  7, 15, 15, 15,  3, 15, 15, 11, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15
};

// Null Move Pruning Values
static const int R = 2;
static const int minDepth = 3;

// Razoring Values
static const int RazorDepth = 2;
static const int RazorMargin[3] = { 0, 200, 400 };

// Futility Values
static const int FutilityDepth = 6;
static const int FutilityMargin[7] = { 0, 200, 325, 450, 575, 700, 825 };

// Reverse Futility Values
static const int RevFutilityDepth = 5;
static const int RevFutilityMargin[6] = { 0, 200, 400, 600, 800, 1000 };

// LMR Values
static const int LateMoveDepth = 3;
static const int FullSearchMoves = 2;
int LMRTable[64][64];

// Game phase constants
const int minorPhase = 1;
const int rookPhase = 2;
const int queenPhase = 4;
const int totalPhase = 24;

// Midgame constants
const int BishopPairMG = 20;
const int PawnPassedMG[8] = { 0, 5, 10, 20, 35, 50, 75, 150 };

// Endgame constants
const int BishopPairEG = 35;
const int PawnPassedEG[8] = { 0, 5, 15, 30, 50, 75, 125, 250 };

// Full game constants
const int PawnIsolated = -10;
const int QueenOpenFile = 5;
const int QueenSemiOpenFile = 3;
const int RookOpenFile = 10;
const int RookSemiOpenFile = 5;

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

int mg_pst[16][64];
int eg_pst[16][64];

const int LoopSlidePce[8] = {
 wB, wR, wQ, 0, bB, bR, bQ, 0
};

const int LoopNonSlidePce[6] = {
 wN, wK, 0, bN, bK, 0
};

const int LoopSlideIndex[2] = { 0, 4 };
const int LoopNonSlideIndex[2] = { 0, 3 };

const int PceDir[13][8] = {
	{ 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0 },
	{ -8, -19,	-21, -12, 8, 19, 21, 12 },
	{ -9, -11, 11, 9, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 },
	{ 0, 0, 0, 0, 0, 0, 0 },
	{ -8, -19,	-21, -12, 8, 19, 21, 12 },
	{ -9, -11, 11, 9, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 }
};

const int NumDir[13] = {
 0, 0, 8, 4, 4, 8, 8, 0, 8, 4, 4, 8, 8
};

const int VictimScore[13] = { 0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6 };
static int MvvLvaScores[13][13];
int mg_material[6] = { 82, 337, 365, 477, 1025,  0 };
int eg_material[6] = { 94, 281, 297, 512,  936,  0 };

static void InitPst() {
	for (int pt = PAWN; pt <= KING; pt++) {
		for (int sq = 0; sq < 64; sq++) {
			int mg = mg_material[pt] + mg_table[pt][sq];
			int eg = eg_material[pt] + eg_table[pt][sq];
			mg_pst[wP+pt][FLIP(sq)] = mg;
			mg_pst[bP+pt][(sq)] = -mg;
			eg_pst[wP+pt][FLIP(sq)] = eg;
			eg_pst[bP+pt][(sq)] = -eg;
		}
	}
}

void InitMvvLva() {
	for (int Attacker = wP; Attacker <= bK; ++Attacker)
		for (int Victim = wP; Victim <= bK; ++Victim)
			MvvLvaScores[Victim][Attacker] = VictimScore[Victim] * 10 - (VictimScore[Attacker]);
}

int MoveExists(Position* pos, const int move) {

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int MoveNum = 0;
	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		if (!MakeMove(pos, list->moves[MoveNum].move)) {
			continue;
		}
		TakeMove(pos);
		if (list->moves[MoveNum].move == move) {
			return TRUE;
		}
	}
	return FALSE;
}

static void AddQuietMove(const Position* pos, int move, S_MOVELIST* list) {


	list->moves[list->count].move = move;

	if (pos->searchKillers[0][pos->ply] == move) {
		list->moves[list->count].score = 900000;
	}
	else if (pos->searchKillers[1][pos->ply] == move) {
		list->moves[list->count].score = 800000;
	}
	else {
		list->moves[list->count].score = pos->searchHistory[pos->pieces[FROMSQ(move)]][TOSQ(move)];
	}
	list->count++;
}

static void AddCaptureMove(const Position* pos, int move, S_MOVELIST* list) {

	list->moves[list->count].move = move;
	list->moves[list->count].score = MvvLvaScores[CAPTURED(move)][pos->pieces[FROMSQ(move)]] + 1000000;
	list->count++;
}

static void AddEnPassantMove(const Position* pos, int move, S_MOVELIST* list) {


	list->moves[list->count].move = move;
	list->moves[list->count].score = 105 + 1000000;
	list->count++;
}

static void AddWhitePawnCapMove(const Position* pos, const int from, const int to, const int cap, S_MOVELIST* list) {


	if (RanksBrd[from] == RANK_7) {
		AddCaptureMove(pos, MOVE(from, to, cap, wQ, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wR, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wB, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wN, 0), list);
	}
	else {
		AddCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
	}
}

static void AddWhitePawnMove(const Position* pos, const int from, const int to, S_MOVELIST* list) {


	if (RanksBrd[from] == RANK_7) {
		AddQuietMove(pos, MOVE(from, to, EMPTY, wQ, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wR, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wB, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wN, 0), list);
	}
	else {
		AddQuietMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
	}
}

static void AddBlackPawnCapMove(const Position* pos, const int from, const int to, const int cap, S_MOVELIST* list) {


	if (RanksBrd[from] == RANK_2) {
		AddCaptureMove(pos, MOVE(from, to, cap, bQ, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bR, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bB, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bN, 0), list);
	}
	else {
		AddCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
	}
}

static void AddBlackPawnMove(const Position* pos, const int from, const int to, S_MOVELIST* list) {

	if (RanksBrd[from] == RANK_2) {
		AddQuietMove(pos, MOVE(from, to, EMPTY, bQ, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bR, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bB, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bN, 0), list);
	}
	else {
		AddQuietMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
	}
}

void GenerateAllMoves(const Position* pos, S_MOVELIST* list) {


	list->count = 0;

	int pce = EMPTY;
	int side = pos->side;
	int sq = 0; int t_sq = 0;
	int pceNum = 0;
	int dir = 0;
	int index = 0;
	int pceIndex = 0;

	if (side == WHITE) {

		for (pceNum = 0; pceNum < pos->pceNum[wP]; ++pceNum) {
			sq = pos->pList[wP][pceNum];

			if (pos->pieces[sq + 10] == EMPTY) {
				AddWhitePawnMove(pos, sq, sq + 10, list);
				if (RanksBrd[sq] == RANK_2 && pos->pieces[sq + 20] == EMPTY) {
					AddQuietMove(pos, MOVE(sq, (sq + 20), EMPTY, EMPTY, MFLAGPS), list);
				}
			}

			if (!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq + 9, pos->pieces[sq + 9], list);
			}
			if (!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq + 11, pos->pieces[sq + 11], list);
			}

			if (pos->enPas != NO_SQ) {
				if (sq + 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq + 9, EMPTY, EMPTY, MFLAGEP), list);
				}
				if (sq + 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq + 11, EMPTY, EMPTY, MFLAGEP), list);
				}
			}
		}

		if (pos->castlePerm & WKCA) {
			if (pos->pieces[F1] == EMPTY && pos->pieces[G1] == EMPTY) {
				if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(F1, BLACK, pos)) {
					AddQuietMove(pos, MOVE(E1, G1, EMPTY, EMPTY, MFLAGCA), list);
				}
			}
		}

		if (pos->castlePerm & WQCA) {
			if (pos->pieces[D1] == EMPTY && pos->pieces[C1] == EMPTY && pos->pieces[B1] == EMPTY) {
				if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(D1, BLACK, pos)) {
					AddQuietMove(pos, MOVE(E1, C1, EMPTY, EMPTY, MFLAGCA), list);
				}
			}
		}

	}
	else {

		for (pceNum = 0; pceNum < pos->pceNum[bP]; ++pceNum) {
			sq = pos->pList[bP][pceNum];

			if (pos->pieces[sq - 10] == EMPTY) {
				AddBlackPawnMove(pos, sq, sq - 10, list);
				if (RanksBrd[sq] == RANK_7 && pos->pieces[sq - 20] == EMPTY) {
					AddQuietMove(pos, MOVE(sq, (sq - 20), EMPTY, EMPTY, MFLAGPS), list);
				}
			}

			if (!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq - 9, pos->pieces[sq - 9], list);
			}

			if (!SQOFFBOARD(sq - 11) && PieceCol[pos->pieces[sq - 11]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq - 11, pos->pieces[sq - 11], list);
			}
			if (pos->enPas != NO_SQ) {
				if (sq - 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq - 9, EMPTY, EMPTY, MFLAGEP), list);
				}
				if (sq - 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq - 11, EMPTY, EMPTY, MFLAGEP), list);
				}
			}
		}

		// castling
		if (pos->castlePerm & BKCA) {
			if (pos->pieces[F8] == EMPTY && pos->pieces[G8] == EMPTY) {
				if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(F8, WHITE, pos)) {
					AddQuietMove(pos, MOVE(E8, G8, EMPTY, EMPTY, MFLAGCA), list);
				}
			}
		}

		if (pos->castlePerm & BQCA) {
			if (pos->pieces[D8] == EMPTY && pos->pieces[C8] == EMPTY && pos->pieces[B8] == EMPTY) {
				if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(D8, WHITE, pos)) {
					AddQuietMove(pos, MOVE(E8, C8, EMPTY, EMPTY, MFLAGCA), list);
				}
			}
		}
	}

	/* Loop for slide pieces */
	pceIndex = LoopSlideIndex[side];
	pce = LoopSlidePce[pceIndex++];
	while (pce != 0) {

		for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];

			for (index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				while (!SQOFFBOARD(t_sq)) {
					// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
					if (pos->pieces[t_sq] != EMPTY) {
						if (PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {
							AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
						}
						break;
					}
					AddQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
					t_sq += dir;
				}
			}
		}

		pce = LoopSlidePce[pceIndex++];
	}

	/* Loop for non slide */
	pceIndex = LoopNonSlideIndex[side];
	pce = LoopNonSlidePce[pceIndex++];

	while (pce != 0) {

		for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];

			for (index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				if (SQOFFBOARD(t_sq)) {
					continue;
				}

				// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
				if (pos->pieces[t_sq] != EMPTY) {
					if (PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {
						AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
					}
					continue;
				}
				AddQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
			}
		}

		pce = LoopNonSlidePce[pceIndex++];
	}

}


void GenerateAllCaps(const Position* pos, S_MOVELIST* list) {

	list->count = 0;

	int pce = EMPTY;
	int side = pos->side;
	int sq = 0; int t_sq = 0;
	int pceNum = 0;
	int dir = 0;
	int index = 0;
	int pceIndex = 0;

	if (side == WHITE) {

		for (pceNum = 0; pceNum < pos->pceNum[wP]; ++pceNum) {
			sq = pos->pList[wP][pceNum];

			if (!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq + 9, pos->pieces[sq + 9], list);
			}
			if (!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq + 11, pos->pieces[sq + 11], list);
			}

			if (pos->enPas != NO_SQ) {
				if (sq + 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq + 9, EMPTY, EMPTY, MFLAGEP), list);
				}
				if (sq + 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq + 11, EMPTY, EMPTY, MFLAGEP), list);
				}
			}
		}

	}
	else {

		for (pceNum = 0; pceNum < pos->pceNum[bP]; ++pceNum) {
			sq = pos->pList[bP][pceNum];

			if (!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq - 9, pos->pieces[sq - 9], list);
			}

			if (!SQOFFBOARD(sq - 11) && PieceCol[pos->pieces[sq - 11]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq - 11, pos->pieces[sq - 11], list);
			}
			if (pos->enPas != NO_SQ) {
				if (sq - 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq - 9, EMPTY, EMPTY, MFLAGEP), list);
				}
				if (sq - 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq - 11, EMPTY, EMPTY, MFLAGEP), list);
				}
			}
		}
	}

	/* Loop for slide pieces */
	pceIndex = LoopSlideIndex[side];
	pce = LoopSlidePce[pceIndex++];
	while (pce != 0) {

		for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];

			for (index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				while (!SQOFFBOARD(t_sq)) {
					// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
					if (pos->pieces[t_sq] != EMPTY) {
						if (PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {
							AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
						}
						break;
					}
					t_sq += dir;
				}
			}
		}

		pce = LoopSlidePce[pceIndex++];
	}

	/* Loop for non slide */
	pceIndex = LoopNonSlideIndex[side];
	pce = LoopNonSlidePce[pceIndex++];

	while (pce != 0) {

		for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];

			for (index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				if (SQOFFBOARD(t_sq)) {
					continue;
				}

				// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
				if (pos->pieces[t_sq] != EMPTY) {
					if (PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {
						AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
					}
					continue;
				}
			}
		}

		pce = LoopNonSlidePce[pceIndex++];
	}
}

static inline int InsufficientMaterial(const Position* pos, int piece) {
	return pos->pceNum[piece] * insufficientMaterial[piece];
};

static inline int MaterialMg(const Position* pos, int piece) {
	return pos->pceNum[piece] * materialMg[piece];
};

static inline int MaterialEg(const Position* pos, int piece) {
	return pos->pceNum[piece] * materialEg[piece];
};

static int MaterialDraw(const Position* pos) {
	int insufficientW = InsufficientMaterial(pos, wP) + InsufficientMaterial(pos, wN) + InsufficientMaterial(pos, wB) + InsufficientMaterial(pos, wR) + InsufficientMaterial(pos, wQ);
	int insufficientB = InsufficientMaterial(pos, bP) + InsufficientMaterial(pos, bN) + InsufficientMaterial(pos, bB) + InsufficientMaterial(pos, bR) + InsufficientMaterial(pos, bQ);
	return max(insufficientW, insufficientB) < 3;
}

static int ScoreMg(const Position* pos) {
	return MaterialMg(pos, wP) + MaterialMg(pos, wN) + MaterialMg(pos, wB) + MaterialMg(pos, wR) + MaterialMg(pos, wQ)
		- MaterialMg(pos, bP) - MaterialMg(pos, bN) - MaterialMg(pos, bB) - MaterialMg(pos, bR) - MaterialMg(pos, bQ);
}

static int ScoreEg(const Position* pos) {
	return MaterialEg(pos, wP) + MaterialEg(pos, wN) + MaterialEg(pos, wB) + MaterialEg(pos, wR) + MaterialEg(pos, wQ)
		- MaterialEg(pos, bP) - MaterialEg(pos, bN) - MaterialEg(pos, bB) - MaterialEg(pos, bR) - MaterialEg(pos, bQ);
}

int EvalPosition(Position* pos) {
	if (MaterialDraw(pos))
		return 0;
	int pce;
	int pceNum;
	int sq;
	int phase = 0;
	int scoreMG = 0;
	int scoreEG = 0;
	int score;

	pce = wP;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		scoreMG += mg_pst[wP][SQ64(sq)];
		scoreEG += eg_pst[wP][SQ64(sq)];

		if ((IsolatedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			scoreMG += PawnIsolated;
			scoreEG += PawnIsolated;
		}

		if ((WhitePassedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			scoreMG += PawnPassedMG[RanksBrd[sq]];
			scoreEG += PawnPassedEG[RanksBrd[sq]];
		}

	}

	pce = bP;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];


		scoreMG += mg_pst[bP][SQ64(sq)];
		scoreEG += eg_pst[bP][SQ64(sq)];

		if ((IsolatedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			scoreMG -= PawnIsolated;
			scoreEG -= PawnIsolated;
		}

		if ((BlackPassedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			scoreMG -= PawnPassedMG[7 - RanksBrd[sq]];
			scoreEG -= PawnPassedEG[7 - RanksBrd[sq]];
		}
	}

	pce = wN;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];


		scoreMG += mg_pst[wN][SQ64(sq)];
		scoreEG += eg_pst[wN][SQ64(sq)];
		phase += minorPhase;
	}

	pce = bN;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];


		scoreMG += mg_pst[bN][SQ64(sq)];
		scoreEG += eg_pst[bN][SQ64(sq)];
		phase += minorPhase;
	}

	pce = wB;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];


		scoreMG += mg_pst[wB][SQ64(sq)];
		scoreEG += eg_pst[wB][SQ64(sq)];
		phase += minorPhase;
	}

	pce = bB;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];


		scoreMG += mg_pst[bB][SQ64(sq)];
		scoreEG += eg_pst[bB][SQ64(sq)];
		phase += minorPhase;
	}

	pce = wR;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];


		scoreMG += mg_pst[wR][SQ64(sq)];
		scoreEG += eg_pst[wR][SQ64(sq)];

		if (!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			scoreMG += RookOpenFile;
			scoreEG += RookOpenFile;
		}
		else if (!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			scoreMG += RookSemiOpenFile;
			scoreEG += RookSemiOpenFile;
		}
		phase += rookPhase;
	}

	pce = bR;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];


		scoreMG += mg_pst[bR][SQ64(sq)];
		scoreEG += eg_pst[bR][SQ64(sq)];

		if (!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			scoreMG -= RookOpenFile;
			scoreEG -= RookOpenFile;
		}
		else if (!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			scoreMG -= RookSemiOpenFile;
			scoreEG -= RookSemiOpenFile;
		}
		phase += rookPhase;
	}

	pce = wQ;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];


		scoreMG += mg_pst[wQ][SQ64(sq)];
		scoreEG += eg_pst[wQ][SQ64(sq)];

		if (!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			scoreMG += QueenOpenFile;
			scoreEG += QueenOpenFile;
		}
		else if (!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			scoreMG += QueenSemiOpenFile;
			scoreEG += QueenSemiOpenFile;
		}
		phase += queenPhase;
	}

	pce = bQ;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];


		scoreMG += mg_pst[bQ][SQ64(sq)];
		scoreEG += eg_pst[bQ][SQ64(sq)];

		if (!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			scoreMG -= QueenOpenFile;
			scoreEG -= QueenOpenFile;
		}
		else if (!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			scoreMG -= QueenSemiOpenFile;
			scoreEG -= QueenSemiOpenFile;
		}
		phase += queenPhase;
	}
	pce = wK;
	sq = pos->pList[pce][0];

	scoreMG += mg_pst[wK][SQ64(sq)];
	scoreEG += eg_pst[wK][SQ64(sq)];

	pce = bK;
	sq = pos->pList[pce][0];

	scoreMG += mg_pst[bK][SQ64(sq)];
	scoreEG += eg_pst[bK][SQ64(sq)];

	if (pos->pceNum[wB] >= 2) {
		scoreMG += BishopPairMG;
		scoreEG += BishopPairEG;
	}
	if (pos->pceNum[bB] >= 2) {
		scoreMG -= BishopPairMG;
		scoreEG -= BishopPairEG;
	}
	phase = min(24, totalPhase);
	score = ((scoreMG * phase) + (scoreEG * (24 - phase))) / 24;
	return pos->side == WHITE ? score : -score;
}


void InitSearch() {
	// creating the LMR table entries (idea from Ethereal)
	for (int moveDepth = 1; moveDepth < 64; moveDepth++)
		for (int played = 1; played < 64; played++)
			LMRTable[moveDepth][played] = 1 + (int)(log(moveDepth) * log(played) / 1.75);
}

static void CheckUp(Position* pos, SearchInfo* info) {
	if ((info->timeLimit && GetTimeMs() - info->timeStart > info->timeLimit) ||
		(info->nodesLimit && info->nodes > info->nodesLimit))
		info->stop = TRUE;
	ReadInput(pos, info);
}

static void PickNextMove(int moveNum, S_MOVELIST* list) {

	S_MOVE temp;
	int index = 0;
	int bestScore = 0;
	int bestNum = moveNum;

	for (index = moveNum; index < list->count; ++index) {
		if (list->moves[index].score > bestScore) {
			bestScore = list->moves[index].score;
			bestNum = index;
		}
	}


	temp = list->moves[moveNum];
	list->moves[moveNum] = list->moves[bestNum];
	list->moves[bestNum] = temp;
}

static int IsRepetition(const Position* pos) {

	int index = 0;

	for (index = pos->hisPly - pos->fiftyMove; index < pos->hisPly - 1; ++index) {
		if (pos->posKey == pos->history[index].posKey) {
			return TRUE;
		}
	}
	return FALSE;
}

static void ClearForSearch(Position* pos, SearchInfo* info) {

	int index = 0;
	int index2 = 0;

	for (index = 0; index < 13; ++index) {
		for (index2 = 0; index2 < BRD_SQ_NUM; ++index2) {
			pos->searchHistory[index][index2] = 0;
		}
	}

	for (index = 0; index < 2; ++index) {
		for (index2 = 0; index2 < MAX_PLY; ++index2) {
			pos->searchKillers[index][index2] = 0;
		}
	}

	pos->hashTable.overWrite = 0;
	pos->hashTable.hit = 0;
	pos->hashTable.cut = 0;
	pos->ply = 0;

	info->stop = 0;
	info->nodes = 0;
}

static void PrintInfo(Position* pos, SearchInfo* info, int bestScore, int depth) {
	if (abs(bestScore) > ISMATE) {
		bestScore = (bestScore > 0 ? INFINITE - bestScore + 1 : -INFINITE - bestScore) / 2;
		printf("info score mate %d depth %d nodes %llu time %llu ", bestScore, depth, info->nodes, GetTimeMs() - info->timeStart);
	}
	else
		printf("info score cp %d depth %d nodes %llu time %llu ", bestScore, depth, info->nodes, GetTimeMs() - info->timeStart);
	int pvMoves = GetPvLine(depth, pos);
	printf("hashfull %d pv", Permill(&pos->hashTable));
	for (int pvNum = 0; pvNum < pvMoves; ++pvNum)
		printf(" %s", MoveToUci(pos->PvArray[pvNum]));
	printf("\n");
}

static int SearchQuiescence(int alpha, int beta, Position* pos, SearchInfo* info) {

	if ((++info->nodes & 2047) == 0) {
		CheckUp(pos, info);
	}

	if (IsRepetition(pos) || pos->fiftyMove >= 100) {
		return 0;
	}

	if (pos->ply > MAX_PLY - 1) {
		return EvalPosition(pos);
	}

	// Mate Distance Pruning
	alpha = MAX(alpha, -INFINITE + pos->ply);
	beta = MIN(beta, INFINITE - pos->ply);
	if (alpha >= beta) {
		return alpha;
	}

	int Score = EvalPosition(pos);


	if (Score >= beta) {
		return beta;
	}

	if (Score > alpha) {
		alpha = Score;
	}

	S_MOVELIST list[1];
	GenerateAllCaps(pos, list);

	int MoveNum = 0;
	int Legal = 0;
	Score = -INFINITE;

	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		PickNextMove(MoveNum, list);

		if (!MakeMove(pos, list->moves[MoveNum].move)) {
			continue;
		}

		Legal++;
		Score = -SearchQuiescence(-beta, -alpha, pos, info);
		TakeMove(pos);

		if (info->stop == TRUE) {
			return 0;
		}

		if (Score > alpha) {
			if (Score >= beta)
				return beta;
			alpha = Score;
		}
	}

	return alpha;
}

static int SearchAlpha(int alpha, int beta, int depth, Position* pos, SearchInfo* info, int DoNull, int DoLMR) {

	int InCheck = SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos);

	// Check Extension (Extend all checks before dropping into Quiescence)
	if (InCheck) {
		depth++;
	}

	if (depth <= 0) {
		return SearchQuiescence(alpha, beta, pos, info);
		// return EvalPosition(pos);
	}

	if ((++info->nodes & 2047) == 0) {
		CheckUp(pos, info);
	}

	if ((IsRepetition(pos) || pos->fiftyMove >= 100) && pos->ply) {
		return 0;
	}

	if (pos->ply > MAX_PLY - 1) {
		return EvalPosition(pos);
	}

	// Mate Distance Pruning (finds mates more quickly)
	alpha = MAX(alpha, -INFINITE + pos->ply);
	beta = MIN(beta, INFINITE - pos->ply);
	if (alpha >= beta) {
		return alpha;
	}

	int Score = -INFINITE;
	int PvMove = NOMOVE;

	if (ProbeHashEntry(pos, &PvMove, &Score, alpha, beta, depth) == TRUE) {
		pos->hashTable.cut++;
		return Score;
	}

	int positionEval = EvalPosition(pos);

	// Razoring (prunes near alpha)
	if (depth <= RazorDepth && !PvMove && !InCheck && positionEval + RazorMargin[depth] <= alpha) {
		// drop into qSearch if move most likely won't beat alpha
		Score = SearchQuiescence(alpha - RazorMargin[depth], beta - RazorMargin[depth], pos, info);
		if (Score + RazorMargin[depth] <= alpha) {
			return Score;
		}
	}

	// Reverse Futility Pruning (prunes near beta)
	if (depth <= RevFutilityDepth && !PvMove && !InCheck && abs(beta) < ISMATE && positionEval - RevFutilityMargin[depth] >= beta) {
		return positionEval - RevFutilityMargin[depth];
	}

	// Null Move Pruning
	if (depth >= minDepth && DoNull && !InCheck && pos->ply && (pos->bigPce[pos->side] > 0) && positionEval >= beta) {
		MakeNullMove(pos);
		Score = -SearchAlpha(-beta, -beta + 1, depth - 1 - R, pos, info, FALSE, FALSE);
		TakeNullMove(pos);
		if (info->stop == TRUE) {
			return 0;
		}

		if (Score >= beta && abs(Score) < ISMATE)
			return beta;
	}

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int MoveNum = 0;
	int Legal = 0;
	int OldAlpha = alpha;
	int BestMove = NOMOVE;

	int BestScore = -INFINITE;

	Score = -INFINITE;

	if (PvMove != NOMOVE) {
		for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {
			if (list->moves[MoveNum].move == PvMove) {
				list->moves[MoveNum].score = 2000000;
				break;
			}
		}
	}

	int FoundPv = FALSE;

	// Futility Pruning flag (if node is futile (unlikely to raise alpha), this flag is set)
	int FutileNode = (depth <= FutilityDepth && positionEval + FutilityMargin[depth] <= alpha && abs(Score) < ISMATE) ? 1 : 0;

	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		PickNextMove(MoveNum, list);

		// Futility Pruning (if node is considered futile, and at least 1 legal move has been searched, don't search any more quiet moves in the position)
		if (Legal && FutileNode && !(list->moves[MoveNum].move & MFLAGCAP) && !(list->moves[MoveNum].move & MFLAGPROM) && !SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos)) {
			continue;
		}

		// if move is legal, play it
		if (!MakeMove(pos, list->moves[MoveNum].move)) {
			continue;
		}

		Legal++;

		// PVS (speeds up search with good move ordering)
		if (FoundPv == TRUE) {

			// Late Move Reductions at Root (reduces moves if past full move search limit (not reducing captures, checks, or promotions))
			if (depth >= LateMoveDepth && !(list->moves[MoveNum].move & MFLAGCAP) && !(list->moves[MoveNum].move & MFLAGPROM) && !SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos) && DoLMR && Legal > FullSearchMoves) {

				// get initial reduction depth
				int reduce = LMRTable[MIN(depth, 63)][MIN(Legal, 63)];

				// reduce less for killer moves
				if ((list->moves[MoveNum].score == 800000 || list->moves[MoveNum].score == 900000)) reduce--;

				// do not fall directly into quiescence search
				reduce = MIN(depth - 1, MAX(reduce, 1));

				// print reduction depth at move number
				// printf("reduction: %d depth: %d moveNum: %d\n", (reduce - 1), depth, Legal);

				// search with the reduced depth
				Score = -SearchAlpha(-alpha - 1, -alpha, depth - reduce, pos, info, TRUE, FALSE);

			}
			else {
				// If LMR conditions not met (not at root, or tactical move), do a null window search (because we are using PVS)
				Score = -SearchAlpha(-alpha - 1, -alpha, depth - 1, pos, info, TRUE, TRUE);

			}
			if (Score > alpha && Score < beta) {
				// If the LMR or the null window fails, do a full search
				Score = -SearchAlpha(-beta, -alpha, depth - 1, pos, info, TRUE, FALSE);

			}
		}
		else {
			// If no PV found, do a full search
			Score = -SearchAlpha(-beta, -alpha, depth - 1, pos, info, TRUE, FALSE);

		}

		TakeMove(pos);

		if (info->stop)
			return 0;
		if (Score > BestScore) {
			BestScore = Score;
			BestMove = list->moves[MoveNum].move;
			if (Score > alpha) {
				if (Score >= beta) {

					if (!(list->moves[MoveNum].move & MFLAGCAP)) {
						pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
						pos->searchKillers[0][pos->ply] = list->moves[MoveNum].move;
					}

					StoreHashEntry(pos, BestMove, beta, HFBETA, depth);

					return beta;
				}
				FoundPv = TRUE;
				alpha = Score;

				if (!(list->moves[MoveNum].move & MFLAGCAP)) {
					pos->searchHistory[pos->pieces[FROMSQ(BestMove)]][TOSQ(BestMove)] += depth;
				}
			}
		}
	}

	if (Legal == 0) {
		if (InCheck) {
			return -INFINITE + pos->ply;
		}
		else {
			return 0;
		}
	}


	if (alpha != OldAlpha) {
		StoreHashEntry(pos, BestMove, BestScore, HFEXACT, depth);
	}
	else {
		StoreHashEntry(pos, BestMove, alpha, HFALPHA, depth);
	}

	return alpha;
}

void SearchIteratively(Position* pos, SearchInfo* info) {
	ClearForSearch(pos, info);
	for (int depth = 1; depth <= info->depthLimit; ++depth) {
		int score = SearchAlpha(-INFINITE, INFINITE, depth, pos, info, TRUE, TRUE);
		if (info->stop)
			break;
		if (info->post)
			PrintInfo(pos, info, score, depth);
	}
	if (info->post)
		printf("bestmove %s\n", MoveToUci(pos->PvArray[0]));
}

static void ClearPiece(const int sq, Position* pos) {


	int pce = pos->pieces[sq];


	int col = PieceCol[pce];
	int index = 0;
	int t_pceNum = -1;


	HASH_PCE(pce, sq);

	pos->pieces[sq] = EMPTY;

	if (PieceBig[pce]) {
		pos->bigPce[col]--;
		if (PieceMaj[pce]) {
			pos->majPce[col]--;
		}
		else {
			pos->minPce[col]--;
		}
	}
	else {
		CLRBIT(pos->pawns[col], SQ64(sq));
		CLRBIT(pos->pawns[BOTH], SQ64(sq));
	}

	for (index = 0; index < pos->pceNum[pce]; ++index) {
		if (pos->pList[pce][index] == sq) {
			t_pceNum = index;
			break;
		}
	}


	pos->pceNum[pce]--;

	pos->pList[pce][t_pceNum] = pos->pList[pce][pos->pceNum[pce]];

}


static void AddPiece(const int sq, Position* pos, const int pce) {


	int col = PieceCol[pce];

	HASH_PCE(pce, sq);

	pos->pieces[sq] = pce;

	if (PieceBig[pce]) {
		pos->bigPce[col]++;
		if (PieceMaj[pce]) {
			pos->majPce[col]++;
		}
		else {
			pos->minPce[col]++;
		}
	}
	else {
		SETBIT(pos->pawns[col], SQ64(sq));
		SETBIT(pos->pawns[BOTH], SQ64(sq));
	}

	pos->pList[pce][pos->pceNum[pce]++] = sq;

}

static void MovePiece(const int from, const int to, Position* pos) {


	int index = 0;
	int pce = pos->pieces[from];
	int col = PieceCol[pce];

#ifdef DEBUG
	int t_PieceNum = FALSE;
#endif

	HASH_PCE(pce, from);
	pos->pieces[from] = EMPTY;

	HASH_PCE(pce, to);
	pos->pieces[to] = pce;

	if (!PieceBig[pce]) {
		CLRBIT(pos->pawns[col], SQ64(from));
		CLRBIT(pos->pawns[BOTH], SQ64(from));
		SETBIT(pos->pawns[col], SQ64(to));
		SETBIT(pos->pawns[BOTH], SQ64(to));
	}

	for (index = 0; index < pos->pceNum[pce]; ++index) {
		if (pos->pList[pce][index] == from) {
			pos->pList[pce][index] = to;
#ifdef DEBUG
			t_PieceNum = TRUE;
#endif
			break;
		}
	}
}

int MakeMove(Position* pos, int move) {

	int from = FROMSQ(move);
	int to = TOSQ(move);
	int side = pos->side;

	pos->history[pos->hisPly].posKey = pos->posKey;

	if (move & MFLAGEP) {
		if (side == WHITE) {
			ClearPiece(to - 10, pos);
		}
		else {
			ClearPiece(to + 10, pos);
		}
	}
	else if (move & MFLAGCA) {
		switch (to) {
		case C1:
			MovePiece(A1, D1, pos);
			break;
		case C8:
			MovePiece(A8, D8, pos);
			break;
		case G1:
			MovePiece(H1, F1, pos);
			break;
		case G8:
			MovePiece(H8, F8, pos);
			break;
		}
	}

	if (pos->enPas != NO_SQ) HASH_EP;
	HASH_CA;

	pos->history[pos->hisPly].move = move;
	pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;
	pos->history[pos->hisPly].enPas = pos->enPas;
	pos->history[pos->hisPly].castlePerm = pos->castlePerm;

	pos->castlePerm &= CastlePerm[from];
	pos->castlePerm &= CastlePerm[to];
	pos->enPas = NO_SQ;

	HASH_CA;

	int captured = CAPTURED(move);
	pos->fiftyMove++;

	if (captured != EMPTY) {
		ClearPiece(to, pos);
		pos->fiftyMove = 0;
	}

	pos->hisPly++;
	pos->ply++;


	if (PiecePawn[pos->pieces[from]]) {
		pos->fiftyMove = 0;
		if (move & MFLAGPS) {
			if (side == WHITE) {
				pos->enPas = from + 10;
			}
			else {
				pos->enPas = from - 10;
			}
			HASH_EP;
		}
	}

	MovePiece(from, to, pos);

	int prPce = PROMOTED(move);
	if (prPce != EMPTY) {
		ClearPiece(to, pos);
		AddPiece(to, pos, prPce);
	}

	if (PieceKing[pos->pieces[to]]) {
		pos->KingSq[pos->side] = to;
	}

	pos->side ^= 1;
	HASH_SIDE;



	if (SqAttacked(pos->KingSq[side], pos->side, pos)) {
		TakeMove(pos);
		return FALSE;
	}

	return TRUE;

}

void TakeMove(Position* pos) {


	pos->hisPly--;
	pos->ply--;


	int move = pos->history[pos->hisPly].move;
	int from = FROMSQ(move);
	int to = TOSQ(move);


	if (pos->enPas != NO_SQ) HASH_EP;
	HASH_CA;

	pos->castlePerm = pos->history[pos->hisPly].castlePerm;
	pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;
	pos->enPas = pos->history[pos->hisPly].enPas;

	if (pos->enPas != NO_SQ) HASH_EP;
	HASH_CA;

	pos->side ^= 1;
	HASH_SIDE;

	if (MFLAGEP & move) {
		if (pos->side == WHITE) {
			AddPiece(to - 10, pos, bP);
		}
		else {
			AddPiece(to + 10, pos, wP);
		}
	}
	else if (MFLAGCA & move) {
		switch (to) {
		case C1: MovePiece(D1, A1, pos); break;
		case C8: MovePiece(D8, A8, pos); break;
		case G1: MovePiece(F1, H1, pos); break;
		case G8: MovePiece(F8, H8, pos); break;
		}
	}

	MovePiece(to, from, pos);

	if (PieceKing[pos->pieces[from]]) {
		pos->KingSq[pos->side] = from;
	}

	int captured = CAPTURED(move);
	if (captured != EMPTY) {
		AddPiece(to, pos, captured);
	}

	if (PROMOTED(move) != EMPTY) {
		ClearPiece(from, pos);
		AddPiece(from, pos, (PieceCol[PROMOTED(move)] == WHITE ? wP : bP));
	}


}


void MakeNullMove(Position* pos) {


	pos->ply++;
	pos->history[pos->hisPly].posKey = pos->posKey;

	if (pos->enPas != NO_SQ) HASH_EP;

	pos->history[pos->hisPly].move = NOMOVE;
	pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;
	pos->history[pos->hisPly].enPas = pos->enPas;
	pos->history[pos->hisPly].castlePerm = pos->castlePerm;
	pos->enPas = NO_SQ;

	pos->side ^= 1;
	pos->hisPly++;
	HASH_SIDE;

	return;
} // MakeNullMove

void TakeNullMove(Position* pos) {

	pos->hisPly--;
	pos->ply--;

	if (pos->enPas != NO_SQ) HASH_EP;

	pos->castlePerm = pos->history[pos->hisPly].castlePerm;
	pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;
	pos->enPas = pos->history[pos->hisPly].enPas;

	if (pos->enPas != NO_SQ) HASH_EP;
	pos->side ^= 1;
	HASH_SIDE;

}

int SqAttacked(const int sq, const int side, const Position* pos) {

	int pce, index, t_sq, dir;

	// pawns
	if (side == WHITE) {
		if (pos->pieces[sq - 11] == wP || pos->pieces[sq - 9] == wP) {
			return TRUE;
		}
	}
	else {
		if (pos->pieces[sq + 11] == bP || pos->pieces[sq + 9] == bP) {
			return TRUE;
		}
	}

	// knights
	for (index = 0; index < 8; ++index) {
		pce = pos->pieces[sq + KnDir[index]];
		if (pce != OFFBOARD && IsKn(pce) && PieceCol[pce] == side) {
			return TRUE;
		}
	}

	// rooks, queens
	for (index = 0; index < 4; ++index) {
		dir = RkDir[index];
		t_sq = sq + dir;
		pce = pos->pieces[t_sq];
		while (pce != OFFBOARD) {
			if (pce != EMPTY) {
				if (IsRQ(pce) && PieceCol[pce] == side) {
					return TRUE;
				}
				break;
			}
			t_sq += dir;
			pce = pos->pieces[t_sq];
		}
	}

	// bishops, queens
	for (index = 0; index < 4; ++index) {
		dir = BiDir[index];
		t_sq = sq + dir;
		pce = pos->pieces[t_sq];
		while (pce != OFFBOARD) {
			if (pce != EMPTY) {
				if (IsBQ(pce) && PieceCol[pce] == side) {
					return TRUE;
				}
				break;
			}
			t_sq += dir;
			pce = pos->pieces[t_sq];
		}
	}

	// kings
	for (index = 0; index < 8; ++index) {
		pce = pos->pieces[sq + KiDir[index]];
		if (pce != OFFBOARD && IsKi(pce) && PieceCol[pce] == side) {
			return TRUE;
		}
	}

	return FALSE;

}

void InitEvalMasks() {
	int sq, tsq, r, f;

	for (sq = 0; sq < 8; ++sq) {
		FileBBMask[sq] = 0ULL;
		RankBBMask[sq] = 0ULL;
	}

	for (r = RANK_8; r >= RANK_1; r--) {
		for (f = FILE_A; f <= FILE_H; f++) {
			sq = r * 8 + f;
			FileBBMask[f] |= (1ULL << sq);
			RankBBMask[r] |= (1ULL << sq);
		}
	}

	for (sq = 0; sq < 64; ++sq) {
		IsolatedMask[sq] = 0ULL;
		WhitePassedMask[sq] = 0ULL;
		BlackPassedMask[sq] = 0ULL;
	}

	for (sq = 0; sq < 64; ++sq) {
		tsq = sq + 8;

		while (tsq < 64) {
			WhitePassedMask[sq] |= (1ULL << tsq);
			tsq += 8;
		}

		tsq = sq - 8;
		while (tsq >= 0) {
			BlackPassedMask[sq] |= (1ULL << tsq);
			tsq -= 8;
		}

		if (FilesBrd[SQ120(sq)] > FILE_A) {
			IsolatedMask[sq] |= FileBBMask[FilesBrd[SQ120(sq)] - 1];

			tsq = sq + 7;
			while (tsq < 64) {
				WhitePassedMask[sq] |= (1ULL << tsq);
				tsq += 8;
			}

			tsq = sq - 9;
			while (tsq >= 0) {
				BlackPassedMask[sq] |= (1ULL << tsq);
				tsq -= 8;
			}
		}

		if (FilesBrd[SQ120(sq)] < FILE_H) {
			IsolatedMask[sq] |= FileBBMask[FilesBrd[SQ120(sq)] + 1];

			tsq = sq + 9;
			while (tsq < 64) {
				WhitePassedMask[sq] |= (1ULL << tsq);
				tsq += 8;
			}

			tsq = sq - 7;
			while (tsq >= 0) {
				BlackPassedMask[sq] |= (1ULL << tsq);
				tsq -= 8;
			}
		}
	}
}

void InitFilesRanksBrd() {

	int index = 0;
	int file = FILE_A;
	int rank = RANK_1;
	int sq = A1;

	for (index = 0; index < BRD_SQ_NUM; ++index) {
		FilesBrd[index] = OFFBOARD;
		RanksBrd[index] = OFFBOARD;
	}

	for (rank = RANK_1; rank <= RANK_8; ++rank) {
		for (file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file, rank);
			FilesBrd[sq] = file;
			RanksBrd[sq] = rank;
		}
	}
}

void InitHashKeys() {

	int index = 0;
	int index2 = 0;
	for (index = 0; index < 13; ++index) {
		for (index2 = 0; index2 < 120; ++index2) {
			PieceKeys[index][index2] = RAND_64;
		}
	}
	SideKey = RAND_64;
	for (index = 0; index < 16; ++index) {
		CastleKeys[index] = RAND_64;
	}

}

void InitBitMasks() {
	for (int index = 0; index < 64; index++) {
		SetMask[index] = 1ULL << index;
		ClearMask[index] = ~SetMask[index];
	}
}

void InitSq120To64() {

	int index = 0;
	int file = FILE_A;
	int rank = RANK_1;
	int sq = A1;
	int sq64 = 0;
	for (index = 0; index < BRD_SQ_NUM; ++index)
		Sq120ToSq64[index] = 65;
	for (rank = RANK_1; rank <= RANK_8; ++rank) {
		for (file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file, rank);
			Sq64ToSq120[sq64] = sq;
			Sq120ToSq64[sq] = sq64;
			sq64++;
		}
	}
}

int PopBit(U64* bb) {
	U64 b = *bb ^ (*bb - 1);
	unsigned int fold = (unsigned)((b & 0xffffffff) ^ (b >> 32));
	*bb &= (*bb - 1);
	return BitTable[(fold * 0x783a9b23) >> 26];
}

int CountBits(U64 b) {
	int r;
	for (r = 0; b; r++, b &= b - 1);
	return r;
}

void PrintBitBoard(U64 bb) {
	int rank = 0;
	int file = 0;
	int sq = 0;
	int sq64 = 0;
	const char* s = "   +---+---+---+---+---+---+---+---+\n";
	const char* t = "     A   B   C   D   E   F   G   H\n";
	printf(t);
	for (rank = RANK_8; rank >= RANK_1; --rank) {
		printf(s);
		printf(" %d |", rank + 1);
		for (file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file, rank);	// 120 based		
			sq64 = SQ64(sq); // 64 based

			if ((1ull << sq64) & bb)
				printf(" X |");
			else
				printf("   |");

		}
		printf(" %d \n", rank + 1);
	}
	printf(s);
	printf(t);
}

void UpdateListsMaterial(Position* pos) {

	int piece, sq, index, colour;

	for (index = 0; index < BRD_SQ_NUM; ++index) {
		sq = index;
		piece = pos->pieces[index];
		if (piece != OFFBOARD && piece != EMPTY) {
			colour = PieceCol[piece];

			if (PieceBig[piece] == TRUE) pos->bigPce[colour]++;
			if (PieceMin[piece] == TRUE) pos->minPce[colour]++;
			if (PieceMaj[piece] == TRUE) pos->majPce[colour]++;


			pos->pList[piece][pos->pceNum[piece]] = sq;
			pos->pceNum[piece]++;


			if (piece == wK) pos->KingSq[WHITE] = sq;
			if (piece == bK) pos->KingSq[BLACK] = sq;

			if (piece == wP) {
				SETBIT(pos->pawns[WHITE], SQ64(sq));
				SETBIT(pos->pawns[BOTH], SQ64(sq));
			}
			else if (piece == bP) {
				SETBIT(pos->pawns[BLACK], SQ64(sq));
				SETBIT(pos->pawns[BOTH], SQ64(sq));
			}
		}
	}
}

int SetFen(char* fen, Position* pos) {


	int  rank = RANK_8;
	int  file = FILE_A;
	int  piece = 0;
	int  count = 0;
	int  i = 0;
	int  sq64 = 0;
	int  sq120 = 0;

	ResetBoard(pos);

	while ((rank >= RANK_1) && *fen) {
		count = 1;
		switch (*fen) {
		case 'p': piece = bP; break;
		case 'r': piece = bR; break;
		case 'n': piece = bN; break;
		case 'b': piece = bB; break;
		case 'k': piece = bK; break;
		case 'q': piece = bQ; break;
		case 'P': piece = wP; break;
		case 'R': piece = wR; break;
		case 'N': piece = wN; break;
		case 'B': piece = wB; break;
		case 'K': piece = wK; break;
		case 'Q': piece = wQ; break;

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
			piece = EMPTY;
			count = *fen - '0';
			break;

		case '/':
		case ' ':
			rank--;
			file = FILE_A;
			fen++;
			continue;

		default:
			printf("FEN error \n");
			return -1;
		}

		for (i = 0; i < count; i++) {
			sq64 = rank * 8 + file;
			sq120 = SQ120(sq64);
			if (piece != EMPTY) {
				pos->pieces[sq120] = piece;
			}
			file++;
		}
		fen++;
	}


	pos->side = (*fen == 'w') ? WHITE : BLACK;
	fen += 2;

	for (i = 0; i < 4; i++) {
		if (*fen == ' ') {
			break;
		}
		switch (*fen) {
		case 'K': pos->castlePerm |= WKCA; break;
		case 'Q': pos->castlePerm |= WQCA; break;
		case 'k': pos->castlePerm |= BKCA; break;
		case 'q': pos->castlePerm |= BQCA; break;
		default:	     break;
		}
		fen++;
	}
	fen++;


	if (*fen != '-') {
		file = fen[0] - 'a';
		rank = fen[1] - '1';


		pos->enPas = FR2SQ(file, rank);
	}

	pos->posKey = GeneratePosKey(pos);

	UpdateListsMaterial(pos);

	return 0;
}

void ResetBoard(Position* pos) {

	int index = 0;

	for (index = 0; index < BRD_SQ_NUM; ++index) {
		pos->pieces[index] = OFFBOARD;
	}

	for (index = 0; index < 64; ++index) {
		pos->pieces[SQ120(index)] = EMPTY;
	}

	for (index = 0; index < 2; ++index) {
		pos->bigPce[index] = 0;
		pos->majPce[index] = 0;
		pos->minPce[index] = 0;
	}

	for (index = 0; index < 3; ++index) {
		pos->pawns[index] = 0ULL;
	}

	for (index = 0; index < 13; ++index) {
		pos->pceNum[index] = 0;
	}

	pos->KingSq[WHITE] = pos->KingSq[BLACK] = NO_SQ;

	pos->side = BOTH;
	pos->enPas = NO_SQ;
	pos->fiftyMove = 0;

	pos->ply = 0;
	pos->hisPly = 0;

	pos->castlePerm = 0;

	pos->posKey = 0ULL;

}

void PrintBoard(const Position* pos) {
	int sq, file, rank, piece;
	const char* s = "   +---+---+---+---+---+---+---+---+\n";
	const char* t = "     A   B   C   D   E   F   G   H\n";
	printf(t);
	for (rank = RANK_8; rank >= RANK_1; rank--) {
		printf(s);
		printf(" %d |", rank + 1);
		for (file = FILE_A; file <= FILE_H; file++) {
			sq = FR2SQ(file, rank);
			piece = pos->pieces[sq];
			printf(" %c |", PceChar[piece]);
		}
		printf(" %d \n", rank + 1);
	}
	printf(s);
	printf(t);
	printf("\n");
	printf("side:%c\n", SideChar[pos->side]);
	printf("enPas:%d\n", pos->enPas);
	printf("castle:%c%c%c%c\n",
		pos->castlePerm & WKCA ? 'K' : '-',
		pos->castlePerm & WQCA ? 'Q' : '-',
		pos->castlePerm & BKCA ? 'k' : '-',
		pos->castlePerm & BQCA ? 'q' : '-'
	);
	printf("PosKey:%llX\n", pos->posKey);
}

U64 GeneratePosKey(const Position* pos) {

	int sq = 0;
	U64 finalKey = 0;
	int piece = EMPTY;

	// pieces
	for (sq = 0; sq < BRD_SQ_NUM; ++sq) {
		piece = pos->pieces[sq];
		if (piece != NO_SQ && piece != EMPTY && piece != OFFBOARD) {
			finalKey ^= PieceKeys[piece][sq];
		}
	}

	if (pos->side == WHITE) {
		finalKey ^= SideKey;
	}

	if (pos->enPas != NO_SQ) {
		finalKey ^= PieceKeys[EMPTY][pos->enPas];
	}


	finalKey ^= CastleKeys[pos->castlePerm];

	return finalKey;
}

U64 GetTimeMs() {
#ifdef WIN32
	return GetTickCount64();
#else
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000 + t.tv_usec / 1000;
#endif
}

char* PrSq(const int sq) {

	static char SqStr[3];

	int file = FilesBrd[sq];
	int rank = RanksBrd[sq];

	sprintf(SqStr, "%c%c", ('a' + file), ('1' + rank));

	return SqStr;

}

char* MoveToUci(const int move) {

	static char MvStr[6];

	int ff = FilesBrd[FROMSQ(move)];
	int rf = RanksBrd[FROMSQ(move)];
	int ft = FilesBrd[TOSQ(move)];
	int rt = RanksBrd[TOSQ(move)];

	int promoted = PROMOTED(move);

	if (promoted) {
		char pchar = 'q';
		if (IsKn(promoted)) {
			pchar = 'n';
		}
		else if (IsRQ(promoted) && !IsBQ(promoted)) {
			pchar = 'r';
		}
		else if (!IsRQ(promoted) && IsBQ(promoted)) {
			pchar = 'b';
		}
		sprintf(MvStr, "%c%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt), pchar);
	}
	else {
		sprintf(MvStr, "%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt));
	}

	return MvStr;
}

int ParseMove(char* ptrChar, Position* pos) {

	if (ptrChar[1] > '8' || ptrChar[1] < '1') return NOMOVE;
	if (ptrChar[3] > '8' || ptrChar[3] < '1') return NOMOVE;
	if (ptrChar[0] > 'h' || ptrChar[0] < 'a') return NOMOVE;
	if (ptrChar[2] > 'h' || ptrChar[2] < 'a') return NOMOVE;

	int from = FR2SQ(ptrChar[0] - 'a', ptrChar[1] - '1');
	int to = FR2SQ(ptrChar[2] - 'a', ptrChar[3] - '1');


	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);
	int MoveNum = 0;
	int Move = 0;
	int PromPce = EMPTY;

	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {
		Move = list->moves[MoveNum].move;
		if (FROMSQ(Move) == from && TOSQ(Move) == to) {
			PromPce = PROMOTED(Move);
			if (PromPce != EMPTY) {
				if (IsRQ(PromPce) && !IsBQ(PromPce) && ptrChar[4] == 'r') {
					return Move;
				}
				else if (!IsRQ(PromPce) && IsBQ(PromPce) && ptrChar[4] == 'b') {
					return Move;
				}
				else if (IsRQ(PromPce) && IsBQ(PromPce) && ptrChar[4] == 'q') {
					return Move;
				}
				else if (IsKn(PromPce) && ptrChar[4] == 'n') {
					return Move;
				}
				continue;
			}
			return Move;
		}
	}

	return NOMOVE;
}

void PrintMoveList(const S_MOVELIST* list) {
	int index = 0;
	int score = 0;
	int move = 0;
	printf("MoveList:\n");

	for (index = 0; index < list->count; ++index) {

		move = list->moves[index].move;
		score = list->moves[index].score;

		printf("Move:%d > %s (score:%d)\n", index + 1, MoveToUci(move), score);
	}
	printf("MoveList Total %d Moves:\n\n", list->count);
}

int GetPvLine(const int depth, Position* pos) {


	int move = ProbePvMove(pos);
	int count = 0;

	while (move != NOMOVE && count < depth) {


		if (MoveExists(pos, move)) {
			MakeMove(pos, move);
			pos->PvArray[count++] = move;
		}
		else {
			break;
		}
		move = ProbePvMove(pos);
	}

	while (pos->ply > 0) {
		TakeMove(pos);
	}

	return count;

}

void ClearHashTable(S_HASHTABLE* table) {

	S_HASHENTRY* tableEntry;

	for (tableEntry = table->pTable; tableEntry < table->pTable + table->numEntries; tableEntry++) {
		tableEntry->posKey = 0ULL;
		tableEntry->move = NOMOVE;
		tableEntry->depth = 0;
		tableEntry->score = 0;
		tableEntry->flags = 0;
	}
	table->newWrite = 0;
}

int Permill(S_HASHTABLE* table) {
	S_HASHENTRY* tableEntry = table->pTable;
	int pm = 0;
	for (int n = 0; n < 1000; n++) {
		if (tableEntry->posKey)
			pm++;
		tableEntry++;
	}
	return pm;
}

void InitHashTable(S_HASHTABLE* table, const int MB) {

	int HashSize = 1000000 * MB;
	table->numEntries = HashSize / sizeof(S_HASHENTRY);
	table->numEntries -= 2;

	if (table->pTable != NULL) {
		free(table->pTable);
	}

	table->pTable = (S_HASHENTRY*)malloc(table->numEntries * sizeof(S_HASHENTRY));
	if (table->pTable == NULL) {
		printf("Hash Allocation Failed, trying %d MB...\n", MB / 2);
		InitHashTable(table, MB / 2);
	}
	else {
		ClearHashTable(table);
		printf("HashTable init complete with %d MB\n", MB);
	}

}

int ProbeHashEntry(Position* pos, int* move, int* score, int alpha, int beta, int depth) {

	int index = pos->posKey % pos->hashTable.numEntries;

	if (pos->hashTable.pTable[index].posKey == pos->posKey) {
		*move = pos->hashTable.pTable[index].move;
		if (pos->hashTable.pTable[index].depth >= depth) {
			pos->hashTable.hit++;


			*score = pos->hashTable.pTable[index].score;
			if (*score > ISMATE) *score -= pos->ply;
			else if (*score < -ISMATE) *score += pos->ply;

			switch (pos->hashTable.pTable[index].flags) {


			case HFALPHA: if (*score <= alpha) {
				*score = alpha;
				return TRUE;
			}
						break;
			case HFBETA: if (*score >= beta) {
				*score = beta;
				return TRUE;
			}
					   break;
			case HFEXACT:
				return TRUE;
				break;
			}
		}
	}

	return FALSE;
}

void StoreHashEntry(Position* pos, const int move, int score, const int flags, const int depth) {

	int index = pos->posKey % pos->hashTable.numEntries;

	if (pos->hashTable.pTable[index].posKey == 0) {
		pos->hashTable.newWrite++;
	}
	else {
		pos->hashTable.overWrite++;
	}

	if (score > ISMATE) score += pos->ply;
	else if (score < -ISMATE) score -= pos->ply;

	pos->hashTable.pTable[index].move = move;
	pos->hashTable.pTable[index].posKey = pos->posKey;
	pos->hashTable.pTable[index].flags = flags;
	pos->hashTable.pTable[index].score = score;
	pos->hashTable.pTable[index].depth = depth;
}

int ProbePvMove(const Position* pos) {

	int index = pos->posKey % pos->hashTable.numEntries;

	if (pos->hashTable.pTable[index].posKey == pos->posKey) {
		return pos->hashTable.pTable[index].move;
	}

	return NOMOVE;
}

static int InputWaiting() {
#ifndef WIN32
	fd_set readfds;
	struct timeval tv;
	FD_ZERO(&readfds);
	FD_SET(fileno(stdin), &readfds);
	tv.tv_sec = 0; tv.tv_usec = 0;
	select(16, &readfds, 0, 0, &tv);

	return (FD_ISSET(fileno(stdin), &readfds));
#else
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
		if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
		return dw;
	}
	else {
		GetNumberOfConsoleInputEvents(inh, &dw);
		return dw <= 1 ? 0 : dw;
	}
#endif
}

void ReadInput(Position* pos, SearchInfo* info) {
	int bytes;
	char input[256] = "";
	if (InputWaiting()) {
		info->stop = TRUE;
		do {
			bytes = read(fileno(stdin), input, 256);
		} while (bytes < 0);
		if (strlen(input) > 0)
			UciCommand(pos, info, input);
	}
}

void ResetInfo(SearchInfo* info) {
	info->stop = FALSE;
	info->post = TRUE;
	info->nodes = 0;
	info->timeStart = GetTimeMs();
	info->depthLimit = MAX_PLY;
	info->timeLimit = 0;
	info->nodesLimit = 0;
}

void PrintPerformanceHeader() {
	printf("-----------------------------\n");
	printf("ply      time        nodes\n");
	printf("-----------------------------\n");
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
	int p = (int)pow(10, sn * 3);
	int b = (int)pow(10, 3);
	printf("-----------------------------\n");
	printf("Time        : %llu\n", time);
	printf("Nodes       : %llu\n", nodes);
	printf("Nps         : %llu (%llu%s/s)\n", nps, nps / p, units[sn]);
	printf("-----------------------------\n");
}

static inline void PerftDriver(Position* pos, SearchInfo* info, int depth) {
	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);
	for (int n = 0; n < list->count; n++) {
		if (!MakeMove(pos, list->moves[n].move))
			continue;
		if (depth)
			PerftDriver(pos, info, depth - 1);
		else
			info->nodes++;
		TakeMove(pos);
	}
}

//performance test
static inline void UciPerformance(Position* pos, SearchInfo* info) {
	ResetInfo(info);
	PrintPerformanceHeader();
	info->depthLimit = 0;
	U64 elapsed = 0;
	while (elapsed < 3000) {
		PerftDriver(pos, info, info->depthLimit++);
		elapsed = GetTimeMs() - info->timeStart;
		printf(" %2d. %8llu %12llu\n", info->depthLimit, elapsed, info->nodes);
	}
	PrintSummary(elapsed, info->nodes);
}

//start benchmark
static void UciBench(Position* pos, SearchInfo* info) {
	ResetInfo(info);
	PrintPerformanceHeader();
	info->depthLimit = 0;
	info->post = FALSE;
	U64 elapsed = 0;
	while (elapsed < 3000)
	{
		++info->depthLimit;
		SearchIteratively(pos, info);
		elapsed = GetTimeMs() - info->timeStart;
		printf(" %2d. %8llu %12llu\n", info->depthLimit, elapsed, info->nodes);
	}
	PrintSummary(elapsed, info->nodes);
}

static void ParseGo(char* command, SearchInfo* info, Position* pos) {
	ResetInfo(info);
	int wtime = 0;
	int btime = 0;
	int winc = 0;
	int binc = 0;
	int movestogo = 32;
	char* argument = NULL;
	if (argument = strstr(command, "binc"))
		binc = atoi(argument + 5);
	if (argument = strstr(command, "winc"))
		winc = atoi(argument + 5);
	if (argument = strstr(command, "wtime"))
		wtime = atoi(argument + 6);
	if (argument = strstr(command, "btime"))
		btime = atoi(argument + 6);
	if ((argument = strstr(command, "movestogo")))
		movestogo = atoi(argument + 10);
	if ((argument = strstr(command, "movetime")))
		info->timeLimit = atoi(argument + 9);
	if ((argument = strstr(command, "depth")))
		info->depthLimit = atoi(argument + 6);
	if (argument = strstr(command, "nodes"))
		info->nodesLimit = atoi(argument + 5);
	int time = pos->side == BLACK ? btime : wtime;
	int inc = pos->side == BLACK ? binc : winc;
	if (time)
		info->timeLimit = min(time / movestogo + inc, time / 2);
	SearchIteratively(pos, info);
}

static void ParsePosition(char* lineIn, Position* pos) {
	lineIn += 9;
	char* ptrChar = lineIn;
	if (strncmp(lineIn, "startpos", 8) == 0) {
		SetFen(START_FEN, pos);
	}
	else {
		ptrChar = strstr(lineIn, "fen");
		if (ptrChar == NULL) {
			SetFen(START_FEN, pos);
		}
		else {
			ptrChar += 4;
			SetFen(ptrChar, pos);
		}
	}

	ptrChar = strstr(lineIn, "moves");
	int move;

	if (ptrChar != NULL) {
		ptrChar += 6;
		while (*ptrChar) {
			move = ParseMove(ptrChar, pos);
			if (move == NOMOVE) break;
			MakeMove(pos, move);
			pos->ply = 0;
			while (*ptrChar && *ptrChar != ' ') ptrChar++;
			ptrChar++;
		}
	}
}

void UciCommand(Position* pos, SearchInfo* info, char* line) {
	if (!strncmp(line, "isready", 7))
		printf("readyok\n");
	else if (!strncmp(line, "position", 8))
		ParsePosition(line, pos);
	else if (!strncmp(line, "ucinewgame", 10))
		ClearHashTable(&pos->hashTable);
	else if (!strncmp(line, "go", 2))
		ParseGo(line, info, pos);
	else if (!strncmp(line, "quit", 4))
		exit(0);
	else if (!strncmp(line, "perft", 5))
		UciPerformance(pos, info);
	else if (!strncmp(line, "bench", 5))
		UciBench(pos, info);
	else if (!strncmp(line, "print", 5))
		PrintBoard(pos);
	else if (!strncmp(line, "uci", 3)) {
		printf("id name %s\n", NAME);
		printf("option name Hash type spin default %d min %d max %d\n", HASH_DEF, HASH_MIN, HASH_MAX);
		printf("uciok\n");
	}
	else if (!strncmp(line, "setoption name Hash value ", 26)) {
		int MB = 256;
		sscanf(line, "%*s %*s %*s %*s %d", &MB);
		if (MB < HASH_MIN) MB = HASH_MIN;
		if (MB > HASH_MAX) MB = HASH_MAX;
		printf("Set Hash to %d MB\n", MB);
		InitHashTable(&pos->hashTable, MB);
	}
}

void UciLoop(Position* pos, SearchInfo* info) {
	char line[4000];
	while (fgets(line, sizeof(line), stdin))
		UciCommand(pos, info, line);
}

int main() {
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	printf("%s %s\n", NAME, VERSION);
	InitPst();
	InitSq120To64();
	InitBitMasks();
	InitHashKeys();
	InitFilesRanksBrd();
	InitEvalMasks();
	InitMvvLva();
	InitSearch();
	pos.hashTable.pTable = NULL;
	InitHashTable(&pos.hashTable, HASH_DEF);
	SetFen(START_FEN, &pos);
	UciLoop(&pos, &info);
	free(pos.hashTable.pTable);
	return 0;
}
