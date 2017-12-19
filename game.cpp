// By Aidan Hunt, first created 24/1/17
// [Project description here]

// TODO
/*
MAJOR:
-- add controls
*/


// Included modules in the program
#include <stdio.h> // standard input/output for debugging
#include <stdlib.h> // constains several constants and random function

#include <SDL.h> // sdl library for graphics features
#undef main
#include <SDL_image.h> // sdl library for using PNGs and other image formats
#include <SDL_ttf.h>
#include <SDL_net.h> // sdl library used for multi computer networking
#include <SDL2_gfxPrimitives.h> // contains library for drawing arbitrary polygons

#include <string> // tools for string manipulations
#include <sstream>
#include <iostream> // contains cout output method
#include <fstream> // used to read in the config file
#include <regex> // used to interpret the config file
#include <math.h> // contains math functions (sqrt, pow, etc) for use
#include <algorithm> // contains min/max functions used
#include <forward_list> // container structure used to hold objects in game
#include <list> // stores objects during map creation
#include <map> // used to store settings during config reading
#include <ctime> // used to randomise the seed
#include <cassert> // used in drivers to test functions

#include "game.h" // contains program constants and definitions to avoid cluttering main file

using namespace std;

int main(int argc, char const *argv[]) {
    if (DEBUG_ENABLE_DRIVERS == true) {
        testDistBetweenPoints();
        testCheckExitMap();
        testGenerateRandInt();
        testGenerateRandDouble();
        testGetLength();
        testStrOfLen();
    }
    srand(time(NULL)); // initialize the seed

                       // control/important variables for throughout the program
    bool gameRunning = true; // variable to control the game loop
    bool inMenu = true;
    bool inLobby = false;
    bool paused = false;
    bool mousePressed = false;
    bool gameOver = false;
    int gameOverTimer = 0;

    SDL_Window* window = NULL; // window the game is displayed on, set in init function
    SDL_Renderer* renderer = NULL; // renderer for the window, set in init function
    SDL_Event eventHandler; // event handler for the game
    hudInfo hudInfoContainer; // stores elements needed for drawing the hud

    // store the start and end times of the frame
    int frameStart = 0;
    int frameEnd = 0;

    // store the type of message sent by the client
    int messageType = 1;

    // main storage structures
    forward_list<Wall*> wallContainer; // stores the game walls
    forward_list<coordSet> spawnPoints; // stores spawn point locations
    forward_list<Player*> playerList; // stores the players
    forward_list<Projectile*> projectileList; // stores the current set of projectiles
    forward_list<BulletExplosion*> explosionList; // stores explosions created by projectile collisions
    forward_list<Projectile*> newList; // temporarily store projectiles that have not collided
    forward_list<BulletExplosion*> explosionUpdated; // temporary store for active projectiles

    Game* game = new Game(&playerList, &wallContainer, &spawnPoints,
     &projectileList, &explosionList, &renderer, &window);

    init(&window, &renderer, game); // Initialize SDL and set the window and renderer for the game


    Menu* menu = new Menu(game); // create the menu screen
    game->setPatterns();
    game->setSockets();

    // load images needed for HUD drawing
    hudInfoContainer.ammoIcon = loadImage(HUD_AMMO_ICON_LOCATION, renderer);
    hudInfoContainer.cooldownIcon = loadImage(HUD_COOLDOWN_ICON_LOCATION, renderer);
    hudInfoContainer.pauseText = loadText(UI_PAUSE_TEXT, UI_FONT_SIZE, renderer);
    hudInfoContainer.winText = loadText(UI_WINCOND_TEXT, UI_FONT_SIZE, renderer);

    // create the text elements for the pause menu
    hudInfoContainer.resume = new Button(BUTTON_RESUME_TOPLEFT_X, BUTTON_RESUME_TOPLEFT_Y,
     BUTTON_RESUME_WIDTH, BUTTON_RESUME_HEIGHT, BUTTON_MENU, "resume", false, "Resume Game", game);
    hudInfoContainer.quit = new Button(BUTTON_QUIT_TOPLEFT_X, BUTTON_QUIT_TOPLEFT_Y,
     BUTTON_QUIT_WIDTH, BUTTON_QUIT_HEIGHT, BUTTON_MENU, "quit", false, "Quit Game", game);

    // load the image for the cursor
    SDL_Texture* gameCursor = loadImage(UI_GAME_CURSOR_LOCATION, renderer);

    // store the coordinates of the current games character
    int playerMainX = 0;
    int playerMainY = 0;


    while (gameRunning == true) { // begin game loop
        messageType = 1;
        frameStart = SDL_GetTicks();
        while (SDL_PollEvent(&eventHandler) != 0) { // check if the game is closed or paused
            if (eventHandler.type == SDL_QUIT) { // If the windows exit button is pressed
                gameRunning = false;
            } else if (eventHandler.type == SDL_KEYDOWN && inMenu == false) {
                // check for the escape key being pressed while ingame
                if (eventHandler.key.keysym.sym == SDLK_ESCAPE) {
                    if (paused == true) { // toggle the pause state
                        paused = false;
                    } else {
                        paused = true;
                    }
                }
            }
        }

        if (game->hosting() == false) {
            // recieve all inbound communications from the host
            while (messageType != MESSAGE_NONE) { // check if a message was recieved
                // recieve and perform action related to messages
                messageType = game->getInput(inLobby, inMenu);
                if (messageType == MESSAGE_QUIT) { // check if the game was quit
                    inMenu = true;
                    paused = false;
                } else if (messageType == MESSAGE_WALL) {
                    // when the host sends map data, start the game and reset the menu for the game end
                    inMenu = false;
                    inLobby = false;
                    menu->reset();
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, UI_COLOR_MAX_VALUE); // set the back of the entire page to black
        SDL_RenderClear(renderer); // clear the previous frame

        if (inMenu == true) { // perform menu actions
            int menuAction = MENU_NONE;
            if (inLobby == true) { // while in the lobby, set up the match
                if (game->hosting() == true) { // recieve inbound players as the host
                    game->recieveConnection();
                    game->sendLobby();
                } else {
                    if (game->isConnected() == false) {
                        // while not in a game, ping a join request to the host
                        game->attemptConnection(menu->getWeapon());
                    }
                }
            }
            // update the menu, and recieve any updates to it
            menuAction = menu->update(game);
            menu->render(game);
            if (menuAction == MENU_QUIT) { // quit the game if the quit button is pressed
                gameRunning = false;
            } else if (menuAction == MENU_LAUNCH) {
                // start the game if the host presses the launch button
                inMenu = false;
                inLobby = false;
                menu->reset();
                game->initialize(menu->getWeapon());
            } else if (menuAction == MENU_SET_LOBBY_CLIENT || menuAction == MENU_SET_LOBBY_HOST) {
                // enter the lobby if the play button is pressed
                inLobby = true;
            }

            if (inLobby == true && menuAction == MENU_SET_MAIN) {
                // leave the lobby if it is exited
                inLobby = false;
                if (game->hosting() == true) {
                    game->endSession();
                } else if (game->isConnected() == true) {
                    game->leaveMatch();
                }
            }
        } else { // if in the game, update the game
            if (game->hosting() == true) {
                // get keyboard inputs from clients
                bool haveMessage = true;
                while (haveMessage == true) { // check if the last check recieved a message
                    haveMessage = game->getClientAction();
                }
                if (game->getNumPlayers() == 1) {
                    // if all other players have left the match, and close if it has
                    cout << "All other players disconnected, exiting game\n";
                    game->endSession();
                    inMenu = true;
                    paused = false;
                }
                // update the state of the controlled character
                for (auto character = playerList.begin(); character != playerList.end(); character++) {
                    if ((*character)->getID() == game->getID()) { // check if the current player is the controlled one
                        (*character)->updateState(game, paused); // update the player
                        if (paused == false) {
                            mousePressed = false;
                        } else {
                            // update the pause menu if it is open
                            coordSet mouseCoords = getMouseCoordinates(game);
                            int mX = mouseCoords.x;
                            int mY = mouseCoords.y;
                            bool press;

                            // check if the mouse was pressed on this frame
                            bool mDown = SDL_GetMouseState(NULL, NULL) && SDL_BUTTON(SDL_BUTTON_LEFT);
                            if (mDown == true && mousePressed == false) {
                                // if the mouse was down this frame but not last
                                press = true;
                            } else {
                                press = false;
                            }
                            mousePressed = mDown;
                            if (hudInfoContainer.resume->mouseHover(mX, mY) == true) {
                                // check if the resume button is being hovered, and change color if it is
                                hudInfoContainer.resume->setActive(true);
                                if (press == true) {
                                    // if resume was pressed, close the pause menu
                                    paused = false;
                                }
                            } else {
                                // if not hovered, set resume to its normal state
                                hudInfoContainer.resume->setActive(false);
                            }

                            if (hudInfoContainer.quit->mouseHover(mX, mY) == true) {
                                hudInfoContainer.quit->setActive(true);
                                if (press == true) {
                                    // quit the game if the quit button was pressed
                                    gameOver = true;
                                }
                            } else {
                                hudInfoContainer.quit->setActive(false);
                            }
                        }
                    } else {
                        // get the keyboard state for the client player of a non-controlled player
                        //  and use it to update their player
                        (*character)->updateState(game->getActions((*character)->getID()), game);
                    }
                }

                // move all players based on their state
                for (auto character = playerList.begin(); character != playerList.end(); character++) {
                    (*character)->move(game->walls());
                }

                // update all projectiles
                newList.clear();
                for (auto bullet = projectileList.begin(); bullet != projectileList.end(); bullet++) {
                    if ((*bullet)->move(game) != true) { // store bullets still in flight
                        newList.push_front(*bullet);
                    } else { // delete and replace with explosions those that have collided
                        BulletExplosion* newExplosion = new BulletExplosion(renderer,
                         (*bullet)->getLocation(), (*bullet)->getColors(), (*bullet)->getOwner());
                        explosionList.push_front(newExplosion);
                        game->addNewExplosion(newExplosion); // put the new explosion in the list to send
                        delete *bullet;
                    }
                }
                projectileList = newList; // store remaining projectiles
                game->sendUpdate(); // send the current game state to the player
            } else {
                // update the game for the client computers
                game->sendUserActions(paused); // send the actions to the host
                if (paused == true) {
                    // update the pause screen if open
                    coordSet mouseCoords = getMouseCoordinates(game);
                    int mX = mouseCoords.x;
                    int mY = mouseCoords.y;
                    bool press;

                    // check if the mouse was pressed this frame
                    bool mDown = SDL_GetMouseState(NULL, NULL) && SDL_BUTTON(SDL_BUTTON_LEFT);
                    if (mDown == true && mousePressed == false) {
                        press = true;
                    } else {
                        press = false;
                    }
                    mousePressed = mDown;
                    if (hudInfoContainer.resume->mouseHover(mX, mY) == true) {
                        hudInfoContainer.resume->setActive(true);
                        if (press == true) {
                            // unpause the if pressed
                            paused = false;
                        }
                    } else {
                        hudInfoContainer.resume->setActive(false);
                    }

                    if (hudInfoContainer.quit->mouseHover(mX, mY) == true) {
                        hudInfoContainer.quit->setActive(true);
                        if (press == true) {
                            // quit the game if pressed
                            inMenu = true;
                            paused = false;
                            game->leaveMatch();
                        }
                    } else {
                        hudInfoContainer.quit->setActive(false);
                    }
                }
            }

            for (auto character = game->players()->begin(); character != game->players()->end(); character++) {
                if ((*character)->getID() == game->getID()) {
                    // find the controlled player and get its coordinates
                    playerMainX = (*character)->getX();
                    playerMainY = (*character)->getY();
                }
            }

            // update the explosion effects from bullets
            explosionUpdated.clear();
            for (auto explosion = explosionList.begin(); explosion != explosionList.end(); explosion++) {
                if ((*explosion)->updateState() == true) { // delete explosions that have decayed, and store ones that have not
                    delete *explosion;
                } else {
                    explosionUpdated.push_front(*explosion); // add explosions that are still existing to the temp
                }
            }
            explosionList = explosionUpdated; // set the list of active explosions to those that remain

            if (game->getWinner() != -1 && gameOver == false) {
                // if a winner exists, begin the game over
                gameOver = true;
                gameOverTimer = GAME_WIN_DELAY;
            }


            // render the game
            renderGameSpace(game, explosionList, playerMainX, playerMainY, game->getWinner()); // draw the gamespace
            for (auto character = playerList.begin(); character != playerList.end(); character++) {
                if ((*character)->getID() == game->getID()) {
                    renderGameUI(game, *character, hudInfoContainer, paused); // draw the HUD
                }
            }

            // draw the cursor to the screen
            SDL_Rect screen = { 0, 0, SCREEN_WIDTH_DEFAULT, SCREEN_HEIGHT_DEFAULT };
            SDL_RenderSetViewport(game->renderer(), &screen);
            
            coordSet mouseCoords = getMouseCoordinates(game);
            int x = mouseCoords.x;
            int y = mouseCoords.y;
            
            // set up to draw the cursor to the screen
            SDL_Rect cursorRect = { x - UI_CURSOR_WIDTH / 2, y - UI_CURSOR_HEIGHT / 2, UI_CURSOR_WIDTH, UI_CURSOR_HEIGHT };
            SDL_SetTextureColorMod(gameCursor, game->secondaryColors().red,
                game->secondaryColors().green, game->secondaryColors().blue);
            SDL_RenderCopy(renderer, gameCursor, NULL, &cursorRect);
        }
        // update the screen to the renderers current state after adding the elements to the renderer
        SDL_RenderPresent(renderer);

        if (gameOver == true) { // if the game is over
            if (gameOverTimer > 0) { // decrease the timer tracking time before returning to the menu
                gameOverTimer--;
            } else {
                // once the timer runs out, reset the game variables and return to the menu
                inMenu = true;
                paused = false;
                gameOver = false;
                if (game->hosting() == true) {
                    game->endSession();
                } else {
                    game->leaveMatch();
                }
            }
        }

        // check if the duration of the frame is less than that needed for the desired framerate
        frameEnd = SDL_GetTicks();
        if (frameEnd - frameStart < SCREEN_TICKRATE) {
            // if the frame has been to short, add a delay
            SDL_Delay(SCREEN_TICKRATE - (frameEnd - frameStart));
        }
    }

    if (game->hosting() == true) { // close any connections before exiting the game
        game->endSession();
    } else {
        game->leaveMatch();
    }

    quitGame(window, game, hudInfoContainer); // quit the game when the session finishes

    return EXIT_SUCCESS; // return success if the program terminates correctly
}


/* -------------------------- FUNCTIONS ------------------------- */

void quitGame(SDL_Window* window, Game* game, hudInfo hudInfoContainer) {
    SDL_DestroyWindow(window); // destroy the window

    // go through all game objects, clearing any memory used and destroying them
    for (auto character = game->players()->begin(); character != game->players()->end(); character++) {
        delete *character;
    }
    for (auto bullet = game->projectiles()->begin(); bullet != game->projectiles()->end(); bullet++) {
        delete* bullet;
    }
    for (auto wall = game->walls()->begin(); wall != game->walls()->end(); wall++) {
        delete *wall;
    }

    delete hudInfoContainer.resume;
    delete hudInfoContainer.quit;
    SDL_DestroyTexture(hudInfoContainer.pauseText);

    delete game;
    SDL_Quit();
    SDLNet_Quit();
    IMG_Quit();
}

bool init(SDL_Window** window, SDL_Renderer** renderer, Game* game) { // initialize important SDL functionalities
    bool success = true; // flag to check whether program runs successfully
    if (SDL_Init(SDL_INIT_VIDEO) < 0) { // Make sure SDL can initialize properly, otherwise return error code
        cout << "Error Initializing SDL./n SDL_Error " << SDL_GetError() << "\n";
        success = false;
    } else {
        *window = SDL_CreateWindow(SCREEN_NAME, SDL_WINDOWPOS_UNDEFINED, //create the window
            SDL_WINDOWPOS_UNDEFINED, game->screenWidth(), game->screenHeight(),
            SDL_WINDOW_SHOWN);
        if (window == NULL) { // check if cratewindow returned a valid pointer
            cout << "Error Creating Window./n SDL_Error " << SDL_GetError() << "\n";
            success = false;
        } else {
            if (game->isFullscreen() == true) { // set the game to fullscreen if required   
                SDL_SetWindowFullscreen(*window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                int w;
                int h;
                SDL_GetWindowSize(*window, &w, &h);
                game->setSize(w, h);
            }
            *renderer = SDL_CreateRenderer(*window, -1,
                SDL_RENDERER_ACCELERATED); // create renderer object
            if (renderer == NULL) { // if the renderer failed to initialize, return an error
                cout << "Renderer failed to initialize. SDL_Error" << SDL_GetError();
                success = false;
            } else {
                SDL_SetRenderDrawColor(*renderer, UI_COLOR_MAX_VALUE,
                    UI_COLOR_MAX_VALUE, UI_COLOR_MAX_VALUE, UI_COLOR_MAX_VALUE); // Give the renderer a default white state
                SDL_RenderSetLogicalSize(*renderer, SCREEN_WIDTH_DEFAULT, SCREEN_HEIGHT_DEFAULT);
                // set the logical render size, which automatically rescales anything in the given size to that of the window

                int imgFlags = IMG_INIT_PNG;
                if (!(IMG_Init(imgFlags) & imgFlags)) { // initialize SDL_image, returning an error if it fails
                    cout << "SDL_Image failed to initialize. Image Error" << IMG_GetError();
                    success = false;
                } else {
                    if (SDLNet_Init() < 0) { // initialize SDL_net, returning an error on failure
                        cout << "Error initializing SDLNet: " << SDLNet_GetError() << "\n";
                        success = false;
                    } else {
                        if (TTF_Init() < 0) {
                            cout << "Error initializing TTF: " << TTF_GetError() << "\n";
                            success = false;
                        }
                    }
                }
            }
        }
    }
    return success; // returns whether any errors occured during the process
}

SDL_Texture* loadImage(string path, SDL_Renderer* renderer) {
    // loads an image at the given filepath and returns its address in memory
    SDL_Texture* output = NULL;
    SDL_Surface* surfaceAtPath = IMG_Load(path.c_str()); // load the spritesheet as a surface
    if (surfaceAtPath == NULL) { //ensure the image loaded correctly
        cout << "Image failed to load\nSDL_image error " << SDL_GetError();
    } else {
        SDL_SetColorKey(surfaceAtPath, SDL_TRUE, SDL_MapRGB( // use color keying to remove the background from the image
            surfaceAtPath->format, COLOR_KEY_RED, COLOR_KEY_GREEN, COLOR_KEY_BLUE));
        output = SDL_CreateTextureFromSurface(renderer, surfaceAtPath); // convert the surface to a texture
        if (output == NULL) { // check the image converted correctly
            cout << "Surface failed conversion to texture.\nSDL_Error " << SDL_GetError();
        }
        SDL_SetTextureBlendMode(output, SDL_BLENDMODE_BLEND);
        SDL_FreeSurface(surfaceAtPath); // remove the surface now that it is no longer needed
    }
    return output;
}

SDL_Texture* loadText(string content, int size, SDL_Renderer* renderer) {
    // creates a texture of text content from the games font at text size size
    SDL_Texture* output = NULL;
    TTF_Font* font = TTF_OpenFont(UI_FONT_LOCATION.c_str(), size);
    if (font == NULL) {
        cout << "Error opening font: " << TTF_GetError() << "\n";
    } 
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, content.c_str(), {0,0,0}); // create the surface
    output = SDL_CreateTextureFromSurface(renderer, textSurface); // convert the surface to a texture
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);
    return output;
}


void renderGameSpace(Game*game, forward_list<BulletExplosion*> explosionList,
    int playerMainX, int playerMainY, int winner) {
    if (DEBUG_SHOW_CURSOR == false) {
        SDL_ShowCursor(SDL_DISABLE);
    }
    // renders the main area of the game

    /*
    Current render order:
    - Pattern
    - Character
    - Projectile
    - Shadow
    - Wall
    - Explosion
    */

    // reset the screen for the next frame
    SDL_Rect gamespaceViewport;
    gamespaceViewport.x = GAMESPACE_TOPLEFT_X;
    gamespaceViewport.y = GAMESPACE_TOPLEFT_Y;
    gamespaceViewport.w = GAMESPACE_WIDTH;
    gamespaceViewport.h = GAMESPACE_HEIGHT;

    SDL_RenderSetViewport(game->renderer(), &gamespaceViewport); // set the game viewport

    // fill the gamespace background in with the colour set
    SDL_SetRenderDrawColor(game->renderer(),
        game->primaryColors().red*UI_BACKGROUND_MULTIPLIER + UI_BACKGROUND_ADDITION,
        game->primaryColors().green*UI_BACKGROUND_MULTIPLIER + UI_BACKGROUND_ADDITION,
        game->primaryColors().blue*UI_BACKGROUND_MULTIPLIER + UI_BACKGROUND_ADDITION,
        UI_COLOR_MAX_VALUE);
    SDL_Rect background{ 0, 0, GAMESPACE_WIDTH, GAMESPACE_HEIGHT };
    SDL_RenderFillRect(game->renderer(), &background);

    // Draw the background pattern
    SDL_SetRenderDrawColor(game->renderer(), 0, 0, 0, UI_COLOR_MAX_VALUE);
    SDL_Rect patternRect;
    patternRect.w = UI_BACKGROUND_PATTERN_WIDTH;
    patternRect.h = UI_BACKGROUND_PATTERN_HEIGHT;
    for (int w = 0; w < UI_BACKGROUND_PATTERN_ROW; w++) {
        for (int h = 0; h < UI_BACKGROUND_PATTERN_COL; h++) {
            patternRect.x = w*UI_BACKGROUND_PATTERN_WIDTH;
            patternRect.y = h*UI_BACKGROUND_PATTERN_HEIGHT;
            SDL_RenderCopy(game->renderer(), game->pattern(w, h), NULL, &patternRect);
        }
    }

    // render all objects to the screen
    for (auto character = game->players()->begin(); character != game->players()->end(); character++) {
        (*character)->render(game);
    }

    for (auto bullet = game->projectiles()->begin(); bullet != game->projectiles()->end(); bullet++) {
        (*bullet)->render(game->renderer());
    }

    if (DEBUG_HIDE_SHADOWS != true) { // draws shadows by default unless debug settings are enabled
        for (auto wall = game->walls()->begin(); wall != game->walls()->end(); wall++) {
            (*wall)->renderShadow(playerMainX, playerMainY, game);
        }
    }

    for (auto wall = game->walls()->begin(); wall != game->walls()->end(); wall++) {
        (*wall)->render(game);
    }

    for (auto explosion = explosionList.begin(); explosion != explosionList.end(); explosion++) {
        (*explosion)->render(game->renderer());
    }

    if (DEBUG_DRAW_SPAWN_POINTS == true) { // if enabled in debug settings, draw points indicating spawpoints
        SDL_SetRenderDrawColor(game->renderer(), 100, 100, 100, UI_COLOR_MAX_VALUE);
        SDL_Rect debugRect;
        debugRect.w = 3;
        debugRect.h = 3;
        for (auto point = game->spawns()->begin(); point != game->spawns()->end(); point++) { // only draws active spawnpoints if enabled
            if (validateSpawnPoint(*point, game->players()) == true || DEBUG_DRAW_VALID_SPAWNS_ONLY == false) {
                debugRect.x = point->x;
                debugRect.y = point->y;
                SDL_RenderFillRect(game->renderer(), &debugRect);
            }
        }
    }

    if (winner != -1) { // if a winner exists, draw a banner preseting them
        SDL_Texture* winText = NULL;
        int textLen = 0;
        stringstream winString;
        string winLabel;
        for (auto player = game->players()->begin(); player != game->players()->end(); player++) {
            // go through each player and find the winner
            if ((*player)->getID() == winner) {
                // create a text label based on the winners username
                winString << game->getUsername(winner) << " wins";
                winLabel = winString.str();
                textLen = winLabel.length();
                winText = loadText(winLabel, UI_FONT_SIZE, game->renderer());
            }
        }
        SDL_Rect winDisplay;
        winDisplay.h = UI_WINDISPLAY_HEIGHT;
        winDisplay.y = (GAMESPACE_HEIGHT-winDisplay.h)/2;
        winDisplay.w = winDisplay.h*textLen*UI_FONT_HEIGHT_TO_WIDTH;
        winDisplay.x = (GAMESPACE_WIDTH-winDisplay.w)/2;

        // draw it to the centre of the screen
        SDL_RenderCopy(game->renderer(), winText, NULL, &winDisplay);
        SDL_DestroyTexture(winText);
    }

    if (DEBUG_DRAW_MOUSE_POINT == true) {
    // draw the mouse location in the gamespace frame to the screen if enabled in debug
        double ratio;
        int mouseX = 0;
        int mouseY = 0;
        if ((double)game->screenHeight() / (double)SCREEN_HEIGHT_DEFAULT <
            (double)game->screenWidth() / (double)SCREEN_WIDTH_DEFAULT) {
            ratio = (double)game->screenHeight() / (double)SCREEN_HEIGHT_DEFAULT;
        } else {
            ratio = (double)game->screenWidth() / (double)SCREEN_WIDTH_DEFAULT;
        }
        SDL_GetMouseState(&mouseX, &mouseY);
        mouseX -= GAMESPACE_TOPLEFT_X*ratio;
        mouseX /= ratio;
        mouseY /= ratio;
        SDL_SetRenderDrawColor(game->renderer(), UI_COLOR_MAX_VALUE, UI_COLOR_MAX_VALUE, UI_COLOR_MAX_VALUE, UI_COLOR_MAX_VALUE);
        SDL_Rect debugRect = { mouseX, mouseY, 10, 10 };
        SDL_RenderFillRect(game->renderer(), &debugRect);
    }

}

void renderGameUI(Game* game, Player* userCharacter, hudInfo hudInfoContainer, bool paused) {
    // draw the game HUD to the screen
    SDL_Rect elementRect; // rectangle object used to draw the different HUD boxes to the screen

    // set the HUD viewport, and fill it in with a background color
    SDL_Rect hudViewport;
    hudViewport.x = 0;
    hudViewport.y = 0;
    hudViewport.w = HUD_WIDTH;
    hudViewport.h = HUD_HEIGHT;
    SDL_RenderSetViewport(game->renderer(), &hudViewport);
    SDL_SetRenderDrawColor(game->renderer(),
        game->primaryColors().red*HUD_COLOR_SCALE,
        game->primaryColors().green*HUD_COLOR_SCALE,
        game->primaryColors().blue*HUD_COLOR_SCALE,
        UI_COLOR_MAX_VALUE);
    SDL_RenderFillRect(game->renderer(), &hudViewport);

    // draw ammo counter
    elementRect.w = HUD_AMMO_WIDTH;
    elementRect.h = HUD_AMMO_HEIGHT;
    elementRect.x = HUD_AMMO_TOPLEFT_X;
    elementRect.y = HUD_AMMO_TOPLEFT_Y;

    // draw the main box
    SDL_SetRenderDrawColor(game->renderer(),
        game->primaryColors().red*HUD_AMMO_BOX_COLOR_SCALE,
        game->primaryColors().green*HUD_AMMO_BOX_COLOR_SCALE,
        game->primaryColors().blue*HUD_AMMO_BOX_COLOR_SCALE,
        UI_COLOR_MAX_VALUE);
    SDL_RenderFillRect(game->renderer(), &elementRect);

    Weapon* userWeapon = userCharacter->getWeapon(); // store the users weapon used in referencing ammo

    // create a second bar over the first to show percent of ammo remaining
    SDL_SetRenderDrawColor(game->renderer(),
        game->primaryColors().red*HUD_AMMO_BAR_COLOR_SCALE,
        game->primaryColors().green*HUD_AMMO_BAR_COLOR_SCALE,
        game->primaryColors().blue*HUD_AMMO_BAR_COLOR_SCALE,
        UI_COLOR_MAX_VALUE);
    if (userWeapon->isReloading() == false) { // if not reloading, show the amount of ammo left relative to the maximum
        elementRect.w *= (double)userWeapon->getCurrAmmo() / userWeapon->getMaxAmmo();
    } else { // if the player is reloading, show how much time left before reload is finished
        elementRect.w *= 1 - (double)userWeapon->getReloadFrames() / userWeapon->getMaxReloadFrames();
    }
    SDL_RenderFillRect(game->renderer(), &elementRect);

    // draw the reload icon on top of the HUD element to indicate its use
    elementRect.w = HUD_AMMO_ICON_WIDTH;
    elementRect.h = HUD_AMMO_ICON_HEIGHT;
    elementRect.x = HUD_AMMO_ICON_TOPLEFT_X;
    elementRect.y = HUD_AMMO_ICON_TOPLEFT_Y;
    SDL_SetTextureAlphaMod(hudInfoContainer.ammoIcon, HUD_AMMO_ICON_ALPHA);
    SDL_SetTextureColorMod(hudInfoContainer.ammoIcon, game->primaryColors().red,
        game->primaryColors().green, game->primaryColors().blue);
    SDL_RenderCopy(game->renderer(), hudInfoContainer.ammoIcon, NULL, &elementRect);


    // draw roll cooldown bar
    elementRect.w = HUD_COOLDOWN_WIDTH;
    elementRect.h = HUD_COOLDOWN_HEIGHT;
    elementRect.x = HUD_COOLDOWN_TOPLEFT_X;
    elementRect.y = HUD_COOLDOWN_TOPLEFT_Y;

    SDL_SetRenderDrawColor(game->renderer(),
        game->secondaryColors().red*HUD_COOLDOWN_BOX_COLOR_SCALE,
        game->secondaryColors().green*HUD_COOLDOWN_BOX_COLOR_SCALE,
        game->secondaryColors().blue*HUD_COOLDOWN_BOX_COLOR_SCALE,
        UI_COLOR_MAX_VALUE);
    SDL_RenderFillRect(game->renderer(), &elementRect);

    // create a second bar over the first to show amount of cooldown remaining
    SDL_SetRenderDrawColor(game->renderer(),
        game->secondaryColors().red*HUD_COOLDOWN_BAR_COLOR_SCALE,
        game->secondaryColors().green*HUD_COOLDOWN_BAR_COLOR_SCALE,
        game->secondaryColors().blue*HUD_COOLDOWN_BAR_COLOR_SCALE,
        UI_COLOR_MAX_VALUE);

    elementRect.w *= 1 - userCharacter->getRollCooldown() / (double)CHARACTER_ROLL_COOLDOWN;
    SDL_RenderFillRect(game->renderer(), &elementRect);

    // draw the cooldown icon on top of the HUD element to indicate its use
    elementRect.w = HUD_COOLDOWN_ICON_WIDTH;
    elementRect.h = HUD_COOLDOWN_ICON_HEIGHT;
    elementRect.x = HUD_COOLDOWN_ICON_TOPLEFT_X;
    elementRect.y = HUD_COOLDOWN_ICON_TOPLEFT_Y;
    SDL_SetTextureAlphaMod(hudInfoContainer.cooldownIcon, HUD_COOLDOWN_ICON_ALPHA);
    SDL_SetTextureColorMod(hudInfoContainer.cooldownIcon, game->secondaryColors().red,
        game->secondaryColors().green, game->secondaryColors().blue);
    SDL_RenderCopy(game->renderer(), hudInfoContainer.cooldownIcon, NULL, &elementRect);



    // draw the health bar

    // draw the background box
    elementRect.w = HUD_HEALTH_WIDTH;
    elementRect.h = HUD_HEALTH_HEIGHT;
    elementRect.x = HUD_HEALTH_TOPLEFT_X;
    elementRect.y = HUD_HEALTH_TOPLEFT_Y;

    SDL_SetRenderDrawColor(game->renderer(), HUD_HEALTH_BOX_RED, HUD_HEALTH_BOX_BLUE, HUD_HEALTH_BOX_GREEN, UI_COLOR_MAX_VALUE);
    SDL_RenderFillRect(game->renderer(), &elementRect);

    // draw a scaled bar over the top to present hp left or respawn time remaining
    if (userCharacter->isAlive()) {
        elementRect.w *= (double)userCharacter->getHealth() / CHARACTER_MAX_HP;
    } else {
        elementRect.w *= 1 - (double)userCharacter->getDeathFrames() / CHARACTER_DEATH_DURATION;
    }
    SDL_SetRenderDrawColor(game->renderer(), HUD_HEALTH_BAR_RED, HUD_HEALTH_BAR_GREEN, HUD_HEALTH_BAR_BLUE, UI_COLOR_MAX_VALUE);
    SDL_RenderFillRect(game->renderer(), &elementRect);

    // draw divides that show the hp points of the player on the bar
    elementRect.w = HUD_HEALTH_DIVIDE_WIDTH;
    elementRect.h = HUD_HEALTH_DIVIDE_HEIGHT;
    elementRect.y = HUD_HEALTH_DIVIDE_TOPLEFT_Y;
    SDL_SetRenderDrawColor(game->renderer(), HUD_HEALTH_DIVIDE_RED,
        HUD_HEALTH_DIVIDE_BLUE, HUD_HEALTH_DIVIDE_GREEN, UI_COLOR_MAX_VALUE);
    for (int i = 1; i<CHARACTER_MAX_HP; i++) {
        elementRect.x = ((double)HUD_HEALTH_WIDTH / CHARACTER_MAX_HP)*i - ((double)elementRect.w / 2.0);
        SDL_RenderFillRect(game->renderer(), &elementRect);
    }

    // render the scores
    SDL_Rect name = {UI_SCORE_TOPLEFT_X, UI_SCORE_TOPLEFT_Y, 0, UI_SCORE_HEIGHT};
    SDL_Rect score = {0, name.y, 0, UI_SCORE_HEIGHT};
    SDL_Texture* currName;
    SDL_Texture* currScore;
    playerScore scoreboard[GAME_MAX_PLAYERS];
    int numScores = 0;

    scoreboard[0] = game->getPlayerScore(0); // set the first score
    for (int i=1; i < game->getNumPlayers(); i++) {
        // go through each other score and sort it into its position in the array by score
        playerScore sortScore = game->getPlayerScore(i);
        scoreboard[i] = sortScore;
        for (int s=numScores; s>=0; s--) {
            if (scoreboard[s].score < sortScore.score) {
                scoreboard[s+1] = scoreboard[s];
                scoreboard[s] = sortScore;
            }
        }
        numScores++;

    }

    for (int i=0; i<game->getNumPlayers(); i++) {
        // draw each scoreboard element to the screen
        playerScore scoreSet = scoreboard[i];
        if (scoreSet.id == game->getID()) {
            // check if the current score is that of the owner, and highlight if if it is
            SDL_Rect scoreHighlight = {0, name.y, HUD_WIDTH, name.h};
            SDL_SetRenderDrawColor(game->renderer(), game->secondaryColors().red,
             game->secondaryColors().green, game->secondaryColors().blue, UI_COLOR_MAX_VALUE);
            SDL_RenderFillRect(game->renderer(), &scoreHighlight); // draw a background
        }

        // create a texture based on the players username, and render it
        currName = loadText(scoreSet.name.c_str(), UI_FONT_SIZE, game->renderer());
        name.w = UI_SCORE_HEIGHT*scoreSet.name.length()*UI_FONT_HEIGHT_TO_WIDTH;
        SDL_RenderCopy(game->renderer(), currName, NULL, &name);

        // create a texture of the players score and render it to the right of the HUD
        currScore = loadText(to_string(scoreSet.score).c_str(), UI_FONT_SIZE, game->renderer());
        score.w = UI_SCORE_HEIGHT*getLength(scoreSet.score)*UI_FONT_HEIGHT_TO_WIDTH;
        score.x = HUD_WIDTH-(score.w+UI_SCORE_MARGIN);
        SDL_RenderCopy(game->renderer(), currScore, NULL, &score);

        // destroy the previous texture and move the render rects to the next position
        SDL_DestroyTexture(currName);
        SDL_DestroyTexture(currScore);
        name.y += UI_SCORE_HEIGHT + UI_SCORE_GAP;
        score.y += UI_SCORE_HEIGHT + UI_SCORE_GAP;
    }

    // draw the score limit
    SDL_Rect winCond = {UI_WINCOND_TOPLEFT_X, UI_WINCOND_TOPLEFT_Y, UI_WINCOND_WIDTH, UI_WINCOND_HEIGHT};
    SDL_RenderCopy(game->renderer(), hudInfoContainer.winText, NULL, &winCond);

    if (paused == true) {
        // draw the banner of the pause screen to the screen
        SDL_Rect pauseMenu = {0, 0, SCREEN_WIDTH_DEFAULT, SCREEN_HEIGHT_DEFAULT};
        SDL_RenderSetViewport(game->renderer(), &pauseMenu);
        SDL_SetRenderDrawColor(game->renderer(), game->secondaryColors().red*UI_PAUSE_COLOR_MUL,
         game->secondaryColors().green*UI_PAUSE_COLOR_MUL,
         game->secondaryColors().blue*UI_PAUSE_COLOR_MUL, UI_PAUSE_ALPHA);
        SDL_SetRenderDrawBlendMode(game->renderer(), SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(game->renderer(), &pauseMenu);

        SDL_Rect pauseText = {UI_PAUSE_TOPLEFT_X, UI_PAUSE_TOPLEFT_Y, UI_PAUSE_WIDTH, UI_PAUSE_HEIGHT};
        SDL_RenderCopy(game->renderer(), hudInfoContainer.pauseText, NULL, &pauseText);
        hudInfoContainer.resume->render(game);
        hudInfoContainer.quit->render(game);
    }
}

double distBetweenPoints(int x1, int y1, int x2, int y2) {
    // Uses pythagorous theorem to find distance between two points
    double output = 0.0;
    int diffX = x2 - x1;
    int diffY = y2 - y1;
    output = sqrt(pow(diffX, 2) + pow(diffY, 2));
    return output;
}

direction getDirections(void) {
    // get the current direction based off the keyboard inputs
    direction output = MOVE_NONE; // set the default output to no keys pressed
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL); // load the array of keyboard states

    // go through each key combination (A, SD, WD, etc) and see if the corresponding key
    // for a given case, if the required keys are pressed, set movement in that direction
    if (keyboardState[SDL_SCANCODE_W] && keyboardState[SDL_SCANCODE_A]) {
        output = MOVE_UP_LEFT;
    } else if (keyboardState[SDL_SCANCODE_W] && keyboardState[SDL_SCANCODE_D]) {
        output = MOVE_UP_RIGHT;
    } else if (keyboardState[SDL_SCANCODE_S] && keyboardState[SDL_SCANCODE_A]) {
        output = MOVE_DOWN_LEFT;
    } else if (keyboardState[SDL_SCANCODE_S] && keyboardState[SDL_SCANCODE_D]) {
        output = MOVE_DOWN_RIGHT;
    } else if (keyboardState[SDL_SCANCODE_A] && !keyboardState[SDL_SCANCODE_D]) {
        output = MOVE_LEFT;
    } else if (keyboardState[SDL_SCANCODE_D] && !keyboardState[SDL_SCANCODE_A]) {
        output = MOVE_RIGHT;
    } else if (keyboardState[SDL_SCANCODE_W] && !keyboardState[SDL_SCANCODE_S]) {
        output = MOVE_UP;
    } else if (keyboardState[SDL_SCANCODE_S] && !keyboardState[SDL_SCANCODE_W]) {
        output = MOVE_DOWN;
    }

    // return the direction of movement
    return output;
}

int getInterceptX(int x1, int y1, int x2, int y2, int interceptY) {
    // get the x intercept of a coordinate to a line between two points, given the points y position
    int output = 0;
    double m = (double)(y1 - y2) / (x1 - x2); // find the gradient of the line
    output = (double)(interceptY - y1) / m + x1; // calculate the x-value
    return output;
}

int getInterceptY(int x1, int y1, int x2, int y2, int interceptX) {
    // get the y intercept of a coordinate to a line between two points, given its x position
    int output = 0;
    double m = (double)(x1 - x2) / (y1 - y2); // find the gradient
    output = (double)(interceptX - x1) / m + y1; // caluculate the y-value
    return output;
}

bool checkExitMap(int x, int y, int r) {
    // check if a circle is contained inside the game space (relative to screen width)
    // returns true if the circle is outside
    return y - r < 0
        || x - r < 0
        || y + r > GAMESPACE_HEIGHT
        || x + r > GAMESPACE_WIDTH;
}

double generateRandDouble(double min, double max) {
    // generate a random double between min and max
    double roll = (double)rand() / RAND_MAX;
    roll = min + roll*(max - min);
    return roll;
}

int generateRandInt(int min, int max) {
    // generate a random int between min and max
    int roll;
    roll = (rand() % (max - min + 1)) + min;
    return roll;
}

coordSet getSpawnPoint(forward_list<coordSet>* spawnPoints, forward_list<Player*>* playerList) {
    // selects a spawn point from all valid spawn points (not within a certain range of a player)
    // returns 0,0 if no points are valid, meaning the player will wait until a point becomes available
    list<coordSet> validPoints;
    coordSet output = { 0, 0 };
    bool valid;
    int chosenPoint;
    int index;

    // go through each spawn point, adding it to the list of valid spawn points if it is found to be usable
    for (auto point = spawnPoints->begin(); point != spawnPoints->end(); point++) {
        valid = validateSpawnPoint(*point, playerList);
        if (valid == true) {
            validPoints.push_front(*point);
        }
    }

    if (validPoints.size() > 0) { // check at least 1 spawn point is available
        chosenPoint = generateRandInt(0, validPoints.size() - 1);
        index = 0;
        // loop through the spawn points until the point at the chosen index is reached
        for (auto point = validPoints.begin(); point != validPoints.end(); point++) {
            if (index == chosenPoint) {
                output = *point;
            }
            index++;
        }
    }
    return output;
}

bool validateSpawnPoint(coordSet point, forward_list<Player*>* playerList) {
    // check if a spawn point is valid (not within CHARACTER_MIN_RESPAWN_RANGE of a player)
    bool valid = true;
    for (auto player = playerList->begin(); player != playerList->end(); player++) {
        if (distBetweenPoints(point.x, point.y, (*player)->getX(),
            (*player)->getY()) < CHARACTER_MIN_RESPAWN_RANGE && (*player)->isAlive() == true) {
            valid = false;
        }
    }
    return valid;
}

string strOfLen(int number, int len) {
    // converts number into a string of length len (leading 0s)
    stringstream output;
    for (int i = 0; i < len-getLength(number); i++) {
        output << "0";
    }
    output << to_string(number);
    return output.str();
}

int getLength(int number) {
    // returns the number of digits in an int
    // Returns 1 for 0 (specific for usage), does not consider negatives
    int length;
    if (number == 0) {
        length = 1;
    } else {
        for (int l = 1; number != 0; l++) {
            number /= 10;
            length = l;
        }
    }
    return length;
}

int sizeOfProj(forward_list<Projectile*>* list) {
    // get the number of elements in the given projectile forward list
    int size = 0;
    for (auto proj = list->begin(); proj != list->end(); proj++) {
        size++;
    }
    return size;
}

coordSet getMouseCoordinates(Game* game) {
    // get the coordinates of the mouse, scaled to the ingame screen resolution
    coordSet coords;
    SDL_GetMouseState(&coords.x, &coords.y);
    double ratio;
    if ((double)game->screenHeight() / SCREEN_HEIGHT_DEFAULT <
        (double)game->screenWidth() / SCREEN_WIDTH_DEFAULT) {
        ratio = (double)game->screenHeight() / SCREEN_HEIGHT_DEFAULT;
    } else {
        ratio = (double)game->screenWidth() / SCREEN_WIDTH_DEFAULT;
    }
    coords.x /= ratio;
    coords.y /= ratio;
    return coords;
}

/* -------------------------- CLASSES --------------------------- */


// Function definitions for each class

Game::Game(forward_list<Player*>* playerSet, forward_list<Wall*>* wallSet,
     forward_list<coordSet>* spawnSet, forward_list<Projectile*>* projSet,
     forward_list<BulletExplosion*>* explList, SDL_Renderer** renderer,
     SDL_Window** window) {
    // initializes the Game object

    // set all the parameters to their required values from input
    playerList = playerSet;
    wallContainer = wallSet;
    spawnPoints = spawnSet;
    projectileList = projSet;
    explosionList = explList;
    gameRenderer = renderer;
    gameWindow = window;

    // generate the pattern board
    for (int x = 0; x < UI_BACKGROUND_PATTERN_ROW; x++) {
        for (int y = 0; y < UI_BACKGROUND_PATTERN_COL; y++) {
            patternBoard[x][y] = generateRandInt(0, UI_BACKGROUND_PATTERN_COUNT - 1);
        }
    }


    // initialize variables used to read the config file
    string line;
    string item;
    string value;
    ifstream configFile(CONFIG_FILE_LOCATION, ifstream::in); // open the config file

    lastConnect = 0; // stores the number of milliseconds into the game the last attempted connection was made at
    winner = -1; // store the game winner if one is found

    // store the previous mouse coordinates
    lastMX = 0;
    lastMY = 0;

    regex pattern("([a-zA-Z]+)=([a-zA-Z0-9\\.]+)"); // create a regex used to extract data from each line
    smatch matches; // datatype to store the regex matches
    map<string, string> configElements; // map used to store the value of each data item

    if (configFile.is_open()) { // check the config file opened correctly
        while (getline(configFile, line)) { // for each line in the config file, store the data item and its value in the map
            regex_search(line, matches, pattern);
            configElements[matches[1]] = matches[2];
        }
    }

    // for each setting needed from the config file, set its value to that found. If none was found, set it to the default
    // value in the constants file
    if (configElements[CONFIG_SWIDTH] != "") {
        swidth = stoi(configElements[CONFIG_SWIDTH]);
        swidthActual = stoi(configElements[CONFIG_SWIDTH]);
    } else {
        swidth = SCREEN_WIDTH;
        swidthActual = SCREEN_WIDTH;
    }

    if (configElements[CONFIG_SHEIGHT] != "") {
        sheight = stoi(configElements[CONFIG_SHEIGHT]);
        sheightActual = stoi(configElements[CONFIG_SHEIGHT]);
    } else {
        sheight = SCREEN_HEIGHT;
        sheightActual = SCREEN_HEIGHT;
    }

    if (configElements[CONFIG_FULLSCREEN] != "") {
        fullscreen = stoi(configElements[CONFIG_FULLSCREEN]);
    } else {
        fullscreen = SCREEN_FULLSCREEN;
    }

    if (configElements[CONFIG_ISHOST] != "") {
        isHost = stoi(configElements[CONFIG_ISHOST]);
    } else {
        isHost = DEBUG_IS_HOST;
    }

    if (configElements[CONFIG_HOSTIP] != "") {
        hostIp = configElements[CONFIG_HOSTIP];
    } else {
        hostIp = DEBUG_HOST_IP;
    }

    if (configElements[CONFIG_HOSTPORT] != "") {
        hostPort = stoi(configElements[CONFIG_HOSTPORT]);
    } else {
        hostPort = DEBUG_HOST_PORT;
    }

    if (configElements[CONFIG_CLIENTPORT] != "") {
        clientPort = stoi(configElements[CONFIG_CLIENTPORT]);
    } else {
        clientPort = DEBUG_CLIENT_PORT;
    }

    string primColor = configElements[CONFIG_PRIMCOLOR];
    if (primColor != "") {
        if (COLOR_VALUES[primColor] != COLOR_NONE) {
            primaryColor = COLOR_VALUES[primColor];
            primary = primColor;
        } else {
            primaryColor = COLOR_VALUES[COLOR_BLUE_NAME];
            primary = COLOR_BLUE_NAME;
        }
    } else {
        primaryColor = COLOR_VALUES[COLOR_BLUE_NAME];
        primary = COLOR_BLUE_NAME;
    }

    string secColor = configElements[CONFIG_SECCOLOR];
    if (secColor != "") {
        if (COLOR_VALUES[secColor] != COLOR_NONE) {
            secondaryColor = COLOR_VALUES[secColor];
            secondary = secColor;
        } else {
            secondaryColor = COLOR_VALUES[COLOR_RED_NAME];
            secondary = COLOR_RED_NAME;
        }
    } else {
        secondaryColor = COLOR_VALUES[COLOR_RED_NAME];
        secondary = COLOR_RED_NAME;
    }

    string usrname = configElements[CONFIG_USERNAME];
    if (usrname != "") {
        username = configElements[CONFIG_USERNAME];
    } else {
        username = GAME_DEFAULT_USERNAME;
    }
    if (username.length() > GAME_USERNAME_MAX_LEN) {
        username = username.substr(0, GAME_USERNAME_MAX_LEN);
    }

    configFile.close();
}

Game::~Game(void) {
    // deconstructor for the game
    for (int p = 0; p < UI_BACKGROUND_PATTERN_COUNT; p++) {
        SDL_DestroyTexture(patterns[p]);
    }
    delete server;
}

void Game::setSockets(void) {
    // create the networking classes and other objects used in networking
    numPlayers = 1;
    currPlayers[0] = {0, 0, CHARACTER_WEAPON_PISTOL, CHARACTER_MAIN_ID, username}; // set a default character for the host
    nextID = 1;
    myID = CHARACTER_MAIN_ID;
    nextID = myID + 1;
    server = new UDPConnectionServer(this);
    client = new UDPConnectionClient(this);
}

void Game::recieveConnection(void) {
    // recieve inbound connection requests from clients as host
    int messageType = MESSAGE_NONE;
    string inboundString;
    connection inboundConnection = server->recieve(&inboundString); // get the connection
    if (inboundConnection.ip != 0) { // check a message was recieved
        messageType = inboundString[0] - '0'; // determine the type of message recieved
        if (messageType == MESSAGE_CONF_CONNECTION) { // check the message was requesting to join
            if (numPlayers < GAME_MAX_PLAYERS) { // check that the game can recieve another connection
                bool match = false;
                // go through current connections and check if the player has joined already
                for (int i = 0; i < numPlayers; i++) {
                    if (inboundConnection == currPlayers[i]) {
                        match = true;
                    }
                }
                if (match == false) { // if the player wasn't in the lobby previously
                    // set the next lobby spot to the new player
                    currPlayers[numPlayers] = inboundConnection;
                    currPlayers[numPlayers].id = nextID;
                    currPlayers[numPlayers].weapon = stoi(inboundString.substr(1, 1));
                    // search through the remainder of the message to recieve the characters username
                    int index = 2;
                    stringstream name;
                    while (inboundString[index] != 0) {
                        name << inboundString[index];
                        index++;
                    }
                    currPlayers[numPlayers].username = name.str();

                    // increase the number of players connected
                    numPlayers++;
                    stringstream message; // reply to the new connection with their id in the game
                    message << MESSAGE_CONF_CONNECTION << strOfLen(nextID, MESSAGE_ID_CHAR);
                    server->send(message.str(), MESSAGE_RESEND_TIMES, inboundConnection);
                    nextID++; // increment the next id
                }
            } 
        } else if (messageType == MESSAGE_QUIT) { // if a player in the lobby tries to quit, remove them
			removePlayer(inboundConnection);
		}
    }
}

void Game::sendLobby(void) {
    // send the current players in the lobby to all clients
    stringstream players;
    players << MESSAGE_LOBBY;
    for (int i = 0; i < numPlayers; i++) {
        // send a string with each players id and username
        players << strOfLen(currPlayers[i].id, MESSAGE_ID_CHAR) << currPlayers[i].username << "|";
    }
    server->send(players.str(),MESSAGE_RESEND_TIMES, currPlayers, numPlayers);
}

void Game::attemptConnection(int weapon) {
    // request a connection to a host
    if (SDL_GetTicks() - lastConnect > GAME_CONNECT_WAIT) { // check the minimum time between requests has been met
        stringstream message;
        message << MESSAGE_CONF_CONNECTION << weapon << username;
        client->send(message.str(), 1);
        lastConnect = SDL_GetTicks();
    }
}

void Game::endSession(void) {
    // tell clients that the host is no longer playing
    server->send(to_string(MESSAGE_QUIT), MESSAGE_RESEND_TIMES, currPlayers, numPlayers);

    // reset game variables
    numPlayers = 1;
    nextID = 1;
    winner = -1;
    connected = false;

    // clear all the game objects now the game is over
    for (auto player = playerList->begin(); player != playerList->end(); player++) {
        delete *player;
    }
    for (auto bullet = projectileList->begin(); bullet != projectileList->end(); bullet++) {
        delete *bullet;
    }
    for (auto wall = wallContainer->begin(); wall != wallContainer->end(); wall++) {
        delete *wall;
    }
    for (auto expl = explosionList->begin(); expl != explosionList->end(); expl++) {
        delete *expl;
    }
    playerList->clear();
    projectileList->clear();
    wallContainer->clear();
    explosionList->clear();
    spawnPoints->clear();
}

void Game::leaveMatch(void) {
    // tell the host the client is leaving the match
    client->send(to_string(MESSAGE_QUIT), MESSAGE_RESEND_TIMES);

    // reset game variables
    numPlayers = 0;
    winner = -1;
    connected = false;


    // delete the game objects
    for (auto player = playerList->begin(); player != playerList->end(); player++) {
        delete *player;
    }
    for (auto bullet = projectileList->begin(); bullet != projectileList->end(); bullet++) {
        delete *bullet;
    }
    for (auto wall = wallContainer->begin(); wall != wallContainer->end(); wall++) {
        delete *wall;
    }
    for (auto expl = explosionList->begin(); expl != explosionList->end(); expl++) {
        delete *expl;
    }
    playerList->clear();
    projectileList->clear();
    wallContainer->clear();
    explosionList->clear();
    spawnPoints->clear();
}

void Game::initialize(int weapon) {
    // starts a new game instance

    // set the host to use their chosen weapon
    currPlayers[0].weapon = weapon;
    currPlayers[0].id = CHARACTER_MAIN_ID;
    generateMap(wallContainer, spawnPoints); // generate a random set of walls and spawn points for the game
    coordSet initSpawn; // temporarily stores spawn points for each player
    playerList->clear();
    for (int i = 0; i<numPlayers; i++) {
        // create each player based off each users id and weapon
        initSpawn = getSpawnPoint(spawnPoints, playerList); // set a spawn point for each player
        playerList->push_front(new Player(this, initSpawn.x, initSpawn.y,
         currPlayers[i].id, currPlayers[i].weapon));
        if (initSpawn.x == 0) {
            // if no valid spawn point was found (i.e. all points to close to players), kill them
            playerList->front()->killPlayer();
        }
    }
    // send the map and playerlist to each client to tell the game has started
    server->send(getMapString(), MESSAGE_RESEND_TIMES, currPlayers, numPlayers);
    server->send(getPlayerString(), MESSAGE_RESEND_TIMES, currPlayers, numPlayers);
}

void Game::sendUpdate(void) {
    // send an update of each game element to the players
    server->send(getPlayerString(), 1, currPlayers, numPlayers);
    server->send(getProjectileString(), 1, currPlayers, numPlayers);
    server->send(getExplosionString(), 1, currPlayers, numPlayers);
}

void Game::addNewExplosion(BulletExplosion* newExplosion) {
    // add a new bullet explosion to the list of explosions
    explosionsToSend.push_front(newExplosion);
}

void Game::sendUserActions(bool paused) {
    // send the keyboard actions of the client to the host
    direction currDirection;
    if (paused == true) { // check the user is able move
        currDirection = MOVE_NONE;
    } else {
        currDirection = getDirections();
    }
    int x, y;
    bool mouseDown;

    if (paused == false) { // check the user can udpate their direction
        double ratio;
        mouseDown = SDL_GetMouseState(&x, &y) && SDL_BUTTON(SDL_BUTTON_LEFT);
        if ((double)sheightActual / SCREEN_HEIGHT_DEFAULT < (double)swidthActual / SCREEN_WIDTH_DEFAULT) {
            ratio = (double)sheightActual / SCREEN_HEIGHT_DEFAULT;
        } else {
            ratio = (double)swidthActual / SCREEN_WIDTH_DEFAULT;
        }
        x -= GAMESPACE_TOPLEFT_X*ratio;
        x /= ratio;
        y /= ratio;

        if (x < 0) {
            x = lastMX;
        }
        if (y < 0) {
            y = lastMY;
        }

        // store the mouse coordinates for this frame incase they are not valid next frame
        lastMX = x;
        lastMY = y;
    } else {
        // if the player is paused, lock the mouse at the coordinates for the last valid frame
        x = lastMX;
        y = lastMY;
        mouseDown = false;
    }

    bool rDown;
    bool shiftDown;
    if (paused == false) { // check if the player can make actions
        const Uint8* keyboard = SDL_GetKeyboardState(NULL);
        rDown = keyboard[SDL_SCANCODE_R];
        shiftDown = keyboard[SDL_SCANCODE_LSHIFT];
    } else {
        rDown = false;
        shiftDown = false;
    }

    // send the data to the host
    stringstream toServer;
    toServer << MESSAGE_CLIENT_ACTION;
    toServer << strOfLen(myID, MESSAGE_ID_CHAR);
    toServer << strOfLen(x, MESSAGE_LOC_CHAR) << strOfLen(y, MESSAGE_LOC_CHAR);
    toServer << mouseDown << rDown << shiftDown;
    toServer << currDirection.x + 2 << currDirection.y + 2;

    client->send(toServer.str(), 1);
}

bool Game::getClientAction(void) {
    // recieve client keyboard input
    bool message = false;
    string serverString;
    connection msg = server->recieve(&serverString);
    if (msg.ip != 0) { // check if a message was recieved
        int messageType = serverString[0] - '0'; // convert the number as ASCII to decimal
        if (messageType == MESSAGE_CLIENT_ACTION) { // ncheck the message involves a player action
            // update the action map
            userAction action;
            // read in the data from the input string
            int id = stoi(serverString.substr(1, MESSAGE_ID_CHAR));
            action.mouseX = stoi(serverString.substr(1+MESSAGE_ID_CHAR, MESSAGE_LOC_CHAR));
            action.mouseY = stoi(serverString.substr(1+MESSAGE_ID_CHAR+MESSAGE_LOC_CHAR,
             MESSAGE_LOC_CHAR));

            action.mouseDown = stoi(serverString.substr(1+MESSAGE_ID_CHAR+MESSAGE_LOC_CHAR*2, 1));
            action.rDown = stoi(serverString.substr(2+MESSAGE_ID_CHAR+MESSAGE_LOC_CHAR*2, 1));
            action.shiftDown = stoi(serverString.substr(3+MESSAGE_ID_CHAR+MESSAGE_LOC_CHAR*2, 1));

            action.moveHor = stoi(serverString.substr(4+MESSAGE_ID_CHAR+MESSAGE_LOC_CHAR*2, 1))-2;
            action.moveVert = stoi(serverString.substr(5+MESSAGE_ID_CHAR+MESSAGE_LOC_CHAR*2, 1))-2;

            clientActions[id] = action;
            message = true;
        } else if (messageType == MESSAGE_QUIT) {
            // remove the player if they quit the game
            removePlayer(msg);
        }
    }
    return message;
}

int Game::getInput(bool inLobby, bool inMenu) {
    // get messages from the host as client
    int messageType = MESSAGE_NONE;
    string serverString;
    connection msg = client->recieve(&serverString);
    if (msg.ip != 0) { // check a message was recieved
        messageType = serverString[0] - '0'; // convert the number as ASCII to decimal
        if (messageType == MESSAGE_CONF_CONNECTION) { // check if the host confirmed the connection
            if (inLobby == true) {
                // set the instance id to that recieved
                myID = stoi(serverString.substr(1, MESSAGE_ID_CHAR));
                cout << "Connection established as player " << myID << "\n";
                connected = true;
            } else { // if the player wasn't looking for a match, leave it
                leaveMatch();
                connected = false;
            }
        // find the type of message recieved and update that part of the game
        } else if (messageType == MESSAGE_WALL) {
            updateMap(serverString);
        } else if (messageType == MESSAGE_PLAYER) {
            updatePlayers(serverString);
        } else if (messageType == MESSAGE_PROJECTILE) {
            updateProjectiles(serverString);
        } else if (messageType == MESSAGE_EXPLOSION) {
            updateExplosions(serverString);
        } else if (messageType == MESSAGE_QUIT) {
            leaveMatch();
        } else if (messageType == MESSAGE_LOBBY) {
            if (inLobby == true || inMenu == false) { // check if the player is able to update lobby
                updateLobby(serverString);
            } else {
                connected = false;
            }
        }
    }
    return messageType;
}

void Game::removePlayer(connection playerip) {
    // remove a player from the match
    int playerID = -1;
    for (int i = 0; i < numPlayers; i++) {
        // find the connection of the player
        if (currPlayers[i] == playerip && i < numPlayers) {
            playerID = currPlayers[i].id;
        }
        if (playerID >= 0) { // if the connection has been found, move all tailing connections forward
            if (i < GAME_MAX_PLAYERS-1) {
                currPlayers[i] = currPlayers[i+1];
            }
        }
    }

    if (playerID >= 0) { // if the player to remove was in the match, delete their player object
        numPlayers--;
        // create a temp player list and clear the main one
        forward_list<Player*> playerTemp = *playerList;
        playerList->clear();
        for (auto player = playerTemp.begin(); player != playerTemp.end(); player++) {
            if ((*player)->getID() == playerID) {
                // delete the player once found
                delete *player;
            } else {
                // re-add the player if they are not to be deleted
                playerList->push_front(*player);
            }
        }
        cout << "Player " << playerID << " quit\n";
    }

    sendLobby(); // update the lobby for all players
}

void Game::updateMap(string serverString) {
    // update the map based off the input string

    // reset all main player objects for the new game
    wallContainer->clear();
    playerList->clear();
    projectileList->clear();

    int x, y, w, h;
    for (int wall = 0; wall < (serverString.length()-1)/(MESSAGE_LOC_CHAR*MAPBOX_NUM_CORNERS); wall++) {
        // go through each wall instance, read the required variables and create a wall based off of the data
        x = stoi(serverString.substr(wall*(MESSAGE_LOC_CHAR*MAPBOX_NUM_CORNERS)+1, MESSAGE_LOC_CHAR));
        y = stoi(serverString.substr(wall*(MESSAGE_LOC_CHAR*MAPBOX_NUM_CORNERS)+MESSAGE_LOC_CHAR+1, MESSAGE_LOC_CHAR));
        w = stoi(serverString.substr(wall*(MESSAGE_LOC_CHAR*MAPBOX_NUM_CORNERS)+MESSAGE_LOC_CHAR*2+1, MESSAGE_LOC_CHAR));
        h = stoi(serverString.substr(wall*(MESSAGE_LOC_CHAR*MAPBOX_NUM_CORNERS)+MESSAGE_LOC_CHAR*3+1, MESSAGE_LOC_CHAR));
        wallContainer->push_front(new Wall(x, y, w, h));
    }
}

void Game::updatePlayers(string serverString) {
    // update all players based of the input string
    int currIndex = 1; // start after the indentifying character
    playerState currState;
    bool found;

    int foundIDs[GAME_MAX_PLAYERS];
    for (int pIndex = 0; pIndex < (serverString.length()-1)/MESSAGE_PLAYER_LEN; pIndex++) {
        // read the data from the string
        /* format:
         x, y, angle, rolling state, roll frames, roll dirx, roll diry, invuln state, invuln frames,
         alive state, health, death frames, deathmarkerX, deathmarkerY, id, weapon type, score, ammo, reload frames, cooldown frames
        */
        currState.x = stoi(serverString.substr(currIndex, MESSAGE_LOC_CHAR));
        currIndex += MESSAGE_LOC_CHAR;
        currState.y = stoi(serverString.substr(currIndex, MESSAGE_LOC_CHAR));
        currIndex += MESSAGE_LOC_CHAR;
        currState.angle = stoi(serverString.substr(currIndex, MESSAGE_ANGLE_CHAR));
        currIndex += MESSAGE_ANGLE_CHAR;

        currState.rolling = stoi(serverString.substr(currIndex, 1));
        currIndex++;
        currState.rollFrames = stoi(serverString.substr(currIndex, MESSAGE_FRAMES_CHAR));
        currIndex += MESSAGE_FRAMES_CHAR;
        currState.rollX = stoi(serverString.substr(currIndex, 1))-2;
        currIndex++;
        currState.rollY = stoi(serverString.substr(currIndex, 1))-2;
        currIndex++;

        currState.invuln = stoi(serverString.substr(currIndex, 1));
        currIndex++;
        currState.invulnFrames = stoi(serverString.substr(currIndex, MESSAGE_FRAMES_CHAR));
        currIndex += MESSAGE_FRAMES_CHAR;
        currState.alive = stoi(serverString.substr(currIndex, 1));
        currIndex++;
        currState.health = stoi(serverString.substr(currIndex, MESSAGE_STAT_CHAR));
        currIndex += MESSAGE_STAT_CHAR;

        currState.deathFrames = stoi(serverString.substr(currIndex, MESSAGE_FRAMES_CHAR));
        currIndex += MESSAGE_FRAMES_CHAR;
        currState.dmX = stoi(serverString.substr(currIndex, MESSAGE_LOC_CHAR));
        currIndex += MESSAGE_LOC_CHAR;
        currState.dmY = stoi(serverString.substr(currIndex, MESSAGE_LOC_CHAR));
        currIndex += MESSAGE_LOC_CHAR;

        currState.id = stoi(serverString.substr(currIndex, MESSAGE_ID_CHAR));
        currIndex += MESSAGE_ID_CHAR;
        currState.weapID = stoi(serverString.substr(currIndex, 1));
        currIndex++;
        currState.score = stoi(serverString.substr(currIndex, MESSAGE_STAT_CHAR));
        currIndex += MESSAGE_STAT_CHAR;
        currState.ammo = stoi(serverString.substr(currIndex, MESSAGE_STAT_CHAR));
        currIndex += MESSAGE_STAT_CHAR;
        currState.reloadFrames = stoi(serverString.substr(currIndex, MESSAGE_FRAMES_CHAR));
        currIndex += MESSAGE_FRAMES_CHAR;
        currState.cooldownFrames = stoi(serverString.substr(currIndex, MESSAGE_FRAMES_CHAR));
        currIndex += MESSAGE_FRAMES_CHAR;

        foundIDs[pIndex] = currState.id; // add the id found to the array of identified ids

        found = false;
        for (auto player = playerList->begin(); player != playerList->end(); player++) {
            if ((*player)->getID() == currState.id) {
                // check if the player of the id was already in existance, and update if they were
                found = true;
                (*player)->updateState(currState);
            }
        }
        if (found == false) {
            // if the player had not been created, create them
            playerList->push_front(new Player(this, currState.x, currState.y,
             currState.id, currState.weapID));
        }
    }

    // go through each player, and delete them if no update was made to them
    forward_list<Player*> temp;
    for (auto player = playerList->begin(); player != playerList->end(); player++) {
        bool match = false;
        for (int i = 0; i < GAME_MAX_PLAYERS; i++) {
            if (foundIDs[i] == (*player)->getID()) {
                match = true;
            }
        }
        if (match == true) {
            temp.push_front(*player);
        } else {
            delete *player;
        }
    }
    *playerList = temp;
}

void Game::updateProjectiles(string serverString) {
    // update each projectile
    // updates existing projectiles where possible to avoid a slow process from occuring

    // find the number of projectiles currently active
    int numProj = (serverString.length()-1)/(2*MESSAGE_LOC_CHAR+MESSAGE_ID_CHAR);
    int moddedProj = 0;
    int x, y, id;
    for (int bullet = 0; bullet < numProj; bullet++) { 
        // for each projectile, get the required data from the string
        x = stoi(serverString.substr(1+(2*MESSAGE_LOC_CHAR+MESSAGE_ID_CHAR)*bullet, MESSAGE_LOC_CHAR));
        y = stoi(serverString.substr(1+MESSAGE_LOC_CHAR+(2*MESSAGE_LOC_CHAR+MESSAGE_ID_CHAR)*bullet, MESSAGE_LOC_CHAR));
        id = stoi(serverString.substr(1+MESSAGE_LOC_CHAR*2+(2*MESSAGE_LOC_CHAR+MESSAGE_ID_CHAR)*bullet, MESSAGE_ID_CHAR));
        if (moddedProj < sizeOfProj(projectileList)) { 
            // check if number of projectiles updated with new data has required the maximum possible
            int index = 0;
            for (auto toMod = projectileList->begin(); toMod != projectileList->end(); toMod++) {
                if (index == bullet) { // go to the next bullet to update and give it the new data
                    (*toMod)->updateState(x, y, this, id);
                    moddedProj++;
                }
                index++;
            }
        } else {
            // if more projectiles are required, create a new one
            projectileList->push_front(new Projectile(x, y, this, id));
        }
    }
    if (numProj < sizeOfProj(projectileList)) {
        // if excess projectiles exist, copy the updated projectiles and delete the rest
        forward_list<Projectile*> newList = *projectileList;
        projectileList->clear();
        int index = 0;
        for (auto toMod = newList.begin(); toMod != newList.end(); toMod++) {
            if (index < moddedProj) {
                projectileList->push_front(*toMod);
            } else {
                delete *toMod;
            }
            index++;
        }
    }
}

void Game::updateExplosions(string serverString) {
    // recieve and new explosions
    int numExpl = (serverString.length()-1)/(MESSAGE_LOC_CHAR*2+MESSAGE_ID_CHAR); // find the number created
    for (int expl = 0; expl < numExpl; expl++) {
        int x, y, id;
        // extract any needed data and create a new explosion from the data
        x = stoi(serverString.substr(1+(2*MESSAGE_LOC_CHAR+MESSAGE_ID_CHAR)*expl, MESSAGE_LOC_CHAR));
        y = stoi(serverString.substr(1+MESSAGE_LOC_CHAR+(2*MESSAGE_LOC_CHAR+MESSAGE_ID_CHAR)*expl, MESSAGE_LOC_CHAR));
        id = stoi(serverString.substr(1+MESSAGE_LOC_CHAR*2+(2*MESSAGE_LOC_CHAR+MESSAGE_ID_CHAR)*expl, MESSAGE_ID_CHAR));
        explosionList->push_front(new BulletExplosion(this, x, y, id));
    }
}

void Game::updateLobby(string serverString) {
    // recieve new updates to the lobby as a client
    int currIndex = 1;
    int currId = -1;
    stringstream name;
    numPlayers = 0;
    while (currIndex < serverString.length()-1) {
        if (serverString.substr(currIndex, 1) == "|") { // check for a divide between insances
            currPlayers[numPlayers].id = currId;
            currPlayers[numPlayers].username = name.str();
            numPlayers++;

            // reset variables for the next player
            currId = -1;
            name.str("");
            currIndex++;

        } else if (currId == -1) { // if an id is required, extract it
            currId = stoi(serverString.substr(currIndex, MESSAGE_ID_CHAR));
            currIndex += 3;
        } else {
            if (serverString[currIndex] != 0) { // add valid characters to the characters username
                name << serverString[currIndex];
            }
            currIndex++;
        }
    }
}

string Game::getMapString(void) {
    // converts the wall objects in the game into a single stream to send to clients
    stringstream wallString;
    SDL_Rect currw;
    wallString << MESSAGE_WALL;
    for (auto wall = wallContainer->begin(); wall != wallContainer->end(); wall++) {
        currw = (*wall)->getLocation();
        wallString << strOfLen(currw.x, MESSAGE_LOC_CHAR) << strOfLen(currw.y, MESSAGE_LOC_CHAR) <<
         strOfLen(currw.w, MESSAGE_LOC_CHAR) << strOfLen(currw.h, MESSAGE_LOC_CHAR);
    }
    return wallString.str();
}

string Game::getPlayerString(void) {
    // create a string representing the state of each player needed for display
    stringstream playerString;
    playerString << MESSAGE_PLAYER;
    for (auto player = playerList->begin(); player != playerList->end(); player++) {
        playerString << (*player)->getStateString();
    }
    return playerString.str();
}

string Game::getProjectileString(void) {
    // get a string representing each projectiles state
    stringstream projString;
    projString << MESSAGE_PROJECTILE;
    for (auto bullet = projectileList->begin(); bullet != projectileList->end(); bullet++) {
        projString << (*bullet)->getStateString();
    }
    return projString.str();
}

string Game::getExplosionString(void) {
    // get a string containting data on each new explosion object
    stringstream explosionString;
    explosionString << MESSAGE_EXPLOSION;
    for (auto explosion = explosionsToSend.begin();
     explosion != explosionsToSend.end(); explosion++) {
        explosionString << (*explosion)->getStateString();
    }
    explosionsToSend.clear();
    return explosionString.str();
}

void Game::setPatterns(void) {
    // load the patterns used to decorate the game board
    stringstream patternLocation;
    for (int i = 0; i < UI_BACKGROUND_PATTERN_COUNT; i++) {
        patternLocation.str("");
        // generate the next string in the list
        patternLocation << UI_BACKGROUND_PATTERN_PREFIX << to_string(i) << UI_BACKGROUND_PATTERN_TYPE;
        patterns[i] = loadImage(patternLocation.str(), *gameRenderer);

        if (!patterns[i]) { // check an image was loaded
            cout << IMG_GetError() << "\n";
        } else {
            // modify the color of the pattern
            SDL_SetTextureAlphaMod(patterns[i], UI_BACKGROUND_PATTERN_ALPHA);
            SDL_SetTextureColorMod(patterns[i], primaryColor.red, primaryColor.green,
                primaryColor.blue);
        }
    }
}

void Game::updatePrimColors(string newColor) {
    // update the games primary colors to the new color
    primary = newColor;
    primaryColor = COLOR_VALUES[newColor];
}

void Game::updateSecColors(string newColor) {
    // update the games secondary color
    secondary = newColor;
    secondaryColor = COLOR_VALUES[newColor];
}

void Game::updateWindow(int w, int h, bool full) {
    // update the size and fullscreen state of the window

	// update window size
    SDL_SetWindowSize(*gameWindow, w, h);
    swidth = w;
    sheight = h;

	// set fullscreen
    if (full == true) {
        SDL_SetWindowFullscreen(*gameWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(*gameWindow, 0);
    }

	// set resolution used in actual game calculation (will vary with fullscreen)
    if (full == true) {
        SDL_GetWindowSize(*gameWindow, &swidthActual, &sheightActual);
    } else {
        swidthActual = w;
        sheightActual = h;
    }
    fullscreen = full;
}

playerScore Game::getPlayerScore(int index) {
    // get the score and username of a player at a particular index in the currPlayers object
    assert(index < numPlayers);
    playerScore output;
    output.name = currPlayers[index].username;
    output.id = currPlayers[index].id;
    for (auto player = playerList->begin(); player != playerList->end(); player++) {
        if ((*player)->getID() == currPlayers[index].id) {
            output.score = (*player)->getScore();
        }
    }
    return output;
}

string Game::getUsername(int id) {
    // get the username of a player with a particular id
    string output = "";
    for (int i=0; i<numPlayers; i++) {
        if (currPlayers[i].id == id) {
            output = currPlayers[i].username;
        }
    }
    return output;
}

int Game::getWinner(void) {
    // check if the game has a winner, returning their id, or -1 if none has been found
    if (winner == -1) {
        for (auto player = playerList->begin(); player != playerList->end(); player++) {
            if ((*player)->getScore() >= GAME_WIN_SCORE) {
                winner = (*player)->getID();
            }
        }
    }
    return winner;
}


Menu::Menu(Game* game) {
    // initialize the menu and all the pages involved
    currMenu = MENU_MAIN; // start on the main menu

    mainMenu = new MainPage(game);
    lobby = new LobbyPage(game);
    optionMenu = new OptionPage(game);
    tutorial = new TutorialPage(game);
    mouseDown = false;

    weapon = CHARACTER_WEAPON_PISTOL; // default to the pistol
}

Menu::~Menu(void) {
    // destroy the menu
    delete mainMenu;
    delete lobby;
    delete optionMenu;
    delete tutorial;
}

void Menu::reset(void) {
    // reset the menu to its original state
    currMenu = MENU_MAIN;
    mainMenu->reset();
    lobby->reset();
    optionMenu->reset();
    tutorial->reset();
    mouseDown = true;
}

int Menu::update(Game* game) {
    // update the menu, and change its state depending on the actions taken by the user
    int action = MENU_NONE;
    coordSet mouseCoords = getMouseCoordinates(game);
    int x = mouseCoords.x;
    int y = mouseCoords.y;

    // get mouse coordinates, and check whether the LMB is pressed this frame
    bool mousePress = SDL_GetMouseState(NULL, NULL) && SDL_BUTTON(SDL_BUTTON_LEFT);

    // check whether the mouse was pressed this frame
    bool press = false;
    if (mousePress == true && mouseDown == false) {
        press = true;
    }

    mouseDown = mousePress;


    // determine the current page of the menu, and update based off of it
    if (currMenu == MENU_MAIN) {
        action = mainMenu->update(x, y, press);
        weapon = mainMenu->getCurrWeapon(); // update the current weapon to that selected
    } else if (currMenu == MENU_LOBBY) {
        action = lobby->update(x, y, press, game);
    } else if (currMenu == MENU_OPTIONS) {
        action = optionMenu->update(x, y, press, game);
    } else if (currMenu == MENU_CONTROLS) {
        action = tutorial->update(x, y, press);
    }

    if (currMenu == MENU_OPTIONS && action != MENU_NONE) {
        // if the player leaves the option menu, update it based on the new settings
        optionMenu->updateGame(game);
    }

    // if the player chose to move to a new page, update the state
    if (action == MENU_SET_MAIN) {
        currMenu = MENU_MAIN;
    } else if (action == MENU_SET_LOBBY_HOST) {
        currMenu = MENU_LOBBY;
        game->setHost(true);
    } else if (action == MENU_SET_LOBBY_CLIENT) {
        currMenu = MENU_LOBBY;
        game->setHost(false);
    } else if (action == MENU_SET_OPTIONS) {
        currMenu = MENU_OPTIONS;
    } else if (action == MENU_SET_CONTROLS) {
        currMenu = MENU_CONTROLS;
    }

    return action;
}

void Menu::render(Game* game) {
    // while in the menu, have the cursor visible (no custom image)
    SDL_ShowCursor(SDL_ENABLE);

    // draw a background
    SDL_SetRenderDrawColor(game->renderer(),
        game->primaryColors().red*UI_BACKGROUND_MULTIPLIER + UI_BACKGROUND_ADDITION,
        game->primaryColors().green*UI_BACKGROUND_MULTIPLIER + UI_BACKGROUND_ADDITION,
        game->primaryColors().blue*UI_BACKGROUND_MULTIPLIER + UI_BACKGROUND_ADDITION,
        UI_COLOR_MAX_VALUE);
    SDL_Rect background = {0, 0, SCREEN_WIDTH_DEFAULT, SCREEN_HEIGHT_DEFAULT};
    SDL_RenderSetViewport(game->renderer(), &background);
    SDL_RenderFillRect(game->renderer(), &background);

    // render the menu currently in use
    if (currMenu == MENU_MAIN) {
        mainMenu->render(game);
    } else if (currMenu == MENU_LOBBY) {
        lobby->render(game);
    } else if (currMenu == MENU_OPTIONS) {
        optionMenu->render(game);
    } else if (currMenu == MENU_CONTROLS) {
        tutorial->render(game);
    }
}


MainPage::MainPage(Game* game) {
    // create the main page
    // create the banner text
    heading = loadText(MAIN_HEADER, UI_FONT_SIZE, game->renderer());

    // create all needed buttons
    playButton = new Button(BUTTON_MAIN_TOPLEFT_X, BUTTON_MAIN_TOPLEFT_Y, BUTTON_MAIN_WIDTH,
     BUTTON_MAIN_HEIGHT, BUTTON_MENU, "play", false, "Play", game);
    optionButton = new Button(BUTTON_MAIN_TOPLEFT_X,
     BUTTON_MAIN_TOPLEFT_Y+BUTTON_MAIN_HEIGHT+BUTTON_MAIN_GAP, BUTTON_MAIN_WIDTH,
     BUTTON_MAIN_HEIGHT, BUTTON_MENU, "options", false, "Options", game);
    controlButton = new Button(BUTTON_MAIN_TOPLEFT_X,
     BUTTON_MAIN_TOPLEFT_Y+(BUTTON_MAIN_HEIGHT+BUTTON_MAIN_GAP)*2, BUTTON_MAIN_WIDTH,
     BUTTON_MAIN_HEIGHT, BUTTON_MENU, "control", false, "Controls", game);
    quitButton = new Button(BUTTON_MAIN_TOPLEFT_X,
     BUTTON_MAIN_TOPLEFT_Y+(BUTTON_MAIN_HEIGHT+BUTTON_MAIN_GAP)*3, BUTTON_MAIN_WIDTH,
     BUTTON_MAIN_HEIGHT, BUTTON_MENU, "quit", false, "Quit Game", game);


    // create the host selection buttons
    hostInfo = loadText(UI_HOST, UI_FONT_SIZE, game->renderer());
    hostSelect = new Button(BUTTON_HOST_TOPLEFT_X, BUTTON_HOST_TOPLEFT_Y,
     BUTTON_HOST_WIDTH, BUTTON_HOST_HEIGHT, BUTTON_RADIO, "host", true, BUTTON_HOST_IMAGE, game);
    clientSelect = new Button(BUTTON_CLIENT_TOPLEFT_X, BUTTON_CLIENT_TOPLEFT_Y,
     BUTTON_CLIENT_WIDTH, BUTTON_CLIENT_HEIGHT, BUTTON_RADIO, "client", true, BUTTON_CLIENT_IMAGE, game);
    hostSelect->setActive(true); // default to being host

    // create the weapons selection buttons
    weaponInfo = loadText(UI_WEAPON, UI_FONT_SIZE, game->renderer());
    pistolSelect = new Button(BUTTON_PISTOL_TOPLEFT_X, BUTTON_PISTOL_TOPLEFT_Y,
     BUTTON_PISTOL_WIDTH, BUTTON_PISTOL_HEIGHT, BUTTON_RADIO, "pistol", true, "images/characterPistol.png", game);
    rifleSelect = new Button(BUTTON_AR_TOPLEFT_X, BUTTON_AR_TOPLEFT_Y,
     BUTTON_AR_WIDTH, BUTTON_AR_HEIGHT, BUTTON_RADIO, "rifle", true, "images/characterAssaultRifle.png", game);
    shotgunSelect = new Button(BUTTON_SHOTGUN_TOPLEFT_X, BUTTON_SHOTGUN_TOPLEFT_Y,
     BUTTON_SHOTGUN_WIDTH, BUTTON_SHOTGUN_HEIGHT, BUTTON_RADIO, "shotgun", true, "images/characterShotgun.png", game);
    pistolSelect->setActive(true); // default to using pistol
}

MainPage::~MainPage(void) {
    // delete the main page
    delete playButton;
    delete optionButton;
    delete controlButton;
    delete quitButton;

    delete hostSelect;
    delete clientSelect;

    delete pistolSelect;
    delete rifleSelect;
    delete shotgunSelect;

    SDL_DestroyTexture(heading);
    SDL_DestroyTexture(weaponInfo);
    SDL_DestroyTexture(hostInfo);
}

void MainPage::reset(void) {
    // reset the page
    playButton->setActive(false);
    optionButton->setActive(false);
    controlButton->setActive(false);
    quitButton->setActive(false);
}

int MainPage::getCurrWeapon(void) {
    // get the weapon currently selected
    int weapon;
    if (pistolSelect->getActive() == true) {
        weapon = CHARACTER_WEAPON_PISTOL;
    } else if (rifleSelect->getActive() == true) {
        weapon = CHARACTER_WEAPON_ASSAULT_RIFLE;
    } else {
        weapon = CHARACTER_WEAPON_SHOTGUN;
    }
    return weapon;
}

int MainPage::update(int x, int y, bool press) {
    //update the page if it is open

    int action = MENU_NONE;
    if (playButton->mouseHover(x, y) == true) { // check if the player enters lobby
        if (press == true) {
            if (hostSelect->getActive() == true) {
                action = MENU_SET_LOBBY_HOST;
            } else {
                action = MENU_SET_LOBBY_CLIENT;
            }
        }
        playButton->setActive(true);
    } else {
        playButton->setActive(false);
    }

    // check if the player opens the options menu
    if (optionButton->mouseHover(x, y) == true) {
        if (press == true) {
            action = MENU_SET_OPTIONS;
        }
        optionButton->setActive(true);
    } else {
        optionButton->setActive(false);
    }

    // check if the player opens the tutorial
    if (controlButton->mouseHover(x, y) == true) {
        if (press == true) {
            action = MENU_SET_CONTROLS;
        }
        controlButton->setActive(true);
    } else {
        controlButton->setActive(false);
    }

    // check if the player quits the game
    if (quitButton->mouseHover(x, y) == true) {
        if (press == true) {
            action = MENU_QUIT;
        }
        quitButton->setActive(true);
    } else {
        quitButton->setActive(false);
    }

    // check whether the player choses either host or client, and update respectively
    if (hostSelect->mouseHover(x, y) == true && press == true) {
        hostSelect->setActive(true);
        clientSelect->setActive(false);
    } else if (clientSelect->mouseHover(x, y) == true && press == true) {
        hostSelect->setActive(false);
        clientSelect->setActive(true);
    }


    // check if the user chooses a weapon, and disable others if they do
    if (pistolSelect->mouseHover(x, y) == true && press == true) {
        pistolSelect->setActive(true);
        rifleSelect->setActive(false);
        shotgunSelect->setActive(false);
    } else if (rifleSelect->mouseHover(x, y) == true && press == true) {
        pistolSelect->setActive(false);
        rifleSelect->setActive(true);
        shotgunSelect->setActive(false);
    } else if (shotgunSelect->mouseHover(x, y) == true && press == true) {
        pistolSelect->setActive(false);
        rifleSelect->setActive(false);
        shotgunSelect->setActive(true);
    }

    return action;
}

void MainPage::render(Game* game) {
    // draw the banner
    SDL_Rect header = {MAIN_HEADER_TOPLEFT_X, MAIN_HEADER_TOPLEFT_Y,
     MAIN_HEADER_WIDTH, MAIN_HEADER_HEIGHT};
    SDL_RenderCopy(game->renderer(), heading, NULL, &header);

    // draw the navigation buttons
    playButton->render(game);
    optionButton->render(game);
    controlButton->render(game);
    quitButton->render(game);


    // draw the host/client selector
    SDL_Rect hcSelector = {UI_HOST_TOPLEFT_X, UI_HOST_TOPLEFT_Y,
     UI_HOST_WIDTH, UI_HOST_HEIGHT};
    SDL_RenderCopy(game->renderer(), hostInfo, NULL, &hcSelector);

    hostSelect->render(game);
    clientSelect->render(game);

    // draw the weapon selector
    SDL_Rect weapSelector = {UI_WEAPON_TOPLEFT_X, UI_WEAPON_TOPLEFT_Y,
     UI_WEAPON_WIDTH, UI_WEAPON_HEIGHT};
    SDL_RenderCopy(game->renderer(), weaponInfo, NULL, &weapSelector);

    pistolSelect->render(game);
    rifleSelect->render(game);
    shotgunSelect->render(game);
}



LobbyPage::LobbyPage(Game* game) {
    // create the lobby page
    backButton = new Button(BUTTON_BACK_TOPLEFT_X, BUTTON_BACK_TOPLEFT_Y,
     BUTTON_BACK_WIDTH, BUTTON_BACK_HEIGHT, BUTTON_MENU, "back", false, "Back", game);
    launchButton = new Button(BUTTON_LAUNCH_TOPLEFT_X, BUTTON_LAUNCH_TOPLEFT_Y,
     BUTTON_LAUNCH_WIDTH, BUTTON_LAUNCH_HEIGHT, BUTTON_MENU, "launch", false, "Launch", game);

    waitingText = loadText(UI_WAITING, UI_FONT_SIZE, game->renderer());
    hostInfo = loadText(UI_HOST_INFO, UI_FONT_SIZE, game->renderer());
}

LobbyPage::~LobbyPage(void) {
    delete backButton;
    delete launchButton;

    SDL_DestroyTexture(waitingText);
    SDL_DestroyTexture(hostInfo);
}

void LobbyPage::reset() {
    // reset the page
    backButton->setActive(false);
    launchButton->setActive(false);
}

int LobbyPage::update(int x, int y, bool press, Game* game) {
    // update the lobby each frame
    int action = MENU_NONE;

    // check if the plyaer leaves the lobby
    if (backButton->mouseHover(x, y) == true) {
        if (press == true) {
            action = MENU_SET_MAIN;
        }
        backButton->setActive(true);
    } else {
        backButton->setActive(false);
    }

    // check if the host launches the game, ensuring at least 2 players are in the game
    if (launchButton->mouseHover(x, y) == true && game->getNumPlayers() > 1 &&
         game->hosting() == true) {
        if (press == true) {
            action = MENU_LAUNCH;
        }
        launchButton->setActive(true);
    } else {
        launchButton->setActive(false);
    }
    return action;
}

void LobbyPage::render(Game* game) {
    // draw the lobby to the screen
    backButton->render(game);

    if (game->hosting() == true) {
        launchButton->render(game);
    }
        // draw the lobby list
    if (game->hosting() == false) {
        if (game->isConnected() == false) { // if the client hasn't connected to a game, render the wait text
            SDL_Rect waitRect = {UI_WAITING_TOPLEFT_X, UI_WAITING_TOPLEFT_Y,
             UI_WAITING_WIDTH, UI_WAITING_HEIGHT};
            SDL_RenderCopy(game->renderer(), waitingText, NULL, &waitRect);
        } else {
            // indicate the host is waiting
            SDL_Rect hostRect = {UI_HOST_INFO_TOPLEFT_X, UI_HOST_INFO_TOPLEFT_Y,
             UI_HOST_INFO_WIDTH, UI_HOST_INFO_HEIGHT};
            SDL_RenderCopy(game->renderer(), hostInfo, NULL, &hostRect);


            SDL_Texture* currName;
            SDL_Rect lobby = {UI_LOBBY_TOPLEFT_X, UI_LOBBY_TOPLEFT_Y, UI_LOBBY_WIDTH, UI_LOBBY_HEIGHT};
            SDL_Rect name = {UI_LOBBY_NAME_TOPLEFT_X, lobby.y+lobby.h*0.1, 0, lobby.h*0.8};
            for (int i = 0; i < GAME_MAX_PLAYERS; i++) {
                // go through each player in the lobby
                SDL_SetRenderDrawColor(game->renderer(), game->primaryColors().red,
                 game->primaryColors().green, game->primaryColors().blue, UI_COLOR_MAX_VALUE);
                if (i < game->getNumPlayers()) { // if their is a player at the current index
                    if (game->getCurrPlayers()[i].id == game->getID()) { // highlight the player if it is this instances id
                        SDL_SetRenderDrawColor(game->renderer(), game->secondaryColors().red,
                         game->secondaryColors().green, game->secondaryColors().blue, UI_COLOR_MAX_VALUE);
                    }
                    // create the texture for the player and render it
                    currName = loadText(game->getCurrPlayers()[i].username, UI_FONT_SIZE, game->renderer());
                    name.w = name.h*UI_FONT_HEIGHT_TO_WIDTH*game->getCurrPlayers()[i].username.length();
                    SDL_RenderFillRect(game->renderer(), &lobby);
                    SDL_RenderCopy(game->renderer(), currName, NULL, &name);
                    SDL_DestroyTexture(currName);

                    // move the image down for the next frame
                    name.y += UI_LOBBY_HEIGHT+UI_LOBBY_GAP;
                } else {
                    // draw an empty box to present an empty place in the lobby
                    SDL_RenderFillRect(game->renderer(), &lobby);
                }
                // move the lobby rect down for the next frame
                lobby.y += UI_LOBBY_HEIGHT+UI_LOBBY_GAP;
            }
        }
    } else {
        SDL_Texture* currName;
        SDL_Rect lobby = {UI_LOBBY_TOPLEFT_X, UI_LOBBY_TOPLEFT_Y, UI_LOBBY_WIDTH, UI_LOBBY_HEIGHT};
        SDL_Rect name = {UI_LOBBY_NAME_TOPLEFT_X, lobby.y+lobby.h*0.1, 0, lobby.h*0.8};
        for (int i = 0; i < GAME_MAX_PLAYERS; i++) {
            // go through each player in the lobby
            SDL_SetRenderDrawColor(game->renderer(), game->primaryColors().red,
             game->primaryColors().green, game->primaryColors().blue, UI_COLOR_MAX_VALUE);
            if (i < game->getNumPlayers()) { // if their is a player at the current index
                if (game->getCurrPlayers()[i].id == game->getID()) { // highlight the player if it is this instances id
                    SDL_SetRenderDrawColor(game->renderer(), game->secondaryColors().red,
                     game->secondaryColors().green, game->secondaryColors().blue, UI_COLOR_MAX_VALUE);
                }
                // create the texture for the player and render it
                currName = loadText(game->getCurrPlayers()[i].username, UI_FONT_SIZE, game->renderer());
                name.w = name.h*UI_FONT_HEIGHT_TO_WIDTH*game->getCurrPlayers()[i].username.length();
                SDL_RenderFillRect(game->renderer(), &lobby);
                SDL_RenderCopy(game->renderer(), currName, NULL, &name);
                SDL_DestroyTexture(currName);

                // move the image down for the next frame
                name.y += UI_LOBBY_HEIGHT+UI_LOBBY_GAP;
            } else {
                // draw an empty box to present an empty place in the lobby
                SDL_RenderFillRect(game->renderer(), &lobby);
            }
            // move the lobby rect down for the next frame
            lobby.y += UI_LOBBY_HEIGHT+UI_LOBBY_GAP;
        }
    }
}



OptionPage::OptionPage(Game* game) {
    // create the options page
    heading = loadText(OPTIONS_HEADER, UI_FONT_SIZE, game->renderer());
    resTitle = loadText(OPTIONS_RESNAME, UI_FONT_SIZE, game->renderer());

    primText = loadText(OPTIONS_PRIMTEXT, UI_FONT_SIZE, game->renderer());
    secText = loadText(OPTIONS_SECTEXT, UI_FONT_SIZE, game->renderer());

    backButton = new Button(BUTTON_BACK_TOPLEFT_X, BUTTON_BACK_TOPLEFT_Y,
     BUTTON_BACK_WIDTH, BUTTON_BACK_HEIGHT, BUTTON_MENU, "back", false, "Back", game);

    fullscreenButton = new Button(BUTTON_FULL_TOPLEFT_X, BUTTON_FULL_TOPLEFT_Y,
     BUTTON_FULL_WIDTH, BUTTON_FULL_HEIGHT, BUTTON_RADIO, "fullscreen", true, BUTTON_FULL_IMAGE, game);
    fullscreenButton->setActive(game->isFullscreen());

    int shiftX;
    int shiftY;
    for (int i = 0; i < COLOR_NUM; i++) {
        // create a color button for each color option

        // find the buttons coordinates relative to the first button
        shiftX = (i%COLOR_NUM_ROW)*(BUTTON_COLOR_GAP+BUTTON_COLOR_WIDTH);
        shiftY = (i/COLOR_NUM_ROW)*(BUTTON_COLOR_GAP+BUTTON_COLOR_WIDTH);

        // find the colors for the button
        colorSet primary = COLOR_VALUES[COLOR_NAMES[i]];
        colorSet secondary = COLOR_VALUES[COLOR_NAMES[i]]*BUTTON_COLOR_SEC_MULTIPLIER;

        // create the button of a fixed color
        primSelector[i] = new Button(BUTTON_COLOR_PRIM_TOPLEFT_X+shiftX,
         BUTTON_COLOR_PRIM_TOPLEFT_Y+shiftY, BUTTON_COLOR_WIDTH,
        BUTTON_COLOR_WIDTH, primary, secondary, BUTTON_RADIO, COLOR_NAMES[i]);

        // check if the button is that of the current color, activating it if it is
        if (COLOR_NAMES[i] == game->primColor()) {
            primSelector[i]->setActive(true);
        }
    }

    for (int j = 0; j < COLOR_NUM; j++) {
        // creaete a color button for each color option
        int shiftX = (j%COLOR_NUM_ROW)*(BUTTON_COLOR_GAP+BUTTON_COLOR_WIDTH);
        int shiftY = (j/COLOR_NUM_ROW)*(BUTTON_COLOR_GAP+BUTTON_COLOR_WIDTH);

        // find the color for the button and assign it
        colorSet primary = COLOR_VALUES[COLOR_NAMES[j]];
        colorSet secondary = COLOR_VALUES[COLOR_NAMES[j]]*BUTTON_COLOR_SEC_MULTIPLIER;

        // create the new button
        secSelector[j] = new Button(BUTTON_COLOR_SEC_TOPLEFT_X+shiftX,
         BUTTON_COLOR_SEC_TOPLEFT_Y+shiftY, BUTTON_COLOR_WIDTH,
        BUTTON_COLOR_WIDTH, primary, secondary, BUTTON_RADIO, COLOR_NAMES[j]);

        // check if the button has the current color, and activate it if it does
        if (COLOR_NAMES[j] == game->secColor()) {
            secSelector[j]->setActive(true);
        }
    }

    // create the buttons for resolution selection
    stringstream text;
    for (int k = 0; k < UI_RESOLUTION_NUM; k++) {
        // create the text for the button
        text.str("");
        text << to_string(UI_RESOLUTIONS[k]) << " X " << to_string((int)(UI_RESOLUTIONS[k]*UI_RESOLUTION_MULTIPLIER));

        // determine how far down the button is
        int shiftY = k*(BUTTON_RES_GAP+BUTTON_RES_HEIGHT);

        // create the button
        resSelector[k] = new Button(BUTTON_RES_TOPLEFT_X, BUTTON_RES_TOPLEFT_Y+shiftY,
         BUTTON_RES_WIDTH, BUTTON_RES_HEIGHT, BUTTON_RADIO,
         to_string(UI_RESOLUTIONS[k]), false, text.str(), game);

        // if the resolution matches the one currently in use, activate the button
        if (UI_RESOLUTIONS[k] == game->configScreenHeight()) {
            resSelector[k]->setActive(true);
        }
    }
}

OptionPage::~OptionPage(void) {
    // destroy the option page
    delete backButton;
    for (int i = 0; i < COLOR_NUM; i++) {
        delete primSelector[i];
        delete secSelector[i];
    }

    for (int i = 0; i < UI_RESOLUTION_NUM; i++) {
        delete resSelector[i];
    }
}

void OptionPage::reset(void) {
    // reset the option page
    backButton->setActive(false);
}

int OptionPage::update(int x, int y, bool press, Game* game) {
    // update the option page for the frame
    int action = MENU_NONE;

    // update the back button
    if (backButton->mouseHover(x, y) == true) {
        if (press == true) {
            action = MENU_SET_MAIN;
        }
        backButton->setActive(true);
    } else {
        backButton->setActive(false);
    }

    // toggle the fullscreen button if it is pressed
    if (fullscreenButton->mouseHover(x, y) == true && press == true) {
        fullscreenButton->setActive(!fullscreenButton->getActive());
    }

    int selection = -1;
    // go through each color button and check if it is pressed
    for (int i = 0; i < COLOR_NUM; i++) {
        if (primSelector[i]->mouseHover(x, y) == true && press == true) {
            selection = i;
        }
    }
    // if a button was pressed, activate it and disable the rest
    if (selection >= 0) {
        for (int i = 0; i < COLOR_NUM; i++) {
            if (i == selection) {
                primSelector[i]->setActive(true);
            } else {
                primSelector[i]->setActive(false);
            }
        }
    }

    selection = -1;
    // check if a color button was pressed
    for (int i = 0; i < COLOR_NUM; i++) {
        if (secSelector[i]->mouseHover(x, y) == true && press == true) {
            selection = i;
        }
    }

    // if one was pressed, activate it and disable the rest
    if (selection >= 0) {
        for (int i = 0; i < COLOR_NUM; i++) {
            if (i == selection) {
                secSelector[i]->setActive(true);
            } else {
                secSelector[i]->setActive(false);
            }
        }
    }

    selection = -1;
    // check if a resolution was chosen
    for (int i = 0; i < UI_RESOLUTION_NUM; i++) {
        if (resSelector[i]->mouseHover(x, y) == true && press == true) {
            selection = i;
        }
    }
    // if a button was pressed, activate it and disable the rest
    if (selection >= 0) {
        for (int i = 0; i < UI_RESOLUTION_NUM; i++) {
            if (i == selection) {
                resSelector[i]->setActive(true);
            } else {
                resSelector[i]->setActive(false);
            }
        }
    }

    return action;
}

void OptionPage::updateGame(Game* game) {
    // update the game object based on the current settings

    // find the current primary color and set it
    for (int i = 0; i < COLOR_NUM; i++) {
        if (primSelector[i]->getActive() == true) {
            game->updatePrimColors(primSelector[i]->getID());
        }
    }

    // find the current secondary color and set it
    for (int i = 0; i < COLOR_NUM; i++) {
        if (secSelector[i]->getActive() == true) {
            game->updateSecColors(secSelector[i]->getID());
        }
    }

    // default the width and height to the current size incase no button is active
    int w = game->configScreenWidth();
    int h = game->configScreenHeight();

    // find the current resolution
    for (int i = 0; i < UI_RESOLUTION_NUM; i++) {
        if (resSelector[i]->getActive() == true) {
            h = stoi(resSelector[i]->getID());
            w = h*UI_RESOLUTION_MULTIPLIER;
        }
    }

    // find if the game is to be set in fullscreen, then update the window based on the resolution and fullscreen state
    bool fullscreen = fullscreenButton->getActive();
    game->updateWindow(w, h, fullscreen);

    // update the config file with the current settings
    ofstream configFile;
    configFile.open(CONFIG_FILE_LOCATION);
    
    configFile << CONFIG_SWIDTH << "=" << game->configScreenWidth() << "\n";
    configFile << CONFIG_SHEIGHT << "=" << game->configScreenHeight() << "\n";
    configFile << CONFIG_FULLSCREEN << "=" << game->isFullscreen() << "\n";

    configFile << CONFIG_PRIMCOLOR << "=" << game->primColor() << "\n";
    configFile << CONFIG_SECCOLOR << "=" << game->secColor() << "\n";

    configFile << CONFIG_USERNAME << "=" << game->getUsername() << "\n";
    configFile << CONFIG_HOSTIP << "=" << game->hIP() << "\n";

    configFile.close();
}

void OptionPage::render(Game* game) {
    // render the option page

    // draw the text objects to the screen
    SDL_Rect header = {OPTIONS_HEADER_TOPLEFT_X, OPTIONS_HEADER_TOPLEFT_Y,
     OPTIONS_HEADER_WIDTH, OPTIONS_HEADER_HEIGHT};
    SDL_RenderCopy(game->renderer(), heading, NULL, &header);

    SDL_Rect resBox = {OPTIONS_RESNAME_TOPLEFT_X, OPTIONS_RESNAME_TOPLEFT_Y,
     OPTIONS_RESNAME_WIDTH, OPTIONS_RESNAME_HEIGHT};
    SDL_RenderCopy(game->renderer(), resTitle, NULL, &resBox);

    SDL_Rect primBox = {OPTIONS_PRIMTEXT_TOPLEFT_X, OPTIONS_PRIMTEXT_TOPLEFT_Y,
     OPTIONS_PRIMTEXT_WIDTH, OPTIONS_PRIMTEXT_HEIGHT};
    SDL_RenderCopy(game->renderer(), primText, NULL, &primBox);

    SDL_Rect secBox = {OPTIONS_SECTEXT_TOPLEFT_X, OPTIONS_SECTEXT_TOPLEFT_Y,
     OPTIONS_SECTEXT_WIDTH, OPTIONS_SECTEXT_HEIGHT};
    SDL_RenderCopy(game->renderer(), secText, NULL, &secBox);

    backButton->render(game);
    fullscreenButton->render(game);

    // go through each button in the arrays and render them
    for (int i = 0; i < COLOR_NUM; i++) {
        primSelector[i]->render(game);
    }

    for (int j = 0; j < COLOR_NUM; j++) {
        secSelector[j]->render(game);
    }

    for (int k = 0; k < UI_RESOLUTION_NUM; k++) {
        resSelector[k]->render(game);
    }
}

TutorialPage::TutorialPage(Game* game) {
    // create the tutorial page
	header = loadText(UI_CONTROLS_HEADER, UI_FONT_SIZE, game->renderer());
	content = loadImage(UI_CONTROLS_CONTENT_LOCATION, game->renderer());

    backButton = new Button(BUTTON_BACK_TOPLEFT_X, BUTTON_BACK_TOPLEFT_Y,
     BUTTON_BACK_WIDTH, BUTTON_BACK_HEIGHT, BUTTON_MENU, "back", false, "Back", game);
}

TutorialPage::~TutorialPage(void) {
    // delete the tutorial page
    delete backButton;
}

void TutorialPage::reset(void) {
    // reset the tutorial page
    backButton->setActive(false);
}

int TutorialPage::update(int x, int y, bool press) {
    // update the tutorial page
    int action = MENU_NONE;

    // update the back button
    if (backButton->mouseHover(x, y) == true) {
        if (press == true) {
            action = MENU_SET_MAIN;
        }
        backButton->setActive(true);
    } else {
        backButton->setActive(false);
    }
    return action;
}

void TutorialPage::render(Game* game) {
    // render the tutorial page
	SDL_Rect headerRect = { UI_CONTROLS_HEADER_TOPLEFT_X, UI_CONTROLS_HEADER_TOPLEFT_Y,
	 UI_CONTROLS_HEADER_WIDTH, UI_CONTROLS_HEADER_HEIGHT };
	SDL_RenderCopy(game->renderer(), header, NULL, &headerRect);

	SDL_Rect contentRect = {0, 0, SCREEN_WIDTH_DEFAULT, SCREEN_HEIGHT_DEFAULT};
	SDL_SetTextureColorMod(content, game->primaryColors().red, game->primaryColors().green, game->primaryColors().blue);
	SDL_RenderCopy(game->renderer(), content, NULL, &contentRect);

    backButton->render(game);
}


Button::Button(int x, int y, int w, int h, const colorSet prim,
 const colorSet sec, int type, const string name) {
    // create a button object, given a set color

    // set the size parameters
    location.x = x;
    location.y = y;
    location.w = w;
    location.h = h;

    // set the button parameters
    fixedColor = true;
    colorPrim = prim;
    colorSec = sec;
    selected = false;

    buttonType = type;
    id = name;

    // have no text for the button
    displayText = NULL;
    textLen = 0;
}

Button::Button(int x, int y, int w, int h, int type, const string name,
 bool useImg, string text, Game* game) {
    // create a button object, with either a text or image
    location.x = x;
    location.y = y;
    location.w = w;
    location.h = h;

    fixedColor = false;
    colorPrim = {0,0,0};
    colorSec = {0,0,0};
    selected = false;

    buttonType = type;
    id = name;

    img = false;
    if (text != "") { // check there is text to present the button with
        if (useImg == false) { // if the button is to be presented with text, load a texture of it and set its length
            displayText = loadText(text, UI_FONT_SIZE, game->renderer());
            textLen = text.length();
        } else {
            // load in the image to be used
            displayText = loadImage(text, game->renderer());
            textLen = 0;
            img = true;
        }
    } else {
        displayText = NULL;
        textLen = 0;
    }
}

Button::~Button(void) {
    SDL_DestroyTexture(displayText);
}

bool Button::mouseHover(int x, int y) {
    //  checks where the coordinate set is located within the button
    return location.x < x &&
        location.x + location.w > x &&
        location.y < y && 
        location.y + location.h > y;
}

void Button::render(Game* game) {
    // draw the button to the screen

    // set the colors used in drawing the button
    colorSet prim;
    colorSet sec;
    if (fixedColor == true) { // use the predefined set if the colors are fixed
        prim = colorPrim;
        sec = colorSec;
    } else { // used the game set if the colors are variable
        prim = game->primaryColors();
        sec = game->secondaryColors();
    }
    if (buttonType == BUTTON_MENU) {
        // modify the colors to their desired strength
        prim = prim * BUTTON_MENU_COLOR_MULTIPLIER;
        sec = sec * BUTTON_MENU_COLOR_MULTIPLIER;
        if (selected == false) { // choose to use primary or secondary colors based on the buttons state
            SDL_SetRenderDrawColor(game->renderer(), prim.red,
             prim.green, prim.blue, UI_COLOR_MAX_VALUE);
        } else {
            SDL_SetRenderDrawColor(game->renderer(), sec.red,
             sec.green, sec.blue, UI_COLOR_MAX_VALUE);
        }

        // draw the button
        SDL_RenderFillRect(game->renderer(), &location);
    } else if (buttonType == BUTTON_RADIO) {
        if (selected == true) { // draw a outline around the button if they are active
            SDL_Rect outlineRect;
            outlineRect.x = location.x - BUTTON_OUTLINE_WIDTH;
            outlineRect.y = location.y - BUTTON_OUTLINE_WIDTH;
            outlineRect.w = location.w + 2*BUTTON_OUTLINE_WIDTH;
            outlineRect.h = location.h + 2*BUTTON_OUTLINE_WIDTH;
            SDL_SetRenderDrawColor(game->renderer(), sec.red,
             sec.green, sec.blue, UI_COLOR_MAX_VALUE);
            SDL_RenderFillRect(game->renderer(), &outlineRect);
        }
        // draw the main button
        SDL_SetRenderDrawColor(game->renderer(), prim.red,
         prim.green, prim.blue, UI_COLOR_MAX_VALUE);
        SDL_RenderFillRect(game->renderer(), &location);
    }

    if (displayText != NULL) { // if a texture is in place on the button, draw it
        SDL_Rect textBox;
        textBox.h = location.h*0.8;
        if (img == true) {
            textBox.w = location.w*0.8;
        } else {
            textBox.w = textLen*textBox.h*UI_FONT_HEIGHT_TO_WIDTH;
        }
        textBox.y = location.y+(location.h-textBox.h)/2;
        textBox.x = location.x+(location.w-textBox.w)/2;
        SDL_SetTextureColorMod(displayText, game->secondaryColors().red,
         game->secondaryColors().green, game->secondaryColors().blue);
        SDL_RenderCopy(game->renderer(), displayText, NULL, &textBox);
    }
}





// Functions related to the player class
Player::Player(Game* game, int startX, int startY, int idNum, int weaponID) {
    // initialization function for the player class
    //set the players default coordinated
    playerRect.x = startX - CHARACTER_WIDTH / 2;
    playerRect.y = startY - CHARACTER_HEIGHT / 2;

    // set the size of the players sprite
    playerRect.w = CHARACTER_WIDTH;
    playerRect.h = CHARACTER_HEIGHT;

    // set the variables used in collision detection based on the location of the players image
    radius = playerRect.w / 2;
    setPlayerCentre();

    // set the default direction the player is looking
    angle = 0.0;

    // load in the players spritesheet
    rollOutline = loadImage(CHARACTER_ROLL_IMAGE, game->renderer());
    invulnImage = loadImage(CHARACTER_INVULN_IMAGE, game->renderer());

    weapID = weaponID;
    if (weaponID == CHARACTER_WEAPON_ASSAULT_RIFLE) {
        playerImage = loadImage(CHARACTER_IMAGE_AR_LOCATION, game->renderer());
        weapon = new AssaultRifle;
    } else if (weaponID == CHARACTER_WEAPON_SHOTGUN) {
        playerImage = loadImage(CHARACTER_IMAGE_SHOTGUN_LOCATION, game->renderer());
        weapon = new Shotgun;
    } else {
        playerImage = loadImage(CHARACTER_IMAGE_PISTOL_LOCATION, game->renderer());
        weapon = new Pistol;
    }

    score = 0;

    // set the players default speed to 0
    velx = 0;
    vely = 0;

    rolling = false;
    rollDirection = MOVE_NONE;
    rollFrames = 0;
    rollCooldown = 0;

    // start the player on max health
    health = CHARACTER_MAX_HP;
    alive = true;
    deathFrames = 0;

    // initialize the player to be invulnerable
    invulnerable = true;
    invulnFrames = CHARACTER_INVULN_FRAMES;

    id = idNum; // set the ID number to the one provided

                // set the players color scheme
    if (id == game->getID()) {
        playerColors.red = game->primaryColors().red;
        playerColors.green = game->primaryColors().green;
        playerColors.blue = game->primaryColors().blue;
    } else {
        playerColors.red = game->secondaryColors().red;
        playerColors.green = game->secondaryColors().green;
        playerColors.blue = game->secondaryColors().blue;
    }
    playerColors.alpha = UI_COLOR_MAX_VALUE;

    playerRenderer = game->renderer();
    deathMarker = new DeathObject(game->renderer(), playerRect, playerColors);
}

Player::~Player(void) {
    // clears any memory used by the player
    SDL_DestroyTexture(playerImage);
    if (deathMarker) {
        delete deathMarker;
    }
    delete weapon;
}

void Player::setPlayerCentre(void) {
    // calculate the X and Y centres of the player
    centreX = playerRect.x + radius;
    centreY = playerRect.y + radius;
}

void Player::updateState(Game* game, bool paused) {
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
    if (alive == true) {
        // get the keyboard state containing which keys are actively pressed
        direction direction = MOVE_NONE;

        // used to store the x and y coordinates of the mouse
        int mouseX = 0;
        int mouseY = 0;
        double ratio;
        if (paused == false) {
            direction = getDirections(); // get the direction of movement for the player at the current frame
        }
        if (keyboardState[SDL_SCANCODE_LSHIFT] && rolling == false
            && rollCooldown == 0 && direction != MOVE_NONE && paused == false) { // if the player presses the roll key while they can start rolling
            rolling = true;
            rollFrames = CHARACTER_ROLL_DURATION;
            rollDirection = direction;
        }
        if (keyboardState[SDL_SCANCODE_R] && weapon->isReloading() == false
         && paused == false) { // reload when r is pressed
            weapon->beginReload();
        }

        if (keyboardState[SDL_SCANCODE_K] && DEBUG_KILL_PLAYER == true) { // if enabled in debug settings, kill the player on command
            killPlayer();
        }
        if (rolling == true) {
            // while rolling, move the player in the initial direction of the roll, and remove the state if they finish
            rollFrames--;
            if (rollDirection == MOVE_UP || rollDirection == MOVE_LEFT ||
                rollDirection == MOVE_DOWN || rollDirection == MOVE_RIGHT) {
                velx = rollDirection.x*CHARACTER_ROLL_SPEED;
                vely = rollDirection.y*CHARACTER_ROLL_SPEED;
            } else { // if the player is moving diagonally, set their velocity to be limited
                velx = rollDirection.x*CHARACTER_ROLL_SPEED / sqrt(2);
                vely = rollDirection.y*CHARACTER_ROLL_SPEED / sqrt(2);
            }
            if (rollFrames == 0) { // if the player finishes rolling, stop it and put them on cooldown
                rolling = false;
                rollDirection = MOVE_NONE;
                rollCooldown = CHARACTER_ROLL_COOLDOWN;
            }
        } else { // if not rolling, resort to standard movement
               // directional movement
               // increment the velocity in each direction according to the movement direction
            velx += direction.x*CHARACTER_ACCEL_PER_FRAME;
            vely += direction.y*CHARACTER_ACCEL_PER_FRAME;

            if (direction == MOVE_UP || direction == MOVE_DOWN || direction == MOVE_NONE) {
                // if the player has no movement in the x direction, reduce speed along that axis
                if (velx > 0) { // decrease toward 0 if the player is moving right 
                    velx -= CHARACTER_DECEL_PER_FRAME;
                } else if (velx < 0) { // increase toward 0 if the player is moving left
                    velx += CHARACTER_DECEL_PER_FRAME;
                }
                if (-CHARACTER_DECEL_PER_FRAME < velx && velx < CHARACTER_DECEL_PER_FRAME) {
                    velx = 0; // if the velocity would wrap the other way next frame, set it to 0
                }
            }
            if (direction == MOVE_LEFT || direction == MOVE_RIGHT || direction == MOVE_NONE) {
                if (vely > 0) {
                    vely -= CHARACTER_DECEL_PER_FRAME;
                } else if (vely < 0) {
                    vely += CHARACTER_DECEL_PER_FRAME;
                }
                if (-CHARACTER_DECEL_PER_FRAME < vely && vely < CHARACTER_DECEL_PER_FRAME) {
                    vely = 0; // if the velocity would wrap the other way next frame, set it to 0
                }
            }

            double currSpeed = sqrt(pow(vely, 2) + pow(velx, 2)); // find the player current speed
            if (currSpeed > CHARACTER_VEL_MAX) { // if the player is moving to fast
                                                 // scale x and y down so they are at the correct speed
                vely *= CHARACTER_VEL_MAX / currSpeed;
                velx *= CHARACTER_VEL_MAX / currSpeed;
            }

            if (rollCooldown > 0) { // if roll is on cooldown, reduce the time remaining
                rollCooldown--;
            }

            if (paused == false) {
                // rotate the player to look toward the mouse
                // Scaling on the player position is used to get the players position relative to the mouse in the screen's scale
                SDL_GetMouseState(&mouseX, &mouseY); // get the x and y coords of the mouse

                                                     // scale the mouse coordinates to those of the viewport
                if ((double)game->screenHeight() / (double)SCREEN_HEIGHT_DEFAULT <
                    (double)game->screenWidth() / (double)SCREEN_WIDTH_DEFAULT) {
                    ratio = (double)game->screenHeight() / (double)SCREEN_HEIGHT_DEFAULT;
                } else {
                    ratio = (double)game->screenWidth() / (double)SCREEN_WIDTH_DEFAULT;
                }
                // scale the mouse coords to be in the gamespace frame
                mouseX -= GAMESPACE_TOPLEFT_X*ratio;
                mouseX /= ratio;
                mouseY /= ratio;
                angle = atan2((double)(centreY - mouseY),
                    (double)(centreX - mouseX))*180.0 / M_PI; // find the angle between the character and the mouse

                
                weapon->takeShot(game, this); // have the player shoot a projectile
            }
        }
        if (invulnFrames > 0) { // if invulnerable, take a frame off the counter
            invulnFrames--;
        } else { // otherwise remove invulnerability
            invulnerable = false;
        }
    } else { // if the player is dead, remove a frame and check if they should respawn
        deathFrames--;
        if (deathFrames <= 0) {
            respawn(game->spawns(), game->players());
        } else {
            deathMarker->updateState();
        }
    }
}

void Player::updateState(userAction userInput, Game* game) {
    if (alive == true) {
        // get the keyboard state containing which keys are actively pressed
        direction direction = {userInput.moveHor, userInput.moveVert};

        if (userInput.shiftDown == true && rolling == false
            && rollCooldown == 0 && direction != MOVE_NONE) { // if the player presses the roll key while they can start rolling
            rolling = true;
            rollFrames = CHARACTER_ROLL_DURATION;
            rollDirection = direction;
        }
        if (userInput.rDown == true && weapon->isReloading() == false) { // reload when r is pressed
            weapon->beginReload();
        }
        if (rolling == true) {
            // while rolling, move the player in the initial direction of the roll, and remove the state if they finish
            rollFrames--;
            if (direction == MOVE_UP || direction == MOVE_LEFT ||
                direction == MOVE_DOWN || direction == MOVE_RIGHT) {
                velx = rollDirection.x*CHARACTER_ROLL_SPEED;
                vely = rollDirection.y*CHARACTER_ROLL_SPEED;
            } else {
                velx = rollDirection.x*CHARACTER_ROLL_SPEED / sqrt(2);
                vely = rollDirection.y*CHARACTER_ROLL_SPEED / sqrt(2);
            }
            if (rollFrames == 0) {
                rolling = false;
                rollDirection = MOVE_NONE;
                rollCooldown = CHARACTER_ROLL_COOLDOWN;
            }
        } else { // if not rolling, resort to standard movement
               // directional movement
               // increment the velocity in each direction according to the movement direction
            velx += direction.x*CHARACTER_ACCEL_PER_FRAME;
            vely += direction.y*CHARACTER_ACCEL_PER_FRAME;

            if (direction == MOVE_UP || direction == MOVE_DOWN || direction == MOVE_NONE) {
                // if the player has no movement in the x direction, reduce speed along that axis
                if (velx > 0) { // decrease toward 0 if the player is moving right 
                    velx -= CHARACTER_DECEL_PER_FRAME;
                } else if (velx < 0) { // increase toward 0 if the player is moving left
                    velx += CHARACTER_DECEL_PER_FRAME;
                }
                if (-CHARACTER_DECEL_PER_FRAME < velx && velx < CHARACTER_DECEL_PER_FRAME) {
                    velx = 0; // if the velocity would wrap the other way next frame, set it to 0
                }
            }
            if (direction == MOVE_LEFT || direction == MOVE_RIGHT || direction == MOVE_NONE) {
                if (vely > 0) {
                    vely -= CHARACTER_DECEL_PER_FRAME;
                } else if (vely < 0) {
                    vely += CHARACTER_DECEL_PER_FRAME;
                }
                if (-CHARACTER_DECEL_PER_FRAME < vely && vely < CHARACTER_DECEL_PER_FRAME) {
                    vely = 0; // if the velocity would wrap the other way next frame, set it to 0
                }
            }

            double currSpeed = sqrt(pow(vely, 2) + pow(velx, 2)); // find the player current speed
            if (currSpeed > CHARACTER_VEL_MAX) { // if the player is moving to fast
                                                 // scale x and y down so they are at the correct speed
                vely *= CHARACTER_VEL_MAX / currSpeed;
                velx *= CHARACTER_VEL_MAX / currSpeed;
            }

            if (rollCooldown > 0) { // if roll is on cooldown, reduce the time remaining
                rollCooldown--;
            }

            // rotate the player to look toward the mouse
            // Scaling on the player position is used to get the players position relative to the mouse in the screen's scale
            angle = atan2((double)(centreY - userInput.mouseY),
                (double)(centreX - userInput.mouseX))*180.0 / M_PI; // find the angle between the character and the mouse

            
            weapon->takeShot(game, this, userInput); // have the player shoot a projectile
        }
        if (invulnFrames > 0) { // if invulnerable, take a frame off the counter
            invulnFrames--;
        } else { // otherwise remove invulnerability
            invulnerable = false;
        }
    } else { // if the player is dead, remove a frame and check if they should respawn
        deathFrames--;
        if (deathFrames <= 0) {
            respawn(game->spawns(), game->players());
        } else {
            deathMarker->updateState();
        }
    }
}

void Player::updateState(playerState newState) {
    // udpate the players state based on host data

    // set position related variables
    playerRect.x = newState.x;
    playerRect.y = newState.y;
    setPlayerCentre();
    angle = newState.angle;
    //set their current score
    score = newState.score;

    // set roll related variables
    rolling = newState.rolling;
    rollFrames = newState.rollFrames;
    rollDirection.x = newState.rollX;
    rollDirection.y = newState.rollY;

    // set life related varaibles
    invulnerable = newState.invuln;
    invulnFrames = newState.invulnFrames;
    alive = newState.alive;
    health = newState.health;

    // update the death state
    deathFrames = newState.deathFrames;
    deathMarker->updateState(deathFrames, newState.dmX, newState.dmY);

    // update cooldowns and weapons
    weapon->setAmmo(newState.ammo);
    weapon->setReloadFrames(newState.reloadFrames);
    rollCooldown = newState.cooldownFrames;
}

void Player::move(forward_list<Wall*>* wallContainer) {
    // store the original location of the player
    int posOrigX = playerRect.x;
    int posOrigY = playerRect.y;
    // moves the player based on the final velocity of each frame

    if (alive == true) {
        // move the player along x and reset the centre
        playerRect.x += velx;
        setPlayerCentre();

        // go through each wall, and if the player movement would cause a collision, undo the movement along x
        for (auto wall = wallContainer->begin(); wall != wallContainer->end(); wall++) {
            if ((*wall)->checkCollision(centreX, centreY, radius) == true
                || checkExitMap(centreX, centreY, radius)) {
                playerRect.x = posOrigX;
                velx = 0;
                setPlayerCentre();
            }
        }

        //repeat the process for x with y
        playerRect.y += vely;
        setPlayerCentre();
        for (auto wall = wallContainer->begin(); wall != wallContainer->end(); wall++) {
            if ((*wall)->checkCollision(centreX, centreY, radius) == true
                || checkExitMap(centreX, centreY, radius)) {
                playerRect.y = posOrigY;
                vely = 0;
                setPlayerCentre();
            }
        }
    }

    // update the gun for the frame
    weapon->updateGun();
}


bool Player::successfulShot(void) {
    //function called when the player takes damage from a bullet
    bool kill = false;
    if (invulnerable == false) {
        health -= 1;
    }
    if (health <= 0) { // check if the shot was a lethal blow
        killPlayer();
        kill = true;
    }
    return kill;
}

void Player::killPlayer(void) { // kill the player, and begin their death animation
    alive = false;
    deathFrames = CHARACTER_DEATH_DURATION;
    velx = 0;
    vely = 0;
    rolling = false;
    rollDirection = MOVE_NONE;
    deathMarker->reset(playerRect);
}

void Player::respawn(forward_list<coordSet>* spawnPoints, forward_list<Player*>* playerList) {
    // respawns the player to a new position if their respawn timer expires
    coordSet newPosition = getSpawnPoint(spawnPoints, playerList); // get a location to spawn to
    if (newPosition.x != 0) { // if the player has a position they are able to spawn at, do so, otherwise respawn will call again next frame
                              // reset the players state
        alive = true;
        invulnerable = true;

        // reset the players position
        playerRect.x = newPosition.x - CHARACTER_WIDTH / 2;
        playerRect.y = newPosition.y - CHARACTER_HEIGHT / 2;
        setPlayerCentre();

        deathFrames = 0;
        invulnFrames = CHARACTER_INVULN_FRAMES;
        health = CHARACTER_MAX_HP;

        rollCooldown = 0;
        weapon->resetGun();
    }
}

void Player::render(Game* game) {
    SDL_Renderer* renderer = game->renderer();

    // draws the player to the screen using the supplied renderer
    if (alive == true) {
        if (invulnerable == true) { // if the player is in an invulnerable state, draw the icon below them
            SDL_Rect invulnRect;
            // set the width and height to scale depending on time remaining, getting smaller as it less time is left
            // smallest size is that of the player, and largest is that of the default size
            invulnRect.w = CHARACTER_INVULN_IMAGE_WIDTH -
                (CHARACTER_INVULN_IMAGE_WIDTH - playerRect.w)*
                (1 - ((double)invulnFrames / CHARACTER_INVULN_FRAMES));
            invulnRect.h = CHARACTER_INVULN_IMAGE_HEIGHT -
                (CHARACTER_INVULN_IMAGE_HEIGHT - playerRect.h)*
                (1 - ((double)invulnFrames / CHARACTER_INVULN_FRAMES));

            // set the upper left corner so that the centre of the player and image are the same
            invulnRect.x = playerRect.x - (invulnRect.w - playerRect.w) / 2;
            invulnRect.y = playerRect.y - (invulnRect.h - playerRect.h) / 2;

            // set the color to that of the player and render
            SDL_SetTextureColorMod(invulnImage, playerColors.red,
                playerColors.green, playerColors.blue);
            SDL_SetTextureAlphaMod(invulnImage, CHARACTER_INVULN_ALPHA);
            SDL_RenderCopy(renderer, invulnImage, NULL, &invulnRect);
        }
        
        SDL_SetTextureColorMod(playerImage, playerColors.red,
            playerColors.green, playerColors.blue); // modulates the color based on the players settings
        SDL_RenderCopyEx(renderer, playerImage, NULL, &playerRect,
            angle, NULL, SDL_FLIP_NONE); // draw the player to the screen

        if (rolling == true) { // draw an outline image around the player if they are rolling
            SDL_Rect outlineRect;
            outlineRect.w = CHARACTER_ROLL_OUTLINE_WIDTH;
            outlineRect.h = CHARACTER_ROLL_OUTLINE_HEIGHT;
            outlineRect.x = playerRect.x - (outlineRect.w - playerRect.w) / 2;
            outlineRect.y = playerRect.y - (outlineRect.h - playerRect.h) / 2;
            SDL_SetTextureColorMod(rollOutline, playerColors.red,
                playerColors.green, playerColors.blue);
            SDL_SetTextureAlphaMod(rollOutline, CHARACTER_ROLL_ALPHA);
            SDL_RenderCopyEx(renderer, rollOutline, NULL, &outlineRect,
                atan2(-rollDirection.y, -rollDirection.x) * 180 / M_PI, NULL, SDL_FLIP_NONE);
        }

        if (DEBUG_DRAW_WEAPONARC == true) {
            weapon->debugRender(game, centreX, centreY, angle);
        }
    } else {
        // draw the death image in place of the player if they are dead
        deathMarker->render(renderer);
    }
}

string Player::getStateString(void) {
    // gets a string representing the players variables for sending through the network
    /* format:
    x, y, angle, rolling state, roll frames, roll dirx, roll diry, invuln state, invuln frames,
     alive state, health, death frames, deathmarkerX, deathmarkerY, id, weapon type, score, ammo, reload frames, cooldown frames
    */
    stringstream statestring;
    statestring << strOfLen(playerRect.x, MESSAGE_LOC_CHAR);
    statestring << strOfLen(playerRect.y, MESSAGE_LOC_CHAR);
    statestring << strOfLen(angle+360, MESSAGE_ANGLE_CHAR);

    statestring << rolling;
    statestring << strOfLen(rollFrames, MESSAGE_FRAMES_CHAR);
    statestring << rollDirection.x+2; // shift the roll values to avoid negative numbers during sending
    statestring << rollDirection.y+2;

    statestring << invulnerable;
    statestring << strOfLen(invulnFrames, MESSAGE_FRAMES_CHAR);
    statestring << alive;
    statestring << strOfLen(health, MESSAGE_STAT_CHAR);


    statestring << strOfLen(deathFrames, MESSAGE_FRAMES_CHAR);
    statestring << strOfLen(deathMarker->getX(), MESSAGE_LOC_CHAR);
    statestring << strOfLen(deathMarker->getY(), MESSAGE_LOC_CHAR);

    statestring << strOfLen(id, MESSAGE_ID_CHAR);
    statestring << weapID;
    statestring << strOfLen(score, MESSAGE_STAT_CHAR);
    statestring << strOfLen(weapon->getCurrAmmo(), MESSAGE_STAT_CHAR);
    statestring << strOfLen(weapon->getReloadFrames(), MESSAGE_FRAMES_CHAR);
    statestring << strOfLen(rollCooldown, MESSAGE_FRAMES_CHAR);

    return statestring.str();
}



DeathObject::DeathObject(SDL_Renderer* renderer, SDL_Rect playerCoordinates,
    colorSet playerColors) {
    // initialize the death marker of a player
    circleImage = loadImage(CHARACTER_DEATH_IMAGE, renderer);
    circleRect.x = playerCoordinates.x;
    circleRect.y = playerCoordinates.y;
    circleRect.w = playerCoordinates.w;
    circleRect.h = playerCoordinates.h;

    // set the color to be that of the player
    circleColors.red = playerColors.red;
    circleColors.blue = playerColors.blue;
    circleColors.green = playerColors.green;
    circleColors.alpha = 0;
}

void DeathObject::reset(SDL_Rect playerCoordinates) {
    // reset the object for display on player death
    circleRect.x = playerCoordinates.x;
    circleRect.y = playerCoordinates.y;
    circleColors.alpha = UI_COLOR_MAX_VALUE;
}

void DeathObject::updateState(void) {
    // update the objects alpha each frame
    circleColors.alpha -= (double)UI_COLOR_MAX_VALUE / CHARACTER_DEATH_DURATION;
}

void DeathObject::updateState(int framesLeft, int x, int y) {
    // update the image based on host data
    circleRect.x = x;
    circleRect.y = y;
    circleColors.alpha = UI_COLOR_MAX_VALUE*((double)framesLeft / CHARACTER_DEATH_DURATION);
}

void DeathObject::render(SDL_Renderer* renderer) { // draw the marker to its current location
    SDL_SetTextureColorMod(circleImage, circleColors.red,
        circleColors.green, circleColors.blue);
    SDL_SetTextureAlphaMod(circleImage, circleColors.alpha);
    SDL_RenderCopy(renderer, circleImage, NULL, &circleRect);
}

DeathObject::~DeathObject(void) { // delete the marker when it expires
    SDL_DestroyTexture(circleImage);
}



// Weapon object functions

Weapon::~Weapon(void) {
    //pure virtual
}

AssaultRifle::AssaultRifle(void) : Weapon() {
    // create an assault rile
    mouseDown = true;
    reloading = false;
    currAmmo = AR_CLIP_SIZE;
    reloadFramesLeft = 0;
    shotDelay = 0;
}

AssaultRifle::~AssaultRifle(void) {
    // delete an assault rifle
}

void AssaultRifle::takeShot(Game* game, Player* player) {
    // called each frame by host to see if a shot is taken
    if (SDL_GetMouseState(NULL, NULL) && SDL_BUTTON(SDL_BUTTON_LEFT)) { // get the current mouse state
        mouseDown = true;
    } else {
        mouseDown = false;
    }
    // set the projectiles angle to be in a certain range
    double projectileAngle = player->getAngle() + generateRandDouble(-AR_MAX_BULLET_SPREAD / 2, AR_MAX_BULLET_SPREAD / 2);
    if (mouseDown == true && shotDelay == 0 && reloadFramesLeft == 0) { // check if a shot is able to be taken
        if (currAmmo > 0) {
            // create a projectile and set the weapon to have taken a shot
            game->projectiles()->push_front(new Projectile(player->getX(), player->getY(),
             projectileAngle, AR_PROJECTILE_SPEED, game, player));
            shotDelay = AR_SHOT_DELAY;
            currAmmo--;
        }
    }
    if (currAmmo == 0 && reloadFramesLeft == 0) { // reload the player weapon if they are out of ammo
        beginReload();
    }
}

void AssaultRifle::takeShot(Game* game, Player* player, userAction userInput) {
    // take a shot based on input from the host
    if (userInput.mouseDown) {
        mouseDown = true;
    } else {
        mouseDown = false;
    }
    double projectileAngle = player->getAngle() + generateRandDouble(-AR_MAX_BULLET_SPREAD / 2, AR_MAX_BULLET_SPREAD / 2);
    if (mouseDown == true && shotDelay == 0 && reloadFramesLeft == 0) {
        if (currAmmo > 0) {
            game->projectiles()->push_front(new Projectile(player->getX(), player->getY(),
             projectileAngle, AR_PROJECTILE_SPEED, game, player));
            shotDelay = AR_SHOT_DELAY;
            currAmmo--;
        }
    }
    if (currAmmo == 0 && reloadFramesLeft == 0) {
        beginReload();
    }
}

void AssaultRifle::beginReload(void) {
    // reload the weapon if it requires ammo
    if (currAmmo < AR_CLIP_SIZE) {
        reloading = true;
        reloadFramesLeft = AR_RELOAD_FRAMES;
    }
}

void AssaultRifle::updateGun(void) {
    // update the guns state each frame
    if (shotDelay > 0) { // if the weapon is on cooldown, decrement the delay
        shotDelay--;
    }
    if (reloading == true) { // update the reload state
        reloadFramesLeft--;
        if (reloadFramesLeft == 0) {
            reloading = false;
            currAmmo = AR_CLIP_SIZE;
        }
    }
}

void AssaultRifle::setReloadFrames(int frames) {
    // set the reload frames to that in the data from the host
    reloadFramesLeft = frames;
    if (frames != 0) {
        reloading = true;
    } else {
        reloading = false;
    }
}

void AssaultRifle::resetGun(void) {
    // reset the gun on respawn
    currAmmo = AR_CLIP_SIZE;
    reloadFramesLeft = 0;
    shotDelay = 0;
    reloading = false;
    mouseDown = false;
}

void AssaultRifle::debugRender(Game* game, int x, int y, double a) {
    // draw a debug image for the possible angles of weapon projectiles
    double a1 = (a + 180) - AR_MAX_BULLET_SPREAD / 2;
    double a2 = (a + 180) + AR_MAX_BULLET_SPREAD / 2;
    lineRGBA(game->renderer(), x, y, x + cos(a1*M_PI / 180)*DEBUG_WEAPONARC_RADIUS, y + sin(a1*M_PI / 180)*DEBUG_WEAPONARC_RADIUS, 0, 0, 0, UI_COLOR_MAX_VALUE);
    lineRGBA(game->renderer(), x, y, x + cos(a2*M_PI / 180)*DEBUG_WEAPONARC_RADIUS, y + sin(a2*M_PI / 180)*DEBUG_WEAPONARC_RADIUS, 0, 0, 0, UI_COLOR_MAX_VALUE);
    arcRGBA(game->renderer(), x, y, DEBUG_WEAPONARC_RADIUS, a1, a2, 0, 0, 0, UI_COLOR_MAX_VALUE);
}


Pistol::Pistol(void) : Weapon() {
    // create a pistol object
    mouseDown = false;
    reloading = false;
    currRecoil = PISTOL_MIN_BULLET_SPREAD;
    currAmmo = PISTOL_CLIP_SIZE;
    reloadFramesLeft = 0;
}

Pistol::~Pistol(void) {
    // delete a pistol
}

void Pistol::takeShot(Game* game, Player* player) {
    // take a shot if with the weapon

    // get an angle for the projectiel
    double projectileAngle = player->getAngle() + generateRandDouble(-currRecoil / 2, currRecoil / 2);
    if (SDL_GetMouseState(NULL, NULL) && SDL_BUTTON(SDL_BUTTON_LEFT)) { // check if the mouse is down
        if (mouseDown != true && reloadFramesLeft == 0) { // check if the pklayer is able to take a shot
            if (currAmmo > 0) {
                // update the weapon to have taken a shot
                mouseDown = true;
                currAmmo--;
                currRecoil += PISTOL_RECOIL_INCREASE_PER_SHOT;

                // add the projectile to the list
                game->projectiles()->push_front(new Projectile(player->getX(), player->getY(),
                    projectileAngle, PISTOL_PROJECTILE_SPEED, game, player));
            }
        }
    } else {
        // if the mouse isn't being pressed this frame, store it
        mouseDown = false;
    }
    if (currAmmo == 0 && reloadFramesLeft == 0) { // reload if out of ammo
        beginReload();
    }
}

void Pistol::takeShot(Game* game, Player* player, userAction userInput) {
    // take a shot based on client input

    // get a shot angle
    double projectileAngle = player->getAngle() + generateRandDouble(-currRecoil / 2, currRecoil / 2);
    if (userInput.mouseDown == true) { // check if the mouse is pressed
        if (mouseDown != true && reloadFramesLeft == 0) { // check if the player is able to shoot
            if (currAmmo > 0) {
                // update the weapons state
                mouseDown = true;
                currAmmo--;
                currRecoil += PISTOL_RECOIL_INCREASE_PER_SHOT;

                // add a projectile
                game->projectiles()->push_front(new Projectile(player->getX(), player->getY(),
                    projectileAngle, PISTOL_PROJECTILE_SPEED, game, player));
            }
        }
    } else {
        mouseDown = false;
    }
    if (currAmmo == 0 && reloadFramesLeft == 0) { // reload if out of ammo
        beginReload();
    }
}

void Pistol::beginReload(void) {
    // reload the gun if out of ammo
    if (currAmmo < PISTOL_CLIP_SIZE) {
        reloading = true;
        reloadFramesLeft = PISTOL_RELOAD_FRAMES;
    }
}

void Pistol::setReloadFrames(int frames) {
    // update the guns state based on host input
    reloadFramesLeft = frames;
    if (frames != 0) {
        reloading = true;
    } else {
        reloading = false;
    }
}

void Pistol::updateGun(void) {
    // update the gun each frame
    if (currRecoil > PISTOL_MIN_BULLET_SPREAD) { // decrease the weapons recoil each frame
        currRecoil -= PISTOL_RECOIL_RECOVERY_PER_FRAME;
        if (currRecoil < PISTOL_MIN_BULLET_SPREAD) { // check if the decrease took it below the minimum
            currRecoil = PISTOL_MIN_BULLET_SPREAD;
        }
    }
    if (currRecoil > PISTOL_MAX_BULLET_SPREAD) { // if the weapon has recoiled to far, set it to the max
        currRecoil = PISTOL_MAX_BULLET_SPREAD;
    }
    if (reloading == true) {
        // update the guns reload if they are in that state
        reloadFramesLeft--;
        if (reloadFramesLeft == 0) {
            reloading = false;
            currAmmo = PISTOL_CLIP_SIZE;
        }
    }
}

void Pistol::resetGun(void) {
    // reset the gun on player respawn
    currAmmo = PISTOL_CLIP_SIZE;
    reloadFramesLeft = 0;
    reloading = false;
    currRecoil = PISTOL_MIN_BULLET_SPREAD;
    mouseDown = false;
}

void Pistol::debugRender(Game* game, int x, int y, double a) {
    // debug render the weapons projectile arc
    double a1 = (a + 180) - currRecoil / 2;
    double a2 = (a + 180) + currRecoil / 2;
    lineRGBA(game->renderer(), x, y, x + cos(a1*M_PI / 180)*DEBUG_WEAPONARC_RADIUS, y + sin(a1*M_PI / 180)*DEBUG_WEAPONARC_RADIUS, 0, 0, 0, UI_COLOR_MAX_VALUE);
    lineRGBA(game->renderer(), x, y, x + cos(a2*M_PI / 180)*DEBUG_WEAPONARC_RADIUS, y + sin(a2*M_PI / 180)*DEBUG_WEAPONARC_RADIUS, 0, 0, 0, UI_COLOR_MAX_VALUE);
    arcRGBA(game->renderer(), x, y, DEBUG_WEAPONARC_RADIUS, a1, a2, 0, 0, 0, UI_COLOR_MAX_VALUE);
}



Shotgun::Shotgun(void) : Weapon() {
    // create a shotgun
    mouseDown = false;
    shotDelay = 0;
}

Shotgun::~Shotgun(void) {
    // delete a shotgun
}

void Shotgun::takeShot(Game* game, Player* player) {
    // check if a shot is taken

    // set the base angle for the weapons spread
    double projectileAngle = player->getAngle() - SHOTGUN_SPREAD_RANGE / 2;
    double shotAngle;
    if (SDL_GetMouseState(NULL, NULL) && SDL_BUTTON(SDL_BUTTON_LEFT)) { // check if the mouse is pressed
        if (mouseDown != true && shotDelay == 0) { // check if a shot is able to be taken
            mouseDown = true;
            for (int n = 0; n < SHOTGUN_PROJECTILES_PER_SHOT; n++) { // generate a projectile for each bullet in a shot
                // create an angle for a projectile between its base angle and its max spread
                shotAngle = generateRandDouble(projectileAngle, projectileAngle + SHOTGUN_PROJECTILE_SPREAD);
                // create the projectile
                game->projectiles()->push_front(new Projectile(player->getX(), player->getY(),
                 shotAngle, SHOTGUN_PROJECTILE_SPEED, game, player));
                // increment the angle for the next shot
                projectileAngle += SHOTGUN_PROJECTILE_SPREAD;
            }
            // put the gun on delay
            shotDelay = SHOTGUN_SHOT_DELAY;
        }
    } else {
        mouseDown = false;
    }
}

void Shotgun::takeShot(Game* game, Player* player, userAction userInput) {
    // take a shot based on client data
    double projectileAngle = player->getAngle() - SHOTGUN_SPREAD_RANGE / 2;
    double shotAngle;
    if (userInput.mouseDown) { // check if the mouse is down
        if (mouseDown != true && shotDelay == 0) { // check if a shot is able to be taken
            mouseDown = true;
            for (int n = 0; n < SHOTGUN_PROJECTILES_PER_SHOT; n++) { // create a bullet for every shot in the spread
                // generate the shots angle
                shotAngle = generateRandDouble(projectileAngle, projectileAngle + SHOTGUN_PROJECTILE_SPREAD);
                // create the projectile
                game->projectiles()->push_front(new Projectile(player->getX(), player->getY(),
                 shotAngle, SHOTGUN_PROJECTILE_SPEED, game, player));
                // increment the angle
                projectileAngle += SHOTGUN_PROJECTILE_SPREAD;
            }
            // put the weapon on delay
            shotDelay = SHOTGUN_SHOT_DELAY;
        }
    } else {
        mouseDown = false;
    }
}

void Shotgun::beginReload(void) {
    // shotgun has no reload state
}

void Shotgun::updateGun(void) {
    // update the gun each frame
    if (shotDelay > 0) {
        shotDelay--;
    }
}

void Shotgun::resetGun() {
    // reset for respawn
    shotDelay = 0;
    mouseDown = false;
}

void Shotgun::debugRender(Game* game, int x, int y, double a) {
    // draw the weapons debug arc, with a subsection for each projectile
    double a1 = (a + 180) - SHOTGUN_SPREAD_RANGE / 2;
    double a2 = (a + 180) + SHOTGUN_SPREAD_RANGE / 2;
    thickLineRGBA(game->renderer(), x, y, x + cos(a1*M_PI / 180)*DEBUG_WEAPONARC_RADIUS,
        y + sin(a1*M_PI / 180)*DEBUG_WEAPONARC_RADIUS, 3, 0, 0, 0, UI_COLOR_MAX_VALUE);
    thickLineRGBA(game->renderer(), x, y, x + cos(a2*M_PI / 180)*DEBUG_WEAPONARC_RADIUS,
        y + sin(a2*M_PI / 180)*DEBUG_WEAPONARC_RADIUS, 3, 0, 0, 0, UI_COLOR_MAX_VALUE);
    for (int n = 1; n < SHOTGUN_PROJECTILES_PER_SHOT; n++) {
        lineRGBA(game->renderer(), x, y,
            x + cos((a1 + n*SHOTGUN_PROJECTILE_SPREAD)*M_PI / 180)*DEBUG_WEAPONARC_RADIUS,
            y + sin((a1 + n*SHOTGUN_PROJECTILE_SPREAD)*M_PI / 180)*DEBUG_WEAPONARC_RADIUS,
            0, 0, 0, UI_COLOR_MAX_VALUE);
    }
    arcRGBA(game->renderer(), x, y, DEBUG_WEAPONARC_RADIUS, a1, a2, 0, 0, 0, UI_COLOR_MAX_VALUE);
}



// Functions related to the wall class
Wall::Wall(int x, int y, int w, int h) { 
    // Initializer for the wall class
    wallLocation.x = x;
    wallLocation.y = y;
    wallLocation.w = w;
    wallLocation.h = h;
}

Wall::~Wall(void) {
    // delete a wall
}

bool Wall::checkCollision(int x, int y, int radius) {
    // checks to see whether an object has collided with the wall
    bool collision = false; // whether a collision has been detected

    /*
    goes through and compares the objects coordinates to the sides of the
    wall to find if the object is located next to. Then, for each case a check
    is made to see if the object is touching the wall
    */
    if (x >= wallLocation.x && x <= (wallLocation.x + wallLocation.w)) {
        if (y < wallLocation.y) { // above wall
            if (y >(wallLocation.y - radius)) {
                collision = true;
            }
        } else { // below wall
            if (y < (wallLocation.y + wallLocation.h + radius)) {
                collision = true;
            }
        }
    } else if (y >= wallLocation.y && y <= (wallLocation.y + wallLocation.h)) {
        if (x < wallLocation.x) { // left of wall
            if (x >(wallLocation.x - radius)) {
                collision = true;
            }
        } else { // right of wall
            if (x < (wallLocation.x + wallLocation.w + radius)) {
                collision = true;
            }
        }
    } else {
        if (x < wallLocation.x) {
            if (y < wallLocation.y) { // top left of wall
                if (distBetweenPoints(x, y, wallLocation.x,
                    wallLocation.y) < radius) {
                    collision = true;
                }
            } else { // bottom left of wall
                if (distBetweenPoints(x, y, wallLocation.x,
                    wallLocation.y + wallLocation.h) < radius) {
                    collision = true;
                }
            }
        } else {
            if (y < wallLocation.y) { // top right of wall
                if (distBetweenPoints(x, y, wallLocation.x + wallLocation.w,
                    wallLocation.y) < radius) {
                    collision = true;
                }
            } else { // bottom right of wall
                if (distBetweenPoints(x, y, wallLocation.x + wallLocation.w,
                    wallLocation.y + wallLocation.h) < radius) {
                    collision = true;
                }
            }
        }
    }
    return collision;
}


void Wall::render(Game* game) {
    // draws the wall to the screen using the supplied renderer
    SDL_SetRenderDrawColor(game->renderer(), game->primaryColors().red*WALL_COLOR_SCALE,
        game->primaryColors().green*WALL_COLOR_SCALE, game->primaryColors().blue*WALL_COLOR_SCALE,
        UI_COLOR_MAX_VALUE);
    SDL_RenderFillRect(game->renderer(), &wallLocation);
}

void Wall::renderShadow(int x, int y, Game* game) {
    // arrays containing the coords of the corner
    Sint16 coordsX[5];
    Sint16 coordsY[5];
    // number of elements in the array being used
    int n;

    // compares the objects coordinates to the walls sides to determine
    // where the shadow should be drawn from. Then, for each case the corners
    // of the shadow to be used are defined
    if (x >= wallLocation.x && x <= (wallLocation.x + wallLocation.w)) {
        if (y < wallLocation.y) { // above wall
            n = 4;
            coordsX[0] = wallLocation.x;
            coordsY[0] = wallLocation.y;

            coordsX[1] = wallLocation.x + wallLocation.w;
            coordsY[1] = wallLocation.y;

            coordsX[2] = getInterceptX(x, y, wallLocation.x + wallLocation.w, wallLocation.y, SCREEN_HEIGHT_DEFAULT);
            coordsY[2] = SCREEN_HEIGHT_DEFAULT;

            coordsX[3] = getInterceptX(x, y, wallLocation.x, wallLocation.y, SCREEN_HEIGHT_DEFAULT);
            coordsY[3] = SCREEN_HEIGHT_DEFAULT;
        } else { // below wall
            n = 4;
            coordsX[0] = wallLocation.x;
            coordsY[0] = wallLocation.y + wallLocation.h-1;

            coordsX[1] = wallLocation.x + wallLocation.w;
            coordsY[1] = wallLocation.y + wallLocation.h-1;

            coordsX[2] = getInterceptX(x, y, wallLocation.x + wallLocation.w, wallLocation.y + wallLocation.h, 0);
            coordsY[2] = 0;

            coordsX[3] = getInterceptX(x, y, wallLocation.x, wallLocation.y + wallLocation.h, 0);
            coordsY[3] = 0;
        }
    } else if (y >= wallLocation.y && y <= (wallLocation.y + wallLocation.h)) {
        if (x < wallLocation.x) { // left of wall
            n = 4;
            coordsX[0] = wallLocation.x;
            coordsY[0] = wallLocation.y;

            coordsX[1] = wallLocation.x;
            coordsY[1] = wallLocation.y + wallLocation.h;

            coordsX[2] = SCREEN_WIDTH_DEFAULT;
            coordsY[2] = getInterceptY(x, y, wallLocation.x, wallLocation.y + wallLocation.h, SCREEN_WIDTH_DEFAULT);

            coordsX[3] = SCREEN_WIDTH_DEFAULT;
            coordsY[3] = getInterceptY(x, y, wallLocation.x, wallLocation.y, SCREEN_WIDTH_DEFAULT);
        } else { // right of wall
            n = 4;
            coordsX[0] = wallLocation.x + wallLocation.w-1;
            coordsY[0] = wallLocation.y;

            coordsX[1] = wallLocation.x + wallLocation.w-1;
            coordsY[1] = wallLocation.y + wallLocation.h;

            coordsX[2] = 0;
            coordsY[2] = getInterceptY(x, y, wallLocation.x + wallLocation.w, wallLocation.y + wallLocation.h, 0);

            coordsX[3] = 0;
            coordsY[3] = getInterceptY(x, y, wallLocation.x + wallLocation.w, wallLocation.y, 0);
        }
    } else {
        if (x < wallLocation.x) {
            if (y < wallLocation.y) { // top left of wall
                n = 5;
                coordsX[0] = wallLocation.x + wallLocation.w;
                coordsY[0] = wallLocation.y;

                coordsX[1] = wallLocation.x;
                coordsY[1] = wallLocation.y + wallLocation.h;

                coordsX[2] = getInterceptX(x, y, wallLocation.x, wallLocation.y + wallLocation.h, SCREEN_HEIGHT_DEFAULT);
                coordsY[2] = SCREEN_HEIGHT_DEFAULT;

                coordsX[3] = SCREEN_WIDTH_DEFAULT;
                coordsY[3] = SCREEN_HEIGHT_DEFAULT;

                coordsX[4] = SCREEN_WIDTH_DEFAULT;
                coordsY[4] = getInterceptY(x, y, wallLocation.x + wallLocation.w, wallLocation.y, SCREEN_WIDTH_DEFAULT);
            } else { // bottom left of wall
                n = 5;
                coordsX[0] = wallLocation.x;
                coordsY[0] = wallLocation.y;

                coordsX[1] = wallLocation.x + wallLocation.w;
                coordsY[1] = wallLocation.y + wallLocation.h;

                coordsX[2] = SCREEN_WIDTH_DEFAULT;
                coordsY[2] = getInterceptY(x, y, wallLocation.x + wallLocation.w, wallLocation.y + wallLocation.h, SCREEN_WIDTH_DEFAULT);

                coordsX[3] = SCREEN_WIDTH_DEFAULT;
                coordsY[3] = 0;

                coordsX[4] = getInterceptX(x, y, wallLocation.x, wallLocation.y, 0);
                coordsY[4] = 0;
            }
        } else {
            if (y < wallLocation.y) { // top right of wall
                n = 5;
                coordsX[0] = wallLocation.x;
                coordsY[0] = wallLocation.y;

                coordsX[1] = wallLocation.x + wallLocation.w;
                coordsY[1] = wallLocation.y + wallLocation.h;

                coordsX[2] = getInterceptX(x, y, wallLocation.x + wallLocation.w, wallLocation.y + wallLocation.h, SCREEN_HEIGHT_DEFAULT);
                coordsY[2] = SCREEN_HEIGHT_DEFAULT;

                coordsX[3] = 0;
                coordsY[3] = SCREEN_HEIGHT_DEFAULT;

                coordsX[4] = 0;
                coordsY[4] = getInterceptY(x, y, wallLocation.x, wallLocation.y, 0);
            } else { // bottom right of wall
                n = 5;
                coordsX[0] = wallLocation.x + wallLocation.w;
                coordsY[0] = wallLocation.y;

                coordsX[1] = wallLocation.x;
                coordsY[1] = wallLocation.y + wallLocation.h;

                coordsX[2] = 0;
                coordsY[2] = getInterceptY(x, y, wallLocation.x, wallLocation.y + wallLocation.h, 0);

                coordsX[3] = 0;
                coordsY[3] = 0;

                coordsX[4] = getInterceptX(x, y, wallLocation.x + wallLocation.w, wallLocation.y, 0);
                coordsY[4] = 0;
            }
        }
    }
    // set the color to be based on the game's primary color
    int r = game->primaryColors().red*SHADOW_COLOR_SCALE;
    int g = game->primaryColors().green*SHADOW_COLOR_SCALE;
    int b = game->primaryColors().blue*SHADOW_COLOR_SCALE;

    filledPolygonRGBA(game->renderer(), coordsX, coordsY, n, r, g, b, UI_COLOR_MAX_VALUE); // draws the shadow using the renderer
}



// Functions related to the projectile class
Projectile::Projectile(int x, int y, double a, const double speed, Game* game, Player* player) {
    // set the location of the projectile
    currPosX = x;
    currPosY = y;

    // set the angle
    angle = a;

    // set the SDL_Rect for the projectile
    projectileRect.x = x;
    projectileRect.y = y;
    projectileRect.w = PROJECTILE_WIDTH;
    projectileRect.h = PROJECTILE_HEIGHT;

    // set the projectiles radius and centre
    radius = PROJECTILE_WIDTH / 2;
    setProjectileCentre();

    // initialize the speed of the projectile based on the angle it fired at
    velx = -speed * cos(angle*M_PI / 180);
    vely = -speed * sin(angle*M_PI / 180);

    // set the projectiles owner
    owner = player;
    ownerID = player->getID();

    // set the projectiles color to be based off whether the owner is that of this game's instance
    if (ownerID == game->getID()) {
        projectileColors.red = game->primaryColors().red;
        projectileColors.blue = game->primaryColors().blue;
        projectileColors.green = game->primaryColors().green;
    } else {
        projectileColors.red = game->secondaryColors().red;
        projectileColors.blue = game->secondaryColors().blue;
        projectileColors.green = game->secondaryColors().green;
    }


    // load the spritesheet to memory
    projectileImage = loadImage(PROJECTILE_IMAGE_LOCATION, game->renderer());
}

Projectile::Projectile(int x, int y, Game* game, int id) {
    // set the location of the projectile
    currPosX = x;
    currPosY = y;

    // set the angle
    angle = 0;

    // set the SDL_Rect for the projectile
    projectileRect.x = x;
    projectileRect.y = y;
    projectileRect.w = PROJECTILE_WIDTH;
    projectileRect.h = PROJECTILE_HEIGHT;

    // set the projectiles radius and centre
    radius = PROJECTILE_WIDTH / 2;
    setProjectileCentre();

    // initialize the speed to 0, since it is not used for clients
    velx = 0;
    vely = 0;

    if (id == game->getID()) {
        projectileColors.red = game->primaryColors().red;
        projectileColors.blue = game->primaryColors().blue;
        projectileColors.green = game->primaryColors().green;
    } else {
        projectileColors.red = game->secondaryColors().red;
        projectileColors.blue = game->secondaryColors().blue;
        projectileColors.green = game->secondaryColors().green;
    }


    ownerID = id;

    // load the spritesheet to memory
    projectileImage = loadImage(PROJECTILE_IMAGE_LOCATION, game->renderer());
}

Projectile::~Projectile(void) {
    SDL_DestroyTexture(projectileImage);
}

void Projectile::setProjectileCentre(void) {
    // set the projectiles new centre
    centreX = projectileRect.x + radius;
    centreY = projectileRect.y + radius;
}

int Projectile::checkCollision(Game* game) {
    int output = PROJECTILE_COLLISION_NONE; // default to no collision
    for (auto character = game->players()->begin(); character != game->players()->end(); character++) {
        if (distBetweenPoints(centreX, centreY, (*character)->getX(),
            (*character)->getY()) < (radius + (*character)->getRadius())) { // check whether the projectile is contacting the player
            if ((*character)->getID() != ownerID && (*character)->isAlive() == true) { // check the player hit is not the one who shot the projectile and is still alive
                if ((*character)->successfulShot() == true) { // tell the player they are hit and see if its lethal
                    owner->addPoint();
                } 
                output = PROJECTILE_COLLISION_PLAYER;
            }
        }
    }
    if (output == PROJECTILE_COLLISION_NONE) { // make sure no other collision has been detected with a wall
        for (auto wall = game->walls()->begin(); wall != game->walls()->end(); wall++) {
            if ((*wall)->checkCollision(centreX, centreY, radius)) {
                output = PROJECTILE_COLLISION_WALL;
            }
        }
    }
    return output;
}

string Projectile::getStateString(void) {
    // get a string representing the projectile
    // format: x, y, ownerID
    stringstream currState;
    currState << strOfLen(projectileRect.x, MESSAGE_LOC_CHAR)
     << strOfLen(projectileRect.y, MESSAGE_LOC_CHAR) << strOfLen(ownerID, MESSAGE_ID_CHAR);
    return currState.str();
}

void Projectile::updateState(int x, int y, Game* game, int id) {
    // set the location of the projectile
    currPosX = x;
    currPosY = y;

    // set the SDL_Rect for the projectile
    projectileRect.x = x;
    projectileRect.y = y;
    setProjectileCentre();

    // set the projectiles new color
    if (id == game->getID()) {
        projectileColors.red = game->primaryColors().red;
        projectileColors.blue = game->primaryColors().blue;
        projectileColors.green = game->primaryColors().green;
    } else {
        projectileColors.red = game->secondaryColors().red;
        projectileColors.blue = game->secondaryColors().blue;
        projectileColors.green = game->secondaryColors().green;
    }

    // set the projectiles new owner
    ownerID = id;
}

bool Projectile::move(Game* game) {
    // moves the projectile,  returns true if the projectile collided and needs to be destroyed
    bool destroyed = false;
    int collision = PROJECTILE_COLLISION_NONE;

    // update positions based on velocity
    currPosX += velx;
    currPosY += vely;
    projectileRect.x = floor(currPosX);
    projectileRect.y = floor(currPosY);

    // move the centre
    setProjectileCentre();

    // check if the projectile collided this frame
    collision = checkCollision(game);
    if (collision != PROJECTILE_COLLISION_NONE) {
        destroyed = true;
    } else if (checkExitMap(projectileRect.x, projectileRect.y, radius) == true) {
        destroyed = true;
    }
    return destroyed;
}

void Projectile::render(SDL_Renderer* renderer) {
    // render the projectile
    SDL_SetTextureColorMod(projectileImage, projectileColors.red,
        projectileColors.green, projectileColors.blue); // modulates the color based on the players settings
    SDL_RenderCopyEx(renderer, projectileImage, NULL, &projectileRect,
        angle, NULL, SDL_FLIP_NONE);// draw the projectile to the window
}

BulletExplosion::BulletExplosion(SDL_Renderer* renderer, SDL_Rect projectileLocation,
    colorSet projectileColors, int id) {
    // create a new explosion object

    // load the image
    explosionImage = loadImage(PROJECTILE_EXPLOSION_IMAGE, renderer);

    // set the objects position based on its parent projectile
    explosionLocation.w = PROJECTILE_EXPLOSION_START_RADIUS * 2;
    explosionLocation.h = PROJECTILE_EXPLOSION_START_RADIUS * 2;
    explosionLocation.x = projectileLocation.x - explosionLocation.w / 2;
    explosionLocation.y = projectileLocation.y - explosionLocation.h / 2;
    radius = PROJECTILE_EXPLOSION_START_RADIUS;

    // set the colors to depend on the projectiles colors
    explosionColors.red = projectileColors.red;
    explosionColors.blue = projectileColors.blue;
    explosionColors.green = projectileColors.green;
    explosionColors.alpha = UI_COLOR_MAX_VALUE;

    // set the owner
    ownerID = id;
}

BulletExplosion::BulletExplosion(Game* game, int x, int y, int id) {
    // create a bullet explosion off of host input

    // load the image
    explosionImage = loadImage(PROJECTILE_EXPLOSION_IMAGE, game->renderer());

    // set the explosions coordinates
    explosionLocation.w = PROJECTILE_EXPLOSION_START_RADIUS * 2;
    explosionLocation.h = PROJECTILE_EXPLOSION_START_RADIUS * 2;
    explosionLocation.x = x;
    explosionLocation.y = y;
    radius = PROJECTILE_EXPLOSION_START_RADIUS;

    // set the color depending on whether the owner was the player of this game instance
    if (id == game->getID()) {
        explosionColors.red = game->primaryColors().red;
        explosionColors.blue = game->primaryColors().blue;
        explosionColors.green = game->primaryColors().green;
    } else {
        explosionColors.red = game->secondaryColors().red;
        explosionColors.blue = game->secondaryColors().blue;
        explosionColors.green = game->secondaryColors().green;
    }
    explosionColors.alpha = UI_COLOR_MAX_VALUE;

    // set the owner
    ownerID = id;
}

BulletExplosion::~BulletExplosion(void) { // destroy the object
    SDL_DestroyTexture(explosionImage);
}

string BulletExplosion::getStateString(void) {
    // get a string representing the objects state
    stringstream output;
    output << strOfLen(explosionLocation.x, MESSAGE_LOC_CHAR)
     << strOfLen(explosionLocation.y, MESSAGE_LOC_CHAR)
     << strOfLen(ownerID, MESSAGE_ID_CHAR);
    return output.str();
}

bool BulletExplosion::updateState(void) {
    // update the explosion each frame
    bool expired = false;
    // find the increase in radius and add it to the current
    double dradius = ((double)(PROJECTILE_EXPLOSION_END_RADIUS - PROJECTILE_EXPLOSION_START_RADIUS) /
        (double)PROJECTILE_EXPLOSION_DURATION);
    radius += dradius;

    // update the alpha to the new color
    explosionColors.alpha -= (double)UI_COLOR_MAX_VALUE / (double)PROJECTILE_EXPLOSION_DURATION;

    // update the objects position parameter
    explosionLocation.h = radius * 2;
    explosionLocation.w = radius * 2;
    explosionLocation.x -= (int)dradius;
    explosionLocation.y -= (int)dradius;

    if (explosionColors.alpha < 0) { // if the object disappears, tell the game to delete it
        expired = true;
    }
    return expired;
}

void BulletExplosion::render(SDL_Renderer* renderer) {
    // render the explosion objects
    SDL_SetTextureColorMod(explosionImage, explosionColors.red,
        explosionColors.green, explosionColors.blue);
    SDL_SetTextureAlphaMod(explosionImage, explosionColors.alpha);
    SDL_RenderCopy(renderer, explosionImage, NULL, &explosionLocation);
}

MapBox::MapBox(int xin, int yin, int win, int hin, int iters, int divChance) {
    // create a new box using the given parameters
    x = xin;
    y = yin;
    w = win;
    h = hin;
    iterations = iters;
    divideChance = divChance;
}

MapBox::~MapBox(void) {
    // clear memory used
}

coordSet MapBox::getCentre(void) {
    // find the centre coordinates of the box
    coordSet output;
    output.x = x + w / 2;
    output.y = y + h / 2;
    return output;
}

list<MapBox*> MapBox::divideBox(void) {
    // split a box into two randomly sized pieces, of at least minimum size
    list<MapBox*> output;
    // roll to determine the direction of the divide
    int num = generateRandInt(0, MAPBOX_DIVIDE_ROLL_MAX);
    bool invalid = false;
    double roll;
    if (num >= divideChance) { // check if the object is chosen to divide vertically or horizontally
        if (h / 2 >= MAPBOX_MINIMUM_HEIGHT) { // check the object is able to divide
            roll = generateRandDouble((double)MAPBOX_MINIMUM_HEIGHT / h,
                1 - (double)MAPBOX_MINIMUM_HEIGHT / h); // generate a fraction to divide the box at along its height
            // create new boxes with 1 less possible iteration, that are less likely to divide along their height
            output.push_front(new MapBox(x, y, w, h*roll, iterations - 1, divideChance + MAPBOX_DIVIDE_CHANCE_CHANGE)); // create a new box for the bottom piece
            output.push_front(new MapBox(x, y + h*roll, w, h*(1 - roll), iterations - 1, divideChance + MAPBOX_DIVIDE_CHANCE_CHANGE)); // create a new box for the top piece
        } else { // tell the game the box can no longer divide
            invalid = true;
        }
    } else {
        if (w / 2 >= MAPBOX_MINIMUM_WIDTH) { // check the box is able to be divided width-wise
            roll = generateRandDouble((double)MAPBOX_MINIMUM_WIDTH / w,
                1 - (double)MAPBOX_MINIMUM_WIDTH / w); // find the point of the divide
            // create new boxes less likely to split along their width
            output.push_front(new MapBox(x, y, w*roll, h, iterations - 1, divideChance - MAPBOX_DIVIDE_CHANCE_CHANGE));
            output.push_front(new MapBox(x + w*roll, y, w*(1 - roll), h, iterations - 1, divideChance - MAPBOX_DIVIDE_CHANCE_CHANGE));
        } else {
            // tell the game the box can no longer divide
            invalid = true;
        }
    }
    if (invalid == true) {
        // put a new box unable to split into the output
        output.push_front(new MapBox(x, y, w, h, 0, 0));
    }
    return output;
}

Wall* MapBox::generateWall(void) {
    // create a wall with the boxes parameters
    return new Wall(x, y, w, h);
}

bool MapBox::checkConnection(MapBox* box) {
    // check if two boxes are within minimum distance of each other
    bool connected = false;
    int cx;
    int cy;
    for (int corner = 0; corner<MAPBOX_NUM_CORNERS; corner++) { // for each corner in the box
        // get the coordinates of the corner
        cx = x + w*(corner % 2);
        cy = y + h*(corner / 2);
        if (box->getX() - MAPBOX_MINIMUM_GAP < cx
         && cx < box->getX() + box->getWidth() + MAPBOX_MINIMUM_GAP) { // if the corners x is potentially colliding
            if (box->getY() - MAPBOX_MINIMUM_GAP < cy
                && cy < box->getY() + box->getHeight() + MAPBOX_MINIMUM_GAP) { // and the corners y is potentially colliding
                // the point is inside the other box, and is invalid
                connected = true;
            }
        }
    }
    for (int corner = 0; corner<MAPBOX_NUM_CORNERS; corner++) { // repeat the process for the other box
        cx = box->getX() + box->getWidth()*(corner % 2);
        cy = box->getY() + box->getHeight()*(corner / 2);
        if (x - MAPBOX_MINIMUM_GAP < cx && cx < x + w + MAPBOX_MINIMUM_GAP) {
            if (y - MAPBOX_MINIMUM_GAP < cy && cy < y + box->h + MAPBOX_MINIMUM_GAP) {
                connected = true;
            }
        }
    }
    return connected;
}

bool MapBox::checkSameBox(MapBox* box) {
    // check if two map boxes are identical
    bool same = false;
    if (box->getX() == x && box->getY() == y && box->getWidth() == w &&
        box->getHeight() == h) {
        same = true;
    }
    return same;
}

MapBox* MapBox::copyBox(void) {
    // create a copy of a box with the same parameters
    return new MapBox(x, y, w, h, iterations, divideChance);
}


void generateMap(forward_list<Wall*>* wallContainer, forward_list<coordSet>* spawnPoints) {
    // create a new map and put its components into the provided forward lists
        
    // create the storage containers used in the process
    list<MapBox*> mapBoxes;
    list<MapBox*> boxesComplete;
    list<MapBox*> boxesFinal;
    list<MapBox*> boxesSplit;
    wallContainer->clear();
    spawnPoints->clear();


    int w;
    int h;
    int x;
    int y;
    for (int i = 0; i<4; i++) { // divide the starting area into 4 even quarters
        w = (GAMESPACE_WIDTH - 2 * GAMESPACE_MARGIN) / 2;
        h = (GAMESPACE_HEIGHT - 2 * GAMESPACE_MARGIN) / 2;
        x = GAMESPACE_MARGIN + w*(i % 2);
        y = GAMESPACE_MARGIN + h*(i / 2);
        mapBoxes.push_front(new MapBox(x, y, w, h, MAPBOX_START_ITERATIONS,
            MAPBOX_DIVIDE_ROLL_MAX/2));
    }

    MapBox* currBox;
    while (mapBoxes.size() > 0) { // while there are still boxes to divide
        // get the first one and remove it from the list
        currBox = mapBoxes.front();
        mapBoxes.pop_front();
        if (currBox->getIterations() == 0) { // check the box is still valid for dividing
            // if its invalid, add a copy to the list of finished boxes
            boxesComplete.push_front(currBox->copyBox());
        } else {
            // otherwise, get a list of boxes created from dividing it, and add them to the list to divide
            boxesSplit = currBox->divideBox();
            for (auto subBox = boxesSplit.begin(); subBox != boxesSplit.end(); subBox++) {
                mapBoxes.push_back(*subBox);
            }
        }
        // delete the box now it has been used
        delete currBox;
    }

    // add a set of boxes to the set that will be used to create the walls
    MapBox* boxSelected = new MapBox(0, 0, 0, 0, 0, 0);
    list<MapBox*> mapBoxesTemp;
    int boxSelection;
    int index;
    while (boxesComplete.size() > 0) { // while there are still boxes that can be added
        mapBoxesTemp.clear();
        boxSelection = generateRandInt(0, boxesComplete.size() - 1); // chose a box from the list
        index = 0;
        for (auto box = boxesComplete.begin(); box != boxesComplete.end(); box++) { // search through the boxes till the desired one is found
            if (index == boxSelection) {
                // add the box to the possible selection
                boxSelected = (*box)->copyBox();
                boxesFinal.push_front(boxSelected);
            }
            index++;
        }

        // go through the remaining boxes and preserve the boxes that can still be used
        for (auto box = boxesComplete.begin(); box != boxesComplete.end(); box++) {
            if (boxSelected->checkConnection(*box) == false) { // check the box is not to close to the current box
                // preserve it if it is still valid
                mapBoxesTemp.push_front(*box);
            } else if (boxSelected->checkSameBox(*box) == false) {
                // if the box wasn't the one last added, put its centre into the list of spawn points
                spawnPoints->push_front((*box)->getCentre());
                delete *box;
            } else {
                // delete the box if it was used
                delete *box;
            }
        }
        boxesComplete.clear();
        boxesComplete = mapBoxesTemp;
    }

    // go through each box and put it wall representing it into the list of walls
    for (auto box = boxesFinal.begin(); box != boxesFinal.end(); box++) {
        wallContainer->push_front((*box)->generateWall());
        delete *box;
    }
}


UDPConnectionClient::UDPConnectionClient(Game* game) {
    // create a object to use in netcode as a client
    socket = SDLNet_UDP_Open(game->cPort()); // open a port
    if (!socket) {
        cout << "SDLNet_UDP_open client: " << SDLNet_GetError() << "\n";
    }
    if (SDLNet_ResolveHost(&connectionIp, game->hIP().c_str(), game->hPort()) == -1) { // translate the hosts ip to a usable value
        cout << "SDLNet_ResolveHost: " << SDLNet_GetError() << "\n";
    }
    // create packets for input and output between the host
    packetIn = SDLNet_AllocPacket(CHARBUFF_LENGTH);
    packetOut = SDLNet_AllocPacket(CHARBUFF_LENGTH);
    // set the output packet to send to the host
    packetOut->address.host = connectionIp.host;
    packetOut->address.port = connectionIp.port;
}

UDPConnectionClient::~UDPConnectionClient(void) {
    // delete the client object
    SDLNet_UDP_Close(socket);
    SDLNet_FreePacket(packetIn);
    SDLNet_FreePacket(packetOut);
}

bool UDPConnectionClient::send(string msg, int times) {
    // send a message to the host
    bool success = true;
    memcpy(packetOut->data, msg.c_str(), msg.length()+1);
    packetOut->len = msg.length()+1;
    for (int j = 0; j < times; j++) { // send the message multiple times to reduce the chance of it being lost
        if (SDLNet_UDP_Send(socket, -1, packetOut) == 0) {
            cout << "SDLNet_UDP_Send client: " << SDLNet_GetError() << "\n";
            success = false;
        }
    }
    return success;
}

connection UDPConnectionClient::recieve(string* field) {
    // recieve a message from the host and put it into field
    // put the sender address into sender to confirm a message was recieved
    connection sender = {0, 0};
    stringstream output;
    if (SDLNet_UDP_Recv(socket, packetIn)) {
        for (int i = 0; i < packetIn->len; i++) {
            output << packetIn->data[i];
        }
        sender.ip = packetIn->address.host;
        sender.port = packetIn->address.port;
    }
    *field = output.str();
    return sender;
}



UDPConnectionServer::UDPConnectionServer(Game* game) {
    // create a host object for networking
    socket = SDLNet_UDP_Open(game->hPort()); // open the port
    if (!socket) {
        cout << "SDLNet_UDP_open server: " << SDLNet_GetError() << "\n";
    }

    // create the packets for sending messages
    packetIn = SDLNet_AllocPacket(CHARBUFF_LENGTH);
    packetOut = SDLNet_AllocPacket(CHARBUFF_LENGTH);
    packetOut->address.host = 0;
    packetOut->address.port = 0;
}

UDPConnectionServer::~UDPConnectionServer(void) {
    // delete the server object
    SDLNet_UDP_Close(socket);
    SDLNet_FreePacket(packetIn);
    SDLNet_FreePacket(packetOut);
}

bool UDPConnectionServer::send(string msg, int times, connection* ips, int numIps) {
    // send a message to all clients
    bool success = true;
    memcpy(packetOut->data, msg.c_str(), msg.length()+1); // load in the message
    packetOut->len = msg.length()+1;
    for (int i = 0; i < numIps; i++) { // go through each connection and send them the message
        if (ips[i].ip != 0) {
            packetOut->address.host = ips[i].ip;
            packetOut->address.port = ips[i].port;
            for (int j = 0; j < times; j++) { // send the message multiple times to reduce the chance of it being lost
                if (SDLNet_UDP_Send(socket, -1, packetOut) == 0) {
                    cout << "SDLNet_UDP_Send server1: " << SDLNet_GetError() << "\n";
                    success = false;
                }
            }
        }
    }
    return success;
}

bool UDPConnectionServer::send(string msg, int times, connection target) {
    // send a message to a single client
    bool success = true;
    memcpy(packetOut->data, msg.c_str(), msg.length()+1); // load the message
    packetOut->len = msg.length()+1;

    // set the destination
    packetOut->address.host = target.ip;
    packetOut->address.port = target.port;

    for (int j = 0; j < times; j++) { // send the message multiple times to reduce the chance of it being lost
        if (SDLNet_UDP_Send(socket, -1, packetOut) == 0) {
            cout << "SDLNet_UDP_Send server2: " << SDLNet_GetError() << "\n";
            success = false;
        }
    }
    return success;
}

connection UDPConnectionServer::recieve(string* field) {
    // recieve a message to the server
    connection sender = {0,0,0};
    stringstream output;
    if (SDLNet_UDP_Recv(socket, packetIn)) { // get a message
        for (int i = 0; i < packetIn->len; i++) {// load the message into the output string
            output << packetIn->data[i];
        }

        // get the sender address
        sender.ip = packetIn->address.host;
        sender.port = packetIn->address.port;
    }

    // get the output into the desired field
    if (field != NULL) {
        *field = output.str();
    }
    return sender;
}



void testDistBetweenPoints() {
    // test distance between points
    assert(distBetweenPoints(0, 0, 1, 1) == sqrt(2));
    assert(distBetweenPoints(0, 0, 0, 0) == sqrt(0));
    assert(distBetweenPoints(1, 0, 1, 1) == 1);
    assert(distBetweenPoints(1, 0, 0, 1) == sqrt(2));
    assert(distBetweenPoints(0, 0, 3, 4) == 5);

    assert(distBetweenPoints(0, 0, 10, 1) == sqrt(101));
    assert(distBetweenPoints(3, 4, 5, 6) == sqrt(8));
    assert(distBetweenPoints(10, 10, -1, 1) == sqrt(202));
    assert(distBetweenPoints(2, 2, 2, 2) == sqrt(0));
    assert(distBetweenPoints(16, 17, 17, 16) == sqrt(2));

    cout << "distBetweenPoints passed all tests\n";
}

void testCheckExitMap(void) {
    // test check exit map
    assert(checkExitMap(0, 0, 10) == true);
    assert(checkExitMap(GAMESPACE_WIDTH, GAMESPACE_HEIGHT, 10) == true);
    assert(checkExitMap(GAMESPACE_WIDTH / 2, GAMESPACE_HEIGHT / 2, 10) == false);
    assert(checkExitMap(GAMESPACE_WIDTH - 10, GAMESPACE_HEIGHT / 2, 10) == false);
    assert(checkExitMap(GAMESPACE_WIDTH / 2, GAMESPACE_HEIGHT - 10, 5) == false);

    assert(checkExitMap(GAMESPACE_WIDTH - 5, GAMESPACE_HEIGHT / 2, 10) == true);
    assert(checkExitMap(GAMESPACE_WIDTH / 2, GAMESPACE_HEIGHT / 2, SCREEN_WIDTH_DEFAULT) == true);
    assert(checkExitMap(10, 10, 10) == false);
    assert(checkExitMap(9, 9, 10) == true);
    assert(checkExitMap(0, 0, 0) == false);

    cout << "checkExitMap passed all tests\n";
}

void testGenerateRandInt(void) {
    // for testing number generation, test a set of ranges a large number
    // of times and make sure results always lie in a range
    int test;
    for (int i = 0; i < 1000; i++) {
        test = generateRandInt(0, 100);
        assert(0 <= test && test <= 100);
    }
    for (int i = 0; i < 1000; i++) {
        test = generateRandInt(2, 10);
        assert(2 <= test && test <= 10);
    }
    for (int i = 0; i < 1000; i++) {
        test = generateRandInt(-10, 10);
        assert(-10 <= test && test <= 10);
    }
    for (int i = 0; i < 1000; i++) {
        test = generateRandInt(-20, -10);
        assert(-20 <= test && test <= -10);
    }
    for (int i = 0; i < 1000; i++) {
        test = generateRandInt(12, 1100); // 12 to 12
        assert(12 <= test && test <= 1100);
    }

    // test single possibility edge cases
    assert(generateRandInt(0, 0) == 0);
    assert(generateRandInt(12, 12) == 12);
    assert(generateRandInt(-5, -5) == -5);

    cout << "generateRandInt passed all tests\n";
}

void testGenerateRandDouble(void) {
    // for testing number generation, test a set of ranges a large number
    // of times and make sure results always lie in a range
    double test;
    for (int i = 0; i < 1000; i++) {
        test = generateRandDouble(0.0, 100.0);
        assert(0.0 <= test && test <= 100.0);
    }
    for (int i = 0; i < 1000; i++) {
        test = generateRandDouble(2.5, 10.89);
        assert(2.5 <= test && test <= 10.89);
    }
    for (int i = 0; i < 1000; i++) {
        test = generateRandDouble(-10.55, 10.55);
        assert(-10.55 <= test && test <= 10.55);
    }
    for (int i = 0; i < 1000; i++) {
        test = generateRandDouble(-245.678, -12.21);
        assert(-245.678 <= test && test <= -12.21);
    }
    for (int i = 0; i < 1000; i++) {
        test = generateRandDouble(1.111, 111111001.1101); // 12 to 12
        assert(1.111 <= test && test <= 111111001.1101);
    }

    // test single possibility edge cases
    assert(generateRandDouble(0.0, 0.0) == 0.0);
    assert(generateRandDouble(12.12, 12.12) == 12.12);
    assert(generateRandDouble(-5.67, -5.67) == -5.67);

    cout << "generateRandDouble passed all tests\n";
}

void testStrOfLen(void) {
    // test str of len
    assert(strOfLen(123, 4) == "0123");
    assert(strOfLen(0, 4) == "0000");
    assert(strOfLen(8989, 4) == "8989");
    assert(strOfLen(123, 3) == "123");
    assert(strOfLen(4, 4) == "0004");

    assert(strOfLen(12, 12) == "000000000012");
    assert(strOfLen(123, 5) == "00123");
    assert(strOfLen(310, 4) == "0310");
    assert(strOfLen(203, 3) == "203");
    assert(strOfLen(4005, 6) == "004005");

    cout << "strOfLen passed all test\n";
}

void testGetLength(void) {
    // test get length
    assert(getLength(123) == 3);
    assert(getLength(4444) == 4);
    assert(getLength(12) == 2);
    assert(getLength(0) == 1);
    assert(getLength(8) == 1);

    assert(getLength(9630369) == 7);
    assert(getLength(2000) == 4);
    assert(getLength(300030003) == 9);
    assert(getLength(9989) == 4);
    assert(getLength(1000000000) == 10);

    cout << "getLength passed all tests\n";
}