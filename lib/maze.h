
/////////////////// maze.h /////////////////// 

// Define constants
#define MAX_MAP_DIMENSIONS 200   
#define MAX_NUM_PLAYERS 10
#define MAX_LINE_LEN 500
#define NUMOFOBJECTS 4
#define DISPLAYUI 0

// Defines data structures used for the game

// All possible cell types
typedef enum  {

	FLOOR_1,
	FLOOR_2,
	WALL_FIXED,
	WALL_UNFIXED,
	HOME_1,
	HOME_2,
	JAIL_1,
	JAIL_2,
	INVALID

} CellType;

// Team types
typedef enum  {  

	TEAM_1,
	TEAM_2

} TeamType;

typedef enum {

	NOT_STARTED,
	IN_PROGRESS,
	TEAM_1_WON,
	TEAM_2_WON

} GameStatus;

typedef enum {

	JACKHAMMER,
	FLAG_1,
	FLAG_2

} ObjectType;

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

	ObjectType type;
	Cell cell;

} Object;


typedef struct {

	Cell **mapBody;
	Position dimension;
	int numHome1;
	int numHome2;
	int numJail1;
	int numJail2;
	int numFixedWall;
	int numNonfixedWall;
	int numFloor1;
	int numFloor2;
	Object objects[NUMOFOBJECTS];

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

	  Player Team1_Players[MAX_NUM_PLAYERS];
	  Player Team2_Players[MAX_NUM_PLAYERS];
	  Maze map;
	  GameStatus status;

} Game;


///// Method Declarations /////////

extern void
parseMaze(char *mazeString, Maze *maze);

