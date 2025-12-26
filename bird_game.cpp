#include "include/raylib.h"
#include <vector>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// --- 物理参数 ---
const float GRAVITY = 950.0f;
const float JUMP_FORCE = -330.0f;
const float PIPE_SPEED = 210.0f;
const int PIPE_GAP = 185;

struct Pipe {
    float x;
    float topHeight;
    bool passed;
};

float birdY = 300.0f;
float birdVelocity = 0.0f;
std::vector<Pipe> pipes;
int score = 0;
bool isGameOver = false;
float pipeTimer = 0;

void ResetGame() {
    birdY = 300.0f;
    birdVelocity = 0.0f;
    pipes.clear();
    score = 0;
    isGameOver = false;
    pipeTimer = 0;
}

// --- 自定义几何小鸟绘制函数 ---
void DrawBird(float x, float y, float velocity) {
    // 计算旋转角度：速度越快，头垂得越低；向上飞时，头抬起
    // 将速度映射到 -30 到 90 度之间
    float rotation = velocity * 0.15f; 
    if (rotation < -30) rotation = -30;
    if (rotation > 90) rotation = 90;

    // 旋转中心设为身体中心
    Vector2 center = { x, y };

    // 1. 绘制翅膀/身体 (用圆角矩形或椭圆)
    // 为了让它旋转，我们使用 DrawCircleSector 或组合图形
    // 简化方案：旋转坐标系后画几何图形
    
    // 绘制身体 (黄色主圆)
    DrawCircleV(center, 16, YELLOW);
    
    // 绘制白眼
    Vector2 eyePos = { x + 8, y - 6 };
    // 简单的旋转坐标变换 (这里简化处理，眼睛随身体微调)
    DrawCircleV({x + 8, y - 4}, 5, WHITE);
    DrawCircleV({x + 10, y - 4}, 2, BLACK);

    // 绘制嘴巴 (橙色三角形)
    // 嘴巴需要跟随旋转，所以用三个点计算位置
    float rad = rotation * (PI / 180.0f);
    Vector2 p1 = { x + 12, y + 2 };
    Vector2 p2 = { x + 12, y + 12 };
    Vector2 p3 = { x + 24, y + 7 };
    // 这里如果要做完美的旋转嘴巴，可以用 DrawTrianglePro，但 Raylib 原生不支持
    // 我们简单用速度补偿嘴巴的位置
    DrawTriangle({x + 10, y}, {x + 10, y + 8}, {x + 20, y + 4}, ORANGE);
}

void UpdateDrawFrame() {
    float dt = GetFrameTime();
    if (dt > 0.1f) dt = 0.1f;

    if (!isGameOver) {
        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            birdVelocity = JUMP_FORCE;
        }
        
        birdVelocity += GRAVITY * dt;
        birdY += birdVelocity * dt;

        if (birdY > SCREEN_HEIGHT || birdY < 0) isGameOver = true;

        pipeTimer += dt;
        if (pipeTimer > 2.2f) {
            pipes.push_back({ (float)SCREEN_WIDTH, (float)GetRandomValue(100, 350), false });
            pipeTimer = 0;
        }

        for (size_t i = 0; i < pipes.size(); i++) {
            pipes[i].x -= PIPE_SPEED * dt;

            // 碰撞判定
            if (CheckCollisionCircleRec({100, birdY}, 14, { pipes[i].x, 0, 60, pipes[i].topHeight }) || 
                CheckCollisionCircleRec({100, birdY}, 14, { pipes[i].x, pipes[i].topHeight + PIPE_GAP, 60, (float)SCREEN_HEIGHT })) {
                isGameOver = true;
            }

            if (!pipes[i].passed && pipes[i].x < 100) {
                pipes[i].passed = true;
                score++;
                #if defined(PLATFORM_WEB)
                EM_ASM({ if (window.parent && window.parent.UpdateWebScore) window.parent.UpdateWebScore($0); }, score);
                #endif
            }
        }
        if (!pipes.empty() && pipes[0].x < -100) pipes.erase(pipes.begin());

    } else {
        if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) ResetGame();
    }

    BeginDrawing();
        ClearBackground({ 110, 190, 230, 255 });

        // 绘制背景装饰 (云朵)
        DrawCircle(150, 100, 30, {255, 255, 255, 100});
        DrawCircle(180, 110, 40, {255, 255, 255, 100});
        DrawCircle(500, 150, 35, {255, 255, 255, 100});

        // 绘制小鸟
        DrawBird(100, birdY, birdVelocity);
        
        // 绘制管道
        for (const auto& p : pipes) {
            // 主管子
            DrawRectangle((int)p.x, 0, 60, (int)p.topHeight, DARKGREEN);
            DrawRectangle((int)p.x, (int)p.topHeight + PIPE_GAP, 60, SCREEN_HEIGHT, DARKGREEN);
            // 管道口凸起部分
            DrawRectangle((int)p.x - 4, (int)p.topHeight - 24, 68, 24, { 0, 80, 0, 255 });
            DrawRectangle((int)p.x - 4, (int)p.topHeight + PIPE_GAP, 68, 24, { 0, 80, 0, 255 });
        }

        DrawText(TextFormat("%d", score), SCREEN_WIDTH/2 - 15, 50, 60, WHITE);

        if (isGameOver) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f));
            DrawText("CRASHED", 310, 260, 40, RED);
            DrawText("CLICK TO RESTART", 295, 320, 20, WHITE);
        }
    EndDrawing();
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "TinyPulse - Bird");
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) UpdateDrawFrame();
#endif
    CloseWindow();
    return 0;
}