#include <GL/glut.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

// Game States
enum GameState { MENU, PLAYING, PAUSED, GAME_OVER };
GameState currentState = MENU;

// Window dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Game variables
int score = 0;
int highScore = 0;
float gameTime = 60.0f; // 60 seconds
float remainingTime = gameTime;
int lastTime = 0;

// Basket variables
float basketX = WINDOW_WIDTH / 2;
float basketY = 50;
float basketWidth = 80;
float basketHeight = 20;
float basketSpeed = 8.0f;

// Chicken variables
struct Chicken {
    float x, y;
    float speed;
    int direction;
};

const int NUM_CHICKENS = 1;
Chicken chickens[NUM_CHICKENS];

float eggSpawnTimer = 0;
float eggSpawnInterval = 1.0f;

// Powerup timers
float largeBasketTimer = 0;
float slowMotionTimer = 0;

// Egg/Item structure
struct FallingItem {
    float x, y;
    int type; // 0=normal(white), 1=blue, 2=golden, 3=poop, 4=large basket, 5=slow motion, 6=extra time
    float speed;
    bool active;
};

std::vector<FallingItem> items;


// Forward declarations
void drawText(float x, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18);
void resetGame();

// Draw text function
void drawText(float x, float y, const char* text, void* font) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(font, *text);
        text++;
    }
}

// Draw filled circle
void drawCircle(float cx, float cy, float r, int segments = 30) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(segments);
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        glVertex2f(x + cx, y + cy);
    }
    glEnd();
}

// Draw basket
void drawBasket() {
    glColor3f(0.6f, 0.4f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(basketX - basketWidth/2, basketY);
    glVertex2f(basketX + basketWidth/2, basketY);
    glVertex2f(basketX + basketWidth/2 - 10, basketY + basketHeight);
    glVertex2f(basketX - basketWidth/2 + 10, basketY + basketHeight);
    glEnd();

    // Basket rim
    glColor3f(0.4f, 0.2f, 0.1f);
    glLineWidth(3);
    glBegin(GL_LINE_LOOP);
    glVertex2f(basketX - basketWidth/2, basketY);
    glVertex2f(basketX + basketWidth/2, basketY);
    glVertex2f(basketX + basketWidth/2 - 10, basketY + basketHeight);
    glVertex2f(basketX - basketWidth/2 + 10, basketY + basketHeight);
    glEnd();
}

// Draw chicken
void drawChicken(float x, float y) {
    // Body
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(x, y, 15);

    // Head
    glColor3f(1.0f, 0.9f, 0.8f);
    drawCircle(x + 10, y + 10, 10);

    // Beak
    glColor3f(1.0f, 0.8f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x + 18, y + 10);
    glVertex2f(x + 25, y + 8);
    glVertex2f(x + 18, y + 6);
    glEnd();

    // Eye
    glColor3f(0.0f, 0.0f, 0.0f);
    drawCircle(x + 13, y + 12, 2);

    // Comb
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x + 8, y + 18);
    glVertex2f(x + 12, y + 25);
    glVertex2f(x + 16, y + 18);
    glEnd();
}

// Draw bamboo stick
void drawBambooStick() {
    float chickenY = WINDOW_HEIGHT - 80;

    glColor3f(0.4f, 0.6f, 0.3f);
    glLineWidth(8);
    glBegin(GL_LINES);
    glVertex2f(50, chickenY);
    glVertex2f(WINDOW_WIDTH - 50, chickenY);
    glEnd();

    // Bamboo segments
    glColor3f(0.3f, 0.5f, 0.2f);
    for (int i = 100; i < WINDOW_WIDTH - 50; i += 50) {
        glLineWidth(2);
        glBegin(GL_LINES);
        glVertex2f(i, chickenY - 5);
        glVertex2f(i, chickenY + 5);
        glEnd();
    }
}

// Draw egg/item
void drawItem(FallingItem& item) {
    if (item.type == 0) { // Normal egg (white)
        glColor3f(1.0f, 1.0f, 1.0f);
        drawCircle(item.x, item.y, 12);
        drawCircle(item.x, item.y + 8, 10);
    } else if (item.type == 1) { // Blue egg
        glColor3f(0.3f, 0.5f, 1.0f);
        drawCircle(item.x, item.y, 12);
        drawCircle(item.x, item.y + 8, 10);
    } else if (item.type == 2) { // Golden egg
        glColor3f(1.0f, 0.84f, 0.0f);
        drawCircle(item.x, item.y, 12);
        drawCircle(item.x, item.y + 8, 10);
        glColor3f(1.0f, 0.95f, 0.3f);
        drawCircle(item.x, item.y, 8);
    } else if (item.type == 3) { // Poop
        glColor3f(0.4f, 0.3f, 0.1f);
        drawCircle(item.x, item.y, 10);
        drawCircle(item.x - 5, item.y + 6, 8);
        drawCircle(item.x + 5, item.y + 6, 8);
    } else if (item.type == 4) { // Large basket powerup
        glColor3f(0.0f, 1.0f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(item.x - 12, item.y - 12);
        glVertex2f(item.x + 12, item.y - 12);
        glVertex2f(item.x + 12, item.y + 12);
        glVertex2f(item.x - 12, item.y + 12);
        glEnd();
        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(2);
        glBegin(GL_LINES);
        glVertex2f(item.x - 8, item.y);
        glVertex2f(item.x + 8, item.y);
        glVertex2f(item.x, item.y - 8);
        glVertex2f(item.x, item.y + 8);
        glEnd();
    } else if (item.type == 5) { // Slow motion powerup
        glColor3f(0.5f, 0.0f, 1.0f);
        glBegin(GL_QUADS);
        glVertex2f(item.x - 12, item.y - 12);
        glVertex2f(item.x + 12, item.y - 12);
        glVertex2f(item.x + 12, item.y + 12);
        glVertex2f(item.x - 12, item.y + 12);
        glEnd();
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(item.x - 8, item.y - 5, "S", GLUT_BITMAP_HELVETICA_18);
    } else if (item.type == 6) { // Extra time powerup
        glColor3f(1.0f, 0.5f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(item.x - 12, item.y - 12);
        glVertex2f(item.x + 12, item.y - 12);
        glVertex2f(item.x + 12, item.y + 12);
        glVertex2f(item.x - 12, item.y + 12);
        glEnd();
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(item.x - 8, item.y - 5, "T", GLUT_BITMAP_HELVETICA_18);
    }
}

// Draw menu
void drawMenu() {
    glColor3f(0.0f, 0.5f, 0.8f);
    drawText(WINDOW_WIDTH/2 - 120, WINDOW_HEIGHT - 100, "CATCH THE EGGS", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 + 50, "Press 'S' to Start Game", GLUT_BITMAP_HELVETICA_18);
    drawText(WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2, "Press 'H' to View High Score", GLUT_BITMAP_HELVETICA_18);
    drawText(WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 - 50, "Press 'ESC' to Exit", GLUT_BITMAP_HELVETICA_18);

    glColor3f(0.8f, 0.8f, 0.0f);
    drawText(50, 100, "Controls:", GLUT_BITMAP_HELVETICA_18);
    drawText(50, 70, "Arrow Keys or Mouse - Move Basket", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 50, "P - Pause/Resume Game", GLUT_BITMAP_HELVETICA_12);

    char scoreText[50];
    sprintf(scoreText, "High Score: %d", highScore);
    glColor3f(1.0f, 0.84f, 0.0f);
    drawText(WINDOW_WIDTH - 200, WINDOW_HEIGHT - 30, scoreText, GLUT_BITMAP_HELVETICA_18);
}

// Draw pause screen
void drawPauseScreen() {
    glColor3f(1.0f, 1.0f, 0.0f);
    drawText(WINDOW_WIDTH/2 - 50, WINDOW_HEIGHT/2, "PAUSED", GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 - 50, "Press 'P' to Resume", GLUT_BITMAP_HELVETICA_18);
    drawText(WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 - 80, "Press 'M' for Menu", GLUT_BITMAP_HELVETICA_18);
}

// Draw game over screen
void drawGameOver() {
    glColor3f(1.0f, 0.0f, 0.0f);
    drawText(WINDOW_WIDTH/2 - 80, WINDOW_HEIGHT/2 + 50, "GAME OVER!", GLUT_BITMAP_TIMES_ROMAN_24);

    char scoreText[50];
    sprintf(scoreText, "Final Score: %d", score);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(WINDOW_WIDTH/2 - 80, WINDOW_HEIGHT/2, scoreText, GLUT_BITMAP_HELVETICA_18);

    if (score > highScore) {
        glColor3f(1.0f, 0.84f, 0.0f);
        drawText(WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 - 40, "NEW HIGH SCORE!", GLUT_BITMAP_HELVETICA_18);
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(WINDOW_WIDTH/2 - 120, WINDOW_HEIGHT/2 - 100, "Press 'R' to Play Again", GLUT_BITMAP_HELVETICA_18);
    drawText(WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 - 130, "Press 'M' for Menu", GLUT_BITMAP_HELVETICA_18);
}

// Draw HUD
void drawHUD() {
    char text[50];

    // Score
    glColor3f(1.0f, 1.0f, 1.0f);
    sprintf(text, "Score: %d", score);
    drawText(10, WINDOW_HEIGHT - 30, text, GLUT_BITMAP_HELVETICA_18);

    // Time
    sprintf(text, "Time: %.1f", remainingTime);
    drawText(WINDOW_WIDTH - 120, WINDOW_HEIGHT - 30, text, GLUT_BITMAP_HELVETICA_18);

    // Active powerups
    if (largeBasketTimer > 0) {
        glColor3f(0.0f, 1.0f, 0.0f);
        sprintf(text, "Large Basket: %.1fs", largeBasketTimer);
        drawText(10, WINDOW_HEIGHT - 60, text, GLUT_BITMAP_HELVETICA_12);
    }

    if (slowMotionTimer > 0) {
        glColor3f(0.5f, 0.0f, 1.0f);
        sprintf(text, "Slow Motion: %.1fs", slowMotionTimer);
        drawText(10, WINDOW_HEIGHT - 85, text, GLUT_BITMAP_HELVETICA_12);
    }
}

// Spawn item
void spawnItem() {
    // Random chicken থেকে egg spawn হবে
    int randomChicken = rand() % NUM_CHICKENS;

    FallingItem item;
    item.x = chickens[randomChicken].x;
    item.y = chickens[randomChicken].y - 20;
    item.active = true;

    int random = rand() % 100;
    if (random < 5) { // 5% chance
        item.type = 2; // Golden egg
    } else if (random < 15) { // 10% chance
        item.type = 1; // Blue egg
    } else if (random < 25) { // 10% chance
        item.type = 3; // Poop
    } else if (random < 30) { // 5% chance
        item.type = 4; // Large basket
    } else if (random < 35) { // 5% chance
        item.type = 5; // Slow motion
    } else if (random < 40) { // 5% chance
        item.type = 6; // Extra time
    } else {
        item.type = 0; // Normal egg (60%)
    }

    float speedMultiplier = (slowMotionTimer > 0) ? 0.5f : 1.0f;
    item.speed = (2.0f + (rand() % 100) / 100.0f) * speedMultiplier;

    items.push_back(item);
}

// Check collision
bool checkCollision(FallingItem& item) {
    float itemBottom = item.y - 12;
    float basketTop = basketY + basketHeight;
    float basketLeft = basketX - basketWidth/2;
    float basketRight = basketX + basketWidth/2;

    return (itemBottom <= basketTop && itemBottom >= basketY &&
            item.x >= basketLeft && item.x <= basketRight);
}

// Update game
void update(int value) {
    if (currentState == PLAYING) {
        // Calculate delta time
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Update time
        remainingTime -= deltaTime;
        if (remainingTime <= 0) {
            remainingTime = 0;
            currentState = GAME_OVER;
            if (score > highScore) {
                highScore = score;
            }
        }

        // Update all chickens position
        for (int i = 0; i < NUM_CHICKENS; i++) {
            chickens[i].x += chickens[i].speed * chickens[i].direction;
            if (chickens[i].x <= 70 || chickens[i].x >= WINDOW_WIDTH - 70) {
                chickens[i].direction *= -1;
            }
        }

        // Spawn eggs/items
        eggSpawnTimer += deltaTime;
        if (eggSpawnTimer >= eggSpawnInterval) {
            spawnItem();
            eggSpawnTimer = 0;
        }

        // Update items
        float speedMultiplier = (slowMotionTimer > 0) ? 0.5f : 1.0f;
        for (auto it = items.begin(); it != items.end();) {
            it->y -= it->speed * speedMultiplier;

            // Check collision
            if (checkCollision(*it)) {
                if (it->type == 0) {
                    score += 1;
                } else if (it->type == 1) {
                    score += 5;
                } else if (it->type == 2) {
                    score += 10;
                } else if (it->type == 3) {
                    score -= 10;
                    if (score < 0) score = 0;
                } else if (it->type == 4) {
                    largeBasketTimer = 5.0f;
                    basketWidth = 120;
                } else if (it->type == 5) {
                    slowMotionTimer = 5.0f;
                } else if (it->type == 6) {
                    remainingTime += 10.0f;
                }
                it = items.erase(it);
            } else if (it->y < -20) {
                it = items.erase(it);
            } else {
                ++it;
            }
        }

        // Update powerup timers
        if (largeBasketTimer > 0) {
            largeBasketTimer -= deltaTime;
            if (largeBasketTimer <= 0) {
                basketWidth = 80;
            }
        }

        if (slowMotionTimer > 0) {
            slowMotionTimer -= deltaTime;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // ~60 FPS
}

// Display callback
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (currentState == MENU) {
        drawMenu();
    } else if (currentState == PLAYING || currentState == PAUSED) {
        // Draw game elements
        drawBambooStick();

        // Draw all 3 chickens
        for (int i = 0; i < NUM_CHICKENS; i++) {
            drawChicken(chickens[i].x, chickens[i].y);
        }

        drawBasket();

        // Draw items
        for (auto& item : items) {
            drawItem(item);
        }

        drawHUD();

        if (currentState == PAUSED) {
            // Semi-transparent overlay
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
            glBegin(GL_QUADS);
            glVertex2f(0, 0);
            glVertex2f(WINDOW_WIDTH, 0);
            glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
            glVertex2f(0, WINDOW_HEIGHT);
            glEnd();
            glDisable(GL_BLEND);

            drawPauseScreen();
        }
    } else if (currentState == GAME_OVER) {
        drawGameOver();
    }

    glutSwapBuffers();
}

// Keyboard callback
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 's':
        case 'S':
            if (currentState == MENU) {
                resetGame();
                currentState = PLAYING;
                lastTime = glutGet(GLUT_ELAPSED_TIME);
            }
            break;
        case 'p':
        case 'P':
            if (currentState == PLAYING) {
                currentState = PAUSED;
            } else if (currentState == PAUSED) {
                currentState = PLAYING;
                lastTime = glutGet(GLUT_ELAPSED_TIME);
            }
            break;
        case 'r':
        case 'R':
            if (currentState == GAME_OVER) {
                resetGame();
                currentState = PLAYING;
                lastTime = glutGet(GLUT_ELAPSED_TIME);
            }
            break;
        case 'm':
        case 'M':
            if (currentState == PAUSED || currentState == GAME_OVER) {
                currentState = MENU;
            }
            break;
        case 27: // ESC
            exit(0);
            break;
    }
}

// Special key callback (arrow keys)
void specialKeys(int key, int x, int y) {
    if (currentState == PLAYING) {
        switch (key) {
            case GLUT_KEY_LEFT:
                basketX -= basketSpeed;
                if (basketX - basketWidth/2 < 0) {
                    basketX = basketWidth/2;
                }
                break;
            case GLUT_KEY_RIGHT:
                basketX += basketSpeed;
                if (basketX + basketWidth/2 > WINDOW_WIDTH) {
                    basketX = WINDOW_WIDTH - basketWidth/2;
                }
                break;
        }
    }
}

// Mouse motion callback
void mouseMotion(int x, int y) {
    if (currentState == PLAYING) {
        basketX = x;
        if (basketX - basketWidth/2 < 0) {
            basketX = basketWidth/2;
        }
        if (basketX + basketWidth/2 > WINDOW_WIDTH) {
            basketX = WINDOW_WIDTH - basketWidth/2;
        }
    }
}

// Reset game
void resetGame() {
    score = 0;
    remainingTime = gameTime;
    items.clear();
    basketX = WINDOW_WIDTH / 2;
    basketWidth = 80;
    eggSpawnTimer = 0;
    largeBasketTimer = 0;
    slowMotionTimer = 0;

    // Initialize 3 chickens at different positions
    for (int i = 0; i < NUM_CHICKENS; i++) {
        chickens[i].x = (WINDOW_WIDTH / 4) * (i + 1); // Space them evenly
        chickens[i].y = WINDOW_HEIGHT - 80;
        chickens[i].speed = 3.0f + (rand() % 20) / 10.0f; // Random speed 3-5
        chickens[i].direction = (i % 2 == 0) ? 1 : -1; // Alternate directions
    }
}

// Initialize OpenGL
void init() {
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // Sky blue
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);

    srand(time(NULL));

    // Initialize chickens for the first time
    for (int i = 0; i < NUM_CHICKENS; i++) {
        chickens[i].x = (WINDOW_WIDTH / 4) * (i + 1);
        chickens[i].y = WINDOW_HEIGHT - 80;
        chickens[i].speed = 3.0f + (rand() % 20) / 10.0f;
        chickens[i].direction = (i % 2 == 0) ? 1 : -1;
    }
}

// Main function
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Catch The Eggs - 3 Chickens");

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutPassiveMotionFunc(mouseMotion);
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}
