#include "program.h"

//#define WAC1 "r1b1k2r/ppppnppp/2n2q2/2b5/3NP3/2P1B3/PP3PPP/RN1QKB1R w KQkq - 0 1"
//#define PERFT "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"

int main() {
	AllInit();
	Position pos[1]={0};
	SearchInfo info[1]={0};
    info->quit = FALSE;
	pos->HashTable->pTable = NULL;
    InitHashTable(pos->HashTable, 256);
	setbuf(stdin, NULL);
    setbuf(stdout, NULL);
	Uci_Loop(pos, info);
	free(pos->HashTable->pTable);
	return 0;
}
