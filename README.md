#Distributed Systems Spring 2013
###TEAM: 323246 The Hackers
**MEMBERS:**

* **Chenkai Liu**, nykai55@bu.edu
* **Tiffany Huang**, tiffh@bu.edu
* **Eric Stanton**, stantone@bu.edu

##Documentation
**HW2 (TIC TAC TOE)**

Specs:

* start the server by entering `./server` in one window
* the server should return the RPC port and Event Session port numbers
* open client window and enter `./client`
* then enter `connect <ip:host>`
* if the values entered are valid, the client establishes a connection with the server and the client is assigned either X or O.
* the client automatically pings the server to ask to be subscribed to the event channel
* server adds the client to its subscribers
* another client connects the same way as the previous one
* each client can make a move by entering a number from 0-9 (each number represents a tile on the board) via the prompt
* if the move is valid, the server returns the board, otherwise it tells the user that it's an invalid move
* everytime one of the clients make a move, the event is published to all subscribers (each client gets the updated board)
* in each RPC move of the board, the server also checks to see if it resulted in a win/lose/draw, and returns the appropriate thing.
* when there is win/draw/lose, the field in the player_state struct is set to reflect the status of the game, and then no more moves are accepted.

Architecture:

### Client Side
* when 

RPC:
* based on what the user inputs at the prompt, we marshall the type (hello, move, goodbye) and the header to shdr then send the session to the server.
* we then wait for a response from the server
* when we get a response, we unmarshall the contents from the rhdr of the session and output according to what we received (the board, win/lose, invalid move)
* the game board is constructed on the client side using what we receive in Proto_Game_State in the header.

Event:
* When we start the connection with the server, we automatically also connect to the the event port (rpc port num+1) and start a listen loop that is runs waiting for an incoming message until the user exits.
* once it receives a message, it goes the appropriate handler (determined by the MT)

### Server Side
* when start the server, a thread for listening to clients that want to subscribe is opened (adds client to subscribers if message is received) (the event_listen loop)

RPC:
* when the we start the server, we start the rpc loop to listen to any client's rpc requests. When the RPC loop receives a message, it unmarshalls it then goes to the appropriate handler according to the MT for the reply back to the client, then sends the reply back to the client

Event:
* the only event channel in this game is the game board - the event is posted to all subscribers when 1) the server executes doUpdateClients() via the prompt, 2) when it receives the hello request from a client (when the client connects to the server) 3) when the user makes a valid move.