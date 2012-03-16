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

void getmoves(char board[BOARDSIZE][BOARDSIZE], char player, char *legalmoves);
void defaultboard(char board[BOARDSIZE][BOARDSIZE]);
void printboard(char board[BOARDSIZE][BOARDSIZE], char player);

main() {	
	char board[BOARDSIZE][BOARDSIZE];
	//defaultboard(board);
	loadboard("infile.txt",board);
	printboard(board,PLAYER2);
}

void defaultboard(char board[BOARDSIZE][BOARDSIZE]) {
	int x, y;
	for (x=0; x<BOARDSIZE; x++)
		for (y=0; y<BOARDSIZE; y++)
			board[x][y] = EMPTY;
	board[4][3] = board[3][4] = PLAYER1;
	board[3][3] = board[4][4] = PLAYER2;
}

int loadboard(char *fname, char board[BOARDSIZE][BOARDSIZE]) {
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

void printboard(char board[BOARDSIZE][BOARDSIZE], char player) {
	int i, j;
	char legalmoves[20];
	char tmpboard[BOARDSIZE][BOARDSIZE];
	getmoves(board,player,legalmoves);
	
	for (i=0; i<BOARDSIZE; i++)
		for (j=0; j<BOARDSIZE; j++)
			tmpboard[i][j] = board[i][j];
	
	for (i=1; i<legalmoves[0]; i++)
		tmpboard[legalmoves[i]%BOARDSIZE][legalmoves[i]/BOARDSIZE] = i+PLAYER2+1;
	
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
	}
	printf("  ");
	for (j=0; j<BOARDSIZE; j++)
		printf("+-----");
	printf("+\n");
	
	// A ton of escape sequences to get the text to appear on the right of the board
	printf("\033[s\033[%dC\033[%dA",5+6*BOARDSIZE,3*BOARDSIZE);
	printf("P%d (%s%c%s)'s turn\033[2B\033[13D",player+1,player==PLAYER1?COL_PLAYER1:COL_PLAYER2,player==PLAYER1?CHR_PLAYER1:CHR_PLAYER2,COL_EMPTY);
	printf("Moves:\033[B\033[7D");
	for (i=1; i<legalmoves[0]; i++)
		printf("%s%2d%s. %c%d\033[B\033[6D",COL_MOVE,i,COL_EMPTY,'A'+legalmoves[i]%BOARDSIZE,legalmoves[i]/BOARDSIZE+1);
	if (!legalmoves[0])
		printf("None");
	printf("\033[u");
}

void getmoves(char board[BOARDSIZE][BOARDSIZE], char player, char *legalmoves) {
	int x, y, i, j, nummoves=1;
	
	for (y=0; y<BOARDSIZE; y++) {
		for (x=0; x<BOARDSIZE; x++) {
			if (board[x][y] == EMPTY) {
				// Northwest
				for (i=x-1,j=y-1; i>=0 && j>=0 && board[i][j] == !player; i--,j--);
				if (i!=x-1 && i>=0 && j>=0 && board[i][j] == player) {
					legalmoves[nummoves++] = y*BOARDSIZE+x;
					break;
				}
				
				// North
				for (j=y-1; j>=0 && board[x][j] == !player; j--);
				if (j!=y-1 && j>=0 && board[x][j] == player) {
					legalmoves[nummoves++] = y*BOARDSIZE+x;
					break;
				}
				
				// Northeast
				for (i=x+1,j=y-1; i<BOARDSIZE && j>=0 && board[i][j] == !player; i++,j--);
				if (i!=x+1 && i<BOARDSIZE && j>=0 && board[i][j] == player) {
					legalmoves[nummoves++] = y*BOARDSIZE+x;
					break;
				}
					
				// West
				for (i=x-1; i>=0 && board[i][y] == !player; i--);
				if (i!=x-1 && i>=0 && board[i][y] == player) {
					legalmoves[nummoves++] = y*BOARDSIZE+x;
					break;
				}
				
				// East
				for (i=x+1; i<BOARDSIZE && board[i][y] == !player; i++);
				if (i!=x+1 && i<BOARDSIZE && board[i][y] == player) {
					legalmoves[nummoves++] = y*BOARDSIZE+x;
					break;
				}
				
				// Southwest
				for (i=x-1,j=y+1; i>=0 && j<BOARDSIZE && board[i][j] == !player; i--,j++);
				if (i!=x-1 && i>=0 && j<BOARDSIZE && board[i][j] == player) {
					legalmoves[nummoves++] = y*BOARDSIZE+x;
					break;
				}
				
				// South
				for (j=y+1; j<BOARDSIZE && board[x][j] == !player; j++);
				if (j!=y+1 && j<BOARDSIZE && board[x][j] == player) {
					legalmoves[nummoves++] = y*BOARDSIZE+x;
					break;
				}
				
				// Southeast
				for (i=x+1,j=y+1; i<BOARDSIZE && j<BOARDSIZE && board[i][j] == !player; i++,j++);
				if (i!=x+1 && i<BOARDSIZE && j<BOARDSIZE && board[i][j] == player) {
					legalmoves[nummoves++] = y*BOARDSIZE+x;
					break;
				}
			}
		}
	}
	legalmoves[0] = nummoves;
}
