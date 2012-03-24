#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <sys/time.h>

#define INVALID -2
#define EMPTY 0
#define PLAYER1 1
#define PLAYER2 -1

#define BOARDSIZE 8
#define INF 2097152
#define END 1048576
#define MAXMOVES 32
#define NUMTILES 64
#define ARRSIZE 95
#define MAXSTRLEN 1024

#define CHR_PLAYER1 '*'
#define CHR_PLAYER2 '-'
#define CHR_EMPTY ' '
#define COL_PLAYER1 "\033[1;36m"
#define COL_PLAYER2 "\033[1;32m"
#define COL_MOVE "\033[1;33m"
#define COL_EMPTY "\033[0m"

#define GETPLAYER(x) (((x)==PLAYER1)?1:2)
#define TURN(x) (x[91])
#define P1PIECES(x) (x[92])
#define ACTIVEPIECES(x) (x[93])
#define JUSTPLAYED(x) (x[94])
#define COORD(x,y) (10+(x)+(y)*9)

#define TALL 0

struct timeval starttimeval, endtimeval;
double starttime, endtime, timelimit;

enum {
	A1 = 10, B1, C1, D1, E1, F1, G1, H1,
	A2 = 19, B2, C2, D2, E2, F2, G2, H2,
	A3 = 28, B3, C3, D3, E3, F3, G3, H3,
	A4 = 37, B4, C4, D4, E4, F4, G4, H4,
	A5 = 46, B5, C5, D5, E5, F5, G5, H5,
	A6 = 55, B6, C6, D6, E6, F6, G6, H6,
	A7 = 64, B7, C7, D7, E7, F7, G7, H7,
	A8 = 73, B8, C8, D8, E8, F8, G8, H8
};

/* Parts of this Othello implementation were based on 
	Gunnar Andersson's endgame solver on his
	"Writing an Othello program" webpage.

   Elements 0-90 of the board array represent the board 
	as follows:

	p p p p p p p p p 
	p . . . . . . . .
	p . . . . . . . .	p is a placeholder position (INVALID)
	p . . . . . . . .	. is either PLAYER1, PLAYER2, or EMPTY
	p . . . . . . . .
	p . . . . . . . .
	p . . . . . . . .
	p . . . . . . . .
	p . . . . . . . .
	p p p p p p p p p p
   
   The array also stores the following info about the game:
	board[91] = Whose turn is it? (PLAYER1 or PLAYER2)
	board[92] = Number of PLAYER1 pieces
	board[93] = Total pieces
	board[94] = The position of the piece that was just played
*/

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
	
/*   	A  B  C  D  E  F  G  H
	1	10 11 12 13 14 15 16 17
	2	19 20 21 22 23 24 25 26
	3	28 29 30 31 32 33 34 35
	4	37 38 39 40 41 42 43 44
	5	46 47 48 49 50 51 52 53
	6	55 56 57 58 59 60 61 62
	7	64 65 66 67 68 69 70 71
	8	73 74 75 76 77 78 79 80
*/
	
const int positions[64] = {
	A1 , H1 , A8 , H8 , // Corners
	D4 , E4 , D5 , E5 , // Center squares
	C1 , F1 , A3 , H3 , A6 , H6 , C8 , F8 , // Edge (C1)
	C3 , F3 , C6 , F6 , // C3
	D1 , E1 , A4 , H4 , A5 , H5 , D8 , E8 , // Edge (D1)
	D3 , E3 , C4 , F4 , C5 , F5 , D6 , E6 , // D3
	D2 , E2 , B4 , G4 , B5 , G5 , D7 , E7 , // D2
	C2 , F2 , B3 , G3 , B6 , G6 , C7 , F7 , // C2
	B1 , G1 , A2 , H2 , A7 , H7 , B8 , G8 , // C-squares
	B2 , G2 , B7 , G7 , // X-squares
};

#define XV 24
#define CV 6
const int disksquare[91] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,500,-CV,  8,  6,  6,  8,-CV,500,
	0,-CV,-XV, -4, -3, -3, -4,-XV,-CV,
	0,  8, -4,  7,  2,  2,  7, -4,  8,
	0,  6, -3,  2,  5,  5,  2, -3,  6,
	0,  6, -3,  2,  5,  5,  2, -3,  6,
	0,  8, -4,  7,  2,  2,  7, -4,  8,
	0,-CV,-XV, -4, -3, -3, -4,-XV,-CV,
	0,500,-CV,  8,  6,  6,  8,-CV,500,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

int decidemove(int *board);
int negamax(int d, int maxd, int *board, int alpha, int beta);
int terminaltest(int *board);
void results(int *board, int move);
int evaluation(int *board);
int h_discdiff(int *board);
int h_mobility(int *board);
int h_pmobility(int *board);
int h_disksquare(int *board);
int h_stability(int *board);
int h_straightlines(int *board);
void getmoves(int *board, int *legalmoves);
void printboard(int *board, int *legalmoves);
void emptyboard(int *board);
void defaultboard(int *board);
int loadboard(char *fname, int *board);
void printscore(int *board);

main() {
	int board[ARRSIZE];
	int i, move, movenum;
	int playermode;
	int legalmoves[MAXMOVES];
	int iscomputer[2];
	char c, fname[1025], str[1025];
	char *endptr;
	
	
	srand(time(NULL));
	
	///////////////////
	// Starting menu //
	///////////////////
	startmenu:
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
			if (!scanf("%d",&i))
				while(getchar()!='\n');
		}
		while (i<1 || i>60);
		printf("The time limit for each of the computer's moves will be %d seconds.\n\n",i);
		
		timelimit = ((double) i) - 0.01;
	}
	while(getchar()!='\n');
	
	do { 
		printf("Load a game from a text file (y/n)? ");
		c = getchar(); 
		while(getchar()!='\n');
	}
	while(c!='y' && c!='n');
	
	if (c=='y') { // Load board from file
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
		
		TURN(board) = (i==1)?PLAYER1:PLAYER2;
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
			if (!iscomputer[ GETPLAYER(TURN(board))-1 ])
				movenum = getplayermove(legalmoves);
			else	// If player is computer-controlled
				movenum = decidemove(board);
			move = legalmoves[movenum];

			printf("-----\nMove #%d (%c%d) was made by P%d.\n\n",movenum,'A'+(move-A1)%9, (move-A1)/9+1, GETPLAYER(TURN(board)));
			printf("That's (%d,%d)\n",(move-A1)/9,(move-A1)%9);//DEBUG
			results(board,move);
		}
		else {
			printf("-----\nP%d had no legal moves.\n",GETPLAYER(TURN(board)));
			TURN(board) = -TURN(board);
		}
		
		if (terminaltest(board)) {
			legalmoves[0] = 0;
			printboard(board,legalmoves);
			printf("\nGame finished! ");
			if (P1PIECES(board) > ACTIVEPIECES(board)-P1PIECES(board))
				printf("P1 (%s%c%s) wins!\n",COL_PLAYER1,CHR_PLAYER1,COL_EMPTY);
			else if (P1PIECES(board) < ACTIVEPIECES(board)-P1PIECES(board))
				printf("P2 (%s%c%s) wins!\n",COL_PLAYER2,CHR_PLAYER2,COL_EMPTY);
			else 
				printf("Tied game!\n");
			
			printf("-----\nReturning to the title screen.\n-----\n");
			goto startmenu;
		}
	}
}

int getplayermove(int *legalmoves) {
	int i, move, movenum;
	char str[1025], *endptr;
	do {
		printf("Type the number of the move you wish to make ");
		if (legalmoves[0] == 1)
			printf("(1): ");
		else
			printf("(1-%d): ",legalmoves[0]);
		
		scanf("%1024s",&str);
		movenum = strtol(str,&endptr,10);
		
		/* If strtol() can't convert to a number, check if the 
			move is expressed in the form A1, A2, A3, etc.  */
		if (!movenum) {
			if (tolower(str[0]) >= 'a' && tolower(str[0]) <= 'h' 
					&& str[1] >= '1' && str[1] <= '8') 
			{
				move = COORD(tolower(str[0])-'a',str[1]-'1');
				
				// Check that the move is a valid one.
				for (i=1; i<=legalmoves[0]; i++) {
					if (legalmoves[i] == move)
						return i;
				}
			}
		}
		else if (*endptr == '\0')
			return movenum;
		else // Invalid input
			movenum = 0;
	}
	while(movenum > legalmoves[0] || movenum <= 0);
}

int decidemove(int *board) {
	int player = TURN(board);
	int depth, movenum, tmp;
	int legalmoves[MAXMOVES];
	
	/*
	if (player==PLAYER2) { // Player 2 chooses randomly
		getmoves(board,legalmoves);
		movenum = rand()%legalmoves[0]+1;
		return movenum;
	}*/
	
	//starttime = clock();
	
	gettimeofday(&starttimeval,NULL);
	starttime = starttimeval.tv_sec+(starttimeval.tv_usec/1000000.0);
	
	getmoves(board,legalmoves);
	if (legalmoves[0] == 1) {
		printf("Not evaluating the game tree: only one legal move.\n");
		return 1;
	}
	
	for (depth=1;;depth++) {
		tmp = negamax(depth, depth, board, -INF, INF);
		
		if (tmp>0)
			movenum = tmp;
		else if (tmp==0) { // Out of time, use movenum from previous iteration
			printf("Completed search to depth %d.\nTime ran out ",depth-1);
			break;
		}
		else { // Reached end of game tree in search
			printf("Reached end of game tree ");
			movenum = -tmp;
			break;
		}
	}
	
	//endtime = clock();
	gettimeofday(&endtimeval,NULL);
	endtime = endtimeval.tv_sec+(endtimeval.tv_usec/1000000.0);
	
	printf("at depth %d (%.4f seconds elapsed).\n",depth,endtime-starttime);
	
	return movenum;
}

int negamax(int d, int maxd, int *board, int alpha, int beta) {
	int legalmoves[MAXMOVES];
	int indexbest = 1;
	int i, val, best, n;

	gettimeofday(&endtimeval,NULL);
	endtime = endtimeval.tv_sec+(endtimeval.tv_usec/1000000.0);
	
	if (endtime-starttime > timelimit)
		return 0;
		
	if (d != maxd && terminaltest(board)) {
		if ((val = TURN(board)*evaluation(board)) > 0)
			return END+val;
		else if (val < 0)
			return val-END;
		else
			return 0;
	}
	if (!d) // Reached depth cutoff
		return TURN(board)*evaluation(board);
	
	best = -INF;
	getmoves(board,legalmoves);
	n = legalmoves[0];
	int tmpboard[ARRSIZE];
	for (i=1; i<=legalmoves[0]; i++) {
		memcpy(tmpboard, board, sizeof tmpboard);
		results(tmpboard, legalmoves[i]);
		
		//printf("Calling negamax at depth %d, move %d (%d) - on P%d's turn\n",maxd-d+1,i,legalmoves[i],GETPLAYER(TURN(board)));
		val = -negamax(d-1,maxd,tmpboard,-alpha,-beta);
		//printf("Returned from negamax at depth %d, move %d (%d) gives val %d - on P%d's turn\n",maxd-d+1,i,legalmoves[i],val,GETPLAYER(TURN(board)));
		
		if (val <= -END || val >= END)
			n--;
		
		if (val>best) {
			best = val;
			indexbest = i;
		}
		else if (val == best && rand() <= RAND_MAX/i)
			indexbest = i;
			
		if (best>beta) {
			if (d == maxd)
				break;
			else
				return best+1;
		}
			
		if (best>alpha)
			alpha = best;
			
		//printf("Current best: move %d with val %d\n",indexbest,best);
	}
	
	if (d == maxd) {
		if (n<=0)
			return -indexbest;
		else
			return indexbest;
	}
	else
		return best;
}

int terminaltest(int *board) {
	int legalmoves[MAXMOVES];
	getmoves(board,legalmoves);
	if (legalmoves[1] != -1)
		return 0;	// Current player has at least one legal move
	
	TURN(board) = -TURN(board);
	getmoves(board,legalmoves);
	TURN(board) = -TURN(board);
	if (legalmoves[1] != -1)
		return 0;	// Other player has at least one legal move
	else
		return 1;	// Neither player has a legal move.
}

void results(int *board, int move) {
	int player = TURN(board);
	int j, n, count, pos;
	if (move == -1) { // No legal moves
		TURN(board) = -player;
		return;
	}
	
	count = 0;
	board[move] = player;
	ACTIVEPIECES(board)++;
	for (j=0; j<8; j++) { // Check each direction
		if (flipdir[pos = move] & (1<<j)) {
			for (n=0, pos+=dirs[j]; board[pos]==-player; n++, pos+=dirs[j]); 
			if (n>0 && board[pos]==player) {
				count += n;
				for (pos-=dirs[j]; board[pos]==-player; pos-=dirs[j])
					board[pos] = player;
			}
		}
	}
	
	if (player == PLAYER1)
		P1PIECES(board) += count+1;
	else
		P1PIECES(board) -= count;
		
	JUSTPLAYED(board) = pos;
	TURN(board) = -player;
	return;
}

#define STAGE1 14
#define STAGE2 36
#define STAGE3 54
int evaluation(int *board) {
	int val=0; 
	
	if (ACTIVEPIECES(board) < STAGE1) {
		val -= h_discdiff(board);
		val += 10*h_mobility(board);
		val += 3*h_pmobility(board);
		val += 2*h_disksquare(board);
		val += 15*h_stability(board);
	}
	else if (ACTIVEPIECES(board) < STAGE2) {
		val -= h_discdiff(board);
		val += 8*h_mobility(board);
		val += 2*h_pmobility(board);
		val += 15*h_stability(board);
	}
	else if (ACTIVEPIECES(board) < STAGE3) {
		val += 5*h_mobility(board);
		val += h_pmobility(board);
		val += 15*h_stability(board);
	}
	else {
		val += h_discdiff(board);
	}
	
	return val;
}

int h_parity(int *board) {
	
}

int h_discdiff(int *board) {
	return 2*P1PIECES(board)-ACTIVEPIECES(board);
}

int h_mobility(int *board) {
	int legalmoves[MAXMOVES];
	int val=0;
	
	// Mobility of player whose turn it is
	getmoves(board,legalmoves);
	if (legalmoves[0]>1)
		val += TURN(board)*legalmoves[0];
	else
		val -= TURN(board)*20;
	TURN(board) = -TURN(board);
	
	// Mobility of other player
	getmoves(board,legalmoves);
	if (legalmoves[0]>1)
		val += TURN(board)*legalmoves[0];
	else
		val -= TURN(board)*20;
	TURN(board) = -TURN(board);
	
	return val;
}

int h_pmobility(int *board) {
	int i, j, val=0;
	for (i=10; i<81; i++) {
		if (board[i] == EMPTY || board[i] == INVALID)
			continue;
		
		for (j=0; j<8; j++) { // Check each direction
			//if (flipdir[i] & (1<<j) && board[i+dirs[j]] == EMPTY)
			if (board[i+dirs[j]] == EMPTY)
				val-=board[i];
		}
	}
	
	return val;
}

int h_disksquare(int *board) {
	int i, val=0;
	
	/* If corner squares are owned, owning the corresponding
		X- and C- squares is actually beneficial. 
	val += board[10]*(CV*(board[11]+board[19])+XV*board[20]);
	val += board[17]*(CV*(board[16]+board[26])+XV*board[25]);
	val += board[73]*(CV*(board[64]+board[74])+XV*board[65]);
	val += board[80]*(CV*(board[71]+board[79])+XV*board[70]);
	val *= 2;*/
	
	for (i=10; i<81; i++)
		val += board[i]*disksquare[i];
	
	return val;
}

/*
int h_parity(int*board) {
	if (ACTIVEPIECES(board) && TURN(board)==PLAYER1)
		;
}*/

int h_stability(int *board) {
	int val=0, i, j, col, stop;
	
	// Top left corner
	if (board[A1]) {
		stop = 8;
		for (j=0; j<8; j++) {
			for (i=10+j*9, col=0; board[i]==board[10] && col<stop; i++, col++)
				val+=board[10];
			if (col==0)
				break;
			if (col==1)
				stop = 1;
			else
				stop = col-1;
		}
	}
	
	// Top right corner
	if (board[A8]) {
		stop = 8;
		for (j=0; j<8; j++) {
			for (i=10+j*9+7, col=0; board[i]==board[17] && col<stop; i--, col++)
				val+=board[17];
			if (col==0)
				break;
			if (col==1)
				stop = 1;
			else
				stop = col-1;
		}
	}
	
	// Bottom left corner
	if (board[H1]) {
		stop = 8;
		for (j=7; j>=0; j--) {
			for (i=10+j*9, col=0; board[i]==board[73] && col<stop; i++, col++)
				val+=board[73];
			if (col==0)
				break;
			if (col==1)
				stop = 1;
			else
				stop = col-1;
		}
	}
	
	// Bottom right corner
	if (board[H8]) {
		stop = 8;
		for (j=7; j>=0; j--) {
			for (i=10+j*9+7, col=0; board[i]==board[80] && col<stop; i--, col++)
				val+=board[80];
			if (col==0)
				break;
			if (col==1)
				stop = 1;
			else
				stop = col-1;
		}
	}
	
	return val;
}

// DEBUG: REMOVE THIS. UNUSED.
#define NEXTROW(x) (x+=9)
int h_straightlines(int *board) {
	int val, pos, count, i;
	int player = TURN(board);
	
	// Vertical lines
	for (pos=10; pos<18; pos++) {
		count = 0;
		for (i=pos; board[i]==player; NEXTROW(i),count++);
		if (i==pos)
			for (NEXTROW(i); board[i]==player; NEXTROW(i),count++);
		if (count >= 7) {
			if (i==pos || i==pos+63)
				if (count==8)
					val+=15;
				else
					val+=5;
			else if (i==pos+9 || i==pos+54)
				val+=3;
			else
				val+=2;
		}
		if (count == 8)
			val++;
	}
	
	// Horizontal lines
	for (pos=10; pos<82; NEXTROW(pos)) {
		count = 0;
		for (i=pos; board[i]==player; i++,count++);
		if (i==pos)
			for (i++; board[i]==player; i++,count++);
		if (count >= 7) {
			if (i==pos || i==pos+7)
				if (count==8)
					val+=15;
				else
					val+=5;
			else if (i==pos+1 || i==pos+6)
				val+=3;
			else
				val+=2;
		}
		if (count == 8)
			val++;
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
				for (count=0, pos+=dirs[j]; board[pos]==-player; count++, pos+=dirs[j]);
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
	printf("Evaluation of this board: %d\n",player*evaluation(board)); // DEBUG!!
	
	if (!terminaltest(board))
		printf("P%d (%s%c%s)'s turn. ",GETPLAYER(player),player==PLAYER1?COL_PLAYER1:COL_PLAYER2,player==PLAYER1?CHR_PLAYER1:CHR_PLAYER2,COL_EMPTY);
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
