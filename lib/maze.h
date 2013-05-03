
/////////////////// maze.h /////////////////// 

#include <time.h>
#include <sys/time.h>

// Define constants
#define MAX_MAP_DIMENSIONS 200   
#define MAX_NUM_PLAYERS 20
#define MAX_LINE_LEN 500
#define NUMOFOBJECTS 4
#define DISPLAYUI 1
// Help debug game by turning off collision detection
#define CANMOVE_THROUGH_WALL 0
#define STRESS_TEST 0
#define TIMING_FUNCTIONS 0
// Control size of UI window
#define WINDOW_SIZE 20

// For documenting speed of various game functions
extern struct timeval rpc_start;
extern struct timeval rpc_pickup_start;

// For marking whether or not we're panning
extern int pan;

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

typedef enum {
	NORMAL,
	RED_FLAG,
	GREEN_FLAG,
	JAIL
} State;

typedef enum {

	UP,
	DOWN,
	LEFT,
	RIGHT

} Direction;

typedef enum {

	RPC_SUCCESS,
	RPC_HELLO_ALREADYJOINED,
	RPC_STARTGAME_NOT_HOST,
	RPC_STARTGAME_UNEVEN_PLAYERS,
	// Return code for moving player
	RPC_MOVE_MOVING_INTO_WALL,
	RPC_MOVE_MOVING_INTO_PLAYER,
	RPC_IN_JAIL,
	// Return code for pickup
	RPC_PICKUP_SUCCESS,
	RPC_DROP,
	RPC_PICKUP_NOTHING,
	RPC_PICKUP_FLAGONSIDE,

} ReturnCode;


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

	NONE,
	JACKHAMMER1,
	JACKHAMMER2,
	FLAG_1,
	FLAG_2,

} ObjectType;

// Data abstraction for position
typedef struct {
	int x;
	int y;
} Position;

// Each cell
typedef struct {

	CellType 	type;
	Position    position;
	int 		occupied;

} Cell;


typedef struct {

	ObjectType type;
	Position cellposition;
	
} Object;


typedef struct {

	Cell **mapBody;
	// Convenience of access cells for when we want to spawn at home location
	// or spawn flag on floor cell 
	Cell* *homeCells_1;
	Cell* *homeCells_2;
	Cell* *floorCells_1;
	Cell* *floorCells_2;
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
	Position cellposition;
	TeamType team;
	Object inventory;
	pthread_mutex_t lock;
	int canMove;
	int playerID;
	int isHost;
	State current_state;

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

