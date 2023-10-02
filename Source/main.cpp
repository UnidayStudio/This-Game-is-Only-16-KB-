/*
    Game Created by Guilherme Teres (@UnidayStudio)

    This code was put together in a few (around 3) hours for the 
    Ludum Dare 54 Game Jam. The theme was "Limited Space" and I
    had this idea to make a game with the final executable having
    the smallest size as possible, ideally bellow 16 KB. 

    The game itself is very simple, just a Sokoban clone with no
    sound and a few levels. But it have some interesting features,
    such as procedurally generated textures for the sprites and a
    hand drawn character pixel art using nothing but binary. :P

    Code is a bit of a mess, but given the short amount of time I
    had, I'm happy with it. It uses no external lib, just Windows.h,
    which sadly makes it window specific, but I think it would be
    a bit harder to make it extremely small in size and still cross
    platform. This was my first attempt to make an executable small,
    I know that there are plenity of advanced techniques out there
    that I'm not using and that I could same a lot more space on 
    stack and not using any default lib, but I'l already happy with
    the result. 

    It was fun to develop this little project! :)
*/
#include <windows.h>
#include <tchar.h>
#include <string>

#define WINDOW_WIDTH  512
#define WINDOW_HEIGHT 512
#define FRAME_RATE 60

// Note: things will reall explode if GRID_RES is higher than 8!
#define GRID_RES 8 // How many pixels in a grid
#define GRID_SIZE 8 // How many grid in the map

struct Color { 
    Color() : r(255), g(0), b(255), a(255){}
    Color(unsigned char val) : r(val), g(val), b(val), a(val){}
    Color(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a=255) : r(_r), g(_g), b(_b), a(_a){}

    unsigned char b, g, r, a; // Windows is BGRA...
};

static Color* canvas;
static int canvasWidth  = 128;
static int canvasHeight = 128;

Color* NewBitMap(int width=GRID_RES, int height=GRID_RES){ 
    return (Color*)malloc(sizeof(Color) * width * height); 
}

void DeleteBitMap(Color* ptr){ 
    free(ptr); 
}

unsigned char LerpColorChannel(unsigned char from, unsigned char to, unsigned char factor){
    float f = float(factor) / 255.f;
    return unsigned char(from * (1.f - f) + to * f);
}

unsigned char Max(unsigned char a, unsigned char b) {
    return a > b ? a : b;
}

void GenerateRandomPattern(Color* bitmap, const Color& minVal, const Color& mask, int width=GRID_RES, int height=GRID_RES){
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int id = y * width + x;
            const unsigned char rVal = rand() % 256;

            bitmap[id].r = Max(minVal.r, unsigned char(rVal * (float(mask.r) / 255.f)));
            bitmap[id].g = Max(minVal.g, unsigned char(rVal * (float(mask.g) / 255.f)));
            bitmap[id].b = Max(minVal.b, unsigned char(rVal * (float(mask.b) / 255.f)));
            bitmap[id].a = mask.a;
        }
    }
}

void BlitToCanvas(int gridX, int gridY, Color* bitmap){
    for (int y = 0; y < GRID_RES; ++y) {
        for (int x = 0; x < GRID_RES; ++x) {
            const int canvasPos = (gridY * GRID_RES + y) * canvasWidth + (gridX * GRID_RES + x);
            const int bitmapPos = y * GRID_RES + x;

            canvas[canvasPos].r = LerpColorChannel(canvas[canvasPos].r, bitmap[bitmapPos].r, bitmap[bitmapPos].a);
            canvas[canvasPos].g = LerpColorChannel(canvas[canvasPos].g, bitmap[bitmapPos].g, bitmap[bitmapPos].a);
            canvas[canvasPos].b = LerpColorChannel(canvas[canvasPos].b, bitmap[bitmapPos].b, bitmap[bitmapPos].a);
        }
    }
}

void FillBitmap(Color* bitmap, const Color& color, int width=GRID_RES, int height=GRID_RES){
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            memcpy_s(&bitmap[y * width + x], sizeof(Color), &color, sizeof(Color));
        }
    }
}

void PaintCell(Color* bitmap, const Color& color, unsigned char* mask){
    for (int y = 0; y < GRID_RES; ++y) {
        for (int x = 0; x < GRID_RES; ++x) {
            if (mask[y] & (1 << x)){
                memcpy_s(&bitmap[y * GRID_RES + x], sizeof(Color), &color, sizeof(Color));
            }
        }
    }
}

struct Level{
    unsigned char walls[GRID_SIZE];
    unsigned char boxes[GRID_SIZE];
    unsigned char dots[GRID_SIZE];

    int startX, startY;
};

#define LEVEL_COUNT 6

static Level levels[LEVEL_COUNT];
static int currentLevel;

static Color* player;
static Color* ground;
static Color* wall;
static Color* box;
static Color* dotInvalid;
static Color* dotValid;

static Color** gameGrid;

static int playerX;
static int playerY;

void MovePlayer(int dirX, int dirY){
    if (playerX + dirX < 0 || playerY + dirY < 0){ return; }
    if (playerX + dirX >= GRID_SIZE || playerY + dirY >= GRID_SIZE){ return; }

    if (gameGrid[(playerY + dirY) * GRID_SIZE + (playerX + dirX)] == ground){
        // Valid...
    }
    else if (gameGrid[(playerY + dirY) * GRID_SIZE + (playerX + dirX)] == box) {
        if (playerX + dirX * 2 < 0 || playerY + dirY * 2 < 0){ return; }
        if (playerX + dirX * 2 >= GRID_SIZE || playerY + dirY * 2 >= GRID_SIZE){ return; }

        if (gameGrid[(playerY + dirY * 2) * GRID_SIZE + (playerX + dirX * 2)] == ground){
            gameGrid[(playerY + dirY * 2) * GRID_SIZE + (playerX + dirX * 2)] = box;
        }
        else {
            return;
        }
    }
    else {
        return;
    }
    gameGrid[(playerY + dirY) * GRID_SIZE + (playerX + dirX)] = player;
    gameGrid[playerY * GRID_SIZE + playerX] = ground;
    playerX += dirX;
    playerY += dirY;
}

void RestartLevel(){
    for (int y=0; y < GRID_SIZE; ++y){
        for (int x=0; x < GRID_SIZE; ++x){
            gameGrid[y * GRID_SIZE + x] = levels[currentLevel].walls[y] & (1 << (7 - x)) ? wall : ground;
            if (levels[currentLevel].boxes[y] & (1 << (7 - x))) {
                gameGrid[y * GRID_SIZE + x] = box;
            }
        }
    }
    playerX = levels[currentLevel].startX;
    playerY = levels[currentLevel].startY;
    gameGrid[playerY * GRID_SIZE + playerX] = player;
}

namespace Game{
    void Start(){
        gameGrid = (Color**)malloc(sizeof(Color*) * GRID_SIZE * GRID_SIZE);

        canvasWidth  = GRID_RES * GRID_SIZE;
        canvasHeight = GRID_RES * GRID_SIZE;

        canvas = NewBitMap(canvasWidth, canvasHeight);
        FillBitmap(canvas, Color(0,0,0), canvasWidth, canvasHeight);

        srand(42);

        // Creating the Ground:
        {
            ground = NewBitMap();
            GenerateRandomPattern(ground, Color(20), Color(40, 40, 40));
        }

        // Creating the Wall:
        {
            wall = NewBitMap();
            GenerateRandomPattern(wall, Color(100), Color(220, 225, 230));

            unsigned char brickPattern[] = {
                0b00000010, 0b11111111, 0b00100000, 0b11111111, 
                0b00000010, 0b11111111, 0b00100000, 0b11111111
            };
            PaintCell(wall, Color(150, 160, 170, 100), brickPattern);
        }

        // Creating the player BitMap:
        {
            player = NewBitMap();
            //FillBitmap(player, Color(0));
            memcpy_s(player, sizeof(Color) * GRID_RES * GRID_RES, ground, sizeof(Color) * GRID_RES * GRID_RES);

            unsigned char playerOutline[] = {
                0b00011000, 0b00011000, 0b00111100, 0b01011010,
                0b01011010, 0b00011000, 0b00100100, 0b01000010
            };
            PaintCell(player, Color(255), playerOutline);
        }

        // Creating the Box:
        {
            box = NewBitMap();
            GenerateRandomPattern(box, Color(140, 100, 30), Color(180, 140, 70));
            unsigned char boxPattern[] = {
                0b00000000, 
                0b01111110, 
                0b01000010, 
                0b01000010,
                0b01000010, 
                0b01000010, 
                0b01111110, 
                0b00000000
            };
            PaintCell(box, Color(60, 50, 30, 100), boxPattern);
        }

        // Creating the Dots:
        {
            dotValid = NewBitMap();
            memcpy_s(dotValid, sizeof(Color) * GRID_RES * GRID_RES, box, sizeof(Color) * GRID_RES * GRID_RES);
            GenerateRandomPattern(dotValid, Color(0, 0, 100), Color(0, 140, 255, 150));

            dotInvalid = NewBitMap();
            FillBitmap(dotInvalid, Color(0));
            unsigned char dotPattern[] = {
                0b00000000, 
                0b00011000, 
                0b00100100, 
                0b01011010,
                0b01011010, 
                0b00100100, 
                0b00011000, 
                0b00000000
            };
            PaintCell(dotInvalid, Color(0, 140, 255, 100), dotPattern);
        }


        currentLevel = 0;

        levels[0] = {
            {   
                0b11111111, 
                0b10000001, 
                0b10000001, 
                0b10000001, 
                0b10000001, 
                0b10000001, 
                0b10000001, 
                0b11111111 }, // Walls
            {   
                0b00000000, 
                0b00000000, 
                0b00000000, 
                0b00000000, 
                0b00000000, 
                0b00001000, 
                0b00000000, 
                0b00000000 }, // Boxes
            {   
                0b00000000, 
                0b00000000, 
                0b00000000, 
                0b00000000, 
                0b00000000, 
                0b00000100, 
                0b00000000, 
                0b00000000 }, // Dots
            2, 2 // Player Start
        };
        levels[1] = {
            {   
                0b01001100, 
                0b01001010, 
                0b01101100, 
                0b00000000, 
                0b01101010, 
                0b01001010, 
                0b00100110, 
                0b01000010 }, // Walls
            {   
                0b00000000, 
                0b00000000, 
                0b00000000, 
                0b00001000, 
                0b00000100, 
                0b00000000, 
                0b00000000, 
                0b00000000 }, // Boxes
            {   
                0b00000000, 
                0b00000000, 
                0b00000000, 
                0b00000000, 
                0b00000000, 
                0b00000100, 
                0b00000000, 
                0b00000001 }, // Dots
            0, 0 // Player Start
        };
        levels[2] = {
            {   
                0b00111000, 
                0b00101000, 
                0b00101111, 
                0b11100001, 
                0b10000111, 
                0b11110100, 
                0b00010100, 
                0b00011100 }, // Walls
            {   
                0b00000000, 
                0b00000000, 
                0b00000000, 
                0b00010100, 
                0b00010000, 
                0b00001000, 
                0b00000000, 
                0b00000000 }, // Boxes
            {   
                0b00000000, 
                0b00010000, 
                0b00000000, 
                0b00000010, 
                0b01000000, 
                0b00000000, 
                0b00001000, 
                0b00000000 }, // Dots
            4, 4 // Player Start
        };
        levels[3] = {
            {0b00111110, 0b11100010, 0b10000010, 0b11100010, 0b10110010, 0b10100011, 0b10000001, 0b10000001 }, // Walls
            {0b00000000, 0b00000000, 0b00010000, 0b00001000, 0b00001000, 0b00000000, 0b01011100, 0b00000000 }, // Boxes
            {0b00000000, 0b00000000, 0b01000000, 0b00000100, 0b01000000, 0b00001000, 0b00010010, 0b00001000 }, // Dots
            2, 2 // Player Start
        };
        levels[4] = {
            {   
                0b11111100, 
                0b10000100, 
                0b10000110, 
                0b10010011, 
                0b11000000, 
                0b01000000, 
                0b01111111, 
                0b00000000 }, // Walls
            {   
                0b00000000, 
                0b00000000, 
                0b00010000, 
                0b00100000, 
                0b00001010, 
                0b00000000, 
                0b00000000, 
                0b00000000 }, // Boxes
            {   
                0b00000000, 
                0b00000000, 
                0b00000000, 
                0b00001100, 
                0b00001100, 
                0b00000000, 
                0b00000000, 
                0b00000000 }, // Dots
            5, 3 // Player Start
        };
        levels[5] = {
            {   
                0b11111111, 
                0b10000001, 
                0b10000001, 
                0b10000001, 
                0b10000001, 
                0b10000001, 
                0b10000001, 
                0b11111111 }, // Walls
            {   
                0b00000000, 
                0b00001000, 
                0b00001000, 
                0b00001000, 
                0b00001000, 
                0b00001000, 
                0b00001000, 
                0b00000000 }, // Boxes
            {   
                0b00000000, 
                0b00000010, 
                0b00000010, 
                0b00000010, 
                0b00000010, 
                0b00000010, 
                0b00000010, 
                0b00000000 }, // Dots
            2, 2 // Player Start
        };
        

        RestartLevel();
    }

    void Update(){
        bool victory = true;   
        for (int y=0; y < GRID_SIZE; ++y){
            for (int x=0; x < GRID_SIZE; ++x){
                BlitToCanvas(x, y, gameGrid[y * GRID_SIZE + x]);

                if (levels[currentLevel].dots[y]  & (1 << (7 - x))){
                    bool isValid = gameGrid[y * GRID_SIZE + x] == box;
                    BlitToCanvas(x, y, isValid ? dotValid : dotInvalid);

                    if (!isValid){
                        victory = false;
                    }
                }
            }
        }

        if (victory){
            if (currentLevel < LEVEL_COUNT - 1){
                ++currentLevel;
            }
            else {
                currentLevel = 0;
                MessageBoxW(NULL, L"Congratulations, you won!\n\nThe game was short, but I guess that was the point of it? (lol). Don't forget to rate the game and if you want to talk about Game and Game Engine dev, consider joining our discord server!\n\nBonus Feature: Now you can press N to skip levels.", L"You Won!", MB_ICONINFORMATION);
            }
            RestartLevel();
        }
    }

    void End(){
        free(gameGrid);

        DeleteBitMap(canvas);

        DeleteBitMap(player);
        DeleteBitMap(ground);
        DeleteBitMap(wall);
        DeleteBitMap(box);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Tutorial:
    MessageBoxW(NULL, L"The iodea behind this Project was to make the smallest game (in disk size) I could. This is how it turned out!\n\nTutorial:\n- Use W, A, S, D to move the character. \n- Press R to restart the Level if you got stuck.\n\nThe goal is to push all the boxes to the blue dots.", L"Welcome to Sokoban for Ludum Dare 54!", MB_ICONINFORMATION);

    Game::Start();

    // Creating a window class...
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("LudumDare54Game"), NULL };
    RegisterClassEx(&wc);

    // Calculate the position for the window to be centered
    int xPos = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2;
    int yPos = (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2;

    // Creating the window...
    auto dwStyle=(WS_OVERLAPPED| WS_SYSMENU);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("Ludum Dare 54 - By @UnidayStudio"), dwStyle, xPos, yPos, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, wc.hInstance, NULL);

    SetTimer(hwnd, 1, 1000 / FRAME_RATE, NULL);

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
 
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);  
        DispatchMessage(&msg); 
    }

    // Clean up 
    KillTimer(hwnd, 1);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    Game::End();

    return 0;
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg){
    case WM_PAINT:
        {   
            Game::Update();

            std::wstring title = L"[Level " + std::to_wstring(currentLevel + 1) + L"/" + std::to_wstring(LEVEL_COUNT) + L"] Sokoban (by @UnidayStudio)";
            SetWindowTextW(hwnd, title.c_str());
                        
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Calculate the scaling factors for the resized bitmap
            float scaleX = static_cast<float>(ps.rcPaint.right - ps.rcPaint.left) / canvasWidth;
            float scaleY = static_cast<float>(ps.rcPaint.bottom - ps.rcPaint.top) / canvasHeight;

            // Create a memory device context for double buffering
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateBitmap(canvasWidth, canvasHeight, 1, 32, &canvas[0]);
            HBITMAP hBitmapOld = (HBITMAP)SelectObject(memDC, hBitmap);

            // Blit the bitmap to the window
            BitBlt(hdc, 0, 0, canvasWidth, canvasHeight, memDC, 0, 0, SRCCOPY);

            // Scale and draw the bitmap to the window
            SetStretchBltMode(hdc, COLORONCOLOR);
            StretchBlt(hdc, 0, 0, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
                memDC, 0, 0, canvasWidth, canvasHeight, SRCCOPY);

            // Clean up
            SelectObject(memDC, hBitmapOld);
            DeleteObject(hBitmap);
            DeleteDC(memDC);

            EndPaint(hwnd, &ps);
        }
        break;
    case WM_KEYDOWN:
        if (wParam == 0x57)         MovePlayer( 0, -1); // W
        else if (wParam == 0x53)    MovePlayer( 0,  1); // S
        else if (wParam == 0x41)    MovePlayer(-1,  0); // A
        else if (wParam == 0x44)    MovePlayer( 1,  0); // D
        
        if (wParam == 0x52) RestartLevel(); // R
        if (wParam == 0x4E) { // N
            if (currentLevel < LEVEL_COUNT - 1){
                ++currentLevel;
                RestartLevel();
            }
        }
    case WM_TIMER:
        InvalidateRect(hwnd, NULL, FALSE); // Force a repaint
        break;
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
