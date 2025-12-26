#include "include/raylib.h"
#include "include/raymath.h"
#include <vector>
#include <algorithm>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

struct Fish {
    Vector2 position;
    float radius;
    float speed;
    bool active;
    Color color;
};

// --- 全局变量 ---
Fish player = { {(float)SCREEN_WIDTH/2, (float)SCREEN_HEIGHT/2}, 20.0f, 4.0f, true, YELLOW };
std::vector<Fish> enemies;
int gameScore = 0;
bool isGameOver = false;

// --- 【核心优化】：自定义绘鱼函数 ---
void DrawFish(Vector2 pos, float radius, Color color, float speed) {
    bool facingRight = (speed >= 0);
    float direction = facingRight ? 1.0f : -1.0f;

    // 1. 绘制尾巴 (使用更稳健的坐标计算)
    // 尾巴尖部连接点
    Vector2 joint = { pos.x - (radius * 1.2f * direction), pos.y };
    // 尾巴上下两个张开的点
    Vector2 tip1 = { pos.x - (radius * 2.2f * direction), pos.y - radius * 0.8f };
    Vector2 tip2 = { pos.x - (radius * 2.2f * direction), pos.y + radius * 0.8f };

    // 关键修复：无论向左向右，确保顶点顺序一致
    if (facingRight) {
        DrawTriangle(joint, tip1, tip2, color);
    } else {
        // 向左时，交换 tip1 和 tip2 的顺序，确保逆时针排列
        DrawTriangle(joint, tip2, tip1, color);
    }

    // 2. 绘制身体 (椭圆)
    DrawEllipse((int)pos.x, (int)pos.y, radius * 1.5f, radius, color);

    // 3. 绘制眼睛
    float eyeOffsetX = radius * 0.8f * direction;
    float eyeOffsetY = -radius * 0.3f;
    DrawCircleV({pos.x + eyeOffsetX, pos.y + eyeOffsetY}, radius * 0.2f, WHITE);
    DrawCircleV({pos.x + eyeOffsetX, pos.y + eyeOffsetY}, radius * 0.1f, BLACK);
}
void SpawnEnemy() {
    float r = (float)GetRandomValue(8, (int)player.radius + 15);
    float s = (float)GetRandomValue(2, 5);
    float y = (float)GetRandomValue(50, SCREEN_HEIGHT - 50);
    
    if (GetRandomValue(0, 1) == 0) { // 从左侧出
        enemies.push_back({ {-100, y}, r, s, true, GREEN });
    } else { // 从右侧出
        enemies.push_back({ {(float)SCREEN_WIDTH + 100, y}, r, -s, true, RED });
    }
}

void UpdateDrawFrame() {
    if (!isGameOver) {
        // 玩家控制：平滑跟随鼠标
        Vector2 mousePos = GetMousePosition();
        float moveX = (mousePos.x - player.position.x) * 0.1f;
        player.position.x += moveX;
        player.position.y += (mousePos.y - player.position.y) * 0.1f;

        // 根据移动方向确定玩家的伪“速度”，用于绘制朝向
        float playerDir = (moveX >= 0) ? 1.0f : -1.0f;

        if (GetRandomValue(0, 100) < 3) SpawnEnemy();

        for (size_t i = 0; i < enemies.size(); i++) {
            enemies[i].position.x += enemies[i].speed;
            
            // 颜色动态逻辑：比玩家大的鱼变红，小的变绿
            enemies[i].color = (enemies[i].radius > player.radius) ? RED : GREEN;

            if (CheckCollisionCircles(player.position, player.radius, enemies[i].position, enemies[i].radius)) {
                if (player.radius >= enemies[i].radius) {
                    enemies[i].active = false;
                    player.radius += 0.8f;
                    gameScore += 10;
                } else {
                    isGameOver = true;
                    #if defined(PLATFORM_WEB)
                    EM_ASM({
                        if (typeof window.parent.UpdateWebScore === 'function') window.parent.UpdateWebScore($0);
                        else if (typeof UpdateWebScore === 'function') UpdateWebScore($0);
                    }, gameScore);
                    #endif
                }
            }
        }

        auto it = enemies.begin();
        while (it != enemies.end()) {
            if (!it->active || it->position.x < -300 || it->position.x > SCREEN_WIDTH + 300) {
                it = enemies.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        if (IsKeyPressed(KEY_R) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            player.radius = 20.0f;
            gameScore = 0;
            enemies.clear();
            isGameOver = false;
        }
    }

    BeginDrawing();
        ClearBackground({ 0, 80, 150, 255 }); // 更深的海洋蓝

        // 绘制背景装饰（小气泡）
        for(int i=0; i<5; i++) DrawCircle(200*i + (int)(GetTime()*20)%50, 100*i, 2, {255,255,255,50});

        if (!isGameOver) {
            // 绘制敌鱼
            for (const auto& enemy : enemies) {
                DrawFish(enemy.position, enemy.radius, enemy.color, enemy.speed);
            }
            // 绘制玩家 (默认向右移动)
            float moveDir = (GetMousePosition().x - player.position.x);
            DrawFish(player.position, player.radius, player.color, moveDir);

            DrawText(TextFormat("SCORE: %d", gameScore), 25, 25, 20, WHITE);
            DrawText(TextFormat("SIZE: %.1f", player.radius), 25, 55, 18, SKYBLUE);
        } else {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.85f));
            DrawText("EVO HALTED - CONSUMED", SCREEN_WIDTH/2 - 160, SCREEN_HEIGHT/2 - 40, 30, RED);
            DrawText(TextFormat("FINAL SCORE: %d", gameScore), SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT/2 + 10, 20, WHITE);
            DrawText("Click to Re-evolve", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 60, 18, GOLD);
        }
    EndDrawing();
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "TinyPulse - Hungry Fish");
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) UpdateDrawFrame();
#endif
    CloseWindow();
    return 0;
}