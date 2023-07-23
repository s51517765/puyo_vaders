// 230 x 315 画面サイズ
#include <Arduino.h>
#include <M5Stack.h> //0.3.9
#define BLOCKSIZE 3
#define STEP_PERIOD 10 // 画面更新周期
#define ALIEN_SPEED 2
#define LEFT -1
#define RIGHT 1
#define PUYO_COLOR_OFFSET 4
// #define printf(...)

uint32_t pre = 0;
uint32_t counterLcdRefresh = 0;

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
    BLACK,
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
    {8, 8, 8, 8, 8, 8, 8, 8, 8},  // 1
    {8, 8, 8, 8, 8, 8, 8, 8, 8},  // 2
    {8, 8, 8, 8, 8, 8, 8, 8, 8},  // 3
    {8, 8, 8, 8, 8, 8, 8, 8, 8},  // 4
    {8, 8, 8, 8, 8, 8, 8, 8, 8}}; // 5
uint8_t ALIEN_MAP_SIZE_X = sizeof(playAreaMap[0]) / sizeof(playAreaMap[0][0]);
uint8_t ALIEN_MAP_SIZE_Y = sizeof(playAreaMap) / sizeof(playAreaMap[0]);

uint32_t alien_posi_y = BLOCKSIZE * ALIEN_IMG_SIZE_Y * 2;

uint8_t choiseRandColor()
{
    return rand() % (sizeof(colors) / sizeof(colors[0]) - 1) + 1;
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
    for (int i = 0; i < ALIEN_IMG_SIZE_X; i++)
    {
        for (int j = 0; j < ALIEN_IMG_SIZE_Y; j++)
        {
            if (alienImg[j][i])
                M5.Lcd.fillRect(i * BLOCKSIZE + x, j * BLOCKSIZE + y, BLOCKSIZE, BLOCKSIZE, color); // x-pos,y-pos,x-size,y-size,color
            else
                M5.Lcd.fillRect(i * BLOCKSIZE + x, j * BLOCKSIZE + y, BLOCKSIZE, BLOCKSIZE, BLACK);
        }
    }
}

void printPuyo(int32_t x, int32_t y, uint32_t color)
{
    for (int i = 0; i < PUYO_IMG_SIZE_X; i++)
    {
        for (int j = 0; j < PUYO_IMG_SIZE_Y; j++)
        {
            if (puyoImg[j][i])
                M5.Lcd.fillRect(i * BLOCKSIZE + x, j * BLOCKSIZE + y, BLOCKSIZE, BLOCKSIZE, color); // x-pos,y-pos,x-size,y-size,color
            else
                M5.Lcd.fillRect(i * BLOCKSIZE + x, j * BLOCKSIZE + y, BLOCKSIZE, BLOCKSIZE, BLACK);
        }
    }
}

void printMap()
{
    printf("[printMap]\n");
    int offset_x = 5;
    int offset_y = 0;
    alien_posi_y += ALIEN_SPEED;

    for (int l = 0; l < ALIEN_MAP_SIZE_Y; l++)
    {
        offset_y = alien_posi_y - BLOCKSIZE * ALIEN_IMG_SIZE_Y * l;
        for (int k = 0; k < ALIEN_MAP_SIZE_X; k++)
        {
            if (playAreaMap[l][k] == 9) // Blank
                continue;

            offset_x = BLOCKSIZE * ALIEN_IMG_SIZE_X * k;
            printf("puyo 0x%x\n", playAreaMap[l][k]);
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
                printAlian(offset_x, offset_y, colors[playAreaMap[l][k]]);
                break;

            case 0x10:
            case 0x20:
            case 0x30:
            case 0x40:
            case 0x50: // Reserved
            case 0x60: // Reserved
                printPuyo(offset_x, offset_y, colors[playAreaMap[l][k] >> PUYO_COLOR_OFFSET]);
                break;

            default:
                printf("default\n");
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
    M5.Lcd.fillRect(0, 0, BLOCKSIZE * ALIEN_IMG_SIZE_X * ALIEN_MAP_SIZE_X, BLOCKSIZE * ALIEN_IMG_SIZE_Y * (ALIEN_MAP_SIZE_Y), BLACK); // x-pos,y-pos,x-size,y-size,color /
}

int32_t player_x_offset = 0;
int32_t player_x = player_x_offset;
int32_t player_y = 216;

void move_player(int32_t direction)
{
    printPuyo(player_x, player_y, BLACK);
    player_x += (BLOCKSIZE * ALIEN_IMG_SIZE_X) * direction;
    printPuyo(player_x, player_y, colors[nextColor[0]]);
}

int32_t shot_speed = 20;
int32_t shot_posi_y = player_y;
bool isShooting = false;
void shot()
{
    if (!isShooting)
        return;

    // 当たり判定
    uint8_t x = (player_x - player_x_offset) / ((BLOCKSIZE * ALIEN_IMG_SIZE_X));

    for (int y = 0; y < ALIEN_MAP_SIZE_Y; y++)
    {
        {
            if (abs(-shot_posi_y + alien_posi_y - BLOCKSIZE * ALIEN_IMG_SIZE_Y * y) < BLOCKSIZE * ALIEN_IMG_SIZE_Y * 3)
            {
                if (playAreaMap[y][x] != 9) // Blankでないとき
                {
                    isShooting = false;
                    shot_posi_y = player_y;
                    if (y - 1 >= 0)
                    {
                        playAreaMap[y - 1][x] = currentColor[0] << PUYO_COLOR_OFFSET; // puyoを置く
                        printf("Hit: x,y= %d,%d\n", x, y);
                    }
                    return;
                }
            }
        }
    }

    printPuyo(player_x, shot_posi_y, BLACK);
    shot_posi_y -= shot_speed;
    printPuyo(player_x, shot_posi_y, colors[currentColor[0]]);
    if (shot_posi_y < 0)
    {
        isShooting = false;
        shot_posi_y = player_y;
    }
    printf("shot\n");
}

void setup()
{
    Serial.begin(115200); // 115200bps

    M5.begin(true, false, true);
    M5.Power.begin();
    initRand();
    int result = 1;
    printf("%d", result);
    M5.Lcd.setCursor(100, 100);
    initStage();
    printMap();
    currentColor[0] = choiseRandColor();
    nextColor[0] = choiseRandColor();
}

void loop()
{

    if (micros() - pre > STEP_PERIOD * 1000)
    {
        pre = micros();
        if ((counterLcdRefresh++) % 50 == 0)
        {
            clearAlian();
            printMap();
        }
        shot();
        printPuyo(player_x, player_y, colors[nextColor[0]]);
    }

    M5.update(); // ボタンを読み取る
    // Button define
    if (M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200))
    {
        // A + C 同時押し
        if (M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200))
        {
            // Pause
            delay(12000);
        }
        move_player(LEFT);
    }

    if (M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200))
    {
        isShooting = true;
        currentColor[0] = nextColor[0];
        nextColor[0] = choiseRandColor();
    }

    if (M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200))
    {
        move_player(RIGHT);
    }
}
