#include "program.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

Position pos;
SearchInfo info;

const int BitTable[64] = {
  63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
  51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
  26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
  58, 20, 37, 17, 36, 8
};

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
		UciCommand(pos,info, line);
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
    InitHashTable(&pos.HashTable,HASH_DEF);
	SetFen(START_FEN, &pos);
	UciLoop(&pos, &info);
	free(pos.HashTable.pTable);
	return 0;
}
