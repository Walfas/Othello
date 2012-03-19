#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <sys/time.h>

#define INVALID -2
#define EMPTY -1
#define PLAYER1 0
#define PLAYER2 1

#define BOARDSIZE 8
#define INF 65536
#define MAXMOVES 32
#define NUMTILES 64
#define ARRSIZE 94

#define CHR_PLAYER1 '-'
#define CHR_PLAYER2 '*'
#define CHR_EMPTY ' '
#define COL_PLAYER1 "\033[1;32m"
#define COL_PLAYER2 "\033[1;36m"
#define COL_EMPTY "\033[0m"
#define COL_MOVE "\033[1;33m"

#define TURN(x) (x[91])
#define P1PIECES(x) (x[92])
#define ACTIVEPIECES(x) (x[93])
#define COORD(x,y) (10+x+y*9)

#define TALL 0

time_t starttime, endtime;
double timelimit;

// Remove later
static jmp_buf jbuf;
struct itimerval timer;
sigset_t mask;

/* Parts of this Othello implementation were based on 
	Gunnar Andersson's endgame solver on his
	"Writing an Othello program" webpage.

   Elements 0-90 of the board array represent the board 
	as follows:

	ppppppppp
	p........
	p........	p is a placeholder position
	p........	. can be either P1, P2, or blank
	p........
	p........
	p........
	p........    
	p........
	pppppppppp 
   
   The array also stores the following info about the game:
	board[91] = Whose turn is it? (PLAYER1 or PLAYER2)
	board[92] = # of PLAYER1 pieces
	board[93] = total pieces	*/

/* dirs[] represents the relative positions adjacent to a 
	given index on board[]. For example, the position directly 
	northwest of board[20] is board[20-10] = board[10].
	Directly north of board[20] is board[20-9] = board[11],
	Directly northeast of board[20] is board[20-8] = board[11],
	etc. 	*/
const int dirs[] = {
	-10, -9, -8,
	 -1,      1,
	  8,  9, 10
};

/* flipdir[] is an array of bitmasked values corresponding
	to indices 0-90 of the board array. The values represent 
	which positions could be flipped if a piece were to be 
	placed there, based on the dirs[] array. 
	
   For example, flipdir[10], representing position A1 on the 
	board, has value 208, which is (1<<4)+(1<<6)+(1<<7).
	dirs[4] corresponds to east, dirs[6] south, and dirs[7]
	southeast. This means a piece placed at A1 can only flip
	pieces in those directions, and there is no need to check 
	any other directions in the getmoves() and results() 
	functions. 	*/
const int flipdir[91] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,208,208,248,248,248,248,104,104,
	0,208,208,248,248,248,248,104,104,
	0,214,214,255,255,255,255,107,107,
	0,214,214,255,255,255,255,107,107,
	0,214,214,255,255,255,255,107,107,
	0,214,214,255,255,255,255,107,107,
	0, 22, 22, 31, 31, 31, 31, 11, 11,
	0, 22, 22, 31, 31, 31, 31, 11, 11,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

/* positions[] is an array of indices, ordered from best to 
	worst. Evaluating moves in this order may result in better 
	move ordering for alpha-beta pruning. 	*/
const int positions[64] = {
	40 , 41 , 49 , 50 , // Center squares
	10 , 17 , 73 , 80 , // Corners
	12 , 15 , 28 , 35 , 55 , 62 , 75 , 78 , // Edge (C1)
	30 , 33 , 57 , 60 , // C3
	13 , 14 , 37 , 44 , 46 , 53 , 76 , 77 , // Edge (D1)
	31 , 32 , 39 , 42 , 48 , 51 , 58 , 59 , // D3
	22 , 23 , 38 , 43 , 47 , 52 , 67 , 68 , // D2
	21 , 24 , 29 , 34 , 56 , 61 , 66 , 69 , // C2
	11 , 16 , 19 , 26 , 64 , 71 , 74 , 79 , // C-squares
	20 , 25 , 65 , 70 , // X-squares
};

void timeout(int sig);
int decidemove(int *board);
int negamax(int d, int maxd, int *board, int alpha, int beta);
int terminaltest(int *board);
void results(int *board, int move);
int evaluation(int *board);
int h_score(int *board);
int h_mobility(int *board);
void getmoves(int *board, int *legalmoves);
void printboard(int *board, int *legalmoves);
void emptyboard(int *board);
void defaultboard(int *board);
int loadboard(char *fname, int *board);
void printscore(int *board);

/** Use alarm interrupts and longjmps to return from negamax */
void timeout(int sig) {
	sigprocmask(SIG_UNBLOCK,&mask,NULL);
	signal(SIGALRM,timeout);
	longjmp(jbuf,1);
}

main() {
	int board[ARRSIZE];
	int i, move, movenum;
	int playermode;
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
		
		timelimit = ((double) i) - 0.01;
		
		/*
		timer.it_interval.tv_sec = timer.it_interval.tv_usec =0 ;
		timer.it_value.tv_sec = i-1;
		timer.it_value.tv_usec = 955000;
		
		signal(SIGALRM,timeout);
		sigemptyset(&mask);
		sigaddset(&mask,SIGALRM); */
		
		//sigprocmask(SIG_BLOCK,&mask,NULL); // DEBUG?
		/*act.sa_handler = timeout;
		act.sa_flags = 0;
		sigemptyset(&act.sa_mask);
		sigaction(SIGALRM, &act, NULL);
		
		sigprocmask(SIG_BLOCK,&mask,NULL);*/
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
		
		do {
			printf("Whose turn is it (1 or 2)? ");
			if (!scanf("%1d",&i))
				while(getchar()!='\n');
		}
		while(i < 1 || i > 2);
		
		TURN(board) = i-1;
	}
	else
		defaultboard(board);
	
	putchar('\n');
	
	///////////////
	// Main Game //
	///////////////
	while(1) {
		getmoves(board,legalmoves);
		printboard(board,legalmoves);
		if (legalmoves[1] != -1) { // At least one legal move exists
			if (!iscomputer[TURN(board)]) {
				do {
					printf("Type the number of the move you wish to make ");
					if (legalmoves[0] == 1)
						printf("(1): ");
					else
						printf("(1-%d): ",legalmoves[0]);
						
					if (!scanf("%d",&movenum))
						while(getchar()!='\n');
				}
				while(movenum > legalmoves[0] || movenum <= 0);
			}
			else // If player is computer-controlled
				movenum = decidemove(board);

			move = legalmoves[movenum];
			printf("-----\nMove #%d (%c%d) was made by P%d.\n\n",movenum,'A'+(move-10)%9, (move-10)/9+1, TURN(board)+1);
			results(board,move);
		}
		else {
			printf("-----\nP%d had no legal moves.\n",TURN(board)+1);
			TURN(board) = !TURN(board);
		}
		
		if (terminaltest(board)) {
			legalmoves[0] = 0;
			printboard(board,legalmoves);
			printf("\nGame finished! ");
			if (P1PIECES(board) > ACTIVEPIECES(board)-P1PIECES(board))
				printf("P1 (%s%c%s) wins!",COL_PLAYER1,CHR_PLAYER1,COL_EMPTY);
			else if (P1PIECES(board) < ACTIVEPIECES(board)-P1PIECES(board))
				printf("P2 (%s%c%s) wins!",COL_PLAYER2,CHR_PLAYER2,COL_EMPTY);
			else 
				printf("Tied game!");
			exit(0);
		}
	}
}

int decidemove(int *board) {
	int player = TURN(board);
	int depth, movenum, tmp;
	
	if (!player) { // Player 1 chooses randomly
		int legalmoves[MAXMOVES];
		getmoves(board,legalmoves);
		movenum = rand()%legalmoves[0]+1;
		return movenum;
	}
	
	starttime = clock();
	
	for (depth=1;;depth++) {
		tmp = negamax(depth, depth, board, -INF, INF);
		
		if (tmp>0)
			movenum = tmp;
		else // Out of time, use movenum from previous iteration
			break;
	}
	
	endtime = clock();
	printf("Time ran out on depth %d (%f seconds elapsed).\n",depth-1,((double)(endtime-starttime))/CLOCKS_PER_SEC);
	
	return movenum;
}

int negamax(int d, int maxd, int *board, int alpha, int beta) {
	int legalmoves[MAXMOVES];
	int player = TURN(board);
	int indexbest = 1;
	int i, val, best;

	if (((double)(clock()-starttime))/CLOCKS_PER_SEC > timelimit)
		return -1;
	
	if (!d || terminaltest(board)) 
		return (player?-1:1)*evaluation(board);
	
	getmoves(board,legalmoves);
	if (d == maxd && legalmoves[0] == 1)
		return 1;	// If only one legal move, choose that move.
	
	best = -INF;
	int tmpboard[ARRSIZE];
	for (i=1; i<=legalmoves[0]; i++) {
		memcpy(tmpboard, board, sizeof tmpboard);
		results(tmpboard, legalmoves[i]);
		val = -negamax(d-1,d,tmpboard,-alpha,-beta);
		
		if (val>best) {
			best = val;
			indexbest = i;
		}
		else if (val == best && rand() <= RAND_MAX/i)
			indexbest = i;
			
		if (best>=beta) {
			if (d == maxd)
				return indexbest;
			else
				return best+1;
		}
			
		if (best>alpha)
			alpha = best;
	}
	
	if (d == maxd)
		return indexbest;
	else
		return best;
}

/*
int negamax(int d, int maxd, int *board, int alpha, int beta) {
	int legalmoves[MAXMOVES];
	int player = TURN(board);
	int indexbest = 1;
	int i, val;

	if (((double)(clock()-starttime))/CLOCKS_PER_SEC > timelimit)
		return -1;
	
	if (!d || terminaltest(board)) 
		return (player?-1:1)*evaluation(board);
	
	getmoves(board,legalmoves);
	if (d == maxd && legalmoves[0] == 1)
		return 1;	// If only one legal move, choose that move.
	
	int tmpboard[ARRSIZE];
	for (i=1; i<=legalmoves[0]; i++) {
		memcpy(tmpboard, board, sizeof tmpboard);
		
		results(tmpboard, legalmoves[i]);
		val = -negamax(d-1, maxd, tmpboard, -alpha, -beta);
		
		if (val > alpha) {
			alpha = val;
			indexbest = i;
		}
		else if (val == alpha && rand() <= RAND_MAX/i) // Tied moves
			indexbest = i;
			
		if (alpha >= beta) { 
			if (d == maxd) // ADDED THIS. Not so sure.
				return indexbest;
			else
				return alpha;
		}
	}
	
	if (d == maxd)
		return indexbest;
	else
		return alpha;
}*/

int terminaltest(int *board) {
	int legalmoves[MAXMOVES];
	getmoves(board,legalmoves);
	if (legalmoves[1] != -1)
		return 0;	// Current player has at least one legal move
	
	TURN(board) = !TURN(board);
	getmoves(board,legalmoves);
	TURN(board) = !TURN(board);
	if (legalmoves[1] != -1)
		return 0;	// Other player has at least one legal move
	else
		return 1;	// Neither player has a legal move.
}

void results(int *board, int move) {
	int player = TURN(board);
	int j, n, count, pos;
	if (move == -1) { // No legal moves
		TURN(board) = !player;
		return;
	}
	
	count = 0;
	board[move] = player;
	ACTIVEPIECES(board)++;
	for (j=0; j<8; j++) { // Check each direction
		if (flipdir[pos = move] & (1<<j)) {
			for (n=0, pos+=dirs[j]; board[pos]==!player; n++, pos+=dirs[j]); 
			if (n>0 && board[pos]==player) {
				count += n;
				for (pos-=dirs[j]; board[pos]==!player; pos-=dirs[j])
					board[pos] = player;
			}
		}
	}
	
	if (player == PLAYER1)
		P1PIECES(board) += count+1;
	else
		P1PIECES(board) -= count;
	
	TURN(board) = !player;
	return;
}

int evaluation(int *board) {
	int i, sign=1, val=0; 
	
	for (i=0; i<2; i++,sign=-1) {
		if (ACTIVEPIECES(board) < 48) {
			//val += sign*h_score(board);
			val += sign*5*h_mobility(board);
			val += sign*10*h_corners(board);
		}
		else {
			val += sign*h_mobility(board);
			val += sign*5*h_corners(board);
			val += sign*30*h_score(board);
		}
		TURN(board) = !TURN(board);
	}
	
	return val;
}

int h_score(int *board) {
	if (TURN(board)) // Player 2
		return ACTIVEPIECES(board)-P1PIECES(board);
	else	// Player 1
		return P1PIECES(board);
}

int h_mobility(int *board) {
	int legalmoves[MAXMOVES];
	
	getmoves(board,legalmoves);
	if (legalmoves[1] != -1)
		return legalmoves[0];
	else // Avoid having no legal moves
		return -25;
}

int h_corners(int *board) {
	int i, player = TURN(board), val=0;
	
	for (i=0;i<4;i++) {
		if (board[positions[4+i]] == player)
			val+=15; // Corners
		else if (board[positions[4+i]] == EMPTY)
			if (board[positions[63-i]] == player) // X-squares
				val -= 10;
			if (board[positions[59-i]] == player) // C-squares
				val -= 5;
			if (board[positions[59-2*i]] == player)
				val -= 5;
		}
	}
	
	return val;
}

void getmoves(int *board, int *legalmoves) {
	int i, j, pos, count, legal, nummoves=1;
	int player = TURN(board);
	
	for (i=0; i<NUMTILES; i++) {
		if (board[positions[i]] != EMPTY)
			continue;
		
		for (j=0; j<8; j++) { // Check each direction
			pos = positions[i];
			if (flipdir[pos] & (1<<j)) {
				for (count=0, pos+=dirs[j]; board[pos]==!player; count++, pos+=dirs[j]);
				if (count!=0 && board[pos]==player) {
					legalmoves[nummoves++] = positions[i];
					break;
				}
			}
		}
	}
	legalmoves[0] = nummoves-1;
	
	if (!legalmoves[0]) {
		legalmoves[0] = 1;
		legalmoves[1] = -1;
	}	
}

void printboard(int *board, int *legalmoves) {
	int i, j, player = TURN(board);
	int tmpboard[ARRSIZE];
	
	memcpy(tmpboard, board, sizeof tmpboard);
	if (legalmoves[1] != -1)
		for (i=1; i<=legalmoves[0]; i++)
			tmpboard[legalmoves[i]] = i+2;
			
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
			switch(tmpboard[COORD(j,i)]) {
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
				printf("%s%2d%s",COL_MOVE,tmpboard[COORD(j,i)]-2,COL_EMPTY);
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
	if (!terminaltest(board))
		printf("P%d (%s%c%s)'s turn. ",player+1,player==PLAYER1?COL_PLAYER1:COL_PLAYER2,player==PLAYER1?CHR_PLAYER1:CHR_PLAYER2,COL_EMPTY);
	printscore(board);
}

void emptyboard(int *board) {
	int x, y;
	
	for (x=0; x<ARRSIZE; x++)
		board[x] = INVALID;
	
	for (x=0; x<BOARDSIZE; x++)
		for (y=0; y<BOARDSIZE; y++)
			board[COORD(x,y)] = EMPTY;
			
	TURN(board) = PLAYER1;
	P1PIECES(board) = 0;
	ACTIVEPIECES(board) = 0;
}

void defaultboard(int *board) {
	emptyboard(board);
	
	board[COORD(3,4)] = board[COORD(4,3)] = PLAYER1;
	board[COORD(3,3)] = board[COORD(4,4)] = PLAYER2;
	
	P1PIECES(board) = 2;
	ACTIVEPIECES(board) = 4;
	
	/*
	for (x=0; x<ARRSIZE; x++) {
		if ((x+1)%9==1) {
			putchar('\n');
		}
		printf("%2d",board[x]);
	}*/
}

int loadboard(char *fname, int *board) {
	int i, j;
	FILE *fp;
	
	if ((fp = fopen(fname,"r")) == NULL) {
		fprintf(stderr,"Error: Could not open file \"%s\" for reading: %s\n",fname,strerror(errno));
		return -1;
	}
	
	emptyboard(board);
	
	for (j=0; j<BOARDSIZE; j++) {
		for (i=0; i<BOARDSIZE; ) {
			switch(getc(fp)) {
			case '0':
				board[COORD(i++,j)] = EMPTY;
				break;
			case '1':
				board[COORD(i++,j)] = PLAYER1;
				P1PIECES(board)++;
				ACTIVEPIECES(board)++;
				break;
			case '2':
				board[COORD(i++,j)] = PLAYER2;
				ACTIVEPIECES(board)++;
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

void printscore(int *board) {
	printf("Score: [ ");
	printf("P1 (%s%c%s): %d / ",COL_PLAYER1,CHR_PLAYER1,COL_EMPTY,P1PIECES(board));
	printf("P2 (%s%c%s): %d ]\n",COL_PLAYER2,CHR_PLAYER2,COL_EMPTY,ACTIVEPIECES(board)-P1PIECES(board));
}
