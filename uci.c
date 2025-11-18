// uci.c

#include "stdio.h"
#include "program.h"
#include "string.h"

#define INPUTBUFFER 400 * 6

// go depth 6 wtime 180000 btime 100000 binc 1000 winc 1000 movetime 1000 movestogo 40
void ParseGo(char* line, S_SEARCHINFO* info, S_BOARD* pos) {

	int movestogo = 30, movetime = -1;
	int time = -1, inc = 0;
	char* ptr = NULL;
	info->starttime = GetTimeMs();
	info->timeset = FALSE;
	info->depthLimit=MAXDEPTH;
	info->nodesLimit = 0;

	if ((ptr = strstr(line, "infinite"))) {
	}

	if ((ptr = strstr(line, "binc")) && pos->side == BLACK) {
		inc = atoi(ptr + 5);
	}

	if ((ptr = strstr(line, "winc")) && pos->side == WHITE) {
		inc = atoi(ptr + 5);
	}

	if ((ptr = strstr(line, "wtime")) && pos->side == WHITE) {
		time = atoi(ptr + 6);
	}

	if ((ptr = strstr(line, "btime")) && pos->side == BLACK) {
		time = atoi(ptr + 6);
	}

	if ((ptr = strstr(line, "movestogo"))) {
		movestogo = atoi(ptr + 10);
	}

	if ((ptr = strstr(line, "movetime"))) {
		movetime = atoi(ptr + 9);
	}

	if ((ptr = strstr(line, "depth"))) {
		info->depthLimit = atoi(ptr + 6);
	}

	if ((ptr = strstr(line, "nodes"))) {
		info->nodesLimit = atoi(ptr + 6);
	}

	if (movetime != -1) {
		time = movetime;
		movestogo = 1;
	}

	if (time != -1) {
		info->timeset = TRUE;
		time /= movestogo;
		time -= 50;
		info->stoptime = info->starttime + time + inc;
	}

	SearchPosition(pos, info);
}

static void ParsePosition(char* lineIn, S_BOARD* pos) {

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

void Uci_Loop(S_BOARD* pos, S_SEARCHINFO* info) {

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

		if (!strncmp(line, "isready", 7)) {
			printf("readyok\n");
			continue;
		}
		else if (!strncmp(line, "position", 8)) {
			ParsePosition(line, pos);
		}
		else if (!strncmp(line, "ucinewgame", 10)) {
			ParsePosition("position startpos\n", pos);
		}
		else if (!strncmp(line, "go", 2)) {
			ParseGo(line, info, pos);
		}
		else if (!strncmp(line, "quit", 4)) {
			info->quit = TRUE;
			break;
		}
		else if (!strncmp(line, "uci", 3)) {
			printf("id name %s\n", NAME);
			printf("option name Hash type spin default 256 min 4 max %d\n", MAX_HASH);
			printf("uciok\n");
		}
		else if (!strncmp(line, "print", 5)) {
			PrintBoard(pos);
		}
		else if (!strncmp(line, "setoption name Hash value ", 26)) {
			sscanf(line, "%*s %*s %*s %*s %d", &MB);
			if (MB < 4) MB = 4;
			if (MB > MAX_HASH) MB = MAX_HASH;
			printf("Set Hash to %d MB\n", MB);
			InitHashTable(pos->HashTable, MB);
		}
		if (info->quit) break;
	}
}
