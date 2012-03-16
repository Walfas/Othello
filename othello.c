#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#define BOARDSIZE 8
#define PLAYER1 0
#define PLAYER2 1
#define EMPTY -1
#define CHR_PLAYER1 '-'
#define CHR_PLAYER2 '*'
#define CHR_EMPTY ' '
#define COL_PLAYER1 "\033[1;32m"
#define COL_PLAYER2 "\033[1;36m"
#define COL_EMPTY "\033[0m"
#define COL_MOVE "\033[1;33m"
#define TALL 0
#define MASK 255
#define DECODE(x) ((x)&MASK)

void getmoves(int board[BOARDSIZE][BOARDSIZE], int player, int *legalmoves);
void defaultboard(int board[BOARDSIZE][BOARDSIZE]);
int loadboard(char *fname, int board[BOARDSIZE][BOARDSIZE]);
void printboard(int board[BOARDSIZE][BOARDSIZE], int player, int *legalmoves);
void results(int player, int move, int board[BOARDSIZE][BOARDSIZE]);
void printscore(int board[BOARDSIZE][BOARDSIZE]);
int terminaltest(int player, int board[BOARDSIZE][BOARDSIZE]);
int decidemove(int *legalmoves);

main() {
	int board[BOARDSIZE][BOARDSIZE];
	//loadboard("infile.txt",board);
	int move;
	int movenum;
	int player = PLAYER1;
	int legalmoves[16];
	int iscomputer[] = {1,0};
	
	defaultboard(board);
	srand(time(NULL));
	
	int i;
	while(1) {
		getmoves(board,player,legalmoves);
		printboard(board,player,legalmoves);
		if (legalmoves[0]) {
			if (!iscomputer[player]) {
				do {
					printf("Type the number of the move you wish to make (1-%d): ",legalmoves[0]);
					scanf("%2d",&movenum); 
				}
				while(movenum > legalmoves[0] || movenum <= 0);
				
				move = legalmoves[movenum];
				printf("-----\nMove %d (%c%d) was made by human.\n\n",movenum,'A'+DECODE(move)%BOARDSIZE, DECODE(move)/BOARDSIZE+1);
			}
			else
				move = decidemove(legalmoves);
			
			results(player, move, board);
		}
		else if (terminaltest(player,board)) {
			printf("Game finished!\n\n\033[A");
			printscore(board);
			exit(0);
		}
		player = !player;
	}
}

void printscore(int board[BOARDSIZE][BOARDSIZE]) {
	int x,y;
	int p1score=0, p2score=0;
	for (y=0; y<BOARDSIZE; y++) {
		for (x=0; x<BOARDSIZE; x++) {
			if (board[x][y] == PLAYER1)
				p1score++;
			else if (board[x][y] == PLAYER2)
				p2score++;
		}
	}
	printf("P1 (%s%c%s): %-2d\033[B\033[10D",COL_PLAYER1,CHR_PLAYER1,COL_EMPTY,p1score);
	printf("P2 (%s%c%s): %-2d\033[B\033[10D",COL_PLAYER2,CHR_PLAYER2,COL_EMPTY,p2score);
}

void results(int player, int move, int board[BOARDSIZE][BOARDSIZE]) {
	int x = DECODE(move)%BOARDSIZE;
	int y = DECODE(move)/BOARDSIZE;
	int i, j;
	board[x][y] = player;
	
	// Northwest
	if (move&(1<<15))
		for (i=x-1,j=y-1; board[i][j] == !player; i--,j--)
			board[i][j] = player;		

	// North
	if (move&(1<<14))
		for (j=y-1; board[x][j] == !player; j--)
			board[x][j] = player;

	// Northeast
	if (move&(1<<13))
		for (i=x+1,j=y-1; board[i][j] == !player; i++,j--)
			board[i][j] = player;

	// West
	if (move&(1<<12))
		for (i=x-1; board[i][y] == !player; i--)
			board[i][y] = player;

	// East
	if (move&(1<<11))
		for (i=x+1; board[i][y] == !player; i++)
			board[i][y] = player;
			
	// Southwest
	if (move&(1<<10))
		for (i=x-1,j=y+1; board[i][j] == !player; i--,j++)
			board[i][j] = player;

	// South
	if (move&(1<<9))
		for (j=y+1; board[x][j] == !player; j++)
			board[x][j] = player;
	
	// Southeast
	if (move&(1<<8))
		for (i=x+1,j=y+1; board[i][j] == !player; i++,j++)
			board[i][j] = player;
}

int terminaltest(int player, int board[BOARDSIZE][BOARDSIZE]) {
	int legalmoves[16];
	getmoves(board,!player,legalmoves);
	return !legalmoves[0];
}

int decidemove(int *legalmoves) {
	int movenum = 1+rand()%legalmoves[0];
	int move = legalmoves[movenum];
	printf("-----\nMove #%d (%c%d) was made by computer.\n\n",movenum,'A'+DECODE(move)%BOARDSIZE, DECODE(move)/BOARDSIZE+1);
	return move;
}

void defaultboard(int board[BOARDSIZE][BOARDSIZE]) {
	int x, y;
	for (x=0; x<BOARDSIZE; x++)
		for (y=0; y<BOARDSIZE; y++)
			board[x][y] = EMPTY;
	board[4][3] = board[3][4] = PLAYER1;
	board[3][3] = board[4][4] = PLAYER2;
}

int loadboard(char *fname, int board[BOARDSIZE][BOARDSIZE]) {
	int i, j;
	FILE *fp;
	
	if ( (fp = fopen(fname,"r")) == NULL) {
		fprintf(stderr,"Error: Could not open file \"%s\" for reading.\n",fname);
		return -1;
	}
	
	for (j=0; j<BOARDSIZE; j++) {
		for (i=0; i<BOARDSIZE; ) {
			switch(getc(fp)) {
			case '0':
				board[i++][j] = EMPTY;
				break;
			case '1':
				board[i++][j] = PLAYER1;
				break;
			case '2':
				board[i++][j] = PLAYER2;
				break;
			}
		}
	}
	
	fclose(fp);
	return 0;
}

void printboard(int board[BOARDSIZE][BOARDSIZE], int player, int *legalmoves) {
	int i, j;
	int tmpboard[BOARDSIZE][BOARDSIZE];
	
	for (i=0; i<BOARDSIZE; i++)
		for (j=0; j<BOARDSIZE; j++)
			tmpboard[i][j] = board[i][j];
	
	for (i=1; i<=legalmoves[0]; i++)
		tmpboard[DECODE(legalmoves[i])%BOARDSIZE][DECODE(legalmoves[i])/BOARDSIZE] = i+PLAYER2+1;
	
	printf("  ");
	for (i=0; i<BOARDSIZE; i++)
		printf("   %c  ",'A'+i);
	
	printf("\n");	
	for (i=0; i<BOARDSIZE; i++) {
		printf("  ");
		for (j=0; j<BOARDSIZE; j++)
			printf("+-----");
		printf("+\n");
		
		printf("  ");
		for (j=0; j<BOARDSIZE; j++)
			printf("|     ");
			
		printf("|\n%d ",i+1);
		
		for (j=0; j<BOARDSIZE; j++) {
			printf("| ");
			switch(tmpboard[j][i]) {
			case EMPTY:
				printf("  ");
				break;
			case PLAYER1:
				printf(" %s%c%s",COL_PLAYER1,CHR_PLAYER1,COL_EMPTY);
				break;
			case PLAYER2:
				printf(" %s%c%s",COL_PLAYER2,CHR_PLAYER2,COL_EMPTY);
				break;
			default:
				printf("%s%2d%s",COL_MOVE,tmpboard[j][i]-2,COL_EMPTY);
				break;
			}
			printf("  ");
		}
		printf("|\n");

		#if TALL
		printf("  ");
		for (j=0; j<BOARDSIZE; j++)
			printf("|     ");	
		printf("|\n");
		#endif
	}
	printf("  ");
	for (j=0; j<BOARDSIZE; j++)
		printf("+-----");
	printf("+\n");
	
	// A ton of escape sequences to get the text to appear on the right of the board
	printf("\033[s\033[%dC\033[%dA",5+6*BOARDSIZE,3*BOARDSIZE);
	
	printf("P%d (%s%c%s)'s turn\033[B\033[13D",player+1,player==PLAYER1?COL_PLAYER1:COL_PLAYER2,player==PLAYER1?CHR_PLAYER1:CHR_PLAYER2,COL_EMPTY);
	
	printf("\033[BCurrent Score:\033[13D\033[B");
	printscore(board);
	
	// List of legal moves
	printf("\033[D\033[BMoves:\033[B\033[7D");
	for (i=1; i<=legalmoves[0]; i++)
		printf("%s%3d%s. %c%d\033[B\033[7D",COL_MOVE,i,COL_EMPTY,'A'+DECODE(legalmoves[i])%BOARDSIZE,DECODE(legalmoves[i])/BOARDSIZE+1);
	if (!legalmoves[0])
		printf("  None");
	
	printf("\033[u\n");
}

void getmoves(int board[BOARDSIZE][BOARDSIZE], int player, int *legalmoves) {
	int x, y, i, j, nummoves=1;
	int illegal;
	
	for (y=0; y<BOARDSIZE; y++) {
		for (x=0; x<BOARDSIZE; x++) {
			if (board[x][y] == EMPTY) {
				illegal = 1;
				
				// Northwest
				for (i=x-1,j=y-1; i>=0 && j>=0 && board[i][j] == !player; i--,j--);
				if (i!=x-1 && i>=0 && j>=0 && board[i][j] == player) {
					legalmoves[nummoves] = (y*BOARDSIZE+x) | (1<<15);
					illegal = !illegal;
				}
				
				// North
				for (j=y-1; j>=0 && board[x][j] == !player; j--);
				if (j!=y-1 && j>=0 && board[x][j] == player) {
					if (illegal) {
						legalmoves[nummoves] = (y*BOARDSIZE+x) | (1<<14);
						illegal = !illegal;
					}
					else
						legalmoves[nummoves] |= (1<<14);
				}
				
				// Northeast
				for (i=x+1,j=y-1; i<BOARDSIZE && j>=0 && board[i][j] == !player; i++,j--);
				if (i!=x+1 && i<BOARDSIZE && j>=0 && board[i][j] == player) {
					if (illegal) {
						legalmoves[nummoves] = (y*BOARDSIZE+x) | (1<<13);
						illegal = !illegal;
					}
					else
						legalmoves[nummoves] |= (1<<13);
				}
					
				// West
				for (i=x-1; i>=0 && board[i][y] == !player; i--);
				if (i!=x-1 && i>=0 && board[i][y] == player) {
					if (illegal) {
						legalmoves[nummoves] = (y*BOARDSIZE+x) | (1<<12);
						illegal = !illegal;
					}
					else
						legalmoves[nummoves] |= (1<<12);
				}
				
				// East
				for (i=x+1; i<BOARDSIZE && board[i][y] == !player; i++);
				if (i!=x+1 && i<BOARDSIZE && board[i][y] == player) {
					if (illegal) {
						legalmoves[nummoves] = (y*BOARDSIZE+x) |(1<<11);
						illegal = !illegal;
					}
					else
						legalmoves[nummoves] |= (1<<11);
				}
				
				// Southwest
				for (i=x-1,j=y+1; i>=0 && j<BOARDSIZE && board[i][j] == !player; i--,j++);
				if (i!=x-1 && i>=0 && j<BOARDSIZE && board[i][j] == player) {
					if (illegal) {
						legalmoves[nummoves] = (y*BOARDSIZE+x) | (1<<10);
						illegal = !illegal;
					}
					else
						legalmoves[nummoves] |= (1<<10);
				}
				
				// South
				for (j=y+1; j<BOARDSIZE && board[x][j] == !player; j++);
				if (j!=y+1 && j<BOARDSIZE && board[x][j] == player) {
					if (illegal) {
						legalmoves[nummoves] = (y*BOARDSIZE+x) | (1<<9);
						illegal = !illegal;
					}
					else
						legalmoves[nummoves] |= (1<<9);
				}
				
				// Southeast
				for (i=x+1,j=y+1; i<BOARDSIZE && j<BOARDSIZE && board[i][j] == !player; i++,j++);
				if (i!=x+1 && i<BOARDSIZE && j<BOARDSIZE && board[i][j] == player) {
					if (illegal) {
						legalmoves[nummoves] = (y*BOARDSIZE+x) | (1<<8);
						illegal = !illegal;
					}
					else
						legalmoves[nummoves] |= (1<<8);
				}
				
				if (!illegal)
					nummoves++;
			}
		}
	}
	legalmoves[0] = nummoves-1;
}
