#include "include/raylib.h"
#include <vector>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

// --- 经典 9x9 规格 ---
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int COLS = 9;
const int ROWS = 9;
const int CELL_SIZE = 60; 
const int OFFSET_X = (SCREEN_WIDTH - (COLS * CELL_SIZE)) / 2;
const int OFFSET_Y = (SCREEN_HEIGHT - (ROWS * CELL_SIZE)) / 2;

struct Cell {
    bool isMine = false;
    bool isRevealed = false;
    bool isFlagged = false;
    int neighborCount = 0;
};

// --- 全局变量 ---
Cell grid[COLS][ROWS];
bool isGameOver = false;
bool isWin = false;
int minesCount = 10; 

// 检查坐标有效性
bool IsValid(int x, int y) {
    return x >= 0 && x < COLS && y >= 0 && y < ROWS;
}

// 检查胜利判定：遍历所有格子，如果没有“未翻开的非雷格”，即为胜利
void CheckWin() {
    if (isGameOver) return; // 已失败则不检查胜利

    bool hasSafeHidden = false;
    for (int x = 0; x < COLS; x++) {
        for (int y = 0; y < ROWS; y++) {
            if (!grid[x][y].isMine && !grid[x][y].isRevealed) {
                hasSafeHidden = true;
                break;
            }
        }
        if (hasSafeHidden) break;
    }

    if (!hasSafeHidden) {
        isWin = true;
        #if defined(PLATFORM_WEB)
        // 胜利时向主门户提交一个高分
        EM_ASM({ if (window.parent && window.parent.UpdateWebScore) window.parent.UpdateWebScore(1000); });
        #endif
    }
}

// 递归展开 (Flood Fill)
void RevealCell(int x, int y) {
    if (!IsValid(x, y) || grid[x][y].isRevealed || grid[x][y].isFlagged) return;

    grid[x][y].isRevealed = true;

    // 如果是数字 0，则递归周围 8 个格子
    if (grid[x][y].neighborCount == 0 && !grid[x][y].isMine) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i != 0 || j != 0) RevealCell(x + i, y + j);
            }
        }
    }
    
    CheckWin(); // 每次递归翻开都触发一次胜利检查
}

void ResetGame() {
    isGameOver = false;
    isWin = false;
    for (int x = 0; x < COLS; x++) {
        for (int y = 0; y < ROWS; y++) grid[x][y] = Cell();
    }

    int placedMines = 0;
    while (placedMines < minesCount) {
        int rx = GetRandomValue(0, COLS - 1);
        int ry = GetRandomValue(0, ROWS - 1);
        if (!grid[rx][ry].isMine) {
            grid[rx][ry].isMine = true;
            placedMines++;
        }
    }

    for (int x = 0; x < COLS; x++) {
        for (int y = 0; y < ROWS; y++) {
            if (grid[x][y].isMine) continue;
            int count = 0;
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    if (IsValid(x + i, y + j) && grid[x + i][y + j].isMine) count++;
                }
            }
            grid[x][y].neighborCount = count;
        }
    }
}

void UpdateDrawFrame() {
    if (!isGameOver && !isWin) {
        // 左键：挖开
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            int gx = (mouse.x - OFFSET_X) / CELL_SIZE;
            int gy = (mouse.y - OFFSET_Y) / CELL_SIZE;
            if (IsValid(gx, gy) && !grid[gx][gy].isFlagged) {
                if (grid[gx][gy].isMine) {
                    isGameOver = true;
                    // 游戏结束，翻开所有地雷给玩家看
                    for(int i=0; i<COLS; i++)
                        for(int j=0; j<ROWS; j++)
                            if(grid[i][j].isMine) grid[i][j].isRevealed = true;
                } else {
                    RevealCell(gx, gy);
                }
            }
        }
        // 右键：插旗
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            int gx = (mouse.x - OFFSET_X) / CELL_SIZE;
            int gy = (mouse.y - OFFSET_Y) / CELL_SIZE;
            if (IsValid(gx, gy) && !grid[gx][gy].isRevealed) {
                grid[gx][gy].isFlagged = !grid[gx][gy].isFlagged;
            }
        }
    } else {
        // 结束状态下点击或按 R 重启
        if (IsKeyPressed(KEY_R) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) ResetGame();
    }

    BeginDrawing();
        ClearBackground({ 45, 50, 60, 255 }); 

        for (int x = 0; x < COLS; x++) {
            for (int y = 0; y < ROWS; y++) {
                Rectangle rect = {(float)OFFSET_X + x * CELL_SIZE, (float)OFFSET_Y + y * CELL_SIZE, (float)CELL_SIZE - 2, (float)CELL_SIZE - 2};
                
                if (grid[x][y].isRevealed) {
                    DrawRectangleRec(rect, { 220, 225, 230, 255 });
                    if (grid[x][y].isMine) {
                        DrawCircleV({rect.x + CELL_SIZE/2, rect.y + CELL_SIZE/2}, 12, {40, 40, 40, 255});
                    } else if (grid[x][y].neighborCount > 0) {
                        Color nColor = (grid[x][y].neighborCount == 1) ? BLUE : (grid[x][y].neighborCount == 2 ? DARKGREEN : RED);
                        DrawText(TextFormat("%d", grid[x][y].neighborCount), rect.x + 22, rect.y + 12, 36, nColor);
                    }
                } else {
                    DrawRectangleRec(rect, { 90, 100, 115, 255 });
                    if (grid[x][y].isFlagged) {
                        // 画一个简单的小红旗
                        DrawRectangle(rect.x + 18, rect.y + 12, 4, 35, RED);
                        DrawTriangle({rect.x + 22, rect.y + 12}, {rect.x + 22, rect.y + 28}, {rect.x + 45, rect.y + 20}, RED);
                    }
                }
            }
        }

        if (isGameOver || isWin) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.75f));
            const char* txt = isGameOver ? "BOOM! MISSION FAILED" : "CONGRATULATIONS! SECURED";
            Color txtCol = isGameOver ? RED : GOLD;
            DrawText(txt, SCREEN_WIDTH/2 - MeasureText(txt, 30)/2, 260, 30, txtCol);
            DrawText("CLICK ANYWHERE TO RESTART", SCREEN_WIDTH/2 - 120, 310, 18, GRAY);
        }
    EndDrawing();
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "TinyPulse - Minesweeper");
    ResetGame();
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) UpdateDrawFrame();
#endif
    CloseWindow();
    return 0;
}