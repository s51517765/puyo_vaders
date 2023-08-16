// 230 x 315 画面サイズ
#include <Arduino.h>
#include <M5Stack.h> //0.3.9
#include <stdbool.h>
#define BLOCKSIZE 3
#define STEP_PERIOD 10 // 画面更新周期
#define ALIEN_SPEED 2
#define LEFT -1
#define RIGHT 1
#define PUYO_COLOR_OFFSET 4
#define INITIAL 0x08
#define BLANK 0x09
#define X_OFFSET_PIXEL 12
#define THRESH 4
#define EVAPORATION_THRESH 8
// #define printf(...)

uint32_t pre = 0;
uint32_t counterLcdRefresh = 0;
int32_t player_x = X_OFFSET_PIXEL;
int32_t player_y = 216;
uint8_t puyoDirection = 0;
int8_t puyoOffset[4][2] = {{1, 0}, {0, -1}, {-1, 0}, {0, -1}}; // x,y //[1]下向きを反転

char alienImg[8][11] = {{0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0},
                        {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
                        {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
                        {0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0},
                        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                        {1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1},
                        {1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1},
                        {0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0}};
const int ALIEN_IMG_SIZE_X = sizeof(alienImg[0]) / sizeof(alienImg[0][0]);
const int ALIEN_IMG_SIZE_Y = sizeof(alienImg) / sizeof(alienImg[0]);

char puyoImg[8][11] = {{0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
                       {0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0},
                       {0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0},
                       {1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
                       {1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
                       {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
                       {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0}};
const int PUYO_IMG_SIZE_X = sizeof(puyoImg[0]) / sizeof(puyoImg[0][0]);
const int PUYO_IMG_SIZE_Y = sizeof(puyoImg) / sizeof(puyoImg[0]);

uint16_t colors[] = {
    BLACK, // Blank
    WHITE,
    YELLOW,
    MAGENTA,
    GREEN,
    CYAN,
    // RED
};

uint16_t currentColor[2];
uint16_t nextColor[2];

uint8_t playAreaMap[11][9] = {
    // 0x01 ~ 0x05:Alien  0x10 ~ x50:Puyo  0x08:Initial  0x09:Blank
    {9, 9, 9, 9, 9, 9, 9, 9, 9},  //-4
    {9, 9, 9, 9, 9, 9, 9, 9, 9},  //-3
    {9, 9, 9, 9, 9, 9, 9, 9, 9},  //-2
    {9, 9, 9, 9, 9, 9, 9, 9, 9},  //-1
    {8, 9, 8, 8, 9, 9, 8, 8, 8},  // 0
    {8, 8, 8, 8, 8, 8, 9, 8, 8},  // 1
    {8, 8, 8, 8, 8, 8, 9, 8, 8},  // 2
    {9, 9, 9, 9, 9, 9, 9, 8, 8},  // 3
    {8, 8, 8, 8, 8, 8, 9, 8, 8},  // 4
    {8, 8, 8, 8, 8, 8, 9, 8, 8},  // 5
    {8, 8, 8, 8, 8, 8, 9, 8, 8}}; // 6
const uint8_t PLAYAREA_MAP_SIZE_X = sizeof(playAreaMap[0]) / sizeof(playAreaMap[0][0]);
const uint8_t PLAYAREA_MAP_SIZE_Y = sizeof(playAreaMap) / sizeof(playAreaMap[0]);

uint32_t alien_posi_y = BLOCKSIZE * ALIEN_IMG_SIZE_Y * 6;

bool isCheckeEvapo[PLAYAREA_MAP_SIZE_Y][PLAYAREA_MAP_SIZE_X] = {false};

void printPlayAreaMap()
{
    for (int i = 0; i < PLAYAREA_MAP_SIZE_Y; i++)
    {
        for (int j = 0; j < PLAYAREA_MAP_SIZE_X; j++)
        {
            printf("%02X,", playAreaMap[PLAYAREA_MAP_SIZE_Y - i][j]);
        }
        printf("\n");
    }
}

uint8_t choiseRandColor()
{
    return rand() % (sizeof(colors) / sizeof(colors[0]) - 1) + 1;
}
uint8_t rand_i(uint8_t s, uint8_t e)
{
    return rand() % (e - s) + s;
}

void initRand(void)
{
    /* 現在の時刻を取得 */
    int rnd = (int)random(3000);
    printf("random= %d\n", rnd);

    /* 現在の時刻を乱数の種として乱数の生成系列を変更 */
    srand((unsigned int)rnd);
}

void printAlian(int32_t x, int32_t y, uint32_t color)
{
    for (int32_t i = 0; i < ALIEN_IMG_SIZE_X; i++)
    {
        for (int32_t j = 0; j < ALIEN_IMG_SIZE_Y; j++)
        {
            if (alienImg[j][i])
                M5.Lcd.fillRect(i * BLOCKSIZE + x, j * BLOCKSIZE + y, BLOCKSIZE, BLOCKSIZE, color); // x-pos,y-pos,x-size,y-size,color
        }
    }
}

void printPuyo(int32_t x, int32_t y, uint16_t color)
{
    for (int32_t i = 0; i < PUYO_IMG_SIZE_X; i++)
    {
        for (int32_t j = 0; j < PUYO_IMG_SIZE_Y; j++)
        {
            if (puyoImg[j][i])
                M5.Lcd.fillRect(i * BLOCKSIZE + x, j * BLOCKSIZE + y, BLOCKSIZE, BLOCKSIZE, color); // x-pos,y-pos,x-size,y-size,color
        }
    }
}

void printMap()
{
    int32_t x;
    int32_t offset_y = 0;
    alien_posi_y += ALIEN_SPEED;

    for (uint8_t l = 0; l < PLAYAREA_MAP_SIZE_Y; l++)
    {
        offset_y = alien_posi_y - BLOCKSIZE * ALIEN_IMG_SIZE_Y * l;
        for (uint8_t k = 0; k < PLAYAREA_MAP_SIZE_X; k++)
        {
            if (playAreaMap[l][k] == BLANK)
                continue;

            x = BLOCKSIZE * ALIEN_IMG_SIZE_X * k + X_OFFSET_PIXEL;
            switch (playAreaMap[l][k])
            {

            case 8: // Initial
                playAreaMap[l][k] = choiseRandColor();
                break;

            case 1:
            case 2:
            case 3:
            case 4:
            case 5: // Reserved
            case 6: // Reserved
                printAlian(x, offset_y, colors[playAreaMap[l][k]]);
                break;

            case 0x10:
            case 0x20:
            case 0x30:
            case 0x40:
            case 0x50: // Reserved
            case 0x60: // Reserved
                printPuyo(x, offset_y, colors[playAreaMap[l][k] >> PUYO_COLOR_OFFSET]);
                break;

            default:
                break;
            }
        }
    }
}

void initStage()
{
    M5.Lcd.setTextSize(3);
    m5.Lcd.clear();
}
void clearAlian()
{
    M5.Lcd.fillRect(X_OFFSET_PIXEL, 0, BLOCKSIZE * ALIEN_IMG_SIZE_X * PLAYAREA_MAP_SIZE_X, BLOCKSIZE * ALIEN_IMG_SIZE_Y * (PLAYAREA_MAP_SIZE_Y), BLACK); // x-pos,y-pos,x-size,y-size,color /
}

void findAdjacentCells(uint8_t x, uint8_t y, bool sameColor[PLAYAREA_MAP_SIZE_Y][PLAYAREA_MAP_SIZE_X], uint8_t color)
{
    // 上下左右のセルを探索して同じ数字のセルを抽出する
    if (color >= 0x10)
        color = color >> PUYO_COLOR_OFFSET;

    if (x < 0 || x >= PLAYAREA_MAP_SIZE_X || y < 0 || y >= PLAYAREA_MAP_SIZE_Y || sameColor[y][x])
    {
        // 範囲外のセルまたはすでに探索したセルは探索しない
        return;
    }

    if (playAreaMap[y][x] == color || playAreaMap[y][x] == (color << PUYO_COLOR_OFFSET))
    {
        sameColor[y][x] = true;

        // 上下左右のセルを探索
        findAdjacentCells(x - 1, y, sameColor, color); // 左
        findAdjacentCells(x + 1, y, sameColor, color); // 右
        findAdjacentCells(x, y - 1, sameColor, color); // 上
        findAdjacentCells(x, y + 1, sameColor, color); // 下
    }
    else
    {
        return;
    }
}

void checkCanErase(uint8_t y, uint8_t x, uint8_t color)
{
    // 座標が範囲外の場合は処理しない
    if (x < 0 || x >= PLAYAREA_MAP_SIZE_X || y < 0 || y >= PLAYAREA_MAP_SIZE_Y)
    {
        return;
    }

    // 探索したセルを記録する配列を初期化
    bool sameColor[PLAYAREA_MAP_SIZE_Y][PLAYAREA_MAP_SIZE_X] = {false};

    // 特定のセルから探索を開始
    if (!sameColor[y][x])
    {
        findAdjacentCells(x, y, sameColor, color);
    }

    uint8_t count = 0;
    for (uint8_t i = 0; i < PLAYAREA_MAP_SIZE_Y; i++)
    {
        for (uint8_t j = 0; j < PLAYAREA_MAP_SIZE_X; j++)
        {
            if (sameColor[i][j])
            {
                // printf("(%d, %d) ", j, i); // (x, y) 形式で座標を表示
                count += 1;
            }
        }
    }
    printf("\n");

    printPlayAreaMap();
    if (count >= THRESH) // 閾値以上同じ色がつながると消える
    {
        for (uint8_t i = 0; i < PLAYAREA_MAP_SIZE_Y; i++)
        {
            for (uint8_t j = 0; j < PLAYAREA_MAP_SIZE_X; j++)
            {
                if (sameColor[i][j])
                {
                    playAreaMap[i][j] = BLANK;
                }
            }
        }
    }
}

void findAdjacentCells2(uint8_t x, uint8_t y, bool sameColor[PLAYAREA_MAP_SIZE_Y][PLAYAREA_MAP_SIZE_X])
{
    // 上下左右のセルを探索して空白でないセルを抽出する
    if (x < 0 || x >= PLAYAREA_MAP_SIZE_X || y < 0 || y >= PLAYAREA_MAP_SIZE_Y || sameColor[y][x])
    {
        // 範囲外のセルは探索しない
        return;
    }
    if (playAreaMap[y][x] != BLANK)
    {
        sameColor[y][x] = true;
        isCheckeEvapo[y][x] = true;

        // 上下左右のセルを探索
        findAdjacentCells2(x - 1, y, sameColor); // 左
        findAdjacentCells2(x + 1, y, sameColor); // 右
        findAdjacentCells2(x, y - 1, sameColor); // 上
        findAdjacentCells2(x, y + 1, sameColor); // 下
    }
    else
    {
        return;
    }
}

void checkEvaporation(uint8_t y, uint8_t x)
{
    // 座標が範囲外の場合は処理しない
    if (x < 0 || x >= PLAYAREA_MAP_SIZE_X || y < 0 || y >= PLAYAREA_MAP_SIZE_Y)
    {
        return;
    }

    // 探索したセルを記録する配列を初期化
    bool sameColor[PLAYAREA_MAP_SIZE_Y][PLAYAREA_MAP_SIZE_X] = {false};

    // 特定のセルから探索を開始
    if (!sameColor[y][x])
    {
        findAdjacentCells2(x, y, sameColor);
    }

    uint8_t count = 0;
    for (uint8_t i = 0; i < PLAYAREA_MAP_SIZE_Y; i++)
    {
        for (uint8_t j = 0; j < PLAYAREA_MAP_SIZE_X; j++)
        {
            if (sameColor[i][j])
            {
                count += 1;
            }
        }
    }
    printf("Count = %d\n", count);

    if (count <= EVAPORATION_THRESH) // 閾値以下の小さな塊は蒸発する
    {
        for (uint8_t i = 0; i < PLAYAREA_MAP_SIZE_Y; i++)
        {
            for (uint8_t j = 0; j < PLAYAREA_MAP_SIZE_X; j++)
            {
                if (sameColor[i][j])
                {
                    playAreaMap[i][j] = BLANK;
                }
            }
        }
    }
}
void move_player(int32_t direction)
{
    printPuyo(player_x, player_y, BLACK);
    printPuyo(player_x + puyoOffset[puyoDirection][0] * PUYO_IMG_SIZE_X * BLOCKSIZE, player_y + puyoOffset[puyoDirection][1] * PUYO_IMG_SIZE_Y * BLOCKSIZE, BLACK);
    player_x += (BLOCKSIZE * ALIEN_IMG_SIZE_X) * direction;
    printPuyo(player_x, player_y, colors[nextColor[0]]);
    printPuyo(player_x + puyoOffset[puyoDirection][0] * PUYO_IMG_SIZE_X * BLOCKSIZE, player_y + puyoOffset[puyoDirection][1] * PUYO_IMG_SIZE_Y * BLOCKSIZE, colors[nextColor[1]]);
}

void rotate(int8_t direction)
{
    puyoDirection = (puyoDirection + direction) % 4;
    if (puyoDirection == 1) // 下向きの時、上向きにして入れ替える
    {
        uint16_t tmp = nextColor[0];
        nextColor[0] = nextColor[1];
        nextColor[1] = tmp;
    }
}

// int32_t shot_speed = 10;
int32_t shot_posi_y = player_y;
int8_t isShooting[2] = {0};

enum state
{
    WAITING = 0,
    SHOOTING,
    PRINTMAP,
    CHECKERASE,
    END
};
void shot()
{
    if (isShooting[0] == WAITING && isShooting[1] == WAITING)
    {
        return;
    }

    // 当たり判定
    uint8_t puyo_xy[2][2] = {{0, 0}, {0, 0}}; // y,x //新しく設置された場所を一時保存
    uint8_t x = (player_x - X_OFFSET_PIXEL) / ((BLOCKSIZE * ALIEN_IMG_SIZE_X));
    int y;
    for (y = 0; y < PLAYAREA_MAP_SIZE_Y; y++)
    {
        if (puyoDirection % 2 == 0) // 横向き
        {
            if (playAreaMap[y][x] != BLANK && isShooting[1] == SHOOTING && y - 1 >= 0) // Blankでないとき
            {
                isShooting[1] = PRINTMAP;
                playAreaMap[y - 1][x] = currentColor[0] << PUYO_COLOR_OFFSET; // puyoを置く
                puyo_xy[0][0] = y - 1;
                puyo_xy[0][1] = x;
            }
            if (playAreaMap[y + puyoOffset[puyoDirection][1]][x + puyoOffset[puyoDirection][0]] != BLANK && isShooting[0] == SHOOTING && y - 1 >= 0)
            {
                isShooting[0] = PRINTMAP;
                playAreaMap[y - 1 + puyoOffset[puyoDirection][1]][x + puyoOffset[puyoDirection][0]] = currentColor[1] << PUYO_COLOR_OFFSET; // puyoを置く
                puyo_xy[1][0] = y - 1;
                puyo_xy[1][1] = x + puyoOffset[puyoDirection][0];
            }
        }
        else // 縦向き
        {
            if (playAreaMap[y][x] != BLANK && isShooting[1] == SHOOTING && y - 1 >= 0) // Blankでないとき
            {
                isShooting[0] = PRINTMAP;
                isShooting[1] = PRINTMAP;
                playAreaMap[y - 1][x] = currentColor[1] << PUYO_COLOR_OFFSET; // puyoを置く
                playAreaMap[y - 2][x] = currentColor[0] << PUYO_COLOR_OFFSET; // puyoを置く
                puyo_xy[0][0] = y - 2;
                puyo_xy[0][1] = x;
                puyo_xy[1][0] = y - 1;
                puyo_xy[1][1] = x;
                break;
            }
        }
    }

    if (isShooting[0] == PRINTMAP && isShooting[1] == PRINTMAP)
    {
        isShooting[0] = CHECKERASE;
        isShooting[1] = CHECKERASE;
        clearAlian();
        printMap();
    }
    if (isShooting[0] == CHECKERASE && isShooting[1] == CHECKERASE)
    {
        checkCanErase(puyo_xy[0][0], puyo_xy[0][1], currentColor[0]);
        checkCanErase(puyo_xy[1][0], puyo_xy[1][1], currentColor[1]);

        isShooting[0] = END;
        isShooting[1] = END;
        rotate(RIGHT);
    }

    printPuyo(player_x, shot_posi_y, BLACK);
    printPuyo(player_x + puyoOffset[puyoDirection][0] * PUYO_IMG_SIZE_X * BLOCKSIZE, shot_posi_y + puyoOffset[puyoDirection][1] * PUYO_IMG_SIZE_Y * BLOCKSIZE, BLACK);
    // shot_posi_y -= shot_speed;
    // printPuyo(player_x, shot_posi_y, colors[currentColor[0]]);
    // printPuyo(player_x + puyoOffset[puyoDirection][0] * PUYO_IMG_SIZE_X * BLOCKSIZE, shot_posi_y + puyoOffset[puyoDirection][1] * PUYO_IMG_SIZE_Y * BLOCKSIZE, colors[currentColor[1]]);
    if (shot_posi_y < 0)
    {
        isShooting[0] = WAITING;
        isShooting[1] = WAITING;
        shot_posi_y = player_y;
    }
}

void setup()
{
    Serial.begin(115200); // 115200bps

    M5.begin(true, false, true);
    M5.Power.begin();
    initRand();
    M5.Lcd.setCursor(100, 100);
    initStage();
    printMap();
    currentColor[0] = choiseRandColor();
    currentColor[1] = choiseRandColor();
    nextColor[0] = choiseRandColor();
    nextColor[1] = choiseRandColor();
}
uint8_t check_y = 0;
uint8_t check_x = 0;

void funcCheckEvaporation(uint8_t *y, uint8_t *x)
{
    if (!isCheckeEvapo[*y][*x])
    {
        checkEvaporation(check_y, check_x);
        isCheckeEvapo[*y][*x] = true;
    }
    else
    {
        *x += 1;
        if (*x == PLAYAREA_MAP_SIZE_X)
        {
            *x = 0;
            *y += 1;
        }
        if (*y == PLAYAREA_MAP_SIZE_Y)
        {
            *y = 0;
            memset(isCheckeEvapo, false, sizeof(isCheckeEvapo));
        }
    }
}
void loop()
{

    if (micros() - pre > STEP_PERIOD * 1000)
    {
        pre = micros();
        if ((counterLcdRefresh++) % 300 == 0)
        {
            clearAlian();
            printMap();
            if (isShooting[0] == WAITING && isShooting[1] == WAITING)
            {
                printPuyo(player_x, player_y, colors[nextColor[0]]);
                printPuyo(player_x + puyoOffset[puyoDirection][0] * PUYO_IMG_SIZE_X * BLOCKSIZE, player_y + puyoOffset[puyoDirection][1] * PUYO_IMG_SIZE_Y * BLOCKSIZE, colors[nextColor[1]]);
            }
        }
        shot();
        if (isShooting[0] == END && isShooting[1] == END)
        {
            isShooting[0] = WAITING;
            isShooting[1] = WAITING;
        }
        funcCheckEvaporation(&check_y, &check_x);
    }

    M5.update(); // ボタンを読み取る
    // Button define
    if (M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200))
    {
        move_player(LEFT);
    }

    if (M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200))
    {
        if (isShooting[0] == WAITING && isShooting[1] == WAITING)
        {
            isShooting[0] = SHOOTING;
            isShooting[1] = SHOOTING;

            for (uint8_t i = 0; i < 2; i++)
            {
                currentColor[i] = nextColor[i];
                nextColor[i] = choiseRandColor();
            }
        }
    }

    if (M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200))
    {
        move_player(RIGHT);
    }
}
