//Using SDL and standard IO
#include <SDL.h>
#include <stdio.h>
#include <vector>
#include <iostream>

//Screen dimension constants
const int SCREEN_WIDTH = 400;
const int SCREEN_HEIGHT = 400;
const uint8_t MAX_DEPTH = 8;

struct position { uint8_t y = 0; uint8_t x = 0; };
struct screenpos { int x1, x2; };
enum dirName{NORTH,EAST,SOUTH, WEST};
uint8_t dx[] = { 0, 1, 0, -1 };
uint8_t dy[] = { -1, 0, 1, 0 };

class Player
{
    position pos = {4,2};
    uint8_t dir = NORTH;

    public:
        position GetPos(){ return pos;};
        uint8_t GetDir() { return dir; };
        void MoveForward() {};

};

const uint8_t map_height = 5, map_width = 5;

class Map
{

    uint8_t map[map_height][map_width] =
    {
        {1,0,1,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,0,0,1,0},
        {0,0,0,0,0}
    };

public:
    uint8_t* getMap() { return *map; }
    bool outBounds(position p)
    {
        if (p.x < 0 || p.x >= map_width || p.y < 0 || p.y >= map_width)
            return true;
        return false;
    }
    bool isWall(position p) { 
        if (outBounds(p)) return false;
        return !!map[p.y][p.x];
    }
};

class Game
{

    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;
    SDL_Renderer* theRenderer = NULL;

    Player* p;
    Map* m;

    std::vector<screenpos> vp_cover; 
    //viewport coverage: used to determine and clamp backsite walls
    //render will stop when it only contains one item from 0 to SCREEN_WIDTH (or amx depth reached)
    //items that intersect will connect

    public:
        Player* getPlayer() { return p; }

        Game() { p = new Player(); m = new Map(); }
        void Start();
        void RenderView();
        bool isViewportFull() {
        
            std::vector<screenpos> full_cover = { {0, SCREEN_WIDTH} };
            return (vp_cover.size() == full_cover.size() && 
                vp_cover.at(0).x1 == full_cover.at(0).x1 &&
                vp_cover.at(0).x2 == full_cover.at(0).x2);
        };
        screenpos ManageWallDrawing(screenpos p);
};

screenpos Game::ManageWallDrawing(screenpos p)
{
    bool notCovered = true;
    bool invisibleWall = false;
    std::cout << "Initial bounds (" << p.x1 << ", " << p.x2 << ")\n";

    for (int i = 0; i < vp_cover.size(); i++)
    {
        if (p.x1 >= vp_cover.at(i).x1 && p.x1 <= vp_cover.at(i).x2) //if x1 is caught up under a block
        {
            notCovered = false;
            std::cout << "The rightside case\n";
            p.x1 = vp_cover.at(i).x2 + 1;
            if (p.x2 > vp_cover.at(i).x2)                           //if x2 gets out from beyond the blockage
            {
                vp_cover.at(i).x2 = p.x2;
                if (i < vp_cover.size() - 1)
                {
                    if (p.x2 >= vp_cover.at(i + 1).x1)               //clamp x2 if there is another blockage
                    {
                        p.x2 = vp_cover.at(i + 1).x1 - 1;
                        vp_cover.at(i).x2 = vp_cover.at(i + 1).x2;
                        vp_cover.erase(vp_cover.begin() + i + 1);   //merge
                    }
                }
            }
            else
            {
                invisibleWall = true;
            }
        }
        //TO DO: Add leftmost checks too [DONE]
        else
        {
            if (p.x2 >= vp_cover.at(i).x1 && p.x2 <= vp_cover.at(i).x2) //if x2 is caught under a block
            {
                notCovered = false;
                std::cout << "The leftside case\n";
                p.x2 = vp_cover.at(i).x1-1;

                if (p.x1 < vp_cover.at(i).x1)
                {
                    vp_cover.at(i).x1 = p.x1;
                }
                else
                {
                    invisibleWall = false;
                }

            }
        }
    }

    if (notCovered)
    {
        std::cout << "This wall is fully shown\n";
        vp_cover.push_back(p);

        for (int i = 0; i < vp_cover.size()-1; i++) //sort all blockages
        {
            for (int j = i+1; j < vp_cover.size(); j++)
            {
                if (vp_cover.at(j).x1 < vp_cover.at(i).x1)
                {
                    std::swap(vp_cover.at(i), vp_cover.at(j));
                    //TO DO: add check to see if blockages are at 1 pixel distance from each and merge them, otherwise the end render condition will never be met [DONE-UNTESTED]
                    if (std::abs(vp_cover.at(j).x1 - vp_cover.at(i).x2) < 2)
                    {
                        vp_cover.at(i).x2 = vp_cover.at(j).x2;
                        vp_cover.erase(vp_cover.begin()+j);
                        j--;
                    }
                }
            }
        }
    }

    if (!invisibleWall)
        std::cout << "Final bounds (" << p.x1 << ", " << p.x2 << ")\n";
    else
        std::cout << "Invisible wall\n";
    for (auto i : vp_cover)
    {
        std::cout << "(" << i.x1 << "," << i.x2 << "), ";
        //SDL_SetRenderDrawColor(theRenderer, 0, 255, 0, 0xFF);
        //SDL_RenderDrawPoint(theRenderer, i.x2, SCREEN_HEIGHT / 2);
    }
    std::cout << '\n';

    if (p.x2 < p.x1)
        return { -1,-1 };

    if (!invisibleWall)
    {
        //SDL_RenderDrawLine(theRenderer, p.x2, 0, p.x2, SCREEN_HEIGHT);
        return p;
    }

    return { -1,-1 };
}

void Game::RenderView()
{
    uint8_t depth = 0;
    screenSurface = SDL_GetWindowSurface(window);

    //SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
    SDL_SetRenderDrawColor(theRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(theRenderer);
    
    SDL_SetRenderDrawColor(theRenderer, 0,0,0, 0xFF);
    SDL_RenderSetScale(theRenderer, 1,1);

    int wallNumber = 0;

    while (depth < MAX_DEPTH && !isViewportFull())
    {
        //std::cout << "Player pos is (" << p->GetPos().x << "," << p->GetPos().y << ")\n";
        int wallSize = SCREEN_WIDTH / (depth * 2 + 1);

        position middleSquare =
        {
            p->GetPos().y + (depth) * (dy[p->GetDir()]) ,
            p->GetPos().x + (depth) * (dx[p->GetDir()])
        };
        //std::cout << "Middle point is (" << (int)middleSquare.x << "," << (int)middleSquare.y << ")\n";

        for (int i = 0; i < 1 + depth * 2; i++)
        {
            position currentSquare =
            {
                middleSquare.y - (depth  - i) * (dy[(p->GetDir() + 1) % 4]),
                middleSquare.x - (depth  - i) * (dx[(p->GetDir() + 1) % 4])

            };
            std::cout << "Touched (" << (int)currentSquare.x << "," << (int)currentSquare.y << ")\n";


            if (m->isWall(currentSquare))
            {
                std::cout << "WALL NUMBER ################ " << wallNumber++ << "\n";
                SDL_RenderDrawPoint(theRenderer, currentSquare.x, currentSquare.y);
                
                screenpos fullWall = { wallSize * i , wallSize * (i + 1) };

                screenpos finalWall = ManageWallDrawing(fullWall);

                SDL_RenderDrawLine(theRenderer, finalWall.x1 , SCREEN_HEIGHT / 2 - wallSize / 2, finalWall.x2 ,SCREEN_HEIGHT / 2 - wallSize / 2); //top
                SDL_RenderDrawLine(theRenderer, finalWall.x1 , SCREEN_HEIGHT / 2 + wallSize / 2, finalWall.x2 ,SCREEN_HEIGHT / 2 + wallSize / 2); //bottom

                std::cout << (bool)(finalWall.x2 == fullWall.x2)<<'\n';
                std::cout << (bool)(finalWall.x1 == fullWall.x1)<<'\n';


                if (finalWall.x2 == fullWall.x2)
                {
                    if (!m->isWall({ (uint8_t)(currentSquare.y + dy[(p->GetDir() + 1) % 4]), (uint8_t)(currentSquare.x + dx[(p->GetDir() + 1) % 4]) }))
                    {
                        SDL_RenderDrawLine(theRenderer, finalWall.x2, SCREEN_HEIGHT / 2 - wallSize / 2, finalWall.x2, SCREEN_HEIGHT / 2 + wallSize / 2);
                    }
                }

                if (finalWall.x1 == fullWall.x1)
                {
                    if (!m->isWall({ (uint8_t)(currentSquare.y - dy[(p->GetDir() + 1) % 4]), (uint8_t)(currentSquare.x - dx[(p->GetDir() + 1) % 4]) }))
                    {
                        SDL_RenderDrawLine(theRenderer, finalWall.x1, SCREEN_HEIGHT / 2 - wallSize / 2, finalWall.x1, SCREEN_HEIGHT / 2 + wallSize / 2);
                    }
                }
                /*
                if (!m->isWall({ (uint8_t)(currentSquare.y + dy[(p->GetDir() + 1) % 4]), (uint8_t)(currentSquare.x + dx[(p->GetDir() + 1) % 4])
                }))
                {
                    if (finalWall.x2 == fullWall.x2)
                    {
                        SDL_RenderDrawLine(theRenderer, finalWall.x2, SCREEN_HEIGHT / 2 - wallSize / 2, finalWall.x2, SCREEN_HEIGHT / 2 + wallSize / 2);
                    }

                    if (i < depth)
                    {
                        int smallerWallSize = SCREEN_WIDTH / ((depth+1) * 2 + 1);
                        screenpos rightSide = {wallSize*(i+1), smallerWallSize*(i+1+1)};
                        screenpos finalRightSide =  ManageWallDrawing(rightSide);

                        SDL_RenderDrawLine(theRenderer, finalRightSide.x1, SCREEN_HEIGHT / 2 - wallSize / 2, finalRightSide.x2, SCREEN_HEIGHT / 2 - smallerWallSize / 2); //top
                        SDL_RenderDrawLine(theRenderer, finalRightSide.x1, SCREEN_HEIGHT / 2 + wallSize / 2, finalRightSide.x2, SCREEN_HEIGHT / 2 + smallerWallSize / 2); //bottom
                    }
                }

                if (!m->isWall({ (uint8_t)(currentSquare.y - dy[(p->GetDir() + 1) % 4]), (uint8_t)(currentSquare.x - dx[(p->GetDir() + 1) % 4])
                    }))
                {
                    if (finalWall.x1 == fullWall.x1)
                    {
                        SDL_RenderDrawLine(theRenderer, finalWall.x1, SCREEN_HEIGHT / 2 - wallSize / 2, finalWall.x1, SCREEN_HEIGHT / 2 + wallSize / 2);
                    }

                    if (i > depth)
                    {
                        int smallerWallSize = SCREEN_WIDTH / ((depth + 1) * 2 + 1);
                        screenpos leftSide = {  smallerWallSize * (i+1 ), wallSize * (i)-1 };
                        screenpos finalLeftSide = ManageWallDrawing(leftSide);


                        SDL_RenderDrawLine(theRenderer, finalLeftSide.x1, SCREEN_HEIGHT / 2 - smallerWallSize / 2, finalLeftSide.x2, SCREEN_HEIGHT / 2 - wallSize / 2); //top
                        SDL_RenderDrawLine(theRenderer, finalLeftSide.x1, SCREEN_HEIGHT / 2 + smallerWallSize / 2, finalLeftSide.x2, SCREEN_HEIGHT / 2 + wallSize / 2); //bottom
                    }
                }
                */
                if (i < depth)//add sidewalls for the walls to the left
                {

                }
            }
            if (!m->isWall(currentSquare))
            {
                position leftWall =
                {
                    (uint8_t)(currentSquare.y - dy[(p->GetDir() + 1) % 4]),
                    (uint8_t)(currentSquare.x - dx[(p->GetDir() + 1) % 4])
                };
                position rightWall =
                {
                    (uint8_t)(currentSquare.y + dy[(p->GetDir() + 1) % 4]),
                    (uint8_t)(currentSquare.x + dx[(p->GetDir() + 1) % 4])
                };

                if (m->isWall(leftWall))
                {
                    std::cout << "WALL TO THE LEFT";
                    int biggerWallSize = SCREEN_WIDTH / ((depth) * 2 +1);
                    int smallerWallSize = (SCREEN_WIDTH / ((depth + 1) * 2 + 1)+1);
                    screenpos leftSide = { biggerWallSize * (i), smallerWallSize*(i+1) };
                    screenpos finalLeftSide = ManageWallDrawing(leftSide);

                    SDL_RenderDrawLine(theRenderer, finalLeftSide.x1, SCREEN_HEIGHT / 2 - biggerWallSize / 2, finalLeftSide.x2, SCREEN_HEIGHT / 2 - smallerWallSize / 2); //top
                    SDL_RenderDrawLine(theRenderer, finalLeftSide.x1, SCREEN_HEIGHT / 2 + biggerWallSize / 2, finalLeftSide.x2, SCREEN_HEIGHT / 2 + smallerWallSize / 2); //bottom
                
                    if (finalLeftSide.x2 == leftSide.x2)
                    {
                        if (!m->isWall({ (uint8_t)(leftWall.y + dy[p->GetDir()]), (uint8_t)(leftWall.x + dx[p->GetDir()]) })
                            || m->isWall({ (uint8_t)(leftWall.y + dy[p->GetDir()] + dy[(p->GetDir()+1)%4] ) , (uint8_t)(leftWall.x + dx[p->GetDir()] + dx[(p->GetDir() + 1) % 4] ) }))
                        {
                            SDL_RenderDrawLine(theRenderer, finalLeftSide.x2, SCREEN_HEIGHT / 2 - smallerWallSize / 2, finalLeftSide.x2, SCREEN_HEIGHT / 2 + smallerWallSize / 2);
                        }
                    }
                }
                if (m->isWall(rightWall))
                {
                    std::cout << "WALL TO THE RIGHT";
                    int biggerWallSize = SCREEN_WIDTH / ((depth) * 2 + 1);
                    int smallerWallSize = (SCREEN_WIDTH / ((depth + 1) * 2 + 1));
                    screenpos rightSide = { smallerWallSize * (i+2 ),  biggerWallSize * (i + 1) -1};
                    screenpos finalRightSide = ManageWallDrawing(rightSide);

                    SDL_RenderDrawLine(theRenderer, finalRightSide.x1, SCREEN_HEIGHT / 2 - smallerWallSize / 2, finalRightSide.x2, SCREEN_HEIGHT / 2 - biggerWallSize / 2); //top
                    SDL_RenderDrawLine(theRenderer, finalRightSide.x1, SCREEN_HEIGHT / 2 + smallerWallSize / 2, finalRightSide.x2, SCREEN_HEIGHT / 2 + biggerWallSize / 2); //bottom
                
                    if (finalRightSide.x1 == rightSide.x1)
                    {
                        if (!m->isWall({ (uint8_t)(rightWall.y + dy[p->GetDir()]), (uint8_t)(rightWall.x + dx[p->GetDir()]) })
                            || m->isWall({ (uint8_t)(rightWall.y + dy[p->GetDir()] - dy[(p->GetDir() + 1) % 4]) , (uint8_t)(rightWall.x + dx[p->GetDir()] - dx[(p->GetDir() + 1) % 4]) }))
                        {
                            SDL_RenderDrawLine(theRenderer, finalRightSide.x1, SCREEN_HEIGHT / 2 - smallerWallSize / 2, finalRightSide.x1, SCREEN_HEIGHT / 2 + smallerWallSize / 2);
                        }
                    }
                }

            }




        }

        std::cout << "Increasing depth\n";
        depth++;
    }

    //SDL_RenderDrawPoint(theRenderer, 5, 5);
    //SDL_UpdateWindowSurface(window);
   



    SDL_RenderPresent(theRenderer);
}

int main(int argc, char* args[])
{
    Game* gameInstance = new Game();
    gameInstance->Start();

    return 0;
}

void Game::Start()
{


    //Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }
    else
    {
        //Create window
        window = SDL_CreateWindow("Tile Crawler", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (window == NULL)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        }
        else
        {
            theRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            RenderView();
            //Get window surface


            //Wait two seconds
            SDL_Delay(200000);
        }
    }
    //Destroy window
    SDL_DestroyWindow(window);

    //Quit SDL subsystems
    SDL_Quit();

}