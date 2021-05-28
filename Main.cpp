/*Anna Carolina Ravazzi Martins
Engenharia da Computacao UTFPR 2020*/


#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

//setting and initializing global variables
constexpr int SCREEN_WIDTH = 1000, SCREEN_HEIGHT = 650;
SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
int gNumBalls = 0;

//class that holds information about the texture
class textureInfo
{
public:
	textureInfo()
	{
		mWidth = 0;
		mHeight = 0;
		mRadius = 0;
		mTexture = NULL;
	}
	~textureInfo()
	{
		freeTexture();
	}
	void freeTexture()
	{
		if (mTexture != NULL)
		{
			SDL_DestroyTexture(mTexture);
			mHeight = 0;
			mWidth = 0;
		}
	}
	bool loadFromFile(const char* path)
	{
		SDL_Texture* newTexture = NULL;
		SDL_Surface* loadedSurface = NULL;

		loadedSurface = IMG_Load(path);

		if (loadedSurface == NULL)
			printf("Error loading image! %s\n", IMG_GetError());
		else
		{
			SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0x00, 0xFF, 0xFF));

			newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);

			if (newTexture == NULL)
				printf("Error creating texture");
			else
			{
				mWidth = loadedSurface->w;
				mHeight = loadedSurface->h;
				mRadius = loadedSurface->w / 2;
			}

			SDL_FreeSurface(loadedSurface);
		}

		mTexture = newTexture;

		return mTexture != NULL;
	}
	void render(int x, int y, bool renderWholeScreen)
	{
		if (!renderWholeScreen)
		{
			SDL_Rect renderQuad = { x, y, mWidth, mHeight };
			SDL_RenderCopy(gRenderer, mTexture, NULL, &renderQuad);
		}
		else
			SDL_RenderCopy(gRenderer, mTexture, NULL, NULL);
	}
	int getRadius(){ return mRadius; }

	SDL_Texture* getTexture() { return mTexture; }

private:
	int mWidth;
	int mHeight;
	int mRadius;
	SDL_Texture* mTexture;
};

textureInfo backgroundTexture;

//struct that holds information about the balls
struct balls
{
	textureInfo ballTexture;

	double vx = 0;
	double vy = 0;
	//center coordinates
	double x = 0;
	double y = 0;

	void setColor(Uint8 red, Uint8 green, Uint8 blue)
	{
		SDL_SetTextureColorMod(ballTexture.getTexture(), red, green, blue);
	}
};
//will be dinamically allocated later
balls* gBalls;

//SDL2 functions
bool init();
bool loadMedia();
void close();

//The balls' functions
void initialSettings(balls* ball);
void collisionAgainstWalls(balls& ball);
void collisionBetween2Balls(balls& ball1, balls& ball2);
void separeteOverlappedBalls(balls& ball1, balls& ball2);
void moveBall(balls& ball);


int main(int argc, char* argv[])
{
	printf("Insert number of balls: ");
	scanf_s("%d", &gNumBalls);

	gBalls = (balls*) malloc(sizeof(balls) * gNumBalls);

	if (!init())
		printf("Could not initialize!\n");
	else
	{
		if (!loadMedia())
			printf("Could not load media!\n");
		else
		{
			SDL_Event e;
			bool quit = false;

			//setting random initial conditions
			srand(time(0));
			initialSettings(gBalls);

			while (!quit)
			{
				while (SDL_PollEvent(&e) != 0)
				{
					if (e.type == SDL_QUIT)
						quit = true;
					else if (e.type == SDL_KEYDOWN)
					{
						//each ball's mass equals to 1u
						double kineticEnergy = 0.0;
						double kineticEnergyX = 0.0;
						double kineticEnergyY = 0.0;

						switch (e.key.keysym.sym)
						{
							//k -> total kinetic energy
						case SDLK_k:

							for (int i = 0; i < gNumBalls; i++)
								kineticEnergy += (gBalls[i].vx * gBalls[i].vx) + (gBalls[i].vy * gBalls[i].vy);

							printf("Kt = %lf\n", kineticEnergy / 2);

							break;

							//x -> kinetic energy on the x axis
						case SDLK_x:

							for (int i = 0; i < gNumBalls; i++)
								kineticEnergyX += (gBalls[i].vx * gBalls[i].vx);

							printf("Kx = %lf\n", kineticEnergyX / 2);

							break;

							//y -> kinetic energy on the y axis
						case SDLK_y:

							for (int i = 0; i < gNumBalls; i++)
								kineticEnergyY += (gBalls[i].vy * gBalls[i].vy);

							printf("Ky = %lf\n", kineticEnergyY / 2);

							break;
						}
					}
				}

				SDL_RenderClear(gRenderer);

				backgroundTexture.render(0, 0, true);

				for (int i = 0; i < gNumBalls; i++)
					gBalls[i].ballTexture.render(gBalls[i].x - gBalls[i].ballTexture.getRadius(), gBalls[i].y - gBalls[i].ballTexture.getRadius(), false);

				//moving the balls
				for (int i = 0; i < gNumBalls; i++)
					moveBall(gBalls[i]);

				//checking for collisions between balls and updating velocities
				for (int i = 0; i < gNumBalls; i++)
					for (int j = i + 1; j < gNumBalls; j++)
						collisionBetween2Balls(gBalls[i], gBalls[j]);

				//checking for collisions against walls and updating velocities
				for (int i = 0; i < gNumBalls; i++)
					collisionAgainstWalls(gBalls[i]);

				SDL_RenderPresent(gRenderer);
			}
		}
	}

	close();
	free(gBalls);

	return 0;
}

//initializing SDL2 library
bool init()
{
	bool success = true;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		success = false;
		printf("Error initializing SDL! %s\n", SDL_GetError());
	}
	else
	{
		gWindow = SDL_CreateWindow("Collision simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

		if (gWindow == NULL)
		{
			success = false;
			printf("Error creating window! %s\n", SDL_GetError());
		}
		else
		{
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

			if(gRenderer==NULL)
			{
				success = false;
				printf("Error creating renderer! %s\n", SDL_GetError());
			}
			else
			{
				SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);

				int imgFlags = IMG_INIT_PNG;
				
				if (!(IMG_Init(imgFlags) & imgFlags))
				{
					success = false;
					printf("Error initializing PNG loading! %s\n", IMG_GetError());
				}
			}
		}
	}

	return success;
}

bool loadMedia()
{
	bool success = true;

	//loading each ball texture
	for (int i = 0; i < gNumBalls; i++)
	{
		if (!gBalls[i].ballTexture.loadFromFile("ball.png"))
		{
			printf("Error loading the balls' texture! %s\n", SDL_GetError());
			success = false;
		}
	}

	//loading background texture
	if (!backgroundTexture.loadFromFile("background.png"))
	{
		printf("Error loading background texture! %s\n", SDL_GetError());
		success = false;
	}

	return success;
}

//quitting SDL and deallocating its structures
void close()
{
	for (int i = 0; i < gNumBalls; i++)
		gBalls[i].ballTexture.freeTexture();

	backgroundTexture.freeTexture();

	SDL_DestroyRenderer(gRenderer);
	gRenderer = NULL;

	SDL_DestroyWindow(gWindow);
	gWindow = NULL;

	SDL_Quit();
	IMG_Quit();
}

void initialSettings(balls* ball)
{
	int R = ball->ballTexture.getRadius();

	for (int i = 0; i < gNumBalls; i++)
	{
		//random initial position (range: within the screen)
		ball[i].x = rand() % (SCREEN_WIDTH - 2 * R) + R;
		ball[i].y = rand() % (SCREEN_HEIGHT - 2 * R) + R;

		//random initial velocity (range: -10 to 10)
		ball[i].vx = rand() % 21 - 10;
		ball[i].vy = rand() % 21 - 10;

		//random texture color
		int red, green, blue;

		red = rand() % 256;
		green = rand() % 256;
		blue = rand() % 256;

		ball[i].setColor(red, green, blue);

		//check if the new ball overlaps with the previous ones
		//if it does, the ball is recreated in another random position
		for (int j = 0; j < i; j++)
		{
			double distance = sqrt((ball[i].x - ball[j].x) * (ball[i].x - ball[j].x) + (ball[i].y - ball[j].y) * (ball[i].y - ball[j].y));

			if (distance <= 2 * R)
			{
				i--;
				break;
			}
		}
	}
}

void collisionAgainstWalls(balls& ball)
{
	int R = ball.ballTexture.getRadius();

	//collision against horizontal walls
				 //floor				//ceiling
	if (ball.y >= SCREEN_HEIGHT - R || ball.y <= R)
	{
		//separating wall and ball if they overlap
		if (ball.y > SCREEN_HEIGHT - R)
			ball.y = SCREEN_HEIGHT - R;
		else if (ball.y < R)
				ball.y = R;
		
		//updating velocity
		ball.vy = - ball.vy;
	}

	//collision against vertical walls
			  //right wall		       //left wall
	if (ball.x >= SCREEN_WIDTH - R	|| ball.x <= R)		
	{
		//separating wall and ball if they overlap
		if (ball.x > SCREEN_WIDTH - R)
			ball.x = SCREEN_WIDTH - R;
		else if (ball.x < R)		
			ball.x = R;

		//updating velocity
		ball.vx = - ball.vx;
	}
}

void collisionBetween2Balls(balls& ball1, balls& ball2)
{
	int R = ball1.ballTexture.getRadius();
	double distanceSquare = (ball1.x - ball2.x) * (ball1.x - ball2.x) + (ball1.y - ball2.y) * (ball1.y - ball2.y);

	//condition for collision
	if (distanceSquare <= 4 * R * R)
	{
		//https://en.wikipedia.org/wiki/Elastic_collision#Two-dimensional_collision_with_two_moving_objects

		double dvx1 = ((ball1.vx - ball2.vx) * (ball1.x - ball2.x) + (ball1.vy - ball2.vy) * (ball1.y - ball2.y)) * (ball1.x - ball2.x) / distanceSquare;
		double dvy1 = ((ball1.vx - ball2.vx) * (ball1.x - ball2.x) + (ball1.vy - ball2.vy) * (ball1.y - ball2.y)) * (ball1.y - ball2.y) / distanceSquare;
		double dvx2 = ((ball2.vx - ball1.vx) * (ball2.x - ball1.x) + (ball2.vy - ball1.vy) * (ball2.y - ball1.y)) * (ball2.x - ball1.x) / distanceSquare;
		double dvy2 = ((ball2.vx - ball1.vx) * (ball2.x - ball1.x) + (ball2.vy - ball1.vy) * (ball2.y - ball1.y)) * (ball2.y - ball1.y) / distanceSquare;

		ball1.vx -= dvx1;
		ball1.vy -= dvy1;
		ball2.vx -= dvx2;
		ball2.vy -= dvy2;

		separeteOverlappedBalls(ball1, ball2);
	}
}

void separeteOverlappedBalls(balls& ball1, balls& ball2)
{
	int R = ball1.ballTexture.getRadius();

	//distance between centers
	double distance = sqrt((ball1.x - ball2.x) * (ball1.x - ball2.x) + (ball1.y - ball2.y) * (ball1.y - ball2.y));

	//displacement needed to separate the overlapped balls
	double displacement = 2 * R - distance;

	//displacement required on x and y axes
	double dy = (ball1.y - ball2.y) / distance * displacement;
	double dx = (ball1.x - ball2.x) / distance * displacement;

	ball1.x += dx / 2;
	ball1.y += dy / 2;

	ball2.x -= dx / 2;
	ball2.y -= dy / 2;
}

void moveBall(balls& ball)
{
	ball.x += ball.vx;
	ball.y += ball.vy;
}