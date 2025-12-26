#include "include/raylib.h"
#include <vector>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int GRID_SIZE = 20;
const int GRID_WIDTH = SCREEN_WIDTH / GRID_SIZE;
const int GRID_HEIGHT = SCREEN_HEIGHT / GRID_SIZE;

struct SnakeNode {
    int x, y;
};

std::vector<SnakeNode> snake;
Vector2 speed = { 1, 0 };
Vector2 nextDir = { 1, 0 };
SnakeNode food;
int score = 0;
bool isGameOver = false;
float moveCounter = 0;
float moveDelay = 0.12f;

void SpawnFood() {
    food.x = GetRandomValue(0, GRID_WIDTH - 1);
    food.y = GetRandomValue(0, GRID_HEIGHT - 1);
}

void ResetGame() {
    snake.clear();
    snake.push_back({ 10, 10 });
    snake.push_back({ 9, 10 });
    snake.push_back({ 8, 10 });
    speed = { 1, 0 };
    nextDir = { 1, 0 };
    score = 0;
    isGameOver = false;
    moveDelay = 0.12f;
    SpawnFood();
}

void UpdateDrawFrame() {
    if (!isGameOver) {
        if ((IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) && speed.y == 0) nextDir = { 0, -1 };
        if ((IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) && speed.y == 0) nextDir = { 0, 1 };
        if ((IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) && speed.x == 0) nextDir = { -1, 0 };
        if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) && speed.x == 0) nextDir = { 1, 0 };

        moveCounter += GetFrameTime();
        if (moveCounter >= moveDelay) {
            moveCounter = 0;
            speed = nextDir;

            SnakeNode nextHead = { snake[0].x + (int)speed.x, snake[0].y + (int)speed.y };

            if (nextHead.x < 0 || nextHead.x >= GRID_WIDTH || nextHead.y < 0 || nextHead.y >= GRID_HEIGHT) {
                isGameOver = true;
            }

            for (const auto& node : snake) {
                if (nextHead.x == node.x && nextHead.y == node.y) {
                    isGameOver = true;
                    break;
                }
            }

            if (!isGameOver) {
                snake.insert(snake.begin(), nextHead);
                if (nextHead.x == food.x && nextHead.y == food.y) {
                    score += 10;
                    moveDelay *= 0.98f;
                    SpawnFood();
                    #if defined(PLATFORM_WEB)
                    EM_ASM({ if (window.parent && window.parent.UpdateWebScore) window.parent.UpdateWebScore($0); }, score);
                    #endif
                } else {
                    snake.pop_back();
                }
            }
        }
    } else {
        if (IsKeyPressed(KEY_R) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) ResetGame();
    }

    BeginDrawing();
        ClearBackground({ 15, 15, 15, 255 });

        // 画食物
        DrawRectangle(food.x * GRID_SIZE + 2, food.y * GRID_SIZE + 2, GRID_SIZE - 4, GRID_SIZE - 4, RED);

        // 画蛇 (修复后的循环)
        for (size_t i = 0; i < snake.size(); i++) {
            Color c = (i == 0) ? LIME : GREEN; // 蛇头颜色区分
            DrawRectangle(snake[i].x * GRID_SIZE + 1, snake[i].y * GRID_SIZE + 1, GRID_SIZE - 2, GRID_SIZE - 2, c);
        }

        DrawText(TextFormat("SCORE: %d", score), 20, 20, 20, DARKGRAY);

        if (isGameOver) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.8f));
            DrawText("GAME OVER", SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT/2 - 20, 30, RED);
            DrawText("Click to Restart", SCREEN_WIDTH/2 - 65, SCREEN_HEIGHT/2 + 20, 15, RAYWHITE);
        }
    EndDrawing();
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "TinyPulse - Snake");
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