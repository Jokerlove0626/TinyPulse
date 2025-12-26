#include "include/raylib.h"
#include <string.h>

// Web 环境判定
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

// --- 游戏常量 ---
const int ROWS = 20;
const int COLS = 10;
const int CELL_SIZE = 30;

// --- 全局变量 ---
int gameGrid[ROWS][COLS] = { 0 };
int score = 0;
int totalLines = 0;
bool isGameOver = false;

int currentMatrix[4][4];
int nextIdx;
int currentIdx;
int posX = 3, posY = 0;
float timer = 0;
float dropInterval = 0.5f;

// 颜色定义
Color shapeColors[8] = {
    {20, 20, 20, 255}, 
    SKYBLUE, BLUE, ORANGE, YELLOW, GREEN, PURPLE, RED
};

// 7 种经典形状数据
int shapes[7][4][4] = {
    {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}}, // I
    {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}, // J
    {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}, // L
    {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}, // O
    {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}}, // S
    {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}, // T
    {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}  // Z
};

// --- 功能函数 ---

bool CheckCollision(int nextX, int nextY, int shape[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (shape[i][j] == 1) {
                int tx = nextX + j, ty = nextY + i;
                if (tx < 0 || tx >= COLS || ty >= ROWS) return true;
                if (ty >= 0 && gameGrid[ty][tx] != 0) return true;
            }
        }
    }
    return false;
}

void ClearFullLines() {
    int linesFound = 0;
    for (int r = ROWS - 1; r >= 0; r--) {
        bool isFull = true;
        for (int c = 0; c < COLS; c++) if (gameGrid[r][c] == 0) { isFull = false; break; }
        if (isFull) {
            linesFound++;
            for (int rh = r; rh > 0; rh--) {
                for (int c = 0; c < COLS; c++) gameGrid[rh][c] = gameGrid[rh - 1][c];
            }
            for (int c = 0; c < COLS; c++) gameGrid[0][c] = 0;
            r++; 
        }
    }
    if (linesFound == 1) score += 100;
    else if (linesFound == 2) score += 300;
    else if (linesFound == 3) score += 500;
    else if (linesFound == 4) score += 800;
    totalLines += linesFound;
}

void UpdateDrawFrame() {
    if (isGameOver) {
        if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            score = 0; totalLines = 0; isGameOver = false;
            for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) gameGrid[r][c] = 0;
            currentIdx = GetRandomValue(0, 6); nextIdx = GetRandomValue(0, 6);
            memcpy(currentMatrix, shapes[currentIdx], sizeof(currentMatrix));
            posX = 3; posY = 0;
        }
    } else {
        if (IsKeyPressed(KEY_UP)) {
            int tmp[4][4];
            for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) tmp[j][3 - i] = currentMatrix[i][j];
            if (!CheckCollision(posX, posY, tmp)) memcpy(currentMatrix, tmp, sizeof(currentMatrix));
        }
        if (IsKeyPressed(KEY_LEFT) && !CheckCollision(posX - 1, posY, currentMatrix)) posX--;
        if (IsKeyPressed(KEY_RIGHT) && !CheckCollision(posX + 1, posY, currentMatrix)) posX++;

        timer += GetFrameTime();
        if (timer >= dropInterval || IsKeyDown(KEY_DOWN)) {
            if (CheckCollision(posX, posY + 1, currentMatrix)) {
                for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++)
                    if (currentMatrix[i][j] == 1 && (posY + i) >= 0) gameGrid[posY + i][posX + j] = currentIdx + 1;
                ClearFullLines();
                currentIdx = nextIdx; nextIdx = GetRandomValue(0, 6);
                memcpy(currentMatrix, shapes[currentIdx], sizeof(currentMatrix));
                posX = 3; posY = 0;
                if (CheckCollision(posX, posY, currentMatrix)) {
                    isGameOver = true;
                    // 【增强版 Web 通信】：穿透 IFrame 寻找门户网站的函数
                    #if defined(PLATFORM_WEB)
                    EM_ASM({
                        var score = $0;
                        console.log("C++: 尝试提交分数 " + score);
                        if (typeof UpdateWebScore === 'function') {
                            UpdateWebScore(score);
                        } else if (window.parent && typeof window.parent.UpdateWebScore === 'function') {
                            window.parent.UpdateWebScore(score);
                        } else {
                            console.warn("未找到 UpdateWebScore 函数");
                        }
                    }, score);
                    #endif
                }
            } else { posY++; }
            timer = 0;
        }
    }

    BeginDrawing();
        ClearBackground({10, 10, 10, 255});
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                DrawRectangleLines(c * CELL_SIZE, r * CELL_SIZE, CELL_SIZE, CELL_SIZE, {40, 40, 40, 255});
                if (gameGrid[r][c] != 0) DrawRectangle(c * CELL_SIZE + 1, r * CELL_SIZE + 1, CELL_SIZE - 2, CELL_SIZE - 2, shapeColors[gameGrid[r][c]]);
            }
        }
        if (!isGameOver) {
            for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++)
                if (currentMatrix[i][j] == 1) DrawRectangle((posX + j) * CELL_SIZE + 1, (posY + i) * CELL_SIZE + 1, CELL_SIZE - 2, CELL_SIZE - 2, shapeColors[currentIdx + 1]);
        }
        int uiX = COLS * CELL_SIZE + 25;
        DrawText("TinyPulse", uiX, 30, 24, GOLD);
        DrawText("SCORE", uiX, 85, 15, LIGHTGRAY);
        DrawText(TextFormat("%06d", score), uiX, 105, 28, RAYWHITE);
        DrawText("NEXT", uiX, 160, 15, LIGHTGRAY);
        DrawRectangle(uiX, 185, 100, 100, {30, 30, 30, 255});
        for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++)
            if (shapes[nextIdx][i][j] == 1) DrawRectangle(uiX + 20 + j * 15, 210 + i * 15, 13, 13, shapeColors[nextIdx + 1]);
        
        if (isGameOver) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.85f));
            DrawText("GAME OVER", GetScreenWidth()/2 - 85, GetScreenHeight()/2 - 50, 30, RED);
            DrawText(TextFormat("FINAL SCORE: %d", score), GetScreenWidth()/2 - 70, GetScreenHeight()/2, 20, RAYWHITE);
            DrawText("Press ENTER to Restart", GetScreenWidth()/2 - 100, GetScreenHeight()/2 + 50, 16, GOLD);
        }
    EndDrawing();
}

int main() {
    InitWindow(COLS * CELL_SIZE + 200, ROWS * CELL_SIZE, "TinyPulse - Tetris");
    currentIdx = GetRandomValue(0, 6); nextIdx = GetRandomValue(0, 6);
    memcpy(currentMatrix, shapes[currentIdx], sizeof(currentMatrix));

#if defined(PLATFORM_WEB)
    EM_ASM({
        window.addEventListener("keydown", function(e) {
            // 拦截：空格(32), 左(37), 上(38), 右(39), 下(40)
            if([32, 37, 38, 39, 40].indexOf(e.keyCode) > -1) {
                e.preventDefault();
            }
        }, false);
        console.log("TinyPulse: 已成功锁定浏览器滚动行为");
    });
    
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    // 桌面端逻辑
    SetTargetFPS(60);
    currentIdx = GetRandomValue(0, 6);
    nextIdx = GetRandomValue(0, 6);
    memcpy(currentMatrix, shapes[currentIdx], sizeof(currentMatrix));
    
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif

    CloseWindow();
    return 0;
}
