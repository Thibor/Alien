#include "program.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

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

#define RAND_64 ((U64)rand() | (U64)rand() << 15 | (U64)rand() << 30 | (U64)rand() << 45 |((U64)rand() & 0xf) << 60 )

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

void ParseGo(char* command, SearchInfo* info, Position* pos) {
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
