#include "SDL2-2.28.5/include/SDL.h"
#undef main
#include "SDL2-2.28.5/include/SDL_timer.h"
#include <chrono>
#include <ctime> 
#include <algorithm>
#include <iostream>
#include <string>
#include <fstream>

class Tetris
{
	public:
	int screenWidth = 0;
	int screenHeight = 0;
	int lastFrame = 1;//The time on the Last Frame
	SDL_Window* screen = SDL_CreateWindow("Tetris", 0, 0, 0, 0, 0);
	SDL_Renderer* renderer;
	bool done = false;//Whether to Quit the Program or Not
	int startTime = 0;//Time Since Boot or Last Reset, Used to Calculate Time Playing

	int squareSize = screenHeight / 23;//The Size of the Squares on the Board
	int boardLeft = screenWidth / 2 - squareSize * 5;//X Value of the Left Side of the Board
	int boardTop = screenHeight / 2 - squareSize * 11;//Y Value of the Top of the Board
	//I, L, J, O, S, Z, T		Piece Order
	const int pieceRed[7] = { 100, 250, 25, 250, 25,  225, 150 };//Red Values of the Colors of the Pieces
	const int pieceGreen[7] = { 100, 100, 25, 200, 225, 25,  25 };//Green Values of the Colors of the Pieces
	const int pieceBlue[7] = { 255, 50,  225, 25, 25,  25,  150 };//Blue Values of the Colors of the Pieces

	int currentPiece = -1;
	int holdPiece = -1;
	int pieceX[4] = { 0, 1, 1, 2 };//X Values of the individual Squares of the Current Piece
	int pieceY[4] = { 3, 3, 2, 3 };//Y Values of the individual Squares of the Current Piece
	int rotateAnchorX = 0;//The Place that the Current Piece will Rotate Around, Measured in Half Squares
	int rotateAnchorY = 0;
	int rotation = 0;//Where the Piece is Rotated to, Default is 0

	int gameMode;//0 = Normal, 1 = bot, 2 = 40 Lines

	int placeTime = (gameMode == 1) ? 150 : 1000;//How long the Piece takes to Place after Hitting the Bottom of the Board
	int placeTimer = placeTime;//Timer to Track When the Piece will Place
	int fallTime = (gameMode == 1) ? 100 : 800;//How long the Piece Takes to Fall
	int fallTimer = fallTime * 2;//Timer to Track When the Piece will Fall
	static const int ARR = 0;//Auto Repeat Rate, or how long in between Each Repeated Movement When Left or Right is Held
	static const int DAS = 100;//Delayed Auto Shift, or How Long Left or Right has to be Held to Start Repeating
	int DASTimer = DAS;//Timer to Track DAS
	int DASDir = 0;//-1 = left, 1 = right, 0 = none
	bool softDrop = false;
	bool held = false;//Whether the Player held on the Current Piece
	bool grounded = false;//If the piece was unable to fall last frame
	const int moveResetLimit = 5;
	int moveResets = 0;

	//Kick Tables, see DoKicks()
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

	int queue[11] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };//The Queue of Upcoming Pieces

	int board[22][10];
	int pieceData[7][10] = {//pieceY values, pieceX values, rotateAnchorX, rotateAnchorY
		{1, 1, 1, 1, 3, 4, 5, 6, 9, 3},//I
		{1, 1, 1, 0, 3, 4, 5, 5, 8, 2},//L
		{0, 1, 1, 1, 3, 3, 4, 5, 8, 2},//J
		{0, 1, 0, 1, 4, 5, 5, 4, 9, 1},//O
		{1, 1, 0, 0, 3, 4, 4, 5, 8, 2},//S
		{1, 1, 0, 0, 5, 4, 4, 3, 8, 2},//Z
		{1, 1, 1, 0, 3, 4, 5, 4, 8, 2}//T
	};

	int lineClears = 0;
	int maxLines = 0;//Most Lines Cleared in this Session in One Reset

	bool sevenSeg[10][7] = {//Data for Each Digit in a Seven Segment Display
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
	static const int sevenSize = 10;
	static const int sevenThickness = 3;

	static const int hiddenLayers = 2;
	static const int hiddenNeurons = 50;
	double inputLayer[276];//220 board, 7 piece type, 35 queue type, 7 hold type, 7 bag pos
	double hiddenLayer[hiddenLayers][hiddenNeurons];
	double outputLayer[11];
	double inputWeights[276][hiddenNeurons];
	double hiddenWeights[hiddenLayers - 1][hiddenNeurons][hiddenNeurons];
	double outputWeights[hiddenNeurons][11];
	double maximumActivation = 0;
	int outputbutton = 0;

	const int lineClearWeight = 1000;
	const double surviveWeight = 0.01;

	double randDouble(double min, double max)
	{
		return (rand() * 1.0/RAND_MAX * (max - min)) + min;
	}

	void drawDigit(int num, int x, int y)//Draws a Seven Segment Digit at (x, y)
	{
		if (sevenSeg[num][0])
		{
			SDL_Rect r{ x + sevenThickness, y, sevenSize, sevenThickness };
			SDL_RenderFillRect(renderer, &r);
		}
		if (sevenSeg[num][1])
		{
			SDL_Rect r{ x, y + sevenThickness, sevenThickness, sevenSize };
			SDL_RenderFillRect(renderer, &r);
		}
		if (sevenSeg[num][2])
		{
			SDL_Rect r{ x + sevenSize + sevenThickness, y + sevenThickness, sevenThickness, sevenSize };
			SDL_RenderFillRect(renderer, &r);
		}
		if (sevenSeg[num][3])
		{
			SDL_Rect r{ x + sevenThickness, y + sevenSize + sevenThickness, sevenSize, sevenThickness };
			SDL_RenderFillRect(renderer, &r);
		}
		if (sevenSeg[num][4])
		{
			SDL_Rect r{ x, y + sevenThickness * 2 + sevenSize, sevenThickness, sevenSize };
			SDL_RenderFillRect(renderer, &r);
		}
		if (sevenSeg[num][5])
		{
			SDL_Rect r{ x + sevenSize + sevenThickness, y + sevenThickness * 2 + sevenSize, sevenThickness, sevenSize };
			SDL_RenderFillRect(renderer, &r);
		}
		if (sevenSeg[num][6])
		{
			SDL_Rect r{ x + sevenThickness, y + sevenSize * 2 + sevenThickness * 2, sevenSize, sevenThickness };
			SDL_RenderFillRect(renderer, &r);
		}
	}

	void drawNum(int num, int x, int y, int digits = 0)//Draw a Seven Segment Number with Multiple Digits at (x, y), Drawing at Least 'digits' Digits Even if num has Fewer
	{
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		digits--;
		while (num > 0 || digits >= 0)
		{
			drawDigit(num % 10, x + (sevenSize + sevenThickness * 3) * digits, y);
			digits--;
			num /= 10;
		}
	}

	void paint()
	{
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
		SDL_RenderFillRect(renderer, NULL);//draw background

		SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
		for (int x = 0; x < 10; x++) for (int y = 2; y < 22; y++)//Draw Board Grid
		{
			SDL_Rect r{ x * squareSize + boardLeft, y * squareSize + boardTop, squareSize, squareSize };
			SDL_RenderDrawRect(renderer, &r);
		}

		for (int x = 0; x < 10; x++) for (int y = 0; y < 22; y++)//Draw Placed Pieces
		{
			if (board[y][x] != 0)
			{
				SDL_SetRenderDrawColor(renderer, pieceRed[board[y][x] - 1], pieceGreen[board[y][x] - 1], pieceBlue[board[y][x] - 1], 255);
				SDL_Rect r{ x * squareSize + boardLeft, y * squareSize + boardTop, squareSize - 1, squareSize - 1 };
				SDL_RenderFillRect(renderer, &r);
			}
		}

		int offset = 0;//Find drop shadow
		while (pieceY[0] + offset < 22 && pieceY[1] + offset < 22 && pieceY[2] + offset < 22 && pieceY[3] + offset < 22 && board[pieceY[0] + offset][pieceX[0]] == 0 && board[pieceY[1] + offset][pieceX[1]] == 0 && board[pieceY[2] + offset][pieceX[2]] == 0 && board[pieceY[3] + offset][pieceX[3]] == 0) offset++;
		offset--;//remove overshoot
		SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);//Draw Drop Shaodw
		for (int i = 0; i < 4; i++)
		{
			SDL_Rect r{ pieceX[i] * squareSize + boardLeft, pieceY[i] * squareSize + boardTop + offset * squareSize, squareSize, squareSize };
			SDL_RenderFillRect(renderer, &r);
		}

		double placeDarkening = 0.25 * (placeTimer - lastFrame) / placeTime + 0.75;
		SDL_SetRenderDrawColor(renderer, pieceRed[currentPiece] * placeDarkening, pieceGreen[currentPiece] * placeDarkening, pieceBlue[currentPiece] * placeDarkening, 255);
		for (int i = 0; i < 4; i++)//draw controlled piece
		{
			SDL_Rect r{ pieceX[i] * squareSize + boardLeft, pieceY[i] * squareSize + boardTop, squareSize, squareSize };
			SDL_RenderFillRect(renderer, &r);
		}

		for (int i = 0; i < 5; i++) for (int j = 0; j < 4; j++)//Draw Queue
		{
			SDL_SetRenderDrawColor(renderer, pieceRed[queue[i]], pieceGreen[queue[i]], pieceBlue[queue[i]], 255);
			SDL_Rect r{ pieceData[queue[i]][j + 4] * squareSize + boardLeft + squareSize * 8, pieceData[queue[i]][j] * squareSize + boardTop + squareSize * 4 * i + squareSize * 2, squareSize, squareSize };
			SDL_RenderFillRect(renderer, &r);
		}

		if (holdPiece != -1)//Draw Held Piece
		{
			SDL_SetRenderDrawColor(renderer, pieceRed[holdPiece], pieceGreen[holdPiece], pieceBlue[holdPiece], 255);
			if (held) SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
			for (int i = 0; i < 4; i++)
			{
				SDL_Rect r{ pieceData[holdPiece][i + 4] * squareSize + boardLeft + squareSize * -8, pieceData[holdPiece][i] * squareSize + boardTop + squareSize, squareSize, squareSize };
				SDL_RenderFillRect(renderer, &r);
			}
		}

		//Draw Numbers
		const int numX = boardLeft + squareSize * -8;
		drawNum(lineClears, 100, 100, 2);
		drawNum(maxLines, 100, 100 + sevenSize * 3, 2);
		drawNum((SDL_GetTicks() - startTime) / 60000, 100, 100 + sevenSize * 9, 2);//Minutes
		drawNum((SDL_GetTicks() - startTime) % 60000, 100 + sevenSize * 2 + sevenThickness * 7, 100 + sevenSize * 9, 5);//Seconds

		SDL_Rect dot = { 100 + sevenSize * 4 + sevenThickness * 12, 100 + sevenSize * 11 + sevenThickness * 2, sevenThickness, sevenThickness };//Decimal Point
		SDL_RenderFillRect(renderer, &dot);
		dot = { 100 + sevenSize * 2 + sevenThickness * 5 + 1, 100 + sevenSize * 9 + (sevenSize * 2 + sevenThickness * 3) / 3, sevenThickness, sevenThickness };//Colon Top Point
		SDL_RenderFillRect(renderer, &dot);
		dot = { 100 + sevenSize * 2 + sevenThickness * 5 + 1, 100 + sevenSize * 9 + (sevenSize * 4 + sevenThickness * 6) / 3, sevenThickness, sevenThickness };//Colon Bottom Point
		SDL_RenderFillRect(renderer, &dot);

		//Draw Board
		SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
		SDL_Rect board = { boardLeft, boardTop + squareSize * 2, squareSize * 10, squareSize * 20 };
		SDL_RenderDrawRect(renderer, &board);

		for(int i = 0; i < 11; i++) if(outputLayer[i] > maximumActivation) maximumActivation = outputLayer[i];//Find maximum output activation

		//Displaying network
		if(gameMode == 1)
		{
			const int ANNHeight = 552;
			const int ANNWidth = 20;

			const int inputHeight = ANNHeight/276;
			for(int i = 0; i < 276; i++)//Display input layer
			{
				SDL_SetRenderDrawColor(renderer, inputLayer[i] * 255, inputLayer[i] * 255, inputLayer[i] * 255, 255);//Filling
				SDL_Rect r = {ANNWidth * 4, inputHeight * i + 300, ANNWidth, inputHeight};
				SDL_RenderFillRect(renderer, &r);
			}

			const int hiddenHeight = ANNHeight/hiddenNeurons;
			for(int i = 0; i < hiddenNeurons; i++) for(int j = 0; j < hiddenLayers; j++)//Display hidden layers
			{
				SDL_SetRenderDrawColor(renderer, hiddenLayer[j][i] / maximumActivation * 255, hiddenLayer[j][i] / maximumActivation * 255, hiddenLayer[j][i] / maximumActivation * 255, 255);//Filling
				SDL_Rect r = {ANNWidth * 5 + ANNWidth * j, hiddenHeight * i + 300, ANNWidth, hiddenHeight};
				SDL_RenderFillRect(renderer, &r);
			}

			const int outputHeight = ANNHeight/11;
			for(int i = 0; i < 11; i++)//Display output layer
			{
				SDL_SetRenderDrawColor(renderer, outputLayer[i]/maximumActivation * 255, outputLayer[i]/maximumActivation * 255, outputLayer[i]/maximumActivation * 255, 255);//Filling
				if(outputbutton == i) SDL_SetRenderDrawColor(renderer, 0, outputLayer[i]/maximumActivation * 255, 0, 255);//Highlight chosen option
				SDL_Rect r = {ANNWidth * 5 + ANNWidth * hiddenLayers, outputHeight * i + 300, ANNWidth, outputHeight};
				SDL_RenderFillRect(renderer, &r);
			}
		}

		SDL_RenderPresent(renderer);
	}

	void nextBag()//Adds another Bag of Pieces to the End of the Queue if it is not Long Enough
	{
		int offset = 0;
		while (queue[offset] >= 0) offset++;
		if (offset >= 5) return;
		int pieces[] = { 0, 1, 2, 3, 4, 5, 6 };
		std::random_shuffle(std::begin(pieces), std::end(pieces));
		for (int i = 0; i < 7; i++) queue[i + offset] = pieces[i];
	}

	void nextPiece()//Sets All of the Data for the Current Piece
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

		for(int i = 0; i < 4; i++) if(board[pieceY[i]][pieceX[i]]) done = true;
	}

	void cyclePiece()//Sets the Current Piece to the First in the Queue and Cycles the Queue
	{
		currentPiece = queue[0];
		for (int i = 0; i < 10; i++) queue[i] = queue[i + 1];
		queue[10] = -1;
		nextBag();
		nextPiece();
	}

	void placePiece(int time)//Place the Current Piece and Cycle to the Next
	{
		for (int i = 0; i < 4; i++) board[pieceY[i]][pieceX[i]] = currentPiece + 1;
		fallTimer = time + fallTime;
		held = false;
		moveResets = 0;
		cyclePiece();
	}

	bool dropPiece()//Move the Current Piece Down 1 if There is Space and Return if it was Succesful
	{
		bool movable = true;
		for (int i = 0; i < 4; i++) if (pieceY[i] > 20 || board[pieceY[i] + 1][pieceX[i]] != 0) movable = false;
		if (movable)
		{
			for (int i = 0; i < 4; i++) pieceY[i]++;
			rotateAnchorY += 2;
		}
		return movable;
	}

	bool moveRight()//Move the Current Piece Right 1 if There is Space and Return if it was Succesful
	{
		bool movable = true;
		for (int i = 0; i < 4; i++) movable = movable && pieceX[i] < 9 && !board[pieceY[i]][pieceX[i] + 1];
		if (movable)
		{
			for (int i = 0; i < 4; i++) pieceX[i]++;
			rotateAnchorX += 2;
		}
		return movable;
	}

	bool moveLeft()//Move the Current Piece Left 1 if There is Space and Return if it was Succesful
	{
		bool movable = true;
		for (int i = 0; i < 4; i++) movable = movable && pieceX[i] > 0 && !board[pieceY[i]][pieceX[i] - 1];
		if (movable)
		{
			for (int i = 0; i < 4; i++) pieceX[i]--;
			rotateAnchorX -= 2;
		}
		return movable;
	}
  
	void timeStep(int time) //handles normal game logic
	{
		bool fallable = true;
		for (int i = 0; i < 4; i++) if (pieceY[i] > 20 || board[pieceY[i] + 1][pieceX[i]] != 0) fallable = false;//check if the piece can move down
		if (fallable) //tick fall timer, and drop piece when it's over
		{
			while (fallTimer <= time)
			{
				fallTimer += fallTime;
				dropPiece();
			}
			grounded = false;
			placeTimer = time + placeTime;
			if (softDrop) while (dropPiece());
		}
		else //tick place timer, and place piece when it's over
		{
			if(grounded == false) moveResets++;
			if (placeTimer <= time || moveResets > moveResetLimit) placePiece(time);
			fallTimer = time + fallTime;
			grounded = true;
		}
		while (DASDir != 0 && DASTimer <= time) //tick DAS timer, and move piece left or right it=f it's over
		{
			DASTimer = time + ARR;
			if (DASDir == 1) if (!moveRight()) DASTimer++;
			if (DASDir == -1) if (!moveLeft()) DASTimer++;
		}
		int linesCleared = 0; //check for line clears and add them to the total
		for (int i = 0; i < 22; i++)
		{
			bool full = true;
			for (int j = 0; j < 10; j++) full = full && board[i][j];
			if (full)
			{
				linesCleared++;
				for (int j = i; j > 0; j--) for (int k = 0; k < 10; k++) board[j][k] = board[j - 1][k];
			}
		}
		lineClears += linesCleared;
		if (gameMode == 2 && lineClears >= 40 && !done) //print 40 line time and end game
		{
			done = true;
			//std::cout << (SDL_GetTicks() - startTime) / 60000 << ":" << ((SDL_GetTicks() - startTime) % 60000) / 1000 << "." << (SDL_GetTicks() - startTime) % 1000;
		}
	}

	int init()
	{
		SDL_DisplayMode dm;//Initialize Screen Size
		if (SDL_GetDesktopDisplayMode(0, &dm))
		{
			std::cout << "Error Getting Display Mode";
			std::cout << SDL_GetError();
			return 1;
		}
		screenWidth = dm.w;
		screenHeight = dm.h;
		squareSize = screenHeight / 23;
		boardLeft = screenWidth / 2 - squareSize * 5;
		boardTop = screenHeight / 2 - squareSize * 11;

		SDL_SetWindowSize(screen, screenWidth, screenHeight);
		renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE);
		if (SDL_Init(SDL_INIT_EVERYTHING) != 0) printf("error initializing SDL: %s\n", SDL_GetError());//initialize SDL
		if (!screen)
		{
			std::cout << "Error making screen";
			return 1;
		}
		if (!renderer)
		{
			std::cout << "Error making renderer";
			std::cout << SDL_GetError();
			return 1;
		}


		SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);//Initialize Renderer
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);

		lastFrame = SDL_GetTicks();//Initialize Time
		std::srand(std::time(0));

//		make weights is used to generate any new files of random weights that we may want to save in the future 
//		makeweights("weights.txt");
		getweights("weights.txt");

	return 0;
}

	void reset()//Reset the Game Without Closing it
	{
		for (int i = 0; i < 11; i++) queue[i] = -1;
		nextBag();
		placePiece(SDL_GetTicks());
		for (int i = 0; i < 22; i++) for (int j = 0; j < 10; j++) board[i][j] = 0;
		holdPiece = -1;
		maxLines = std::max(lineClears, maxLines);
		lineClears = 0;
		startTime = SDL_GetTicks();
	}
  
	void moveReset(int time)//run every time the player moves the piece, resets timers
	{
		if(gameMode == 1) return;
		placeTimer = time + placeTime;
		DASTimer = time + DAS;
	}

	bool doKicks(int pieceX[4], int pieceY[4], int rotateDir)//rotateDir = 1 for cw, 3 for cc, 2 for 180	//Moves the Current Piece After it is Rotated so that it Fits
	{
		if (currentPiece == 3) return true;//O Always Fits
		for (int i = 0; i < 5; i++)//Implementation of SRS, the Super Rotation System, as Describbed here: https://tetris.wiki/Super_Rotation_System
		{
			int offsetX = 0;
			int offsetY = 0;
			if (currentPiece == 0)
			{
				offsetX = IKickTableX[rotation][i] - IKickTableX[(rotation + rotateDir) % 4][i];
				offsetY = IKickTableY[rotation][i] - IKickTableY[(rotation + rotateDir) % 4][i];
			}
			else
			{
				offsetX = kickTableX[rotation][i] - kickTableX[(rotation + rotateDir) % 4][i];
				offsetY = kickTableY[rotation][i] - kickTableY[(rotation + rotateDir) % 4][i];
			}
			bool movable = true;
			for (int j = 0; j < 4; j++) if (pieceX[j] + offsetX > 9 || pieceX[j] + offsetX < 0 || pieceY[j] + offsetY > 21 || pieceY[j] + offsetY < 0 || board[pieceY[j] + offsetY][pieceX[j] + offsetX] != 0) movable = false;
			if (movable)
			{
				for (int j = 0; j < 4; j++)
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

	void gameKeys(int button, int time)// 0 = left, 1 = right, 2 = hard drop, 3 = soft drop, 4 = hold, 5 = das left, 6 = das right, 7 = clockwise rotation, 8 = couterclockwise, 9 = 180 rotation, 10 = do nothing 
	{
		int tempPieceX[4] = {pieceX[0], pieceX[1], pieceX[2], pieceX[3]};
		int tempPieceY[4] = {pieceY[0], pieceY[1], pieceY[2], pieceY[3]};
		switch(button)
		{
			case 0:
				if(gameMode != 1) DASDir = -1;
				moveReset(time);
				moveLeft();
				break;
			case 1:
				if(gameMode != 1) DASDir = 1;
				moveReset(time);
				moveRight();
				break;
			case 2:
				while (dropPiece());
				placeTimer = 0;
				break;
			case 3:
				softDrop = true;
				break;
			case 4:
				if (!held)//hold piece
				{
					if (holdPiece == -1)//no held peice already
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
				break;
			case 5:
				DASDir = -1;
				moveReset(time);
				while(moveLeft());
				break;
			case 6:
				DASDir = 1;
				moveReset(time);
				while(moveRight());
				break;
			case 7:
				for (int i = 0; i < 4; i++) tempPieceX[i] = (-(pieceY[i] * 2 - rotateAnchorY) + rotateAnchorX) / 2;
				for (int i = 0; i < 4; i++) tempPieceY[i] = ((pieceX[i] * 2 - rotateAnchorX) + rotateAnchorY) / 2;
				moveReset(time);

				//move piece to fit
				if (doKicks(tempPieceX, tempPieceY, 1))
				{
					for (int i = 0; i < 4; i++)
					{
						pieceX[i] = tempPieceX[i];
						pieceY[i] = tempPieceY[i];
					}
					rotation += 1;
					rotation %= 4;
				}
				break;
			case 8:
				for (int i = 0; i < 4; i++) tempPieceX[i] = ((pieceY[i] * 2 - rotateAnchorY) + rotateAnchorX) / 2;
				for (int i = 0; i < 4; i++) tempPieceY[i] = (-(pieceX[i] * 2 - rotateAnchorX) + rotateAnchorY) / 2;
				moveReset(time);

				//move piece to fit
				if (doKicks(tempPieceX, tempPieceY, 3))
				{
					for (int i = 0; i < 4; i++)
					{
						pieceX[i] = tempPieceX[i];
						pieceY[i] = tempPieceY[i];
					}
					rotation += 3;
					rotation %= 4;
				}
				break;
			case 9:
				for (int i = 0; i < 4; i++) tempPieceX[i] = (-(pieceX[i] * 2 - rotateAnchorX) + rotateAnchorX) / 2;
				for (int i = 0; i < 4; i++) tempPieceY[i] = (-(pieceY[i] * 2 - rotateAnchorY) + rotateAnchorY) / 2;
				moveReset(time);

				//moce piece to fit
				if (doKicks(tempPieceX, tempPieceY, 2))
				{
					for (int i = 0; i < 4; i++)
					{
						pieceX[i] = tempPieceX[i];
						pieceY[i] = tempPieceY[i];
					}
					rotation += 2;
					rotation %= 4;
				}
				break;
		}
		timeStep(time);
	}

	void doKeys() //handle key presses
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))//get the next key event in the queue
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
				if (!event.key.repeat)
				{
					if (event.key.keysym.sym == SDLK_ESCAPE) done = true;//quit the game
					else if (event.key.keysym.sym == SDLK_r) reset();
					else if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_KP_5) gameKeys(3, event.key.timestamp);//soft drop
					else if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_KP_8) gameKeys(2, event.key.timestamp);//hard drop
					else if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_KP_6) gameKeys(1, event.key.timestamp);//move right
					else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_KP_4) gameKeys(0, event.key.timestamp);//move left
					else if (event.key.keysym.sym == SDLK_a) gameKeys(8, event.key.timestamp);//rotate CCW
					else if (event.key.keysym.sym == SDLK_s) gameKeys(7, event.key.timestamp);//rotate CW
					else if (event.key.keysym.sym == SDLK_d) gameKeys(9, event.key.timestamp);//rotate 180
					else if (event.key.keysym.sym == SDLK_LSHIFT && !held) gameKeys(4, event.key.timestamp);//hold piece
				}
				break;
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_KP_6 && DASDir == 1 || event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_KP_4 && DASDir == -1) DASDir = 0;//reset DAS
				if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_KP_5) softDrop = false;
			default:
				break;
			}
		}
	}

	double sigmoid(double d)
	{
		return 1 / (1 + exp(-d));
	}

	void getweights(std::string fileName) {
		std::fstream weights(fileName);
		std::string weightstr;
		double weight;
		// inital layer to hidden layer weights
		for (int i = 0; i < 276; i++) { 
			for (int j = 0; j < hiddenNeurons; j++) {
				getline(weights, weightstr);
				weight = atof(weightstr.c_str());
				inputWeights[i][j] = weight;
			}
		}

		// hidden layer to hidden layer weights
		for (int i = 0; i < hiddenLayers - 1; i++) {
			for (int j = 0; j < hiddenNeurons; j++) {
				for (int k = 0; k < hiddenNeurons; k++) {
					getline(weights, weightstr);
					weight = atof(weightstr.c_str());
					hiddenWeights[i][j][k] = weight;
				}
			}
		}

		// hidden to output layer
		for (int i = 0; i < hiddenNeurons; i++) {
			for (int j = 0; j < 11; j++) {
				getline(weights, weightstr);
				weight = atof(weightstr.c_str());;
				outputWeights[i][j] = weight;
			}
		}
		weights.close();

	}

	void setweights(std::string fileName) {
		std::fstream weights(fileName);
		std::string weightstr;
		double weight;

		// input to hidden layer
		for (int i = 0; i < 276; i++) {
			for (int j = 0; j < hiddenNeurons; j++) {
				weight = inputWeights[i][j];
				weightstr = std::to_string(weight);
				weights << weightstr + "\n";
			}
		}

		// hidden layer to hidden layer weights
		for (int i = 0; i < hiddenLayers - 1; i++) {
			for (int j = 0; j < hiddenNeurons; j++) {
				for (int k = 0; k < hiddenNeurons; k++) {
					weight = hiddenWeights[i][j][k];
					weightstr = std::to_string(weight);
					weights << weightstr + "\n";
				}
			}
		}

		// hidden to output layer
		for (int i = 0; i < hiddenNeurons; i++) {
			for (int j = 0; j < 11; j++) {
				weight = outputWeights[i][j];
				weightstr = std::to_string(weight);
				weights << weightstr + "\n";
			}
		}

		weights.close();
	}

	void makeweights(std::string fileName) {
		//makes random weights should become obsolote
			// makes the weights file with random weights 
		std::fstream weights(fileName);
		for (int i = 0; i < 276; i++) {
			for (int j = 0; j < hiddenNeurons; j++) {
				double weightx = randDouble(-1, 1);
				std::string weighty = std::to_string(weightx);
				weights << weighty + "\n";
			}
		}

		for (int i = 0; i < hiddenLayers - 1; i++) {
			for (int j = 0; j < hiddenNeurons; j++) {
				for (int k = 0; k < hiddenNeurons; k++) {
					double weightx = randDouble(-1, 1);
					std::string weighty = std::to_string(weightx);
					weights << weighty + "\n";
				}
			}
		}

		for (int i = 0; i < hiddenNeurons; i++) {
			for (int j = 0; j < 11; j++) {
				double weightx = randDouble(-1, 1);
				std::string weighty = std::to_string(weightx);
				weights << weighty + "\n";

			}
		}
		weights << "x";
		weights.close();

	}

	void doNet()//Run Neural Network
	{
		//Set Inputs
		for(int i = 0; i < 276; i++) inputLayer[i] = 0;

		for(int i = 0; i < 10; i++) for(int j = 0; j < 22; j++) inputLayer[i * 22 + j] = board[j][i];//Board
		for(int i = 0; i < 4; i++) inputLayer[pieceX[i] * 22 + pieceY[i]] = 0.5;
		inputLayer[220 + currentPiece] = 1;//Current Piece
		for (int i = 0; i < 5; i++) inputLayer[227 + i * 7 + queue[i]] = 1;//Queue
		if (holdPiece != -1) inputLayer[262 + holdPiece] = 1;//Hold Piece
		int bagPos = 0;//Bag Position
		for (; bagPos < 11; bagPos++)if (queue[bagPos] == -1) break;
		bagPos %= 7;
		bagPos = 6 - bagPos;
		inputLayer[269 + bagPos] = 1;

		for(int i = 0; i < hiddenLayers; i++) for(int j = 0; j < hiddenNeurons; j++) hiddenLayer[i][j] = 0;
		//First Hidden Layer
		for (int i = 0; i < hiddenNeurons; i++) {
			for (int j = 0; j < 276; j++) {
				hiddenLayer[0][i] += inputLayer[j] * inputWeights[j][i];//Add Activations
			}
		}

		for(int i = 0; i < hiddenNeurons; i++) hiddenLayer[0][i] = sigmoid(hiddenLayer[0][i]);

		//Other Hidden Layers
		for(int k = 1; k < hiddenLayers; k++)
		{
			for(int i = 0; i < hiddenNeurons; i++) for(int j = 0; j < 276; j++) hiddenLayer[k][i] += hiddenLayer[k - 1][j] * hiddenWeights[k - 1][i][j];//Add Activations
			for(int i = 0; i < hiddenNeurons; i++) hiddenLayer[k][i] = sigmoid(hiddenLayer[k][i]);
		}

		//Output Layer
		for (int k = 0; k < 11; k++) for (int x = 0; x < hiddenNeurons; x++) outputLayer[k] += hiddenLayer[k][x] * outputWeights[x][k];

	  for (int k = 0; k < 11; k++) outputLayer[k] = sigmoid( outputLayer[k] );
		double outputoption = 0;
		outputbutton = 0;
		for (int y = 0; y < 11; y++) {
			if (outputLayer[y] > outputoption) {
				outputoption = outputLayer[y];
				outputbutton = y;
			}
    
		}
		//for(int i = 0; i < 11; i++) std::cout << outputLayer[i] << "\n";
		//std::cout << "\n";
	gameKeys(outputbutton, SDL_GetTicks());
		




		//Simulate Outputs https://gamedev.stackexchange.com/questions/117600/simulate-keyboard-button-press

	}

	Tetris(int modeGame)
	{
		gameMode = modeGame;
		placeTime = (gameMode == 1) ? 150 : 1000;
		fallTime = (gameMode == 1) ? 100 : 800;
	}

	Tetris()
	{
		placeTime = (gameMode == 1) ? 150 : 1000;
		fallTime = (gameMode == 1) ? 100 : 800;
	}

	void run()
	{
		if (init() == 1) return;
		nextBag();
		placePiece(SDL_GetTicks());
		while(!done)
		{
			doKeys();
			if(gameMode == 1)doNet();
			timeStep(lastFrame);
			paint();
			//SDL_Delay(std::max(frameTime - (SDL_GetTicks() - lastFrame) * 1.0, 1.0));
			lastFrame = SDL_GetTicks();
		}
		SDL_DestroyWindow(screen);
	}

	int calcScore()
	{
		return lineClears * lineClearWeight/(lastFrame - startTime) + int((lastFrame - startTime) * surviveWeight);
	}
};

int main()
{
	Tetris *bots[10];
	for(int i = 0; i < 10; i++) bots[i] = new Tetris(1);
	for(int j = 0; j < 20; j++)
	{
		for(int i = 0; i < 10; i++)
		{
			(*bots[i]).startTime = SDL_GetTicks();
			(*bots[i]).run();
		}
		for(int i = 0; i < 10; i++) std::cout << (*bots[i]).calcScore() << "\n";
		Tetris* best = NULL;
		int bestScore = 0;
		Tetris* runnerUp = NULL;
		int runnerUpScore = 0;
		for(int i = 0; i < 10; i++) if((*bots[i]).calcScore() > bestScore)
		{
			runnerUp = best;
			runnerUpScore = bestScore;
			best = bots[i];
			bestScore = (*best).calcScore();
		}
	}
	SDL_Quit();
	return 0;
}