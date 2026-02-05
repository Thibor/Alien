#include "program.h"

#define INPUTBUFFER 4000

void ResetInfo(SearchInfo* info) {
	info->stop = FALSE;
	info->post = TRUE;
	info->nodes = 0;
	info->timeStart = GetTimeMs();
	info->depthLimit = MAXDEPTH;
	info->timeLimit = 0;
	info->nodesLimit = 0;
}

void PrintPerformanceHeader() {
	printf("-----------------------------\n");
	printf("ply      time        nodes\n");
	printf("-----------------------------\n");
}

static int ShrinkNumber(U64 n) {
	if (n < 1000)
		return 0;
	if (n < 1000000)
		return 1;
	if (n < 1000000000)
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
	ParseFen(START_FEN, pos);
	info->depthLimit = 0;
	U64 elapsed = 0;
	while (elapsed < 3000) {
		PerftDriver(pos, info, info->depthLimit++);
		elapsed = GetTimeMs() - info->timeStart;
		printf(" %2llu. %8llu %12llu\n", info->depthLimit, elapsed, info->nodes);
	}
	PrintSummary(elapsed, info->nodes);
}

//start benchmark
static void UciBench(Position* pos, SearchInfo* info) {
	ResetInfo(info);
	PrintPerformanceHeader();
	ParseFen(START_FEN, pos);
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
		ParseFen(START_FEN, pos);
	}
	else {
		ptrChar = strstr(lineIn, "fen");
		if (ptrChar == NULL) {
			ParseFen(START_FEN, pos);
		}
		else {
			ptrChar += 4;
			ParseFen(ptrChar, pos);
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

static void PrintWelcome() {
	printf("%s %s\n", NAME, VERSION);
}

void UciQuit() {
	exit(0);
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
		UciQuit();
	else if (!strncmp(line, "perft", 5))
		UciPerformance(pos, info);
	else if (!strncmp(line, "bench", 5))
		UciBench(pos, info);
	else if (!strncmp(line, "print", 5))
		PrintBoard(pos);
	else if (!strncmp(line, "uci", 3)) {
		printf("id name %s\n", NAME);
		printf("option name Hash type spin default 256 min 4 max %d\n", MAX_HASH);
		printf("uciok\n");
	}
	else if (!strncmp(line, "setoption name Hash value ", 26)) {
		int MB = 256;
		sscanf(line, "%*s %*s %*s %*s %d", &MB);
		if (MB < 4) MB = 4;
		if (MB > MAX_HASH) MB = MAX_HASH;
		printf("Set Hash to %d MB\n", MB);
		InitHashTable(pos->HashTable, MB);
	}
}

void UciLoop(Position* pos, SearchInfo* info) {
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	char line[INPUTBUFFER];
	int MB = 256;
	PrintWelcome();
	ParseFen(START_FEN, pos);
	while (TRUE) {
		memset(&line[0], 0, sizeof(line));
		fflush(stdout);
		if (!fgets(line, INPUTBUFFER, stdin))
			continue;
		if (line[0] == '\n')
			continue;
		UciCommand(pos, info, line);
	}
}
