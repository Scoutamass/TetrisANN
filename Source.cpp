#include "SDL2-2.28.5/include/SDL.h"
#undef main
#include "SDL2-2.28.5/include/SDL_timer.h"
#include <chrono>
#include <ctime> 
#include <algorithm>
#include <iostream>
#include <string>

int screenWidth = 1920;
int screenHeight = 1080;
int lastFrame = 0;
const int frameTime = 1000/100;
SDL_Window *screen = SDL_CreateWindow("Tetris", 0, 0, screenWidth, screenHeight, 0);
SDL_Renderer *renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE);
bool done = false;
int startTime = 0;

const int squareSize = screenHeight/23;
const int boardLeft = screenWidth/2 - squareSize * 5;
const int boardTop = screenHeight/2 - squareSize * 11;
//I, L, J, O, S, Z, T		Piece Order
const int pieceRed[7] =   {100, 250, 25, 250, 25,  225, 150};
const int pieceGreen[7] = {100, 100, 25, 200, 225, 25,  25};
const int pieceBlue[7] =  {255, 50,  225, 25, 25,  25,  150};

int currentPiece = -1;
int holdPiece = -1;
int pieceX[4] = {0, 1, 1, 2};
int pieceY[4] = {3, 3, 2, 3};
int rotateAnchorX = 0;
int rotateAnchorY = 0;
int rotation = 0;

const int placeTime = 1000;
int placeTimer = placeTime;
int fallTime = 800;
int fallTimer = fallTime * 2;
const int ARR = 0;
const int DAS = 100;
int DASTimer = DAS;
int DASDir = 0;//-1 = left, 1 = right, 0 = none
bool softDrop = false;
bool held = false;

int IKickTableX[4][5] = {
	{0, -1, 2, -1, 2},
	{0, 1, 1, 1, 1},
	{0, 2, -1, 2, -1},
	{0, 0, 0, 0, 0}
};
int IKickTableY[4][5] = {
	{0, 0, 0, 0, 0},
	{0, 0, 0, -1, 2},
	{0, 0, 0, 1, 1},
	{0, 0, 0, 2, -1}
};
int kickTableX[4][5] = {
	{0, 0, 0, 0, 0},
	{0, 1, 1, 0, 1},
	{0, 0, 0, 0, 0},
	{0, -1, -1, 0, -1}
};
int kickTableY[4][5] = {
	{0, 0, 0, 0, 0},
	{0, 0, 1, -2, -2},
	{0, 0, 0, 0, 0},
	{0, 0, 1, -2, -2}
};

int queue[11] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

int board[22][10]; 
int pieceData[7][10] = {
	{1, 1, 1, 1, 3, 4, 5, 6, 9, 3},//I
	{1, 1, 1, 0, 3, 4, 5, 5, 8, 2},//L
	{0, 1, 1, 1, 3, 3, 4, 5, 8, 2},//J
	{0, 1, 0, 1, 4, 5, 5, 4, 9, 1},//O
	{1, 1, 0, 0, 3, 4, 4, 5, 8, 2},//S
	{1, 1, 0, 0, 5, 4, 4, 3, 8, 2},//Z
	{1, 1, 1, 0, 3, 4, 5, 4, 8, 2}//T
}; //pieceY values, pieceX values, rotateAnchorX, rotateAnchorY

int score = 0;
int highScore = 0;
int lineClears = 0;
int maxLines = 0;
int combo = 0;

const bool sevenSeg[10][7] = {
	{true, true, true, false, true, true, true},
	{false, true, false, false, true, false, false},
	{true, false, true, true, true, false, true},
	{true, false, true, true, false, true, true},
	{false, true, true, true, false, true, false},
	{true, true, false, true, false, true, true},
	{true, true, false, true, true, true, true},
	{true, false, true, false, false, true, false},
	{true, true, true, true, true, true, true},
	{true, true, true, true, false, true, true}
};
const int sevenSize = 10;
const int sevenThickness = 3;

const int gameMode = 2;//0 = Normal, 1 = bot, 2 = 40 Lines

double inputLayer[231];
const int hiddenLayers = 2;
const int hiddenNeurons = 50;
double outputLayer[11];
double inputWeights[232][hiddenNeurons];
double hiddenWeights[hiddenLayers - 1][hiddenNeurons][hiddenNeurons];
double outputWeights[hiddenNeurons][11];

void drawDigit(int num, int x, int y)
{
	if(sevenSeg[num][0])
	{
		SDL_Rect r{x + sevenThickness, y, sevenSize, sevenThickness};
		SDL_RenderFillRect(renderer, &r);
	}
	if(sevenSeg[num][1])
	{
		SDL_Rect r{x, y + sevenThickness, sevenThickness, sevenSize};
		SDL_RenderFillRect(renderer, &r);
	}
	if(sevenSeg[num][2])
	{
		SDL_Rect r{x + sevenSize + sevenThickness, y + sevenThickness, sevenThickness, sevenSize};
		SDL_RenderFillRect(renderer, &r);
	}
	if(sevenSeg[num][3])
	{
		SDL_Rect r{x + sevenThickness, y + sevenSize + sevenThickness, sevenSize, sevenThickness};
		SDL_RenderFillRect(renderer, &r);
	}
	if(sevenSeg[num][4])
	{
		SDL_Rect r{x, y + sevenThickness * 2 + sevenSize, sevenThickness, sevenSize};
		SDL_RenderFillRect(renderer, &r);
	}
	if(sevenSeg[num][5])
	{
		SDL_Rect r{x + sevenSize + sevenThickness, y + sevenThickness * 2 + sevenSize, sevenThickness, sevenSize};
		SDL_RenderFillRect(renderer, &r);
	}
	if(sevenSeg[num][6])
	{
		SDL_Rect r{x + sevenThickness, y + sevenSize * 2 + sevenThickness * 2, sevenSize, sevenThickness};
		SDL_RenderFillRect(renderer, &r);
	}
}

void drawNum(int num, int x, int y, int digits = 0)
{
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	digits--;
	while(num > 0 || digits >= 0)
	{
		drawDigit(num%10, x + (sevenSize + sevenThickness * 3) * digits, y);
		digits--;
		num/=10;
	}
}

void paint()
{
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
	SDL_RenderFillRect(renderer, NULL);
	
	SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
	for(int x  = 0; x < 10; x++) for(int y = 2; y < 22; y++)//Draw Board Grid
	{
		SDL_Rect r{x * squareSize + boardLeft, y * squareSize + boardTop, squareSize, squareSize};
		SDL_RenderDrawRect(renderer, &r);
	}

	for(int x  = 0; x < 10; x++) for(int y = 0; y < 22; y++)//Draw Placed Pieces
	{
		if (board[y][x] != 0) 
		{
			SDL_SetRenderDrawColor(renderer, pieceRed[board[y][x] - 1], pieceGreen[board[y][x] - 1], pieceBlue[board[y][x] - 1], 255);
			SDL_Rect r{x * squareSize + boardLeft, y * squareSize + boardTop, squareSize - 1, squareSize - 1};
			SDL_RenderFillRect(renderer, &r);
		} 
	}

	int offset = 0;//Draw drop shadow
	while(pieceY[0] + offset < 22 && pieceY[1] + offset < 22 && pieceY[2] + offset < 22 && pieceY[3] + offset < 22 && board[pieceY[0] + offset][pieceX[0]] == 0 && board[pieceY[1] + offset][pieceX[1]] == 0 && board[pieceY[2] + offset][pieceX[2]] == 0 && board[pieceY[3] + offset][pieceX[3]] == 0) offset++;
	offset--;
	SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
	for(int i = 0; i < 4; i++)
	{
		SDL_Rect r{pieceX[i] * squareSize + boardLeft, pieceY[i] * squareSize + boardTop + offset * squareSize, squareSize, squareSize};
		SDL_RenderFillRect(renderer, &r);
	}

	double placeDarkening = 0.25 * (placeTimer - lastFrame)/placeTime + 0.75;
	SDL_SetRenderDrawColor(renderer, pieceRed[currentPiece] * placeDarkening, pieceGreen[currentPiece] * placeDarkening, pieceBlue[currentPiece] * placeDarkening, 255);
	for(int i = 0; i < 4; i++)//draw controlled piece
	{
		SDL_Rect r{pieceX[i] * squareSize + boardLeft, pieceY[i] * squareSize + boardTop, squareSize, squareSize};
		SDL_RenderFillRect(renderer, &r);
	}

	for(int i = 0; i < 5; i++) for(int j = 0; j < 4; j++)//Draw Queue
	{
		SDL_SetRenderDrawColor(renderer, pieceRed[queue[i]], pieceGreen[queue[i]], pieceBlue[queue[i]], 255);
		SDL_Rect r{pieceData[queue[i]][j + 4] * squareSize + boardLeft + squareSize * 8, pieceData[queue[i]][j] * squareSize + boardTop + squareSize * 4 * i + squareSize * 2, squareSize, squareSize};
		SDL_RenderFillRect(renderer, &r);
	}

	if(holdPiece != -1)//Draw Held Piece
	{
		SDL_SetRenderDrawColor(renderer, pieceRed[holdPiece], pieceGreen[holdPiece], pieceBlue[holdPiece], 255);
		if(held) SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
		for(int i = 0; i < 4; i++)
		{
			SDL_Rect r{pieceData[holdPiece][i + 4] * squareSize + boardLeft + squareSize * -8, pieceData[holdPiece][i] * squareSize + boardTop + squareSize, squareSize, squareSize};
			SDL_RenderFillRect(renderer, &r);
		}
	}

	const int numX = boardLeft + squareSize * -8;
	//Draw Number Labels

	//Draw Numbers
	drawNum(lineClears, 100, 100, 2);
	drawNum(maxLines, 100, 100 + sevenSize * 3, 2);
	drawNum((SDL_GetTicks() - startTime)/60000, 100, 100 + sevenSize * 9, 2);//Minutes
	drawNum((SDL_GetTicks() - startTime)%60000, 100 + sevenSize * 2 + sevenThickness * 7, 100 + sevenSize * 9, 5);//Seconds

	SDL_Rect dot = {100 + sevenSize * 4 + sevenThickness * 12, 100 + sevenSize * 11 + sevenThickness * 2, sevenThickness, sevenThickness};//Decimal Point
	SDL_RenderFillRect(renderer, &dot);
	dot = {100 + sevenSize * 2 + sevenThickness * 5 + 1, 100 + sevenSize * 9 + (sevenSize * 2 + sevenThickness * 3)/3, sevenThickness, sevenThickness};//Colon Top Point
	SDL_RenderFillRect(renderer, &dot);
	dot = {100 + sevenSize * 2 + sevenThickness * 5 + 1, 100 + sevenSize * 9 + (sevenSize * 4 + sevenThickness * 6)/3, sevenThickness, sevenThickness};//Colon Bottom Point
	SDL_RenderFillRect(renderer, &dot);

	//Draw Board
	SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
	SDL_Rect board = {boardLeft, boardTop + squareSize * 2, squareSize * 10, squareSize * 20};
	SDL_RenderDrawRect(renderer, &board);

	SDL_RenderPresent(renderer);
}

void nextBag()
{
	int offset = 0;
	while(queue[offset] >= 0) offset++;
	if(offset >= 5) return;
	int pieces[] = {0, 1, 2, 3, 4, 5, 6};
	std::random_shuffle(std::begin(pieces), std::end(pieces));
	for(int i = 0; i < 7; i++) queue[i + offset] = pieces[i];
}

void nextPiece()
{
	rotation = 0;
	pieceY[0] = pieceData[currentPiece][0];
	pieceY[1] = pieceData[currentPiece][1];
	pieceY[2] = pieceData[currentPiece][2];
	pieceY[3] = pieceData[currentPiece][3];
	pieceX[0] = pieceData[currentPiece][4];
	pieceX[1] = pieceData[currentPiece][5];
	pieceX[2] = pieceData[currentPiece][6];
	pieceX[3] = pieceData[currentPiece][7];
	rotateAnchorX = pieceData[currentPiece][8];
	rotateAnchorY = pieceData[currentPiece][9];
}

void cyclePiece()
{
	currentPiece = queue[0];
	for(int i = 0; i < 10; i++) queue[i] = queue[i + 1];
	queue[10] = -1;
	nextBag();
	nextPiece();
}

void placePiece(int time)
{
	for(int i = 0; i < 4; i++) board[pieceY[i]][pieceX[i]] = currentPiece + 1;
	fallTimer = time + fallTime;
	held = false;
	cyclePiece();
}

bool dropPiece()
{
	bool movable = true;
	for(int i = 0; i < 4; i++) if(pieceY[i] > 20 || board[pieceY[i] + 1][pieceX[i]] != 0) movable = false;
	if(movable)
	{
		for(int i = 0; i < 4; i++) pieceY[i]++;
		rotateAnchorY += 2;
	}
	return movable;
}

bool moveRight()
{
	bool movable = true;
	for(int i = 0; i < 4; i++) movable = movable && pieceX[i] < 9 && !board[pieceY[i]][pieceX[i] + 1];
	if(movable)
	{
		for(int i = 0; i < 4; i++) pieceX[i]++;
		rotateAnchorX += 2;
	}
	return movable;
}

bool moveLeft()
{
	bool movable = true;
	for(int i = 0; i < 4; i++) movable = movable && pieceX[i] > 0 && !board[pieceY[i]][pieceX[i] - 1];
	if(movable)
	{
		for(int i = 0; i < 4; i++) pieceX[i]--;
		rotateAnchorX -= 2;
	}
	return movable;
}

void scorePoints(int lines)
{

}

void timeStep(int time)
{
	bool fallable = true;
	for(int i = 0; i < 4; i++) if(pieceY[i] > 20 || board[pieceY[i] + 1][pieceX[i]] != 0) fallable = false;
	if(fallable)
	{
		while(fallTimer <= time)
		{
			fallTimer += fallTime;
			dropPiece();
		}
		placeTimer = time + placeTime;
		if(softDrop) while(dropPiece());
	} else
	{
		if(placeTimer <= time) placePiece(time);
		fallTimer = time + fallTime;
	}
	while(DASDir != 0 && DASTimer <= time)
	{
		DASTimer = time + ARR;
		if(DASDir == 1) if(!moveRight()) DASTimer++;
		if(DASDir == -1) if(!moveLeft()) DASTimer++;
	}
	int linesCleared = 0;
	for(int i = 0; i < 22; i++)
	{
		bool full = true;
		for(int j = 0; j < 10; j++) full = full && board[i][j];
		if(full)
		{
			linesCleared++;
			for(int j = i; j > 0; j--) for(int k = 0; k < 10; k++) board[j][k] = board[j - 1][k];
		}
	}
	scorePoints(linesCleared);
	lineClears += linesCleared;
	if(gameMode == 2 && lineClears >= 40 && !done)
	{
		done = true;
		std::cout << (SDL_GetTicks() - startTime)/60000 << ":" << ((SDL_GetTicks() - startTime)%60000)/1000 << "." << (SDL_GetTicks() - startTime) % 1000;
	}
}

int init()
{
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) printf("error initializing SDL: %s\n", SDL_GetError());
	if(!screen)
	{
		std::cout << "Error making screen";
		return 1;
	}
	if(!renderer)
	{
		std::cout << "Error making renderer";
		return 1;
	}
	SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	lastFrame = SDL_GetTicks();
	std::srand(std::time(0));

	SDL_DisplayMode dm;
	SDL_GetDesktopDisplayMode(0, &dm);
	screenWidth = dm.w;
	screenHeight = dm.h;

	return 0;
}

void reset()
{
	for(int i = 0; i < 11; i++) queue[i] = -1;
	nextBag();
	placePiece(SDL_GetTicks());
	for(int i = 0; i < 22; i++) for(int j = 0; j < 10; j++) board[i][j] = 0;
	holdPiece = -1;
	maxLines = std::max(lineClears, maxLines);
	lineClears = 0;
	highScore = std::max(score, highScore);
	score = 0;
	startTime = SDL_GetTicks();
}

void moveReset(int time)//run every time the player moves the piece, resets timers
{
	placeTimer = time + placeTime;
	DASTimer = time + DAS;
}

bool doKicks(int pieceX[4], int pieceY[4], int rotateDir)//rotateDir = 1 for cw, 3 for cc, 2 for 180
{
	if(currentPiece == 3) return true;
	for(int i = 0; i < 5; i++)
	{
		int offsetX = 0;
		int offsetY = 0;
		if(currentPiece == 0)
		{
			offsetX = IKickTableX[rotation][i] - IKickTableX[(rotation + rotateDir)%4][i];
			offsetY = IKickTableY[rotation][i] - IKickTableY[(rotation + rotateDir)%4][i];
		}
		else
		{
			offsetX = kickTableX[rotation][i] - kickTableX[(rotation + rotateDir)%4][i];
			offsetY = kickTableY[rotation][i] - kickTableY[(rotation + rotateDir)%4][i];
		}
		bool movable = true;
		for(int j = 0; j < 4; j++) if(pieceX[j] + offsetX > 9 || pieceX[j] + offsetX < 0 || pieceY[j] + offsetY > 21 || pieceY[j] + offsetY < 0 || board[pieceY[j] + offsetY][pieceX[j] + offsetX] != 0) movable = false;
		if(movable)
		{
			for(int j = 0; j < 4; j++)
			{
				pieceX[j] += offsetX;
				pieceY[j] += offsetY;
			}
			rotateAnchorX += offsetX * 2;
			rotateAnchorY += offsetY * 2;
			return true;
		}
	}
	return false;
}

void doKeys()
{
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_KEYDOWN:
				if(!event.key.repeat)
				{
					if(event.key.keysym.sym == SDLK_ESCAPE) done = true;
					if(gameMode == 1) return;
					else if(event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_KP_5) softDrop = true;
					else if(event.key.keysym.sym == SDLK_r) reset();
					else if(event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_KP_8)
					{
						while(dropPiece());
						placeTimer = 0;
					}
					else if(event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_KP_6)
					{
						DASDir = 1;
						moveReset(event.key.timestamp);
						moveRight();
					}
					else if(event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_KP_4)
					{
						DASDir = -1;
						moveReset(event.key.timestamp);
						moveLeft();
					}
					else if(event.key.keysym.sym == SDLK_a)
					{
						int tempPieceX[4] = {pieceX[0], pieceX[1], pieceX[2], pieceX[3]};
						int tempPieceY[4] = {pieceY[0], pieceY[1], pieceY[2], pieceY[3]};
						for(int i = 0; i < 4; i++) tempPieceX[i] = ((pieceY[i] * 2 - rotateAnchorY) + rotateAnchorX)/2;
						for(int i = 0; i < 4; i++) tempPieceY[i] = (-(pieceX[i] * 2 - rotateAnchorX) + rotateAnchorY)/2;
						moveReset(event.key.timestamp);

						if(doKicks(tempPieceX, tempPieceY, 3))
						{
							for(int i = 0; i < 4; i++)
							{
								pieceX[i] = tempPieceX[i];
								pieceY[i] = tempPieceY[i];
							}
							rotation += 3;
							rotation%=4;
						}
					}
					else if(event.key.keysym.sym == SDLK_s)
					{
						int tempPieceX[4] = {pieceX[0], pieceX[1], pieceX[2], pieceX[3]};
						int tempPieceY[4] = {pieceY[0], pieceY[1], pieceY[2], pieceY[3]};
						for(int i = 0; i < 4; i++) tempPieceX[i] = (-(pieceY[i] * 2 - rotateAnchorY) + rotateAnchorX)/2;
						for(int i = 0; i < 4; i++) tempPieceY[i] = ((pieceX[i] * 2 - rotateAnchorX) + rotateAnchorY)/2;
						moveReset(event.key.timestamp);

						if(doKicks(tempPieceX, tempPieceY, 1))
						{
							for(int i = 0; i < 4; i++)
							{
								pieceX[i] = tempPieceX[i];
								pieceY[i] = tempPieceY[i];
							}
							rotation += 1;
							rotation%=4;
						}
					}
					else if(event.key.keysym.sym == SDLK_d)
					{
						int tempPieceX[4] = {pieceX[0], pieceX[1], pieceX[2], pieceX[3]};
						int tempPieceY[4] = {pieceY[0], pieceY[1], pieceY[2], pieceY[3]};
						for(int i = 0; i < 4; i++) tempPieceX[i] = (-(pieceX[i] * 2 - rotateAnchorX) + rotateAnchorX)/2;
						for(int i = 0; i < 4; i++) tempPieceY[i] = (-(pieceY[i] * 2 - rotateAnchorY) + rotateAnchorY)/2;
						moveReset(event.key.timestamp);

						if(doKicks(tempPieceX, tempPieceY, 2))
						{
							for(int i = 0; i < 4; i++)
							{
								pieceX[i] = tempPieceX[i];
								pieceY[i] = tempPieceY[i];
							}
							rotation += 2;
							rotation%=4;
						}
					}
					else if(event.key.keysym.sym == SDLK_LSHIFT && !held)
					{
						if(holdPiece == -1)
						{
							holdPiece = currentPiece;
							cyclePiece();
						}
						else
						{
							int temp = holdPiece;
							holdPiece = currentPiece;
							currentPiece = temp;
							nextPiece();
						}
						held = true;
					}
				}
				timeStep(event.key.timestamp);
				break;
			case SDL_KEYUP:
				if(event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_KP_6 && DASDir == 1 || event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_KP_4 && DASDir == -1) DASDir = 0;
				if(event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_KP_5) softDrop = false;
			default:
				break;
		}
	}
}

void doNet()
{

}

int main()
{
	if(init() == 1) return 1;
	nextBag();
	placePiece(SDL_GetTicks());
	while(!done)
	{
		doKeys();
		timeStep(lastFrame);
		paint();
		//SDL_Delay(std::max(frameTime - (SDL_GetTicks() - lastFrame) * 1.0, 1.0));
		lastFrame = SDL_GetTicks();
	}
	SDL_DestroyWindow(screen);
	SDL_Quit();
	return 0;
}