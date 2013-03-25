
/////////////////// maze.h /////////////////// 

// Define constants
#define MAX_MAP_DIMENSIONS 200   
#define MAX_NUM_PLAYERS 10
#define MAX_LINE_LEN 500

// Defines data structures used for the game

// All possible cell types
typedef enum  {

	FLOOR,
	WALL,
	HOME_1,
	HOME_2,
	JAIL_1,
	JAIL_2,
	FLAG_1,
	FLAG_2,
	INVALID

} CellType;

// Team types
typedef enum  {  

	TEAM_1,
	TEAM_2

} TeamType;

typedef enum {

	NOT_STARTED,
	PLAYER1_TURN,
	PLAYER2_TURN,
	PLAYER1_WIN,
	PLAYER2_WIN

} GameStatus;


// Data abstraction for position
typedef struct {
	int x;
	int y;
} Position;

// Each cell
typedef struct {

	CellType 	type;
	int 		occupied;

} Cell;

typedef struct {

	Cell **mapBody;
	Position dimension;
	int numHome1;
	int numHome2;
	int numJail1;
	int numJail2;
	int numWall;
	int numFloor;

} Maze;

typedef struct 
{
	Cell cellPosition;
	TeamType team;
	int holdingFlag;
	int canMove;
	int playerID;
	int isHost;

} Player;

typedef struct {

	int 		Team1_Score;
	int 		Team2_Score;
	GameStatus 	status;

} GameState;

typedef struct {

	  Player Team1_Players[MAX_NUM_PLAYERS];
	  Player Team2_Players[MAX_NUM_PLAYERS];
	  Maze map;
	  GameState state;

} Game;


///// Method Declarations /////////

extern void
parseMaze(char *mazeString, Maze *maze);

