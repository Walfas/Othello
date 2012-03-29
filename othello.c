#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// Configurable display options
#define TALL 0				/* Make the board take up more 
								vertical space on the display */
								
#define SHOWRECENT 1		/* Mark the most recent move on the 
								board with a colored background */
								
#define SHOWFLIPPED 1		/* Mark the recently-flipped pieces 
								on the board with a dot */
								
#define LISTMOVESONSIDE 1	/* List legal moves on the right (messes up if 
								the entire board doesn't fit in the window) */
								
#define SHOWMOVESONBOARD 1	/* Show legal moves on the board itself, 
								in the corresponding position */

#define DEBUG 1 			/* Print eval-function's output from the perspective 
								of the current player on each turn */

// Numeric constants
#define BOARDSIZE 8
#define MAXMOVES 32
#define NUMTILES 64
#define ARRSIZE 234

#define INF 2097152
#define END 1048576
#define NEAREND 1000000

// Values representing pieces on board
#define INVALID -INF
#define EMPTY 0
#define PLAYER1 1
#define PLAYER2 -1

// Characters representing pieces on board
#define CHR_PLAYER1 '*'
#define CHR_PLAYER2 '-'
#define CHR_EMPTY ' '

// Color codes
#define COL_PLAYER1 "\033[0m\033[1;36m"
#define COL_PLAYER2 "\033[0m\033[1;32m"
#define COL_EMPTY "\033[0m"
#define COL_PLAYER1_RECENT "\033[1;36;46m"
#define COL_PLAYER2_RECENT "\033[1;32;42m"
#define COL_MOVE "\033[1;33m"

// Macros related to stability
#define ROWFILLED(b,n) (b[95+n])
#define COLFILLED(b,n) (b[103+n])
#define DIAG1FILLED(b,n) (b[111+n])
#define DIAG2FILLED(b,n) (b[126+n])
#define P1NUMSTABLE(b) (b[232])
#define TOTALNUMSTABLE(b) (b[233])
#define STABLE(b,n) (b[141+n])

// Other macros
#define GETPLAYER(x) (((x)==PLAYER1)?1:2)
#define GETPOS(x,y) (10+(x)+(y)*9)
#define GETDIAG1(pos) (GETX((pos))+GETY((pos)))
#define GETDIAG2(pos) (7-GETX((pos))+GETY((pos)))
#define GETX(pos) (((pos)-10)%9)
#define GETY(pos) (((pos)-10)/9)
#define NOMOVES(lm) (lm[1] == -1)

#define TURN(b) (b[91])
#define P1PIECES(b) (b[92])
#define ACTIVEPIECES(b) (b[93])
#define JUSTPLAYED(b) (b[94])

#define terminaltest(legalmoves,board) (NOMOVES(legalmoves) && oppskipcheck(board))

// Function prototypes
int decidemove(int *board);
int negamax(int d, int maxd, int *board, int alpha, int beta);
int oppskipcheck(int *board);
void results(int *board, int move, int *flipped);
void unflip(int *board, int *flipped);
int evaluation(int *board);
int init_stability(int *board);
void updatestability(int *board);
int h_diskdiff(int *board);
int h_mobility(int *board);
int h_pmobility(int *board);
int h_disksquare(int *board);
int h_stability(int *board);
//int h_cornerstability(int *board);
int h_topology(int *board);
void getmoves(int *board, int *legalmoves);
void printboard(int *board, int *legalmoves, int *flipped);
void emptyboard(int *board);
void defaultboard(int *board);
int loadboard(char *fname, int *board);
void printscore(int *board);

// Global variables and constants
struct timeval starttimeval, endtimeval;
double starttime, endtime, timelimit;
int endgame;

/* Elements 0-90 of the board array represent the board 
	as follows (based on implementations by Gunnar Anderson,
	Richard Delorme, etc):

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
	board[93] = Total pieces on the board
	board[94] = The position of the piece that was just played
	
	board[95] and above relate to stability calculations.
*/

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

/* Use dirs[] to access elements directly 
	Northwest, North, Northeast, West, etc. */
#define NW -10
#define N  -9
#define NE -8
#define  W -1
#define  E  1
#define SW  8
#define S   9
#define SE  10

const int dirs[] = {
	NW, N, NE,
	 W,     E,
	SW, S, SE
};

/* flipdir[] is a set of bitmasked values that represent which 
	positions could be flipped if a piece were to be placed 
	there, based on the dirs[] array. 
	
   For example, flipdir[10], representing position A1 on the 
	board, has value 208, which is (1<<4)+(1<<6)+(1<<7).
	dirs[4] corresponds to east, dirs[6] south, and dirs[7]
	southeast. */
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

/* positions[] is an array of positions 
	ordered from best to worst. */
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
#define CV 5
// Assign constant values to each square.
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

// Divide board into four quadrants
const int quadrant[91] = {
	4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  0,  0,  0,  0,  1,  1,  1,  1,
	4,  0,  0,  0,  0,  1,  1,  1,  1,
	4,  0,  0,  0,  0,  1,  1,  1,  1,
	4,  0,  0,  0,  0,  1,  1,  1,  1,
	4,  2,  2,  2,  2,  3,  3,  3,  3,
	4,  2,  2,  2,  2,  3,  3,  3,  3,
	4,  2,  2,  2,  2,  3,  3,  3,  3,
	4,  2,  2,  2,  2,  3,  3,  3,  3,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4
};

main() {
	int legalmoves[MAXMOVES], flipped[MAXMOVES];
	int board[ARRSIZE];
	int i, move, movenum, playermode;
	int iscomputer[2];
	char c, fname[1025], str[1025];
	char *endptr;
	
	srand(time(NULL));
	
	///////////////////
	// Starting menu //
	///////////////////
startmenu:
	i = c = endgame = 0;
	playermode = -1;
	flipped[0] = -1;
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
	}
	else
		defaultboard(board);
	
	putchar('\n');
	
	///////////////
	// Main Game //
	///////////////
	while(1) {
		getmoves(board,legalmoves);
		printboard(board,legalmoves,flipped);
		
		if (terminaltest(legalmoves,board)) { // End of game
			legalmoves[0] = 0;
			legalmoves[1] = -1;
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
		
		if (legalmoves[1] != -1) { // At least one legal move exists
			if (!iscomputer[ GETPLAYER(TURN(board))-1 ])
				do
					movenum = getplayermove(legalmoves);
				while (movenum<1 || movenum>legalmoves[0]);
			else	// If player is computer-controlled
				movenum = decidemove(board);
			move = legalmoves[movenum];

			printf("-----\nMove #%d (%c%d) was made by P%d.\n\n",movenum,'A'+GETX(move), 1+GETY(move), GETPLAYER(TURN(board)));
			printf("That's (%d,%d)\n",(move-A1)/9,(move-A1)%9);//DEBUG
			results(board,move,flipped);
		}
		else {
			printf("-----\nP%d had no legal moves.\n",GETPLAYER(TURN(board)));
			results(board,-1,flipped);
		}
	}
}

/** Get the player's move in the form of a number or position */
int getplayermove(int *legalmoves) {
	int i, move, movenum;
	char str[1025], *endptr;
	do {
		printf("Type the move you wish to make ");
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
				move = GETPOS(tolower(str[0])-'a',str[1]-'1');
				
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

/** Computer-controlled player uses iterative deepening with negamax
	and alpha-beta pruning to determine its move.	*/
int decidemove(int *board) {
	int player = TURN(board);
	int depth, movenum, tmp;
	int legalmoves[MAXMOVES];

	gettimeofday(&starttimeval,NULL);
	starttime = starttimeval.tv_sec+(starttimeval.tv_usec/1000000.0);
	
	getmoves(board,legalmoves);
	if (legalmoves[0] == 1) {
		printf("Not evaluating the game tree: only one legal move.\n");
		return 1;
	}
	
	// Begin iterative deepening
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
	
	gettimeofday(&endtimeval,NULL);
	endtime = endtimeval.tv_sec+(endtimeval.tv_usec/1000000.0);
	
	printf("at depth %d (%.4f seconds elapsed).\n",depth,endtime-starttime);
	
	return movenum;
}

int negamax(int d, int maxd, int *board, int alpha, int beta) {
	int legalmoves[MAXMOVES], flipped[MAXMOVES], status[3];
	int tmpboard[ARRSIZE];
	int indexbest = 1;
	int i, val, best, tnodes;

	// Check if out of time
	gettimeofday(&endtimeval,NULL);
	if ((endtimeval.tv_sec+(endtimeval.tv_usec/1000000.0))-starttime > timelimit)
		return 0;
	
	getmoves(board,legalmoves);
	if (d != maxd && terminaltest(legalmoves,board)) {
		if ((val = TURN(board)*evaluation(board)) >= 0)
			return END+val;
		else
			return val-END;
	}
	if (d == 0) // Reached depth cutoff
		return TURN(board)*evaluation(board);
	
	tnodes = 0;
	best = -INF;
	for (i=1; i<=legalmoves[0]; i++) {
		memcpy(tmpboard,board,sizeof tmpboard);
		results(tmpboard, legalmoves[i], flipped);  // Apply the move
		val = -negamax(d-1,maxd,tmpboard,-alpha,-beta);
		
		if (val <= -NEAREND || val >= NEAREND)
			tnodes++; // Terminal node found
		
		if (val>best) {
			best = val;
			indexbest = i;
		}
		else if (val == best && rand() <= RAND_MAX/i)
			indexbest = i; // Moves with the same evaluation get chosen randomly
			
		if (best>beta) {
			if (d == maxd)
				break;
			else
				return best+1;
		}
			
		if (best>alpha)
			alpha = best;
	}
	
	if (d == maxd) {
		if (tnodes>=legalmoves[0]) {
			endgame = 1; // Search reached terminal nodes
			return -indexbest;
		}
		else
			return indexbest;
	}
	else
		return best;
}

#define INCR_STABILITY(board,pos) 			\
		COLFILLED(board,GETX(pos))++; 		\
		ROWFILLED(board,GETY(pos))++; 		\
		DIAG1FILLED(board,GETDIAG1(pos))++; \
		DIAG2FILLED(board,GETDIAG2(pos))++ 
		
/** Apply the move and keep track of which pieces were
	flipped in the "flipped" array so the move can be undone.	*/
void results(int *board, int move, int *flipped) {
	int player = TURN(board);
	int j, n, count, pos;
	if (move == -1) { // No legal moves
		flipped[0] = -1;
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
				for (pos-=dirs[j]; board[pos]==-player; pos-=dirs[j]) {
					board[pos] = player;
					flipped[count++] = pos;
				}
			}
		}
	}
	flipped[count] = -1;
	
	if (player == PLAYER1)
		P1PIECES(board) += count+1;
	else
		P1PIECES(board) -= count;
		
	JUSTPLAYED(board) = move;
	TURN(board) = -player;
	
	INCR_STABILITY(board,move);
	updatestability(board);
	
	return;
}
		
void updatestability(int *board) {
	int i;
	
	for (i=A1; i<=H8; i++) {
		if (STABLE(board,i) == 0 && (board[i] == PLAYER1 || board[i] == PLAYER2)) {
			if ((COLFILLED(board,GETX(i))==8) || 
				(GETY(i)==0) || (GETY(i)==7) ||
				(board[i+ N]==board[i] && STABLE(board,i+N )) ||
				(board[i+ S]==board[i] && STABLE(board,i+S )))
			if ((ROWFILLED(board,GETY(i))==8) || 
				(GETX(i)==0) || (GETX(i)==7) ||
				(board[i+ W]==board[i] && STABLE(board,i+ W)) ||
				(board[i+ E]==board[i] && STABLE(board,i+ E)))
			if ((DIAG1FILLED(board,GETDIAG1(i))==8) ||
				(GETDIAG1(i) == ((GETDIAG2(i) > 7)?(GETDIAG2(i)-7):(7-GETDIAG2(i)))) ||
				(board[i+NE]==board[i] && STABLE(board,i+NE)) ||
				(board[i+SW]==board[i] && STABLE(board,i+SW)))
			if ((DIAG2FILLED(board,GETDIAG2(i))==8) ||
				(GETDIAG2(i) == ((GETDIAG1(i) > 7)?(GETDIAG1(i)-7):(7-GETDIAG1(i)))) ||
				(board[i+NW]==board[i] && STABLE(board,i+NW)) ||
				(board[i+SE]==board[i] && STABLE(board,i+SE))) 
			{
				STABLE(board,i) = 1;
				TOTALNUMSTABLE(board)++;
				
				//printf("%c%d is stable\n",GETX(i)+'A',GETY(i)+1);
				
				if (board[i] == PLAYER1)
					P1NUMSTABLE(board)++;
			}
		}
	}
}
		 
/** Unflip the flipped pieces in the "flipped" array. */
void unflip(int *board, int *flipped) {
	int i;
	for (i=0; flipped[i]!=-1; i++)
		board[flipped[i]] = TURN(board);
	board[JUSTPLAYED(board)] = EMPTY;
}

/** Check if the opponent's turn would be skipped if 
	this were their turn. Used in terminaltest().	 */
int oppskipcheck(int *board) {
	int legalmoves[MAXMOVES];

	TURN(board) = -TURN(board);
	getmoves(board,legalmoves);
	TURN(board) = -TURN(board);
	
	if (NOMOVES(legalmoves))
		return 1;
	else
		return 0;
}		

/** Evaluation function with different weights for various stages 
	of the game. This and all heuristics are from PLAYER1's point of view. */
#define MIDGAME 28
#define ENDGAME 48
int evaluation(int *board) {	
	if (endgame) {
		return	h_diskdiff(board);
	}
	else if (ACTIVEPIECES(board) < MIDGAME) {
		return 	//-h_diskdiff(board)/2
				h_disksquare(board)
				+15*h_mobility(board)
				+h_pmobility(board)
				+10*h_edges(board)
				//+2*h_topology(board)
				+30*h_stability(board);
				//+20*h_cornerstability(board);
	}
	else if (ACTIVEPIECES(board) < ENDGAME) {
		return	// h_disksquare(board)/2
				+8*h_mobility(board)
				//+2*h_pmobility(board)
				+5*h_edges(board)
				+30*h_stability(board);
				//+20*h_cornerstability(board);
	}
	else {
		return	h_diskdiff(board);
	}
}

/** Return the disk difference */
int h_diskdiff(int *board) {
	return 2*P1PIECES(board)-ACTIVEPIECES(board);
}

/** Evaluate a few edge patterns */
int h_edges(int *board) {
	int i, val=0;
		
	// Control of north edge
	for (i=A1; i<G1; i++) {
		if (board[i]==board[i+2]) {
			if (board[i+1] == board[i])
				val += board[i];
			else
				val -= board[i];
		}
	}
	
	if (board[C1] == board[F1] && board[D1] == board[E1]) {
		if (board[D1] == EMPTY)
			val += board[C1];
		else if (board[D1] == board[F1]) {
			val += 2*board[C1];
			if (board[B1] == board[G1] && board[B1] == board[C1])
				val += 3*board[C1];
		}
	}

	// Control of south edge
	for (i=A8; i<G8; i++) {
		if (board[i]==board[i+2]) {
			if (board[i+1] == board[i])
				val += board[i];
			else
				val -= board[i];
		}
	}
		
	if (board[C8] == board[F8] && board[D8] == board[E8]) {
		if (board[D8] == EMPTY)
			val += board[C8];
		else if (board[D8] == board[F8]) {
			val += 2*board[C8];
			if (board[B8] == board[G8] && board[B8] == board[C8])
				val += 3*board[C8];
		}
	}
	
	// Control of west edge
	for (i=A1; i<A6; i+=9) {
		if (board[i]==board[i+18]) {
			if (board[i+9] == board[i])
				val += board[i];
			else
				val -= board[i];
		}
	}
	
	if (board[A3] == board[A6] && board[A4] == board[A5]) {
		if (board[A4] == EMPTY)
			val += board[A3];
		else if (board[A3] == board[A4]) {
			val += 2*board[A3];
			if (board[A2] == board[A7] && board[A2] == board[A3])
				val += 3*board[A3];
		}
	}
	
	// Control of east edge
	for (i=H1; i<H6; i+=9) {
		if (board[i]==board[i+18]) {
			if (board[i+9] == board[i])
				val += board[i];
			else
				val -= board[i];
		}
	}
		
	if (board[H3] == board[H6] && board[H4] == board[H5]) {
		if (board[H4] == EMPTY)
			val += board[H3];
		else if (board[H3] == board[H4]) {
			val += 2*board[H3];
			if (board[H2] == board[H7] && board[H2] == board[H3])
				val += 3*board[H3];
		}
	}
	
	return val;
}

/** Evaluate the topology of the board. Based on Itamar Faybish's 
	"Thesis on Genetic Algorithm applied to Othello" and also
	incorporates an estimation of parity based on quadrants. 	*/
#define PARITYWEIGHT 10
int h_topology(int *board) {
	int i, count1[5]={0}, count2[5]={0}, parity[5]={0};
	int total1, total2, val=0;
	for (i=A1; i<=H8; i++) {
		if (board[i] == PLAYER1)
			count1[quadrant[i]]++;
		else if (board[i] == PLAYER2)
			count2[quadrant[i]]++;
		else
			parity[quadrant[i]] ^= 1;
	}
	total1 = count1[0]+count1[1]+count1[2]+count1[3];
	total2 = count2[0]+count2[1]+count2[2]+count2[3];
	
	for (i=0; i<4; i++) {
		if (total1-count1[i]!=0)
			val += count1[i]/(total1-count1[i]);
		else
			val += count1[i]*10;
	}
	
	for (i=0; i<4; i++) {
		if (total2-count2[i]!=0)
			val -= count2[i]/(total2-count2[i]);
		else
			val -= count2[i]*10;
	}
	
	if (JUSTPLAYED(board)>0)
		val -= PARITYWEIGHT*TURN(board)*(parity[quadrant[JUSTPLAYED(board)]] ? -1:1 );
	
	return val;
}

/** Mobility difference (based on number of legal moves) */
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

/** Potential mobility difference based on how many empty
	squares are next to an occupied square. */
int h_pmobility(int *board) {
	int i, j, val=0;
	for (i=B2; i<=G7; i++) {
		if (board[i] == EMPTY || GETX(i)==0 || GETX(i)==7 || board[i] == INVALID)
			continue;
		
		for (j=0; j<8; j++) { // Check each direction
			if (board[i+dirs[j]] == EMPTY)
				val-=board[i];
		}
	}
	return val;
}

/** Assign fixed values to each position on the board. */
int h_disksquare(int *board) {
	int i, val=0;
	
	for (i=A1; i<=H8; i++)
		val += board[i]*disksquare[i];
	
	return val;
}

/** Difference in stable pieces */
int h_stability(int *board) {
	return 2*P1NUMSTABLE(board)-TOTALNUMSTABLE(board);
}

/** Estimate stability based on pieces 'attached' to corner pieces. */
/*int h_cornerstability(int *board) {
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
	if (board[H1]) {
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
	if (board[A8]) {
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
}*/

/** Initialize stability of board */
int init_stability(int *board) {
	int i;
	P1NUMSTABLE(board) = 0;
	TOTALNUMSTABLE(board) = 0;
	
	for (i=0; i<8; i++) {
		COLFILLED(board,i) = 0;
		ROWFILLED(board,i) = 0;
	}
	for (i=0; i<15; i++) {
		DIAG1FILLED(board,i) = (i<7)?(7-i):(i-7);
		DIAG2FILLED(board,i) = (i<7)?(7-i):(i-7);
	}
	
	for (i=A1; i<=H8; i++) {
		STABLE(board,i) = 0;
		if (board[i] == PLAYER1 || board[i] == PLAYER2) {
			INCR_STABILITY(board,i);
			updatestability(board);
		}
	}

	for (i=A1; i<=H8; i++)
		updatestability(board);
}

/** Find legal moves and place them in the "legalmoves" array. 
	legalmoves[0] contains the number of legal moves, and other
	elements of the array contain the positions of the moves. */
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
	
	/* If no legal moves, say there is one legal move (-1) 
		which will be interpreted as a skipped turn. */
	if (legalmoves[0] == 0) {
		legalmoves[0] = 1;
		legalmoves[1] = -1;
	}	
}

/** Print an ASCII representation of the board. */
void printboard(int *board, int *legalmoves, int *flipped) {
	int i, j, player = TURN(board);
	int tmpboard[ARRSIZE];
	
	memcpy(tmpboard, board, sizeof tmpboard);
	#if SHOWMOVESONBOARD
	if (legalmoves[1] != -1)
		for (i=1; i<=legalmoves[0]; i++)
			tmpboard[legalmoves[i]] = i+END;
	#endif
	
	#if SHOWFLIPPED
	for (i=0; flipped[i]>=A1; i++)
		tmpboard[flipped[i]] = -player*2;
	#endif
	
	// Column labels at the top (A->H)
	printf("  ");
	for (i=0; i<BOARDSIZE; i++)
		printf("   %c  ",'A'+i);
	
	printf("\n");	
	for (i=0; i<BOARDSIZE; i++) {
		printf("  ");
		for (j=0; j<BOARDSIZE; j++)
			printf("+-----");
		printf("+\n");
		
		/* If SHOWFLIPPED is set, label pieces that have 
			just been flipped by placing a dot above it. 	*/
		printf("  ");
		for (j=0; j<BOARDSIZE; j++) {
			#if SHOWFLIPPED
			printf("| ");
			switch(tmpboard[GETPOS(j,i)]) {
			case PLAYER1:
				if (GETPOS(j,i)!=JUSTPLAYED(tmpboard)) {
					printf("   ");
					break;
				}
			case PLAYER1*2:
				printf(" %s.%s ",COL_PLAYER1,COL_EMPTY);
				break;
			case PLAYER2:
				if (GETPOS(j,i)!=JUSTPLAYED(tmpboard)) {
					printf("   ");
					break;
				}
			case PLAYER2*2:
				printf(" %s.%s ",COL_PLAYER2,COL_EMPTY);
				break;
			default:
				printf("   ");
				break;
			}
			printf(" ");
			#else
			printf("|     ");
			#endif
		}
		
		// Row labels at the left (1-8)
		printf("|\n%d ",i+1);
		
		/* Display the pieces on the board, as well as any legal moves and the 
			corresponding move number. If SHOWRECENT is set, the most recently-
			played piece will have a colored background. 	*/
		for (j=0; j<BOARDSIZE; j++) {
			printf("| ");
			switch(tmpboard[GETPOS(j,i)]) {
			case EMPTY:
				printf("   ");
				break;
			case PLAYER1:
			case PLAYER1*2:
				#if SHOWRECENT
				if (GETPOS(j,i)==JUSTPLAYED(tmpboard))
					printf(" %s%c%s ",COL_PLAYER1_RECENT,CHR_PLAYER1,COL_EMPTY);
				else
				#endif
					printf(" %s%c%s ",COL_PLAYER1,CHR_PLAYER1,COL_EMPTY);
				break;
			case PLAYER2:
			case PLAYER2*2:
				#if SHOWRECENT
				if (GETPOS(j,i)==JUSTPLAYED(tmpboard))
					printf(" %s%c%s ",COL_PLAYER2_RECENT,CHR_PLAYER2,COL_EMPTY);
				else
				#endif
					printf(" %s%c%s ",COL_PLAYER2,CHR_PLAYER2,COL_EMPTY);
				break;
			default: // Show move numbers
				printf("%s%2d%s ",COL_MOVE,tmpboard[GETPOS(j,i)]-END,COL_EMPTY);
				break;
			}
			printf(" ");
		}
		printf("|\n");

		#if TALL // Board takes up more vertical space
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
	
	#if LISTMOVESONSIDE
	// A ton of escape sequences to get the list to appear on the right of the board
	printf("\033[s\033[%dC\033[%dA",5+6*BOARDSIZE,3*BOARDSIZE);
	printf("P%d (%s%c%s)'s turn\033[B\033[12D",GETPLAYER(player),player==PLAYER1?COL_PLAYER1:COL_PLAYER2,player==PLAYER1?CHR_PLAYER1:CHR_PLAYER2,COL_EMPTY);
	
	// List of legal moves
	printf("\033[D\033[BMoves:\033[B\033[7D");
	if (legalmoves[1] == -1)
		printf("  None");
	else {
		for (i=1; i<=legalmoves[0]; i++)
			printf("%s%3d%s. %c%d\033[B\033[7D",COL_MOVE,i,COL_EMPTY,'A'+GETX(legalmoves[i]), 1+GETY(legalmoves[i]));
	}
	
	printf("\033[u");
	#endif
	
	#if DEBUG
	printf("Evaluation of this board: %d\n",player*evaluation(board));
	#endif
	
	#if !LISTMOVESONSIDE
	if (!terminaltest(legalmoves,board))
		printf("P%d (%s%c%s)'s turn. ",GETPLAYER(player),player==PLAYER1?COL_PLAYER1:COL_PLAYER2,player==PLAYER1?CHR_PLAYER1:CHR_PLAYER2,COL_EMPTY);
	#endif
	
	printscore(board);
}

/** Initialize the board to be empty. */
void emptyboard(int *board) {
	int x, y;
	
	for (x=0; x<ARRSIZE; x++)
		board[x] = INVALID;
	
	for (x=0; x<BOARDSIZE; x++)
		for (y=0; y<BOARDSIZE; y++)
			board[GETPOS(x,y)] = EMPTY;
			
	TURN(board) = PLAYER1;
	P1PIECES(board) = 0;
	ACTIVEPIECES(board) = 0;
	JUSTPLAYED(board) = -1;
}

/** Initialize board to the default start position. */
void defaultboard(int *board) {
	emptyboard(board);
	
	board[E4] = board[D5] = PLAYER1;
	board[D4] = board[E5] = PLAYER2;
	
	INCR_STABILITY(board,E4);
	INCR_STABILITY(board,D5);
	INCR_STABILITY(board,D4);
	INCR_STABILITY(board,E5);
	
	P1PIECES(board) = 2;
	ACTIVEPIECES(board) = 4;
	init_stability(board);
}

/** Load board from file. The format is as follows:
	- 64 characters each with values of either 0, 1, or 2 
		corresponding to EMPTY, PLAYER1, and PLAYER2 respectively.
		The 1st character corresponds to position A1 and the 64th
		character corresponds to position H8.
	- 1 character (0, 1, or 2) corresponding to whose turn it is.
	- Characters that are not 0, 1, or 2 are ignored.
*/
int loadboard(char *fname, int *board) {
	int i, j;
	char c;
	FILE *fp;
	
	if ((fp = fopen(fname,"r")) == NULL) {
		fprintf(stderr,"Error: Could not open file \"%s\" for reading: %s\n",fname,strerror(errno));
		return -1;
	}
	
	emptyboard(board);
	
	for (j=0; j<BOARDSIZE; j++) {
		for (i=0; i<BOARDSIZE; ) {
			if (feof(fp)) {
				fprintf(stderr,"Error: File \"%s\" is formatted incorrectly.\n",fname);
				return -1;
			}
			switch(getc(fp)) {
			case '0':
				board[GETPOS(i++,j)] = EMPTY;
				break;
			case '1':
				board[GETPOS(i++,j)] = PLAYER1;
				P1PIECES(board)++;
				ACTIVEPIECES(board)++;
				break;
			case '2':
				board[GETPOS(i++,j)] = PLAYER2;
				ACTIVEPIECES(board)++;
				break;
			}
		}
	}
	do {
		if (feof(fp)) {
			fprintf(stderr,"Error: File \"%s\" is formatted incorrectly.\n",fname);
			return -1;
		}
		switch(c=getc(fp)) {
			case '1': 
				TURN(board) = PLAYER1;
				break;
			case '2':
				TURN(board) = PLAYER2;
				break;
		}
	}
	while (c!='0' && c!='1');
	
	fclose(fp);
	
	init_stability(board);
	return 0;
}

/** Print the number of PLAYER1 pieces and PLAYER2 pieces. */
void printscore(int *board) {
	printf("Score: [ ");
	printf("P1 (%s%c%s): %d / ",COL_PLAYER1,CHR_PLAYER1,COL_EMPTY,P1PIECES(board));
	printf("P2 (%s%c%s): %d ]\n",COL_PLAYER2,CHR_PLAYER2,COL_EMPTY,ACTIVEPIECES(board)-P1PIECES(board));
}
