
/////////////////// maze.h /////////////////// 

// Define constants
#define MAX_MAP_DIMENSIONS 200   
#define MAX_NUM_PLAYERS 10

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
	// mapBody = new Cell[MAX_MAP_DIMENSIONS][MAX_MAP_DIMENSIONS];
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

} Player;

typedef struct {

	int Team1_Score;
	int Team2_Score;

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
