// Created by Aidan Hunt 24/1/17

using namespace std;

// declarations of different classes
class Game;
class Menu;
class MainPage;
class LobbyPage;
class OptionPage;
class TutorialPage;
class Button;
class Player;
class DeathObject;
class Weapon;
class Wall;
class Projectile;
class BulletExplosion;
class MapBox;
class UDPConnectionServer;
class UDPConnectionClient;


typedef struct _direction {
    int x;
    int y;
} direction;

bool operator==(const direction lhs, const direction rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator!=(const direction lhs, const direction rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

typedef struct _connection {
    Uint16 port;
    Uint32 ip;
    int weapon;
    int id;
    string username;
} connection;

bool operator==(const connection lhs, const connection rhs) {
    return lhs.port == rhs.port && lhs.ip == rhs.ip;
}

typedef struct _hudInfo {
    SDL_Texture* ammoIcon;
    SDL_Texture* cooldownIcon;
    SDL_Texture* pauseText;
    SDL_Texture* winText;
    Button* resume;
    Button* quit;
} hudInfo;

typedef struct _colorSet {
    int red;
    int green;
    int blue;
    double alpha;
} colorSet;

colorSet operator*(const colorSet lhs, const double rhs) {
    return {static_cast<int>(lhs.red*rhs), static_cast<int>(lhs.green*rhs), static_cast<int>(lhs.blue*rhs)};
}

bool operator==(const colorSet lhs, const colorSet rhs) {
    return lhs.red == rhs.red && lhs.green == rhs.green && lhs.blue == rhs.blue;
}

bool operator!=(const colorSet lhs, const colorSet rhs) {
    return !(lhs == rhs);
}

typedef struct _coordSet {
    int x;
    int y;
} coordSet;

typedef struct _playerState {
    int x;
    int y;
    int angle;

    bool rolling;
    int rollFrames;
    int rollX;
    int rollY;

    bool invuln;
    int invulnFrames;
    bool alive;
    int health;

    int deathFrames;
    int dmX;
    int dmY;

    int id;
    int weapID;
    int score;

    int ammo;
    int reloadFrames;
    int cooldownFrames;
} playerState;

typedef struct _userAction {
    int mouseX;
    int mouseY;
    bool mouseDown;
    bool rDown;
    bool shiftDown;
    int moveHor;
    int moveVert;
} userAction;

typedef struct _playerScore {
    string name;
    int score;
    int id;
} playerScore;

/*------------- All program constants defined here ------------------*/

// General Parameters
const int SCREEN_FPS = 60; // desired framerate of the screen


// constants used in setting up the game instance
const int GAME_MAX_PLAYERS = 4;
const int GAME_USERNAME_MAX_LEN = 8;
const string GAME_DEFAULT_USERNAME = "Twelve";
const int GAME_CONNECT_WAIT = 100;

const int GAME_WIN_SCORE = 10;
const int GAME_WIN_DELAY_SEC = 3;
const int GAME_WIN_DELAY = GAME_WIN_DELAY_SEC*SCREEN_FPS;

// values of the different colour schemes that can be used
const colorSet COLOR_NONE = {0,0,0};

// red
const string COLOR_RED_NAME = "red";
const int COLOR_RED_RED = 255;
const int COLOR_RED_GREEN = 0;
const int COLOR_RED_BLUE = 0;

// green
const string COLOR_GREEN_NAME = "green";
const int COLOR_GREEN_RED = 0;
const int COLOR_GREEN_GREEN = 200;
const int COLOR_GREEN_BLUE = 0;

// blue
const string COLOR_BLUE_NAME = "blue";
const int COLOR_BLUE_RED = 50;
const int COLOR_BLUE_GREEN = 50;
const int COLOR_BLUE_BLUE = 255;

// purple
const string COLOR_PURPLE_NAME = "purple";
const int COLOR_PURPLE_RED = 200;
const int COLOR_PURPLE_GREEN = 0;
const int COLOR_PURPLE_BLUE = 200;

// yellow
const string COLOR_YELLOW_NAME = "yellow";
const int COLOR_YELLOW_RED = 255;
const int COLOR_YELLOW_GREEN = 255;
const int COLOR_YELLOW_BLUE = 0;

// cyan
const string COLOR_CYAN_NAME = "cyan";
const int COLOR_CYAN_RED = 0;
const int COLOR_CYAN_GREEN = 255;
const int COLOR_CYAN_BLUE = 255;

// pink
const string COLOR_PINK_NAME = "pink";
const int COLOR_PINK_RED = 255;
const int COLOR_PINK_GREEN = 20;
const int COLOR_PINK_BLUE = 147;

// orange
const string COLOR_ORANGE_NAME = "orange";
const int COLOR_ORANGE_RED = 255;
const int COLOR_ORANGE_GREEN = 160;
const int COLOR_ORANGE_BLUE = 0;

// white
const string COLOR_WHITE_NAME = "white";
const int COLOR_WHITE_RED = 255;
const int COLOR_WHITE_GREEN = 255;
const int COLOR_WHITE_BLUE = 255;

// grey
const string COLOR_GREY_NAME = "grey";
const int COLOR_GREY_RED = 155;
const int COLOR_GREY_GREEN = 155;
const int COLOR_GREY_BLUE = 155;

const int COLOR_NUM = 9;
const int COLOR_NUM_ROW = 3;

const string COLOR_NAMES[] = {
    COLOR_RED_NAME,
     COLOR_GREEN_NAME,
     COLOR_BLUE_NAME,
     COLOR_PURPLE_NAME,
     COLOR_YELLOW_NAME,
     COLOR_CYAN_NAME,
     COLOR_ORANGE_NAME,
     COLOR_PINK_NAME,
     COLOR_WHITE_NAME
};

map<string, colorSet> COLOR_VALUES = 
{
    {COLOR_RED_NAME, {COLOR_RED_RED, COLOR_RED_GREEN, COLOR_RED_BLUE}},
    {COLOR_GREEN_NAME, {COLOR_GREEN_RED, COLOR_GREEN_GREEN, COLOR_GREEN_BLUE}},
    {COLOR_BLUE_NAME, {COLOR_BLUE_RED, COLOR_BLUE_GREEN, COLOR_BLUE_BLUE}},
    {COLOR_PURPLE_NAME, {COLOR_PURPLE_RED, COLOR_PURPLE_GREEN, COLOR_PURPLE_BLUE}},
    {COLOR_YELLOW_NAME, {COLOR_YELLOW_RED, COLOR_YELLOW_GREEN, COLOR_YELLOW_BLUE}},
    {COLOR_CYAN_NAME, {COLOR_CYAN_RED, COLOR_CYAN_GREEN, COLOR_CYAN_BLUE}},
    {COLOR_ORANGE_NAME, {COLOR_ORANGE_RED, COLOR_ORANGE_GREEN, COLOR_ORANGE_BLUE}},
    {COLOR_PINK_NAME, {COLOR_PINK_RED, COLOR_PINK_GREEN, COLOR_PINK_BLUE}},
    {COLOR_WHITE_NAME, {COLOR_WHITE_RED, COLOR_WHITE_GREEN, COLOR_WHITE_BLUE}}
};


// RGB color of background on images to allow tranparency
const int COLOR_KEY_RED = 255;
const int COLOR_KEY_GREEN = 255;
const int COLOR_KEY_BLUE = 255;

// directions returned by the get direction function for movement
const direction MOVE_NONE = { 0,0 };
const direction MOVE_UP = { 0,-1 };
const direction MOVE_DOWN = { 0,1 };
const direction MOVE_LEFT = { -1,0 };
const direction MOVE_RIGHT = { 1,0 };
const direction MOVE_UP_LEFT = { -1,-1 };
const direction MOVE_UP_RIGHT = { 1,-1 };
const direction MOVE_DOWN_LEFT = { -1,1 };
const direction MOVE_DOWN_RIGHT = { 1,1 };

// Character related constants
const string CHARACTER_IMAGE_AR_LOCATION = "images/characterAssaultRifle.png"; // path to the character spritesheet
const string CHARACTER_IMAGE_SHOTGUN_LOCATION = "images/characterShotgun.png"; // path to the character spritesheet
const string CHARACTER_IMAGE_PISTOL_LOCATION = "images/characterPistol.png"; // path to the character spritesheet
const string CHARACTER_ROLL_IMAGE = "images/dash.png";
const string CHARACTER_DEATH_IMAGE = "images/deathCircle.png";
const string CHARACTER_INVULN_IMAGE = "images/invuln.png";

const double CHARACTER_VEL_MAX_PS = 300;
const double CHARACTER_VEL_MAX = CHARACTER_VEL_MAX_PS / SCREEN_FPS; // Max movementspeed of the player in any direction
const double CHARACTER_ACCEL_PS = 36;
const double CHARACTER_ACCEL_PER_FRAME = CHARACTER_ACCEL_PS / SCREEN_FPS; // Acceleration speed of the plater
const double CHARACTER_DECCEL_PS = 24;
const double CHARACTER_DECEL_PER_FRAME = CHARACTER_DECCEL_PS / SCREEN_FPS; // Multiplier used to decelerate player when not giving input movement

const int CHARACTER_WIDTH = 40; // width of the player on the default screen size
const int CHARACTER_HEIGHT = 40; // height of the player in the default screen size

const double CHARACTER_ROLL_DURATION_SEC = 0.25;
const double CHARACTER_ROLL_DURATION = CHARACTER_ROLL_DURATION_SEC*SCREEN_FPS;
const double CHARACTER_ROLL_SPEED_PS = 850;
const double CHARACTER_ROLL_SPEED = CHARACTER_ROLL_SPEED_PS / SCREEN_FPS;
const double CHARACTER_ROLL_COOLDOWN_SEC = 3;
const double CHARACTER_ROLL_COOLDOWN = CHARACTER_ROLL_COOLDOWN_SEC*SCREEN_FPS;
const int CHARACTER_ROLL_ALPHA = 100;

const int CHARACTER_MAIN_ID = 0; // ID number of the main character for the game instance

const int CHARACTER_MAX_HP = 3; // max health a player can have
const double CHARACTER_DEATH_DURATION_SEC = 3;
const double CHARACTER_DEATH_DURATION = CHARACTER_DEATH_DURATION_SEC*SCREEN_FPS; // the number of frames the player remains dead for
const int CHARACTER_MIN_RESPAWN_RANGE = CHARACTER_WIDTH * 4;
const double CHARACTER_INVULN_SEC = 2.5;
const double CHARACTER_INVULN_FRAMES = CHARACTER_INVULN_SEC*SCREEN_FPS;

const int CHARACTER_ROLL_OUTLINE_WIDTH = CHARACTER_WIDTH*1.5;
const int CHARACTER_ROLL_OUTLINE_HEIGHT = CHARACTER_HEIGHT*1.5;

const int CHARACTER_INVULN_IMAGE_WIDTH = CHARACTER_WIDTH*1.7;
const int CHARACTER_INVULN_IMAGE_HEIGHT = CHARACTER_HEIGHT*1.7;
const int CHARACTER_INVULN_ALPHA = 150;

// codes for the different weapons player can use
enum CHARACTER_WEAPONS {
    CHARACTER_WEAPON_ASSAULT_RIFLE,
    CHARACTER_WEAPON_PISTOL,
    CHARACTER_WEAPON_SHOTGUN,
    CHARACTER_WEAPON_SNIPER
};

// Weapon Related Constants
// ASSAULT RIFLE
const int AR_CLIP_SIZE = 24; // number of shots before AR reloads
const double AR_MAX_BULLET_SPREAD = 14; // max angle bullets can deflect by
const double AR_RELOAD_SEC = 1.4;
const double AR_RELOAD_FRAMES = AR_RELOAD_SEC*SCREEN_FPS; // number of frames in reload animation
const double AR_SHOT_DELAY_SEC = 0.2;
const double AR_SHOT_DELAY = AR_SHOT_DELAY_SEC*SCREEN_FPS; //number of frames between each projectile firing
const double AR_PROJECTILE_SPEED_PS = 900;
const double AR_PROJECTILE_SPEED = AR_PROJECTILE_SPEED_PS / SCREEN_FPS; // speed of an AR projectile

                                                                        // PISTOL
const int PISTOL_CLIP_SIZE = 8;
const int PISTOL_MIN_BULLET_SPREAD = 5;
const int PISTOL_MAX_BULLET_SPREAD = 50;
const double PISTOL_RELOAD_SEC = 1.2;
const double PISTOL_RELOAD_FRAMES = PISTOL_RELOAD_SEC*SCREEN_FPS;
const double PISTOL_RECOIL_INCREASE_PER_SHOT = 20;
const double PISTOL_RECOIL_RECOVERY_PS = 55;
const double PISTOL_RECOIL_RECOVERY_PER_FRAME = PISTOL_RECOIL_RECOVERY_PS / SCREEN_FPS;
const double PISTOL_PROJECTILE_SPEED_PS = 1020;
const double PISTOL_PROJECTILE_SPEED = PISTOL_PROJECTILE_SPEED_PS / SCREEN_FPS;

// SHOTGUN
const int SHOTGUN_PROJECTILES_PER_SHOT = 5;
const double SHOTGUN_SPREAD_RANGE = 42;
const double SHOTGUN_PROJECTILE_SPREAD = SHOTGUN_SPREAD_RANGE / SHOTGUN_PROJECTILES_PER_SHOT;
const double SHOTGUN_SHOT_DELAY_SEC = 1.3;
const double SHOTGUN_SHOT_DELAY = SHOTGUN_SHOT_DELAY_SEC*SCREEN_FPS;
const double SHOTGUN_PROJECTILE_SPEED_PS = 500;
const double SHOTGUN_PROJECTILE_SPEED = SHOTGUN_PROJECTILE_SPEED_PS / SCREEN_FPS;


/* Defaults
// Weapon Related Constants
// ASSAULT RIFLE
const int AR_CLIP_SIZE = 15; // number of shots before AR reloads
const double AR_MAX_BULLET_SPREAD = 13; // max angle bullets can deflect by
const double AR_RELOAD_SEC = 1.8;
const double AR_RELOAD_FRAMES = AR_RELOAD_SEC*SCREEN_FPS; // number of frames in reload animation
const double AR_SHOT_DELAY_SEC = 0.23;
const double AR_SHOT_DELAY = AR_SHOT_DELAY_SEC*SCREEN_FPS; //number of frames between each projectile firing
const double AR_PROJECTILE_SPEED_PS = 1000;
const double AR_PROJECTILE_SPEED = AR_PROJECTILE_SPEED_PS/SCREEN_FPS; // speed of an AR projectile

// PISTOL
const int PISTOL_CLIP_SIZE = 6;
const int PISTOL_MIN_BULLET_SPREAD = 5;
const int PISTOL_MAX_BULLET_SPREAD = 40;
const double PISTOL_RELOAD_SEC = 0.9;
const double PISTOL_RELOAD_FRAMES = PISTOL_RELOAD_SEC*SCREEN_FPS;
const double PISTOL_RECOIL_INCREASE_PER_SHOT = 25;
const double PISTOL_RECOIL_RECOVERY_PS = 60;
const double PISTOL_RECOIL_RECOVERY_PER_FRAME = PISTOL_RECOIL_RECOVERY_PS/SCREEN_FPS;
const double PISTOL_PROJECTILE_SPEED_PS = 900;
const double PISTOL_PROJECTILE_SPEED = PISTOL_PROJECTILE_SPEED_PS/SCREEN_FPS;

// SHOTGUN
const int SHOTGUN_PROJECTILES_PER_SHOT = 5;
const double SHOTGUN_SPREAD_RANGE = 50;
const double SHOTGUN_PROJECTILE_SPREAD = SHOTGUN_SPREAD_RANGE/SHOTGUN_PROJECTILES_PER_SHOT;
const double SHOTGUN_SHOT_DELAY_SEC = 0.8;
const double SHOTGUN_SHOT_DELAY = SHOTGUN_SHOT_DELAY_SEC*SCREEN_FPS;
const double SHOTGUN_PROJECTILE_SPEED_PS = 600;
const double SHOTGUN_PROJECTILE_SPEED = SHOTGUN_PROJECTILE_SPEED_PS/SCREEN_FPS;
*/

// SNIPER




// Projectile related constants
const string PROJECTILE_IMAGE_LOCATION = "images/invuln.png"; // location of the bulet sprite
const int PROJECTILE_WIDTH = 8; // width of projectile image on default screen size
const int PROJECTILE_HEIGHT = 8; // height of projectile image on default screen size

                                 // identifiers for which object type an object collides with
enum PROJECTILE_COLLISION_IDENTIFIERS {
    PROJECTILE_COLLISION_NONE,
    PROJECTILE_COLLISION_PLAYER,
    PROJECTILE_COLLISION_WALL
};

const int PROJECTILE_EXPLOSION_START_RADIUS = 8;
const int PROJECTILE_EXPLOSION_END_RADIUS = 12;
const double PROJECTILE_EXPLOSION_DURATION_SEC = 0.3;
const int PROJECTILE_EXPLOSION_DURATION = PROJECTILE_EXPLOSION_DURATION_SEC*SCREEN_FPS;
const string PROJECTILE_EXPLOSION_IMAGE = "images/deathCircle.png";

// Wall related constats
const double WALL_COLOR_SCALE = 0.65;
const double SHADOW_COLOR_SCALE = 0.3;

// Screen Parameters
const double SCREEN_TICKRATE = 1000.0 / SCREEN_FPS; // duration of each frame on the screen

const int SCREEN_FULLSCREEN = false; // whether the screen should be fullscreen

const int SCREEN_WIDTH_DEFAULT = 1366; // width of screen to scale against
const int SCREEN_HEIGHT_DEFAULT = 768; // height of screen to scale against
const char* SCREEN_NAME = "QuickShot"; // Name of window seen at the top of the screen

const int SCREEN_WIDTH = 1156; // size of screen when no value is in the config file
const int SCREEN_HEIGHT = 650;


// Gamespace parameters
const int GAMESPACE_WIDTH = SCREEN_WIDTH_DEFAULT*0.76; // the gameplay space takes a square on the far right
const int GAMESPACE_HEIGHT = SCREEN_HEIGHT_DEFAULT;
const int GAMESPACE_TOPLEFT_X = SCREEN_WIDTH_DEFAULT - GAMESPACE_WIDTH;
const int GAMESPACE_TOPLEFT_Y = 0;

const int GAMESPACE_MARGIN = 100; // margin within the gamespace where no walls can appear


                                  // Game UI parameters
const int UI_COLOR_MAX_VALUE = 255;
const string UI_GAME_CURSOR_LOCATION = "images/gameCursor.png";
const string UI_FONT_LOCATION = "images/ethnocentric.ttf";

const int UI_CURSOR_WIDTH = 24;
const int UI_CURSOR_HEIGHT = 24;

const int UI_FONT_SIZE = 120; // affects detail and size of the text loaded
const double UI_FONT_HEIGHT_TO_WIDTH = 0.5;

const int UI_PAUSE_ALPHA = 175;
const double UI_PAUSE_COLOR_MUL = 0.4;

const string UI_HOST = "Host:";
const int UI_HOST_LEN = UI_HOST.length();
const int UI_HOST_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.08;
const int UI_HOST_WIDTH = UI_HOST_LEN*UI_HOST_HEIGHT*UI_FONT_HEIGHT_TO_WIDTH;
const int UI_HOST_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.7;
const int UI_HOST_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.25;

const string UI_WEAPON = "Weapon:";
const int UI_WEAPON_LEN = UI_WEAPON.length();
const int UI_WEAPON_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.08;
const int UI_WEAPON_WIDTH = UI_WEAPON_LEN*UI_WEAPON_HEIGHT*UI_FONT_HEIGHT_TO_WIDTH;
const int UI_WEAPON_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.7;
const int UI_WEAPON_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.55;

const string UI_WAITING = "Waiting for Connection...";
const int UI_WAITING_LEN = UI_WAITING.length();
const int UI_WAITING_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.1;
const int UI_WAITING_WIDTH = UI_WAITING_LEN*UI_WAITING_HEIGHT*UI_FONT_HEIGHT_TO_WIDTH;
const int UI_WAITING_TOPLEFT_X = (SCREEN_WIDTH_DEFAULT-UI_WAITING_WIDTH)/2;
const int UI_WAITING_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.45;

const int UI_LOBBY_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.15;
const int UI_LOBBY_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.25;
const int UI_LOBBY_WIDTH = SCREEN_WIDTH_DEFAULT*0.4;
const int UI_LOBBY_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.08;
const int UI_LOBBY_GAP = SCREEN_HEIGHT_DEFAULT*0.03;

const int UI_LOBBY_NAME_TOPLEFT_X = UI_LOBBY_TOPLEFT_X+SCREEN_WIDTH_DEFAULT*0.01;

const string UI_PAUSE_TEXT = "Game Paused";
const int UI_PAUSE_LEN = UI_PAUSE_TEXT.length();
const int UI_PAUSE_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.25;
const int UI_PAUSE_WIDTH = UI_PAUSE_LEN*UI_PAUSE_HEIGHT*UI_FONT_HEIGHT_TO_WIDTH;
const int UI_PAUSE_TOPLEFT_X = (SCREEN_WIDTH_DEFAULT-UI_PAUSE_WIDTH)/2;
const int UI_PAUSE_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.03;

const int UI_SCORE_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.01;
const int UI_SCORE_TOPLEFT_Y = SCREEN_WIDTH_DEFAULT*0.01;
const int UI_SCORE_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.06;
const int UI_SCORE_GAP = SCREEN_HEIGHT_DEFAULT*0.02;
const int UI_SCORE_MARGIN = SCREEN_WIDTH_DEFAULT*0.01;

const string UI_HOST_INFO = "Waiting for host";
const int UI_HOST_INFO_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.08;
const int UI_HOST_INFO_WIDTH = UI_HOST_INFO_HEIGHT*UI_FONT_HEIGHT_TO_WIDTH*UI_HOST_INFO.length();
const int UI_HOST_INFO_TOPLEFT_X = SCREEN_WIDTH_DEFAULT-(SCREEN_WIDTH_DEFAULT*0.03+UI_HOST_INFO_WIDTH);
const int UI_HOST_INFO_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT-(SCREEN_WIDTH_DEFAULT*0.03+UI_HOST_INFO_HEIGHT);

const string UI_CONTROLS_HEADER = "Controls";
const int UI_CONTROLS_HEADER_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.13;
const int UI_CONTROLS_HEADER_WIDTH = UI_CONTROLS_HEADER_HEIGHT*UI_FONT_HEIGHT_TO_WIDTH*UI_CONTROLS_HEADER.length();
const int UI_CONTROLS_HEADER_TOPLEFT_X = (SCREEN_WIDTH_DEFAULT-UI_CONTROLS_HEADER_WIDTH)/2;
const int UI_CONTROLS_HEADER_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.03;

const string UI_CONTROLS_CONTENT_LOCATION = "images/controlsMenu.png";


const int UI_WINDISPLAY_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.15;


const int UI_BACKGROUND_ADDITION = 150; //number added during calculation of background color
                                        // multiplier on the primary color used in determining background color
const double UI_BACKGROUND_MULTIPLIER = 1 - (double)UI_BACKGROUND_ADDITION*1.3 / UI_COLOR_MAX_VALUE;

const int UI_BACKGROUND_PATTERN_WIDTH = 50;
const int UI_BACKGROUND_PATTERN_HEIGHT = 50;
const int UI_BACKGROUND_PATTERN_ROW = GAMESPACE_WIDTH / UI_BACKGROUND_PATTERN_WIDTH + 1;
const int UI_BACKGROUND_PATTERN_COL = GAMESPACE_HEIGHT / UI_BACKGROUND_PATTERN_HEIGHT + 1;
const int UI_BACKGROUND_PATTERN_COUNT = 6;
const string UI_BACKGROUND_PATTERN_PREFIX = "images/pattern";
const string UI_BACKGROUND_PATTERN_TYPE = ".png";
const int UI_BACKGROUND_PATTERN_ALPHA = 20;

const int UI_RESOLUTION_NUM = 5;
const double UI_RESOLUTION_MULTIPLIER = (double)1366/768; // multiplier on a resolution height to get its length

const int UI_RESOLUTIONS[] = {
    350,
    525,
    650,
    900,
    1200,
};



const int MENU_NONE = 0;
const int MENU_QUIT = 1;
const int MENU_LAUNCH = 2;
const int MENU_SET_MAIN = 3;
const int MENU_SET_OPTIONS = 4;
const int MENU_SET_LOBBY_HOST = 5;
const int MENU_SET_LOBBY_CLIENT = 6;
const int MENU_SET_CONTROLS = 7;

enum BUTTON_TYPE {
    BUTTON_MENU,
    BUTTON_RADIO
};
const int BUTTON_OUTLINE_WIDTH = 10;

const int BUTTON_MAIN_WIDTH = SCREEN_WIDTH_DEFAULT*0.4;
const int BUTTON_MAIN_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.11;
const int BUTTON_MAIN_TOPLEFT_X = (SCREEN_WIDTH_DEFAULT-BUTTON_MAIN_WIDTH)/2-SCREEN_WIDTH_DEFAULT*0.1;
const int BUTTON_MAIN_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.3;
const int BUTTON_MAIN_GAP = SCREEN_HEIGHT_DEFAULT*0.04;

const int BUTTON_HOST_WIDTH = SCREEN_WIDTH_DEFAULT*0.08;
const int BUTTON_HOST_HEIGHT = BUTTON_HOST_WIDTH;
const int BUTTON_HOST_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.7;
const int BUTTON_HOST_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.35;
const string BUTTON_HOST_IMAGE = "images/tick.png";

const int BUTTON_CLIENT_WIDTH = BUTTON_HOST_WIDTH;
const int BUTTON_CLIENT_HEIGHT = BUTTON_HOST_HEIGHT;
const int BUTTON_CLIENT_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.7+BUTTON_HOST_WIDTH+SCREEN_WIDTH_DEFAULT*0.05;
const int BUTTON_CLIENT_TOPLEFT_Y = BUTTON_HOST_TOPLEFT_Y;
const string BUTTON_CLIENT_IMAGE = "images/cross.png";

const int BUTTON_PISTOL_WIDTH = SCREEN_WIDTH_DEFAULT*0.06;
const int BUTTON_PISTOL_HEIGHT = BUTTON_PISTOL_WIDTH;
const int BUTTON_PISTOL_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.7;
const int BUTTON_PISTOL_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.65;

const int BUTTON_AR_WIDTH = BUTTON_PISTOL_WIDTH;
const int BUTTON_AR_HEIGHT = BUTTON_PISTOL_HEIGHT;
const int BUTTON_AR_TOPLEFT_X = BUTTON_PISTOL_TOPLEFT_X+BUTTON_PISTOL_WIDTH+SCREEN_WIDTH_DEFAULT*0.03;
const int BUTTON_AR_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.65;

const int BUTTON_SHOTGUN_WIDTH = BUTTON_PISTOL_WIDTH;
const int BUTTON_SHOTGUN_HEIGHT = BUTTON_PISTOL_HEIGHT;
const int BUTTON_SHOTGUN_TOPLEFT_X = BUTTON_AR_TOPLEFT_X+BUTTON_AR_WIDTH+SCREEN_WIDTH_DEFAULT*0.03;
const int BUTTON_SHOTGUN_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.65;



const int BUTTON_BACK_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.05;
const int BUTTON_BACK_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.85;
const int BUTTON_BACK_WIDTH = SCREEN_WIDTH_DEFAULT*0.14;
const int BUTTON_BACK_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.08;

const int BUTTON_LAUNCH_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.81;
const int BUTTON_LAUNCH_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.85;
const int BUTTON_LAUNCH_WIDTH = SCREEN_WIDTH_DEFAULT*0.14;
const int BUTTON_LAUNCH_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.08;

const int BUTTON_FULL_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.3;
const int BUTTON_FULL_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.82;
const int BUTTON_FULL_WIDTH = SCREEN_HEIGHT_DEFAULT*0.15;
const int BUTTON_FULL_HEIGHT = BUTTON_FULL_WIDTH;
const string BUTTON_FULL_IMAGE = "images/fullscreen.png";

const double BUTTON_MENU_COLOR_MULTIPLIER = 0.7;

const int BUTTON_COLOR_WIDTH = SCREEN_HEIGHT_DEFAULT*0.07;
const int BUTTON_COLOR_GAP = SCREEN_HEIGHT_DEFAULT*0.04;

const int BUTTON_COLOR_PRIM_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.75;
const int BUTTON_COLOR_PRIM_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.23;

const int BUTTON_COLOR_SEC_TOPLEFT_X = BUTTON_COLOR_PRIM_TOPLEFT_X;
const int BUTTON_COLOR_SEC_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.62;

const double BUTTON_COLOR_SEC_MULTIPLIER = 0.8;

const int BUTTON_RES_WIDTH = SCREEN_HEIGHT_DEFAULT*0.5;
const int BUTTON_RES_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.075;
const int BUTTON_RES_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.12;
const int BUTTON_RES_TOPLEFT_Y = SCREEN_WIDTH_DEFAULT*0.15;
const int BUTTON_RES_GAP = SCREEN_HEIGHT_DEFAULT*0.035;


const int BUTTON_RESUME_WIDTH = SCREEN_WIDTH_DEFAULT*0.6;
const int BUTTON_RESUME_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.175;
const int BUTTON_RESUME_TOPLEFT_X = (SCREEN_WIDTH_DEFAULT-BUTTON_RESUME_WIDTH)/2;
const int BUTTON_RESUME_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.45;

const int BUTTON_QUIT_WIDTH = BUTTON_RESUME_WIDTH;
const int BUTTON_QUIT_HEIGHT = BUTTON_RESUME_HEIGHT;
const int BUTTON_QUIT_TOPLEFT_X = BUTTON_RESUME_TOPLEFT_X;
const int BUTTON_QUIT_TOPLEFT_Y = BUTTON_RESUME_TOPLEFT_Y+BUTTON_RESUME_HEIGHT+SCREEN_HEIGHT_DEFAULT*0.05;


const string MAIN_HEADER = SCREEN_NAME;
const int MAIN_HEADER_LEN = MAIN_HEADER.length();
const int MAIN_HEADER_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.25;
const int MAIN_HEADER_WIDTH = MAIN_HEADER_LEN*MAIN_HEADER_HEIGHT*UI_FONT_HEIGHT_TO_WIDTH;
const int MAIN_HEADER_TOPLEFT_X = (SCREEN_WIDTH_DEFAULT-MAIN_HEADER_WIDTH)/2;
const int MAIN_HEADER_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.03;



const string OPTIONS_HEADER = "Options";
const int OPTIONS_HEADER_LEN = OPTIONS_HEADER.length();
const int OPTIONS_HEADER_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.14;
const int OPTIONS_HEADER_WIDTH = OPTIONS_HEADER_LEN*OPTIONS_HEADER_HEIGHT*UI_FONT_HEIGHT_TO_WIDTH;
const int OPTIONS_HEADER_TOPLEFT_X = (SCREEN_WIDTH_DEFAULT-OPTIONS_HEADER_WIDTH)/2;
const int OPTIONS_HEADER_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.03;

const string OPTIONS_RESNAME = "Resolution:";
const int OPTIONS_RESNAME_LEN = OPTIONS_HEADER.length();
const int OPTIONS_RESNAME_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.08;
const int OPTIONS_RESNAME_WIDTH = OPTIONS_RESNAME_LEN*OPTIONS_RESNAME_HEIGHT*UI_FONT_HEIGHT_TO_WIDTH;
const int OPTIONS_RESNAME_TOPLEFT_X = SCREEN_WIDTH_DEFAULT*0.05;
const int OPTIONS_RESNAME_TOPLEFT_Y = SCREEN_HEIGHT_DEFAULT*0.17;

const string OPTIONS_PRIMTEXT = "Primary:";
const int OPTIONS_PRIMTEXT_LEN = OPTIONS_PRIMTEXT.length();
const int OPTIONS_PRIMTEXT_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.06;
const int OPTIONS_PRIMTEXT_WIDTH = OPTIONS_PRIMTEXT_LEN*OPTIONS_PRIMTEXT_HEIGHT*UI_FONT_HEIGHT_TO_WIDTH;
const int OPTIONS_PRIMTEXT_TOPLEFT_X = BUTTON_COLOR_PRIM_TOPLEFT_X - (OPTIONS_PRIMTEXT_WIDTH+SCREEN_WIDTH_DEFAULT*0.03);
const int OPTIONS_PRIMTEXT_TOPLEFT_Y = BUTTON_COLOR_PRIM_TOPLEFT_Y
 + (3*BUTTON_COLOR_WIDTH + 2*BUTTON_COLOR_GAP)/2 - OPTIONS_PRIMTEXT_HEIGHT/2;

const string OPTIONS_SECTEXT = "Secondary:";
const int OPTIONS_SECTEXT_LEN = OPTIONS_SECTEXT.length();
const int OPTIONS_SECTEXT_HEIGHT = OPTIONS_PRIMTEXT_HEIGHT;
const int OPTIONS_SECTEXT_WIDTH = OPTIONS_SECTEXT_LEN*OPTIONS_SECTEXT_HEIGHT*UI_FONT_HEIGHT_TO_WIDTH;
const int OPTIONS_SECTEXT_TOPLEFT_X = BUTTON_COLOR_SEC_TOPLEFT_X - (OPTIONS_SECTEXT_WIDTH+SCREEN_WIDTH_DEFAULT*0.03);
const int OPTIONS_SECTEXT_TOPLEFT_Y = BUTTON_COLOR_SEC_TOPLEFT_Y
 + (3*BUTTON_COLOR_WIDTH + 2*BUTTON_COLOR_GAP)/2 - OPTIONS_SECTEXT_HEIGHT/2;


// Constants related to map generation
const int MAPBOX_START_ITERATIONS = 4;
const int MAPBOX_DIVIDE_ROLL_MAX = 100;
const int MAPBOX_DIVIDE_CHANCE_CHANGE = 15;
const int MAPBOX_NUM_CORNERS = 4;

const int MAPBOX_MINIMUM_WIDTH = CHARACTER_WIDTH*1.4;
const int MAPBOX_MINIMUM_HEIGHT = CHARACTER_WIDTH*1.4;

const int MAPBOX_MINIMUM_GAP = CHARACTER_WIDTH * 3;

/* Default generation settings
const int MAPBOX_START_ITERATIONS = 4;
const int MAPBOX_DIVIDE_ROLL_MAX = 100;
const int MAPBOX_NUM_CORNERS = 4;

const int MAPBOX_MINIMUM_WIDTH = CHARACTER_WIDTH*1.4;
const int MAPBOX_MINIMUM_HEIGHT = CHARACTER_WIDTH*1.4;

const int MAPBOX_MINIMUM_GAP = CHARACTER_WIDTH*3;
*/

// HUD parameters
const int HUD_WIDTH = SCREEN_WIDTH_DEFAULT - GAMESPACE_WIDTH;
const int HUD_HEIGHT = SCREEN_HEIGHT_DEFAULT;

const double HUD_COLOR_SCALE = 0.8;

// ammo box parameters
const string HUD_AMMO_ICON_LOCATION = "images/ammoIcon.png";

const int HUD_AMMO_WIDTH = HUD_WIDTH / 2;
const int HUD_AMMO_HEIGHT = HUD_HEIGHT*0.2;
const int HUD_AMMO_TOPLEFT_X = HUD_AMMO_WIDTH;
const int HUD_AMMO_TOPLEFT_Y = HUD_HEIGHT - HUD_AMMO_HEIGHT;

const int HUD_AMMO_ICON_WIDTH = HUD_AMMO_WIDTH*0.6;
const int HUD_AMMO_ICON_HEIGHT = HUD_AMMO_HEIGHT*0.6;
const int HUD_AMMO_ICON_TOPLEFT_X = HUD_AMMO_TOPLEFT_X + (HUD_AMMO_WIDTH - HUD_AMMO_ICON_WIDTH) / 2;
const int HUD_AMMO_ICON_TOPLEFT_Y = HUD_AMMO_TOPLEFT_Y + (HUD_AMMO_HEIGHT - HUD_AMMO_ICON_HEIGHT) / 2;

const double HUD_AMMO_BOX_COLOR_SCALE = 0.55;
const double HUD_AMMO_BAR_COLOR_SCALE = 0.75;
const int HUD_AMMO_ICON_ALPHA = 155;

// rool cooldown display parameters
const string HUD_COOLDOWN_ICON_LOCATION = "images/rollIcon.png";

const int HUD_COOLDOWN_WIDTH = HUD_WIDTH / 2;
const int HUD_COOLDOWN_HEIGHT = HUD_HEIGHT*0.2;
const int HUD_COOLDOWN_TOPLEFT_X = 0;
const int HUD_COOLDOWN_TOPLEFT_Y = HUD_HEIGHT - HUD_COOLDOWN_HEIGHT;

const int HUD_COOLDOWN_ICON_HEIGHT = HUD_COOLDOWN_HEIGHT*0.75;
const int HUD_COOLDOWN_ICON_WIDTH = HUD_COOLDOWN_ICON_HEIGHT;
const int HUD_COOLDOWN_ICON_TOPLEFT_X = HUD_COOLDOWN_TOPLEFT_X + (HUD_COOLDOWN_WIDTH - HUD_COOLDOWN_ICON_WIDTH) / 2;
const int HUD_COOLDOWN_ICON_TOPLEFT_Y = HUD_COOLDOWN_TOPLEFT_Y + (HUD_COOLDOWN_HEIGHT - HUD_COOLDOWN_ICON_HEIGHT) / 2;

const double HUD_COOLDOWN_BOX_COLOR_SCALE = 0.55;
const double HUD_COOLDOWN_BAR_COLOR_SCALE = 0.75;
const int HUD_COOLDOWN_ICON_ALPHA = 155;

// health bar parameters
const int HUD_HEALTH_WIDTH = HUD_WIDTH;
const int HUD_HEALTH_HEIGHT = HUD_HEIGHT*0.05;
const int HUD_HEALTH_TOPLEFT_X = 0;
const int HUD_HEALTH_TOPLEFT_Y = HUD_HEIGHT - (HUD_AMMO_HEIGHT + HUD_HEALTH_HEIGHT);

const int HUD_HEALTH_BOX_RED = 150;
const int HUD_HEALTH_BOX_BLUE = 0;
const int HUD_HEALTH_BOX_GREEN = 0;

const int HUD_HEALTH_BAR_RED = 255;
const int HUD_HEALTH_BAR_GREEN = 20;
const int HUD_HEALTH_BAR_BLUE = 20;

const int HUD_HEALTH_DIVIDE_WIDTH = HUD_WIDTH*0.01;
const int HUD_HEALTH_DIVIDE_HEIGHT = HUD_HEALTH_HEIGHT;
const int HUD_HEALTH_DIVIDE_TOPLEFT_Y = HUD_HEALTH_TOPLEFT_Y;

const int HUD_HEALTH_DIVIDE_RED = 100;
const int HUD_HEALTH_DIVIDE_GREEN = 0;
const int HUD_HEALTH_DIVIDE_BLUE = 0;

const string UI_WINCOND_PREFIX = "Score to win: ";
const string UI_WINCOND_TEXT = UI_WINCOND_PREFIX + to_string(GAME_WIN_SCORE);
const int UI_WINCOND_TOPLEFT_X = SCREEN_HEIGHT_DEFAULT*0.01;
const int UI_WINCOND_HEIGHT = SCREEN_HEIGHT_DEFAULT*0.05;
const int UI_WINCOND_TOPLEFT_Y = HUD_HEALTH_TOPLEFT_Y-UI_WINCOND_HEIGHT;
const int UI_WINCOND_WIDTH = UI_WINCOND_HEIGHT*UI_FONT_HEIGHT_TO_WIDTH*UI_WINCOND_TEXT.length();

// constants used in using the config file
// location of the config file
const string CONFIG_FILE_LOCATION = "config.txt";
const string CONFIG_SWIDTH = "swidth";
const string CONFIG_SHEIGHT = "sheight";
const string CONFIG_FULLSCREEN = "fullscreen";
const string CONFIG_ISHOST = "ishost";
const string CONFIG_HOSTIP = "hostip";
const string CONFIG_HOSTPORT = "hostport";
const string CONFIG_CLIENTPORT = "clientport";
const string CONFIG_PRIMCOLOR = "primaryColor";
const string CONFIG_SECCOLOR = "secondaryColor";
const string CONFIG_USERNAME = "username";


// constants used in netcode
const int CHARBUFF_LENGTH = 1024;
const int MESSAGE_NONE = 0;
const int MESSAGE_CONF_CONNECTION = 1;
const int MESSAGE_WALL = 2;
const int MESSAGE_PLAYER = 3;
const int MESSAGE_PROJECTILE = 4;
const int MESSAGE_EXPLOSION = 5;
const int MESSAGE_CLIENT_ACTION = 6;
const int MESSAGE_LOBBY = 7;
const int MESSAGE_QUIT = 8;

const int MESSAGE_PLAYER_LEN = 59;

// amounts of characters used to represent different variables types in networking strings
const int MESSAGE_ID_CHAR = 3;
const int MESSAGE_LOC_CHAR = 4;
const int MESSAGE_ANGLE_CHAR = 3;
const int MESSAGE_FRAMES_CHAR = 5;
const int MESSAGE_STAT_CHAR = 2;

// the number of times single send messages are sent
const int MESSAGE_RESEND_TIMES = 3;




// constants used in debugging
const bool DEBUG_ENABLE_DRIVERS = false;
const bool DEBUG_HIDE_SHADOWS = false;
const bool DEBUG_KILL_PLAYER = false;
const bool DEBUG_SHOW_CURSOR = false;
const bool DEBUG_DRAW_MOUSE_POINT = false;
const bool DEBUG_DRAW_SPAWN_POINTS = false;
const bool DEBUG_DRAW_VALID_SPAWNS_ONLY = false;
const bool DEBUG_DRAW_WEAPONARC = false;
const int DEBUG_WEAPONARC_RADIUS = 500;

const bool DEBUG_IS_HOST = true;
const Uint16 DEBUG_HOST_PORT = 2880;
const Uint16 DEBUG_CLIENT_PORT = 2881;
const string DEBUG_HOST_IP = "0.0.0.0";

/*-------------------------- Typedefs ------------------------------*/

typedef char charbuff[CHARBUFF_LENGTH];



/*-------------------------- Class Definitions -------------------------*/

// Class definitions
class Game {
protected:
    forward_list<Player*>* playerList;
    forward_list<Wall*>* wallContainer;
    forward_list<coordSet>* spawnPoints;
    forward_list<Projectile*>* projectileList;
    forward_list<BulletExplosion*>* explosionList;

    SDL_Renderer** gameRenderer;
    SDL_Window** gameWindow;

    forward_list<BulletExplosion*> explosionsToSend;

    string hostIp;
    Uint16 hostPort;
    Uint16 clientPort;

    map<int, userAction> clientActions;

    int myID;
    int nextID;

    string username;
    int numPlayers;
    connection currPlayers[GAME_MAX_PLAYERS];

    bool isHost;
    UDPConnectionServer* server;
    UDPConnectionClient* client;
    bool connected;

    int lastConnect;
    int lastMX;
    int lastMY;

    int winner;

    colorSet primaryColor;
    string primary;
    colorSet secondaryColor;
    string secondary;

    SDL_Texture* patterns[UI_BACKGROUND_PATTERN_COUNT];
    int patternBoard[UI_BACKGROUND_PATTERN_ROW][UI_BACKGROUND_PATTERN_COL];

    // store the player set resolution
    int swidth;
    int sheight;
    // store the actual resolution (changes in fullscreen mode)
    int swidthActual;
    int sheightActual;
    bool fullscreen;
public:
    Game(forward_list<Player*>* playerSet, forward_list<Wall*>* wallSet,
        forward_list<coordSet>* spawnSet, forward_list<Projectile*>* projSet,
        forward_list<BulletExplosion*>* explList, SDL_Renderer** renderer,
        SDL_Window** window);
    ~Game(void);

    void setPatterns(void);
    void setSockets(void);

    void initialize(int weapon);
    void recieveConnection(void);
    void attemptConnection(int weapon);

    void endSession(void);
    void leaveMatch(void);

    string getMapString(void);
    string getPlayerString(void);
    string getProjectileString(void);
    string getExplosionString(void);

    void removePlayer(connection playerip);

    int getInput(bool inLobby, bool inMenu);
    void sendUserActions(bool paused);
    bool getClientAction(void);
    void sendUpdate(void);
    void sendLobby(void);
    void updateMap(string serverString);
    void updatePlayers(string serverString);
    void updateProjectiles(string serverString);
    void updateExplosions(string serverString);
    void updateLobby(string serverString);

    void addNewExplosion(BulletExplosion* newExplosion);

    forward_list<Player*>* players(void) { return playerList; }
    forward_list<Wall*>* walls(void) { return wallContainer; }
    forward_list<coordSet>* spawns(void) { return spawnPoints; }
    forward_list<Projectile*>* projectiles(void) { return projectileList; }
    SDL_Renderer* renderer(void) { return *gameRenderer; }

    SDL_Texture* pattern(int x, int y) { return patterns[patternBoard[x][y]]; }
    int configScreenWidth(void) {return swidth;}
    int configScreenHeight(void) {return sheight;}
    int screenWidth(void) {return swidthActual;}
    int screenHeight(void) {return sheightActual;}
    int isFullscreen(void) {return fullscreen;}

    string hIP(void) {return hostIp;}
    Uint16 hPort(void) {return hostPort;}
    Uint16 cPort(void) {return clientPort;}
    bool isConnected(void) {return connected;}
    bool hosting(void) {return isHost;}
    void setHost(bool host) {isHost = host;}
    userAction getActions(int clientID) {return clientActions[clientID];}

    int getID(void) {return myID;}
    connection* getCurrPlayers(void) {return currPlayers;}
    int getNumPlayers(void) {return numPlayers;}
    string getUsername(void) {return username;}
    string getUsername(int id);
    playerScore getPlayerScore(int index);
    int getWinner(void);

    colorSet primaryColors(void) {return primaryColor;}
    colorSet secondaryColors(void) {return secondaryColor;}
    string primColor(void) {return primary;}
    string secColor(void) {return secondary;}

    void updatePrimColors(string newColor);
    void updateSecColors(string newColor);
    void updateWindow(int w, int h, bool full);

    void setSize(int w, int h) {swidthActual = w; sheightActual = h;}
    void setID(int newID) {myID = newID;}
};



class Menu {
protected:
    enum MENU_TYPES {
        MENU_MAIN,
        MENU_LOBBY,
        MENU_OPTIONS,
        MENU_CONTROLS
    };
    int currMenu;
    int weapon;

    bool mouseDown;

    MainPage* mainMenu;
    LobbyPage* lobby;
    OptionPage* optionMenu;
    TutorialPage* tutorial;
public:
    Menu(Game* game);
    ~Menu(void);
    void reset(void);
    int update(Game* game);
    void render(Game* game);

    int getWeapon(void) {return weapon;}
};

class MainPage {
protected:
    SDL_Texture* heading;

    Button* playButton;
    Button* optionButton;
    Button* controlButton;
    Button* quitButton;

    SDL_Texture* hostInfo;
    Button* hostSelect;
    Button* clientSelect;

    SDL_Texture* weaponInfo;
    Button* pistolSelect;
    Button* rifleSelect;
    Button* shotgunSelect;
public:
    MainPage(Game* game);
    ~MainPage(void);

    void reset(void);
    int getCurrWeapon(void);
    int update(int x, int y, bool press);
    void render(Game* game);
};

class LobbyPage {
protected:
    Button* backButton;
    Button* launchButton;

    SDL_Texture* waitingText;
    SDL_Texture* hostInfo;
public:
    LobbyPage(Game* game);
    ~LobbyPage(void);

    void reset(void);
    int update(int x, int y, bool press, Game* game);
    void render(Game* game);
};

class OptionPage {
protected:
    SDL_Texture* heading;
    SDL_Texture* resTitle;

    SDL_Texture* primText;
    SDL_Texture* secText;

    Button* backButton;
    Button* fullscreenButton;

    Button* primSelector[COLOR_NUM];
    Button* secSelector[COLOR_NUM];
    Button* resSelector[UI_RESOLUTION_NUM];
public:
    OptionPage(Game* game);
    ~OptionPage(void);

    void reset(void);
    int update(int x, int y, bool press, Game* game);
    void updateGame(Game* game);
    void render(Game* game);
};

class TutorialPage {
protected:
	SDL_Texture* header;
	SDL_Texture* content;

    Button* backButton;
public:
    TutorialPage(Game* game);
    ~TutorialPage(void);

    void reset(void);
    int update(int x, int y, bool press);
    void render(Game* game);
};

class Button {
protected:
    SDL_Rect location;

    bool fixedColor; // stores whether the button uses a certain color, or the games color settings
    colorSet colorPrim;
    colorSet colorSec;

    bool selected;
    int buttonType;

    string id;

    bool img;
    int textLen;
    SDL_Texture* displayText;
public:
    Button(int x, int y, int w, int h, const colorSet prim,
     const colorSet sec, int type, const string name);
    Button(int x, int y, int w, int h, int type, const string name,
     bool useImg, string text, Game* game);
    ~Button(void);
    bool mouseHover(int x, int y);
    void render(Game* game);

    void setActive(bool active) {selected = active;}
    bool getActive(void) {return selected;}

    string getID(void) {return id;}
};





// Class to represent all player controlled characters
class Player {
protected:
    // details about the characters location
    SDL_Rect playerRect; // constains the position and size of the player's image for rendering
    double angle; // stores angle player is pointing
    int centreX; // Contain the x and y coordinates of the centre of the player
    int centreY;
    int radius; // contains the radius of the character circle in pixels

                // players velocity in each direction
    double velx;
    double vely;

    // variables to store the state of the players roll
    bool rolling;
    direction rollDirection;
    int rollFrames;
    int rollCooldown;

    bool invulnerable;
    int invulnFrames;


    // color of the players sprite
    colorSet playerColors;

    Weapon* weapon; // address of the player's weapon object
    int weapID; // type of weapon used by the player

    int health; // life values of the player
    bool alive;
    int deathFrames; // counter containing how long the player remains dead

    int id; // ID number of the player object to differentiate it
    int score; // how many kills the player has earned

            // the sprite sheet for the player
    SDL_Texture* playerImage;
    SDL_Texture* rollOutline;
    SDL_Texture* invulnImage;
    SDL_Renderer* playerRenderer;

    DeathObject* deathMarker;

public:
    // initializer function for the class
    Player(Game* game, int startX, int startY, int idNum, int weaponID);
    ~Player(void);

    //getters for the private variables
    Weapon* getWeapon(void) { return weapon; }
    int getX(void) { return centreX; }
    int getY(void) { return centreY; }
    int getRadius(void) { return radius; }
    int getID(void) { return id; }
    double getAngle(void) { return angle; }
    int getRollCooldown(void) { return rollCooldown; }
    int getDeathFrames(void) { return deathFrames; }
    int getHealth(void) { return health; }
    bool isAlive(void) { return alive; }
    int getScore(void) {return score;}
    void addPoint(void) {score += 1;}

    string getStateString(void);

    // functions to update the players state
    void updateState(Game* game, bool paused);
    void updateState(userAction userInput, Game* game);
    void updateState(playerState newState);
    void move(forward_list<Wall*>* wallContainer); // moves the player based on their velocity
    void setPlayerCentre(void); // resets the players centre based on their location of the top left corner
    bool successfulShot(void); // called when the player is hit by a bullet
    void killPlayer(void); // kills the player
    void respawn(forward_list<coordSet>* spawnPoints, forward_list<Player*>* playerList); // respawn the player after death

    //draws the player to the screen
    void render(Game* game);
};

class DeathObject {
protected:
    SDL_Texture* circleImage;
    SDL_Rect circleRect;
    colorSet circleColors;
public:
    DeathObject(SDL_Renderer* renderer, SDL_Rect playerCoordinates, colorSet playerColors);
    ~DeathObject(void);

    int getX(void) {return circleRect.x;}
    int getY(void) {return circleRect.y;}

    void reset(SDL_Rect playerCoordinates);
    void updateState(void);
    void updateState(int framesLeft, int x, int y);
    void render(SDL_Renderer* renderer);
};

// Weapon base class
class Weapon {
protected:
    int currAmmo;
    int reloadFramesLeft;
    bool reloading;
public:
    virtual ~Weapon(void) = 0;
    virtual int getMaxAmmo(void) = 0;
    virtual int getCurrAmmo(void) = 0;
    virtual int getReloadFrames(void) = 0;
    virtual int getMaxReloadFrames(void) = 0;
    virtual bool isReloading(void) = 0;

    virtual void setAmmo(int ammo) = 0;
    virtual void setReloadFrames(int frames) = 0;

    virtual void takeShot(Game* game, Player* player) = 0;
    virtual void takeShot(Game* game, Player* player, userAction userInput) = 0;
    virtual void beginReload(void) = 0;
    virtual void updateGun() = 0;
    virtual void resetGun() = 0;
    virtual void debugRender(Game* game, int x, int y, double a) = 0;
};

// Weapon Derived classes
// Assault rifle
class AssaultRifle : public Weapon {
private:
    int shotDelay;
    bool mouseDown;
public:
    AssaultRifle(void);
    ~AssaultRifle(void);

    int getMaxAmmo(void) { return AR_CLIP_SIZE; }
    int getCurrAmmo(void) { return currAmmo; }
    int getReloadFrames(void) { return reloadFramesLeft; }
    int getMaxReloadFrames(void) { return AR_RELOAD_FRAMES; }
    bool isReloading(void) { return reloading; }

    void setAmmo(int ammo) {currAmmo = ammo;};
    void setReloadFrames(int frames);

    void takeShot(Game* game, Player* player);
    void takeShot(Game* game, Player* player, userAction userInput);
    void beginReload(void);
    void updateGun(void);
    void resetGun(void);
    void debugRender(Game* game, int x, int y, double a);
};

class Pistol : public Weapon {
private:
    double currRecoil;
    bool mouseDown;
public:
    Pistol(void);
    ~Pistol(void);

    int getMaxAmmo(void) { return PISTOL_CLIP_SIZE; }
    int getCurrAmmo(void) { return currAmmo; }
    int getReloadFrames(void) { return reloadFramesLeft; }
    int getMaxReloadFrames(void) { return PISTOL_RELOAD_FRAMES; }
    bool isReloading(void) { return reloading; }

    void setAmmo(int ammo) {currAmmo = ammo;}
    void setReloadFrames(int frames);

    void takeShot(Game* game, Player* player);
    void takeShot(Game* game, Player* player, userAction userInput);
    void beginReload(void);
    void updateGun(void);
    void resetGun(void);
    void debugRender(Game* game, int x, int y, double a);
};

class Shotgun : public Weapon {
private:
    int shotDelay;
    bool mouseDown;
public:
    Shotgun(void);
    ~Shotgun(void);

    int getMaxAmmo(void) { return 1; } // shotguns do not use ammo, so return 1 as default (not 0 to avoid errors)
    int getCurrAmmo(void) { return currAmmo; }
    int getReloadFrames(void) { return shotDelay; }
    int getMaxReloadFrames(void) { return SHOTGUN_SHOT_DELAY; }
    bool isReloading(void) { return true; } // treat shot delay as reloading

    void setAmmo(int ammo) {}
    void setReloadFrames(int frames) {shotDelay = frames;}

    void takeShot(Game* game, Player* player);
    void takeShot(Game* game, Player* player, userAction userInput);
    void beginReload(void);
    void updateGun(void);
    void resetGun(void);
    void debugRender(Game* game, int x, int y, double a);
};



// Wall objects found throughout the environment
class Wall {
private:
    // Rect object to hold info on the wall location
    SDL_Rect wallLocation;


public:
    Wall(int x, int y, int w, int h); // initializer function
    ~Wall(void); // frees any memory associated with the wall
    SDL_Rect getLocation(void) { return wallLocation; } // returns the SDL_Rect describing the wall
    bool checkCollision(int x, int y, int radius); // checks if the object at (x, y) with radius r is in contact with the wall
    void render(Game* game); // draw the wall to the screen
    void renderShadow(int x, int y, Game* game); // draw the LOS shadow by the wall to the screen
};


// projectiles shot by the player
class Projectile {
private:
    // detail about the projectile (location of top left corner, width and height)
    SDL_Rect projectileRect;
    int centreX; // location of the bullets centre
    int centreY;
    double angle; // rotation of the projectile

    int radius; // radius of the bullet

                // velocity of the bullet in each direction
    double velx;
    double vely;

    // double version of the projectiles location
    double currPosX;
    double currPosY;

    // color of the projectiles
    colorSet projectileColors;

    // ID of the player who shot the projectile instance
    int ownerID;
    Player* owner;

    // spritesheet of the projectile
    SDL_Texture* projectileImage;
public:
    Projectile(int x, int y, double a, const double speed, Game* game, Player* player);
    Projectile(int x, int y, Game* game, int id);
    ~Projectile(void);

    SDL_Rect getLocation(void) { return projectileRect; }
    colorSet getColors(void) { return projectileColors; }
    int getOwner(void) {return ownerID;}

    int checkCollision(Game* game);
    string getStateString(void);
    bool move(Game* game);
    void updateState(int x, int y, Game* game, int id);
    void setProjectileCentre(void);
    void render(SDL_Renderer* renderer);
};

class BulletExplosion {
protected:
    SDL_Texture* explosionImage;
    SDL_Rect explosionLocation;
    colorSet explosionColors;
    double radius;
    int ownerID;
public:
    BulletExplosion(SDL_Renderer* render, SDL_Rect projectileLocation, colorSet projectileColors, int id);
    BulletExplosion(Game* game, int x, int y, int id);
    ~BulletExplosion(void);
    bool updateState(void);
    string getStateString(void);
    void render(SDL_Renderer* renderer);
};

class MapBox {
protected:
    double x;
    double y;
    double w;
    double h;
    int iterations;
    int divideChance;
public:
    MapBox(int xinp, int yinp, int winp, int hinp, int iters, int divChance);
    ~MapBox(void);

    int getIterations(void) { return iterations; }
    int getX(void) { return x; }
    int getY(void) { return y; }
    int getWidth(void) { return w; }
    int getHeight(void) { return h; }
    coordSet getCentre(void);

    list<MapBox*> divideBox(void);
    bool checkConnection(MapBox* box);
    bool checkSameBox(MapBox* box);
    Wall* generateWall(void);
    MapBox* copyBox(void);

    void render(SDL_Renderer* renderer);
};

// Classes related to networking aspect of the game
class UDPConnectionClient {
private:
    IPaddress connectionIp;

    UDPpacket* packetIn;
    UDPpacket* packetOut;
    UDPsocket socket;
public:
    UDPConnectionClient(Game* game);
    ~UDPConnectionClient(void);
    bool send(string msg, int times);
    connection recieve(string* field);
};

class UDPConnectionServer {
private:
    IPaddress connectionIp;

    UDPpacket* packetIn;
    UDPpacket* packetOut;
    UDPsocket socket;
public:
    UDPConnectionServer(Game* game);
    ~UDPConnectionServer(void);
    bool send(string msg, int times, connection* ips, int numIps);
    bool send(string msg, int times, connection target);
    connection recieve(string* field);
};

/*--------------------- Function definitions -------------------------*/

void quitGame(SDL_Window* window, Game* game, hudInfo hudInfoContainer); // frees any used memory at the end of runtime
bool init(SDL_Window** window, SDL_Renderer** renderer, Game* game); // initializes the same (including SDL)
SDL_Texture* loadImage(string path, SDL_Renderer* renderer); // loads a image from path path and return the pointer to it
SDL_Texture* loadText(string content, int size, SDL_Renderer* renderer);
double distBetweenPoints(int x1, int y1, int x2, int y2); // finds the distance between (x1, y1) and (x2,  y2)
int getInterceptX(int x1, int y1, int x2, int y2, int interceptY); // finds the x-intercept of a line between (x1, y1) and (x2,  y2) at the y point interceptY
int getInterceptY(int x1, int y1, int x2, int y2, int interceptX); // finds the y-intercept of a line between (x1, y1) and (x2,  y2) at the x point interceptX
direction getDirections(void);
bool checkExitMap(int x, int y, int r); //checks if an object pos (x, y) radius r is outside the map
void renderGameSpace(Game* game, forward_list<BulletExplosion*> explosionList,
    int playerMainX, int playerMainY, int winner); // render the gameplay area of the screen
void renderGameUI(Game* game, Player* userCharacter,
    hudInfo hudInfoContainer, bool paused); // render the HUD area of the screen
void generateMap(forward_list<Wall*>* wallContainer, forward_list<coordSet>* spawnPoints);
int generateRandInt(int min, int max);
double generateRandDouble(double min, double max);
bool validateSpawnPoint(coordSet point, forward_list<Player*>* playerList);
coordSet getSpawnPoint(forward_list<coordSet>* spawnPoints, forward_list<Player*>* playerList);
string strOfLen(int number, int len);
int getLength(int number);
int sizeOfProj(forward_list<Projectile*>* list);
coordSet getMouseCoordinates(Game* game);

//drivers
void testDistBetweenPoints(void);
void testCheckExitMap(void);
void testGenerateRandInt(void);
void testGenerateRandDouble(void);
void testStrOfLen(void);
void testGetLength(void);