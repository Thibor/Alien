// evaluate.c

#include "stdio.h"
#include "program.h"

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

// Piece Square Tables (by Lyudmil)
const int PawnMG[64] =
{
	0,   0,   0,   0,   0,   0,   0,   0,
	-5,  -2,   4,   5,   5,   4,  -2,  -5,
	-4,  -2,   5,   7,   7,   5,  -2,  -4,
	-2,  -1,   9,  13,  13,   9,  -1,  -2,
	2,   4,  13,  21,  21,  13,   4,   2,
	10,  21,  25,  29,  29,  25,  21,  10,
	1,   2,   5,   9,   9,   5,   2,   1,             // Pawns 7 Rank
	0,   0,   0,   0,   0,   0,   0,   0
};

const int PawnEG[64] =
{
	0,   0,   0,   0,   0,   0,   0,   0,
	-3,  -1,   2,   3,   3,   2,  -1,  -3,
	-2,  -1,   3,   4,   4,   3,  -1,  -2,
	-1,   0,   4,   7,   7,   4,   0,  -1,
	1,   2,   7,  11,  11,   7,   2,   1,
	5,  11,  13,  14,  14,  13,  11,   5,
	0,   1,   3,   5,   5,   3,   1,   0,    // Pawns 7 Rank
	0,   0,   0,   0,   0,   0,   0,   0
};

const int KnightMG[64] =
{
	-31, -23, -20, -16, -16, -20, -23, -31,
	-23, -16, -12,  -8,  -8, -12, -16, -23,
	-8,  -4,   0,   8,   8,   0,  -4,  -8,
	-4,   8,  12,  16,  16,  12,   8,  -4,
	8,  16,  20,  23,  23,  20,  16,   8,
	23,  27,  31,  35,  35,  31,  27,  23,
	4,   8,  12,  16,  16,  12,   8,   4,
	4,   4,   4,   4,   4,   4,   4,   4,
};

const int KnightEG[64] =
{
	-39, -27, -23, -20, -20, -23, -27, -39,
	-27, -20, -12,  -8,  -8, -12, -20, -27,
	-8,  -4,   0,   8,   8,   0,  -4,  -8,
	-4,   8,  12,  16,  16,  12,   8,  -4,
	8,  16,  20,  23,  23,  20,  16,   8,
	12,  23,  27,  31,  31,  27,  23,  12,
	-2,   2,   4,   8,   8,   4,   2,  -2,
	-16,  -8,  -4,  -4,  -4,  -4,  -8, -16,
};

const int BishopMG[64] =
{
	-31, -23, -20, -16, -16, -20, -23, -31,
	-23, -16, -12,  -8,  -8, -12, -16, -23,
	-8,  -4,   0,   8,   8,   0,  -4,  -8,
	-4,   8,  12,  16,  16,  12,   8,  -4,
	8,  16,  20,  23,  23,  20,  16,   8,
	23,  27,  31,  35,  35,  31,  27,  23,
	4,   8,  12,  16,  16,  12,   8,   4,
	4,   4,   4,   4,   4,   4,   4,   4,
};

const int BishopEG[64] =
{
	-39, -27, -23, -20, -20, -23, -27, -39,
	-27, -20, -12,  -8,  -8, -12, -20, -27,
	-8,  -4,   0,   8,   8,   0,  -4,  -8,
	-4,   8,  12,  16,  16,  12,   8,  -4,
	8,  16,  20,  23,  23,  20,  16,   8,
	12,  23,  27,  31,  31,  27,  23,  12,
	-2,   2,   4,   8,   8,   4,   2,  -2,
	-16,  -8,  -4,  -4,  -4,  -4,  -8, -16,
};

const int RookMG[64] =
{
	-10,  -8,  -6,  -4,  -4,  -6,  -8, -10,
	-8,  -6,  -4,  -2,  -2,  -4,  -6,  -8,
	-4,  -2,   0,   4,   4,   0,  -2,  -4,
	-2,   2,   4,   8,   8,   4,   2,  -2,
	2,   4,   8,  12,  12,   8,   4,   2,
	4,   8,   12, 16,  16,  12,   8,   4,
	20,  21,   23, 23,  23,  23,  21,  20,
	18,  18,   20, 20,  20,  20,  18,  18,
};

const int RookEG[64] =
{
	-10,  -8,  -6,  -4,  -4,  -6,  -8, -10,
	-8,  -6,  -4,  -2,  -2,  -4,  -6,  -8,
	-4,  -2,   0,   4,   4,   0,  -2,  -4,
	-2,   2,   4,   8,   8,   4,   2,  -2,
	2,   4,   8,  12,  12,   8,   4,   2,
	4,   8,  12,  16,  16,  12,   8,   4,
	20,  21,  23,  23,  23,  23,  21,  20,
	18,  18,  20,  20,  20,  20,  18,  18,
};

const int QueenMG[64] =
{
	-23, -20, -16, -12, -12, -16, -20, -23,
	-18, -14, -12,  -8,  -8, -12, -14, -18,
	-16,  -8,   0,   8,   8,   0,  -8, -16,
	-8,   0,  12,  16,  16,  12,   0,  -8,
	4,  12,  16,  23,  23,  16,  12,   4,
	16,  23,  27,  31,  31,  27,  23,  16,
	4,  12,  16,  23,  23,  16,  12,   4,
	2,   8,  12,  12,  12,  12,   8,   2,
};

const int QueenEG[64] =
{
	-23, -20, -16, -12, -12, -16, -20, -23,
	-18, -14, -12,  -8,  -8, -12, -14, -18,
	-16,  -8,   0,   8,   8,   0,  -8, -16,
	-8,   0,  12,  16,  16,  12,   0,  -8,
	4,  12,  16,  23,  23,  16,  12,   4,
	16,  23,  27,  31,  31,  27,  23,  16,
	4,  12,  16,  23,  23,  16,  12,   4,
	2,   8,  12,  12,  12,  12,   8,   2,
};

const int KingMG[64] =
{
	40,  50,  30,  10,  10,  30,  50,  40,
	30,  40,  20,   0,   0,  20,  40,  30,
	10,  20,   0, -20, -20,   0,  20,  10,
	0,  10, -10, -30, -30, -10,  10,   0,
	-10,   0, -20, -40, -40, -20,   0, -10,
	-20, -10, -30, -50, -50, -30, -10, -20,
	-30, -20, -40, -60, -60, -40, -20, -30,
	-40, -30, -50, -70, -70, -50, -30, -40
};

const int KingEG[64] =
{
	-34, -30, -28, -27, -27, -28, -30, -34,
	-17, -13, -11, -10, -10, -11, -13, -17,
	-2,   2,   4,   5,   5,   4,   2,  -2,
	11,  15,  17,  18,  18,  17,  15,  11,
	22,  26,  28,  29,  29,  28,  26,  22,
	31,  34,  37,  38,  38,  37,  34,  31,
	38,  41,  44,  45,  45,  44,  41,  38,
	42,  46,  48,  50,  50,  48,  46,  42,
};

static int MaterialDraw(const S_BOARD *pos) {

	ASSERT(CheckBoard(pos));

    if (!pos->pceNum[wR] && !pos->pceNum[bR] && !pos->pceNum[wQ] && !pos->pceNum[bQ]) {
	  if (!pos->pceNum[bB] && !pos->pceNum[wB]) {
	      if (pos->pceNum[wN] < 3 && pos->pceNum[bN] < 3) {  return TRUE; }
	  } else if (!pos->pceNum[wN] && !pos->pceNum[bN]) {
	     if (abs(pos->pceNum[wB] - pos->pceNum[bB]) < 2) { return TRUE; }
	  } else if ((pos->pceNum[wN] < 3 && !pos->pceNum[wB]) || (pos->pceNum[wB] == 1 && !pos->pceNum[wN])) {
	    if ((pos->pceNum[bN] < 3 && !pos->pceNum[bB]) || (pos->pceNum[bB] == 1 && !pos->pceNum[bN]))  { return TRUE; }
	  }
	} else if (!pos->pceNum[wQ] && !pos->pceNum[bQ]) {
        if (pos->pceNum[wR] == 1 && pos->pceNum[bR] == 1) {
            if ((pos->pceNum[wN] + pos->pceNum[wB]) < 2 && (pos->pceNum[bN] + pos->pceNum[bB]) < 2)	{ return TRUE; }
        } else if (pos->pceNum[wR] == 1 && !pos->pceNum[bR]) {
            if ((pos->pceNum[wN] + pos->pceNum[wB] == 0) && (((pos->pceNum[bN] + pos->pceNum[bB]) == 1) || ((pos->pceNum[bN] + pos->pceNum[bB]) == 2))) { return TRUE; }
        } else if (pos->pceNum[bR] == 1 && !pos->pceNum[wR]) {
            if ((pos->pceNum[bN] + pos->pceNum[bB] == 0) && (((pos->pceNum[wN] + pos->pceNum[wB]) == 1) || ((pos->pceNum[wN] + pos->pceNum[wB]) == 2))) { return TRUE; }
        }
    }
  return FALSE;
}

int EvalPosition(S_BOARD *pos) {

	ASSERT(CheckBoard(pos));

	// test for drawn position before doing anything
	if(!pos->pceNum[wP] && !pos->pceNum[bP] && MaterialDraw(pos) == TRUE) {
		return 0;
	}

	int pce;
	int pceNum;
	int sq;
	int phase = totalPhase;
	int scoreMG, scoreEG;
	scoreMG = scoreEG = pos->material[WHITE] - pos->material[BLACK];
	int score;

	pce = wP;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];

		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

		scoreMG += PawnMG[SQ64(sq)];
		scoreEG += PawnEG[SQ64(sq)];

		if( (IsolatedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			//printf("wP Iso:%s\n",PrSq(sq));
			scoreMG += PawnIsolated;
			scoreEG += PawnIsolated;
		}

		if( (WhitePassedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			//printf("wP Passed:%s\n",PrSq(sq));
			scoreMG += PawnPassedMG[RanksBrd[sq]];
			scoreEG += PawnPassedEG[RanksBrd[sq]];
		}

	}

	pce = bP;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];

		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);

		scoreMG -= PawnMG[MIRROR64(SQ64(sq))];
		scoreEG -= PawnEG[MIRROR64(SQ64(sq))];

		if( (IsolatedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			//printf("bP Iso:%s\n",PrSq(sq));
			scoreMG -= PawnIsolated;
			scoreEG -= PawnIsolated;
		}

		if( (BlackPassedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			//printf("bP Passed:%s\n",PrSq(sq));
			scoreMG -= PawnPassedMG[7 - RanksBrd[sq]];
			scoreEG -= PawnPassedEG[7 - RanksBrd[sq]];
		}
	}

	pce = wN;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];

		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

		scoreMG += KnightMG[SQ64(sq)];
		scoreEG += KnightEG[SQ64(sq)];
		phase -= minorPhase;
	}

	pce = bN;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];

		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);

		scoreMG -= KnightMG[MIRROR64(SQ64(sq))];
		scoreEG -= KnightEG[MIRROR64(SQ64(sq))];
		phase -= minorPhase;
	}

	pce = wB;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];

		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

		scoreMG += BishopMG[SQ64(sq)];
		scoreEG += BishopEG[SQ64(sq)];
		phase -= minorPhase;
	}

	pce = bB;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];

		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);

		scoreMG -= BishopMG[MIRROR64(SQ64(sq))];
		scoreEG -= BishopEG[MIRROR64(SQ64(sq))];
		phase -= minorPhase;
	}

	pce = wR;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];

		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ASSERT(FileRankValid(FilesBrd[sq]));

		scoreMG += RookMG[SQ64(sq)];
		scoreEG += RookEG[SQ64(sq)];

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			scoreMG += RookOpenFile;
			scoreEG += RookOpenFile;
		} else if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			scoreMG += RookSemiOpenFile;
			scoreEG += RookSemiOpenFile;
		}
		phase -= rookPhase;
	}

	pce = bR;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];

		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		ASSERT(FileRankValid(FilesBrd[sq]));

		scoreMG -= RookMG[MIRROR64(SQ64(sq))];
		scoreEG -= RookEG[MIRROR64(SQ64(sq))];

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			scoreMG -= RookOpenFile;
			scoreEG -= RookOpenFile;
		} else if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			scoreMG -= RookSemiOpenFile;
			scoreEG -= RookSemiOpenFile;
		}
		phase -= rookPhase;
	}

	pce = wQ;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];

		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ASSERT(FileRankValid(FilesBrd[sq]));

		scoreMG += QueenMG[SQ64(sq)];
		scoreEG += QueenEG[SQ64(sq)];

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			scoreMG += QueenOpenFile;
			scoreEG += QueenOpenFile;
		} else if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			scoreMG += QueenSemiOpenFile;
			scoreEG += QueenSemiOpenFile;
		}
		phase -= queenPhase;
	}

	pce = bQ;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];

		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ASSERT(FileRankValid(FilesBrd[sq]));

		scoreMG -= QueenMG[MIRROR64(SQ64(sq))];
		scoreEG -= QueenEG[MIRROR64(SQ64(sq))];

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			scoreMG -= QueenOpenFile;
			scoreEG -= QueenOpenFile;
		} else if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			scoreMG -= QueenSemiOpenFile;
			scoreEG -= QueenSemiOpenFile;
		}
		phase -= queenPhase;
	}
	pce = wK;
	sq = pos->pList[pce][0];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

	scoreMG += KingMG[SQ64(sq)];
	scoreEG += KingEG[SQ64(sq)];

	pce = bK;
	sq = pos->pList[pce][0];
	ASSERT(SqOnBoard(sq));
	ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);

	scoreMG -= KingMG[MIRROR64(SQ64(sq))];
	scoreEG -= KingEG[MIRROR64(SQ64(sq))];

	if(pos->pceNum[wB] >= 2) {
		scoreMG += BishopPairMG;
		scoreEG += BishopPairEG;
	}
	if(pos->pceNum[bB] >= 2) {
		scoreMG -= BishopPairMG;
		scoreEG -= BishopPairEG;
	}

	// calculating game phase and interpolating score values between phases
	phase = (phase * 256 + (totalPhase / 2)) / totalPhase;
	score = ((scoreMG * (256 - phase)) + (scoreEG * phase)) / 256;

	return pos->side == WHITE ? score : -score;

}
