#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>

#define PLAYER1 0
#define PLAYER2 1
#define EMPTY -1

#define BOARDSIZE 8
#define INF 65536
#define MAXMOVES 20

#define CHR_PLAYER1 '-'
#define CHR_PLAYER2 '*'
#define CHR_EMPTY ' '
#define COL_PLAYER1 "\033[1;32m"
#define COL_PLAYER2 "\033[1;36m"
#define COL_EMPTY "\033[0m"
#define COL_MOVE "\033[1;33m"

#define DECODE(x) ((x)&255)
#define SCORE_P1(x) ((x)&255)
#define SCORE_P2(x) ((x)>>8)

#define TALL 0

void timeout(int sig);
int decidemove(int player, int board[BOARDSIZE][BOARDSIZE]);
int negamax(int player, int d, int dmax, int board[BOARDSIZE][BOARDSIZE], int alpha, int beta);
int evaluation(int player, int board[BOARDSIZE][BOARDSIZE]);
int getscore(int board[BOARDSIZE][BOARDSIZE]);
void printscore(int board[BOARDSIZE][BOARDSIZE]);
void results(int player, int move, int board[BOARDSIZE][BOARDSIZE]);
int terminaltest(int board[BOARDSIZE][BOARDSIZE]);
void defaultboard(int board[BOARDSIZE][BOARDSIZE]);
int loadboard(char *fname, int board[BOARDSIZE][BOARDSIZE]);
void printboard(int board[BOARDSIZE][BOARDSIZE], int player, int *legalmoves);
void getmoves(int board[BOARDSIZE][BOARDSIZE], int player, int *legalmoves);

int h_mobility(int player, int board[BOARDSIZE][BOARDSIZE]);
int h_score(int player, int board[BOARDSIZE][BOARDSIZE]);

static jmp_buf jbuf;
struct itimerval timer;
sigset_t mask;

main() {
	int board[BOARDSIZE][BOARDSIZE];
	int i, move, movenum;
	int playermode;
	int player = PLAYER1;
	int legalmoves[MAXMOVES];
	int iscomputer[2];
	char c;
	char fname[1025];
	
	srand(time(NULL));
	
	///////////////////
	// Starting menu //
	///////////////////
	printf("Welcome to Othello!\n\n");
	printf(	"Player configuration:\n"
			" 1. Human vs Human\n"
			" 2. Human vs Computer\n"
			" 3. Computer vs Human\n"
			" 4. Computer vs Computer\n\n");
	do {
		printf("Choose the configuration you would like to play (1-4): ");
		if (!scanf("%1d",&playermode))
			while(getchar()!='\n');
	}
	while(playermode < 1 || playermode > 4);
	printf("Configuration #%d will be used.\n\n",playermode);
	
	iscomputer[0] = (playermode-1)&2;
	iscomputer[1] = (playermode-1)&1;
	
	if (iscomputer[0] || iscomputer[1]) {
		do {
			printf("Specify a time limit in seconds (1-60): ");
			if (!scanf("%2d",&i))
				while(getchar()!='\n');
		}
		while (i<1 || i>60);
		printf("The time limit for each of the computer's moves will be %d seconds.\n\n",i);
		timer.it_interval.tv_sec = timer.it_interval.tv_usec =0 ;
		timer.it_value.tv_sec = i-1;
		timer.it_value.tv_usec = 900000;
		
		signal(SIGALRM,timeout);
		sigemptyset(&mask);
		sigaddset(&mask,SIGALRM);
	}
	while(getchar()!='\n');
	
	do { 
		printf("Load a game from a text file (y/n)? ");
		c = getchar(); 
		while(getchar()!='\n');
	}
	while(c!='y' && c!='n');
	
	if (c=='y') {
		do {
			printf("Specify the name of the file: ");
			scanf("%1024s",&fname); 
		}
		while (loadboard(fname,board) < 0);
	}
	else
		defaultboard(board);
	
	putchar('\n');
	
	///////////////
	// Main Game //
	///////////////
	while(1) {
		getmoves(board,player,legalmoves);
		printboard(board,player,legalmoves);
		if (legalmoves[0]) {
			if (!iscomputer[player]) {
				do {
					printf("Type the number of the move you wish to make ");
					if (legalmoves[0] == 1)
						printf("(1): ");
					else
						printf("(1-%d): ",legalmoves[0]);
						
					if (!scanf("%2d",&movenum))
						while(getchar()!='\n');
				}
				while(movenum > legalmoves[0] || movenum <= 0);
			}
			else
				movenum = decidemove(player,board);

			move = legalmoves[movenum];
			printf("-----\nMove #%d (%c%d) was made by P%d.\n\n",movenum,'A'+DECODE(move)%BOARDSIZE, DECODE(move)/BOARDSIZE+1, player+1);
			results(player, move, board);
		}
		else {
			printf("-----\nP%d had no legal moves.\n",player+1);
		}
		
		if (terminaltest(board)) {
			legalmoves[0] = 0;
			printboard(board,player,legalmoves);
			printf("\nGame finished!\n");
			printscore(board);
			exit(0);
		}
			
		player = !player;
	}
}

void timeout(int sig) {
	sigprocmask(SIG_UNBLOCK,&mask,NULL);
	signal(SIGALRM,timeout);
	//printf("Timeout!\n");
	longjmp(jbuf,1);
}

int decidemove(int player, int board[BOARDSIZE][BOARDSIZE]) {
	int depth, movenum;
	time_t starttime, endtime;
	starttime = clock();
	
	if (setitimer(ITIMER_REAL,&timer,NULL) < 0)  //start timer
		perror("setitimer"); // DEBUG
	
	for (depth=1;;depth++) {
		if (!setjmp(jbuf))
			movenum = negamax(player, depth, depth, board, -INF, INF);
		else // Timeout
			break;
	}
	
	endtime = clock();
	printf("Time ran out on depth %d (%f seconds elapsed).\n",depth-1,((double)(endtime-starttime))/CLOCKS_PER_SEC);

	//DEBUG TEMP. PLAYER1 chooses randomly.
	/*
	if (player == PLAYER1) {
		int legalmoves[MAXMOVES]; 
		
		getmoves(board,player,legalmoves);
		movenum = 1+rand()%legalmoves[0];
	}*/
	
	return movenum;
	// END DEBUG
	
}

int negamax(int player, int d, int maxd, int board[BOARDSIZE][BOARDSIZE], int alpha, int beta) {
	int legalmoves[MAXMOVES];
	int i, val, sign; 
	int best = -INF;
	int indexbest = -1;

	/*
	if (player)
		sign = 1;
	else
		sign = -1;*/
	
	if (!d || terminaltest(board)) // Note: removed "sign*" because evaluation depends on user
		return evaluation(player,board); // TO BE IMPLEMENTED: If only one legal move, take it
	
	int tmpboard[BOARDSIZE][BOARDSIZE];
	getmoves(board,player,legalmoves);
	if (!legalmoves[0]) // No legal moves
		return -1; // Handle the case of no moves
	
	for (i=1; i<=legalmoves[0]; i++) {
		memcpy(tmpboard, board, sizeof(int)*BOARDSIZE*BOARDSIZE);
		results(player,legalmoves[i],tmpboard);
		val = negamax(!player, d-1, maxd, tmpboard, -alpha, -beta);
		
		if (val > best) {
			best = val;
			indexbest = i;
		}
		else if (val == best && rand() < RAND_MAX/i)
			indexbest = i;
		if (best > alpha)
			alpha = best;
		if (alpha >= beta) { 
			if (d == maxd) // ADDED THIS. Not so sure.
				return indexbest;
			else
				return alpha;
			//return alpha;
		}
	}
	
	if (d == maxd)
		return indexbest;
	else
		return best;
}

int evaluation(int player, int board[BOARDSIZE][BOARDSIZE]) {
	int mobility, score; 
	
	mobility = h_mobility(player,board)-h_mobility(!player,board);
	score = h_score(player,board)-h_mobility(!player,board);
	
	if (player)
		return mobility*10+score;
	else // DEBUG
		return -(mobility*10+score);
}

int h_mobility(int player, int board[BOARDSIZE][BOARDSIZE]) {
	int legalmoves[MAXMOVES];
	getmoves(board,player,legalmoves);
	
	if (legalmoves[0] > 0)
		return legalmoves[0];
	else
		return -5;
}

int h_score(int player, int board[BOARDSIZE][BOARDSIZE]) {
	int score = getscore(board);
	
	if (player == PLAYER1)
		return SCORE_P1(score);
	else
		return SCORE_P2(score);
}

int getscore(int board[BOARDSIZE][BOARDSIZE]) {
	int x, y, score = 0;
	for (y=0; y<BOARDSIZE; y++) {
		for (x=0; x<BOARDSIZE; x++) {
			if (board[x][y] == PLAYER1)
				score++; 	// Least significant byte: p1's score
			else if (board[x][y] == PLAYER2)
				score += 1<<8; 	// 2nd least significant byte: p2's score
		}
	}
	return score;
}

void printscore(int board[BOARDSIZE][BOARDSIZE]) {
	int score = getscore(board);
	printf("P1 (%s%c%s): %-2d\n",COL_PLAYER1,CHR_PLAYER1,COL_EMPTY,SCORE_P1(score));
	printf("P2 (%s%c%s): %-2d\n",COL_PLAYER2,CHR_PLAYER2,COL_EMPTY,SCORE_P2(score));
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

int terminaltest(int board[BOARDSIZE][BOARDSIZE]) {
	int legalmoves[MAXMOVES];
	getmoves(board,PLAYER1,legalmoves);
	if (legalmoves[0])
		return 0;	// PLAYER1 has at least one legal move
		
	getmoves(board,PLAYER2,legalmoves);
	if (legalmoves[0])
		return 0;	// PLAYER2 has at least one legal move
	else
		return 1;	// Neither player has a legal move.
}
/*
int decidemove(int *legalmoves) {
	int movenum = 1+rand()%legalmoves[0];
	int move = legalmoves[movenum];
	printf("-----\nMove #%d (%c%d) was made by computer.\n\n",movenum,'A'+DECODE(move)%BOARDSIZE, DECODE(move)/BOARDSIZE+1);
	return move;
}*/

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
	
	if ((fp = fopen(fname,"r")) == NULL) {
		fprintf(stderr,"Error: Could not open file \"%s\" for reading: %s\n",fname,strerror(errno));
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
			if (feof(fp)) {
				fprintf(stderr,"Error: File \"%s\" is formatted incorrectly.\n",fname);
				return -1;
			}
		}
	}
	
	fclose(fp);
	return 0;
}

void printboard(int board[BOARDSIZE][BOARDSIZE], int player, int *legalmoves) {
	int i, j;
	int tmpboard[BOARDSIZE][BOARDSIZE];
			
	memcpy(tmpboard, board, sizeof(int)*BOARDSIZE*BOARDSIZE);
	
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
	/*printf("\033[s\033[%dC\033[%dA",5+6*BOARDSIZE,3*BOARDSIZE);
	
	printf("P%d (%s%c%s)'s turn\033[B\033[13D",player+1,player==PLAYER1?COL_PLAYER1:COL_PLAYER2,player==PLAYER1?CHR_PLAYER1:CHR_PLAYER2,COL_EMPTY);
	
	printf("\033[BCurrent Score:\033[13D\033[B");
	printscore(board);
	
	// List of legal moves
	printf("\033[D\033[BMoves:\033[B\033[7D");
	for (i=1; i<=legalmoves[0]; i++)
		printf("%s%3d%s. %c%d\033[B\033[7D",COL_MOVE,i,COL_EMPTY,'A'+DECODE(legalmoves[i])%BOARDSIZE,DECODE(legalmoves[i])/BOARDSIZE+1);
	if (!legalmoves[0])
		printf("  None");
	
	printf("\033[u\n"); */
	printf("P%d (%s%c%s)'s turn\n",player+1,player==PLAYER1?COL_PLAYER1:COL_PLAYER2,player==PLAYER1?CHR_PLAYER1:CHR_PLAYER2,COL_EMPTY);
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
