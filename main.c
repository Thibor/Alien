
#include <math.h>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#include "program.h"

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
int PieceVal[13] = { 0, 100, 325, 325, 550, 1000, 50000, 100, 325, 325, 550, 1000, 50000 };
int PieceCol[13] = { BOTH, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
	BLACK, BLACK, BLACK, BLACK, BLACK, BLACK };

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

	ASSERT(moveNum >= 0 && moveNum < list->count);
	ASSERT(bestNum >= 0 && bestNum < list->count);
	ASSERT(bestNum >= moveNum);

	temp = list->moves[moveNum];
	list->moves[moveNum] = list->moves[bestNum];
	list->moves[bestNum] = temp;
}

static int IsRepetition(const Position* pos) {

	int index = 0;

	for (index = pos->hisPly - pos->fiftyMove; index < pos->hisPly - 1; ++index) {
		ASSERT(index >= 0 && index < MAXGAMEMOVES);
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

	pos->HashTable.overWrite = 0;
	pos->HashTable.hit = 0;
	pos->HashTable.cut = 0;
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
	printf("hashfull %d pv", Permill(&pos->HashTable));
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

	ASSERT(Score > -INFINITE && Score < INFINITE);

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

	ASSERT(alpha >= OldAlpha);

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
		pos->HashTable.cut++;
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

	ASSERT(alpha >= OldAlpha);

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

	ASSERT(PieceValid(pce));

	int col = PieceCol[pce];
	int index = 0;
	int t_pceNum = -1;

	ASSERT(SideValid(col));

	HASH_PCE(pce, sq);

	pos->pieces[sq] = EMPTY;
	pos->material[col] -= PieceVal[pce];

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

	ASSERT(t_pceNum != -1);
	ASSERT(t_pceNum >= 0 && t_pceNum < 10);

	pos->pceNum[pce]--;

	pos->pList[pce][t_pceNum] = pos->pList[pce][pos->pceNum[pce]];

}


static void AddPiece(const int sq, Position* pos, const int pce) {

	ASSERT(PieceValid(pce));
	ASSERT(SqOnBoard(sq));

	int col = PieceCol[pce];
	ASSERT(SideValid(col));

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

	pos->material[col] += PieceVal[pce];
	pos->pList[pce][pos->pceNum[pce]++] = sq;

}

static void MovePiece(const int from, const int to, Position* pos) {

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

	int index = 0;
	int pce = pos->pieces[from];
	int col = PieceCol[pce];
	ASSERT(SideValid(col));
	ASSERT(PieceValid(pce));

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
	ASSERT(t_PieceNum);
}

int MakeMove(Position* pos, int move) {

	int from = FROMSQ(move);
	int to = TOSQ(move);
	int side = pos->side;

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(SideValid(side));
	ASSERT(PieceValid(pos->pieces[from]));
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAX_PLY);

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
		default: ASSERT(FALSE); break;
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
		ASSERT(PieceValid(captured));
		ClearPiece(to, pos);
		pos->fiftyMove = 0;
	}

	pos->hisPly++;
	pos->ply++;

	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAX_PLY);

	if (PiecePawn[pos->pieces[from]]) {
		pos->fiftyMove = 0;
		if (move & MFLAGPS) {
			if (side == WHITE) {
				pos->enPas = from + 10;
				ASSERT(RanksBrd[pos->enPas] == RANK_3);
			}
			else {
				pos->enPas = from - 10;
				ASSERT(RanksBrd[pos->enPas] == RANK_6);
			}
			HASH_EP;
		}
	}

	MovePiece(from, to, pos);

	int prPce = PROMOTED(move);
	if (prPce != EMPTY) {
		ASSERT(PieceValid(prPce) && !PiecePawn[prPce]);
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

	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAX_PLY);

	int move = pos->history[pos->hisPly].move;
	int from = FROMSQ(move);
	int to = TOSQ(move);

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

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
		default: ASSERT(FALSE); break;
		}
	}

	MovePiece(to, from, pos);

	if (PieceKing[pos->pieces[from]]) {
		pos->KingSq[pos->side] = from;
	}

	int captured = CAPTURED(move);
	if (captured != EMPTY) {
		ASSERT(PieceValid(captured));
		AddPiece(to, pos, captured);
	}

	if (PROMOTED(move) != EMPTY) {
		ASSERT(PieceValid(PROMOTED(move)) && !PiecePawn[PROMOTED(move)]);
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
		ASSERT(PceValidEmptyOffbrd(pce));
		if (pce != OFFBOARD && IsKn(pce) && PieceCol[pce] == side) {
			return TRUE;
		}
	}

	// rooks, queens
	for (index = 0; index < 4; ++index) {
		dir = RkDir[index];
		t_sq = sq + dir;
		ASSERT(SqIs120(t_sq));
		pce = pos->pieces[t_sq];
		ASSERT(PceValidEmptyOffbrd(pce));
		while (pce != OFFBOARD) {
			if (pce != EMPTY) {
				if (IsRQ(pce) && PieceCol[pce] == side) {
					return TRUE;
				}
				break;
			}
			t_sq += dir;
			ASSERT(SqIs120(t_sq));
			pce = pos->pieces[t_sq];
		}
	}

	// bishops, queens
	for (index = 0; index < 4; ++index) {
		dir = BiDir[index];
		t_sq = sq + dir;
		ASSERT(SqIs120(t_sq));
		pce = pos->pieces[t_sq];
		ASSERT(PceValidEmptyOffbrd(pce));
		while (pce != OFFBOARD) {
			if (pce != EMPTY) {
				if (IsBQ(pce) && PieceCol[pce] == side) {
					return TRUE;
				}
				break;
			}
			t_sq += dir;
			ASSERT(SqIs120(t_sq));
			pce = pos->pieces[t_sq];
		}
	}

	// kings
	for (index = 0; index < 8; ++index) {
		pce = pos->pieces[sq + KiDir[index]];
		ASSERT(PceValidEmptyOffbrd(pce));
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
	int index = 0;

	for (index = 0; index < 64; index++) {
		SetMask[index] = 0ULL;
		ClearMask[index] = 0ULL;
	}

	for (index = 0; index < 64; index++) {
		SetMask[index] |= (1ULL << index);
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
		ASSERT(PceValidEmptyOffbrd(piece));
		if (piece != OFFBOARD && piece != EMPTY) {
			colour = PieceCol[piece];
			ASSERT(SideValid(colour));

			if (PieceBig[piece] == TRUE) pos->bigPce[colour]++;
			if (PieceMin[piece] == TRUE) pos->minPce[colour]++;
			if (PieceMaj[piece] == TRUE) pos->majPce[colour]++;

			pos->material[colour] += PieceVal[piece];

			ASSERT(pos->pceNum[piece] < 10 && pos->pceNum[piece] >= 0);

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

	ASSERT(fen != NULL);
	ASSERT(pos != NULL);

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

	ASSERT(*fen == 'w' || *fen == 'b');

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

	ASSERT(pos->castlePerm >= 0 && pos->castlePerm <= 15);

	if (*fen != '-') {
		file = fen[0] - 'a';
		rank = fen[1] - '1';

		ASSERT(file >= FILE_A && file <= FILE_H);
		ASSERT(rank >= RANK_1 && rank <= RANK_8);

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
		pos->material[index] = 0;
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
			ASSERT(piece >= wP && piece <= bK);
			finalKey ^= PieceKeys[piece][sq];
		}
	}

	if (pos->side == WHITE) {
		finalKey ^= SideKey;
	}

	if (pos->enPas != NO_SQ) {
		ASSERT(pos->enPas >= 0 && pos->enPas < BRD_SQ_NUM);
		ASSERT(SqOnBoard(pos->enPas));
		ASSERT(RanksBrd[pos->enPas] == RANK_3 || RanksBrd[pos->enPas] == RANK_6);
		finalKey ^= PieceKeys[EMPTY][pos->enPas];
	}

	ASSERT(pos->castlePerm >= 0 && pos->castlePerm <= 15);

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

	ASSERT(SqOnBoard(from) && SqOnBoard(to));

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

	ASSERT(depth < MAX_PLY && depth >= 1);

	int move = ProbePvMove(pos);
	int count = 0;

	while (move != NOMOVE && count < depth) {

		ASSERT(count < MAX_PLY);

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

	int index = pos->posKey % pos->HashTable.numEntries;

	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);
	ASSERT(depth >= 1 && depth < MAX_PLY);
	ASSERT(alpha < beta);
	ASSERT(alpha >= -INFINITE && alpha <= INFINITE);
	ASSERT(beta >= -INFINITE && beta <= INFINITE);
	ASSERT(pos->ply >= 0 && pos->ply < MAX_PLY);

	if (pos->HashTable.pTable[index].posKey == pos->posKey) {
		*move = pos->HashTable.pTable[index].move;
		if (pos->HashTable.pTable[index].depth >= depth) {
			pos->HashTable.hit++;

			ASSERT(pos->HashTable->pTable[index].depth >= 1 && pos->HashTable->pTable[index].depth < MAX_PLY);
			ASSERT(pos->HashTable->pTable[index].flags >= HFALPHA && pos->HashTable->pTable[index].flags <= HFEXACT);

			*score = pos->HashTable.pTable[index].score;
			if (*score > ISMATE) *score -= pos->ply;
			else if (*score < -ISMATE) *score += pos->ply;

			switch (pos->HashTable.pTable[index].flags) {

				ASSERT(*score >= -INFINITE && *score <= INFINITE);

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
			default: ASSERT(FALSE); break;
			}
		}
	}

	return FALSE;
}

void StoreHashEntry(Position* pos, const int move, int score, const int flags, const int depth) {

	int index = pos->posKey % pos->HashTable.numEntries;

	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);
	ASSERT(depth >= 1 && depth < MAX_PLY);
	ASSERT(flags >= HFALPHA && flags <= HFEXACT);
	ASSERT(score >= -INFINITE && score <= INFINITE);
	ASSERT(pos->ply >= 0 && pos->ply < MAX_PLY);

	if (pos->HashTable.pTable[index].posKey == 0) {
		pos->HashTable.newWrite++;
	}
	else {
		pos->HashTable.overWrite++;
	}

	if (score > ISMATE) score += pos->ply;
	else if (score < -ISMATE) score -= pos->ply;

	pos->HashTable.pTable[index].move = move;
	pos->HashTable.pTable[index].posKey = pos->posKey;
	pos->HashTable.pTable[index].flags = flags;
	pos->HashTable.pTable[index].score = score;
	pos->HashTable.pTable[index].depth = depth;
}

int ProbePvMove(const Position* pos) {

	int index = pos->posKey % pos->HashTable.numEntries;
	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);

	if (pos->HashTable.pTable[index].posKey == pos->posKey) {
		return pos->HashTable.pTable[index].move;
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
		ParsePosition("position startpos\n", pos);
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
		InitHashTable(&pos->HashTable, MB);
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
	InitSq120To64();
	InitBitMasks();
	InitHashKeys();
	InitFilesRanksBrd();
	InitEvalMasks();
	InitMvvLva();
	InitSearch();
	pos.HashTable.pTable = NULL;
	InitHashTable(&pos.HashTable, HASH_DEF);
	SetFen(START_FEN, &pos);
	UciLoop(&pos, &info);
	free(pos.HashTable.pTable);
	return 0;
}
