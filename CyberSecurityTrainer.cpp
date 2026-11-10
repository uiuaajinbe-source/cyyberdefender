#include "raylib.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

enum GameState {
    MENU,
    PLAYING,
    INSTRUCTIONS,
    GAMEOVER,
    SHOP,
    LEADERBOARD
};

struct GameEvent {
    string title = "";
    string sender = "";
    string message = "";
    string type = "";
    bool isThreat = false;
    string correctAction = "";
    int moneyChange = 0;
    int difficulty = 1;
};

struct Player {
    int lives = 3;
    int money = 1000;
    int score = 0;
    int level = 1;
    int eventsHandled = 0;
    int correctAnswers = 0;
    int wrongAnswers = 0;
    int combo = 0;
    int maxCombo = 0;

    bool hasAntivirus = false;
    bool hasFirewall = false;
    bool hasSpamFilter = false;
    bool hasTimeExtension = false;
};

struct HighScore {
    string name = "Player";
    int score = 0;
    int level = 1;
};

struct Particle {
    float x, y;
    float vx, vy;
    float life;
    Color color;
};

class CyberDefender {
private:
    const int screenWidth = 1200;
    const int screenHeight = 800;

    GameState currentState = MENU;
    Player player;
    vector<GameEvent> eventPool;
    vector<HighScore> highScores;
    vector<Particle> particles;
    GameEvent currentEvent;

    Font font;

    bool eventActive = false;
    bool showFeedback = false;
    string feedbackText = "";
    Color feedbackColor = WHITE;

    float eventTimeout = 15.0f;
    float timeLeft = 15.0f;
    float pauseTimer = 0;
    float menuAnimation = 0;
    int selectedMenuItem = 0;
    int selectedShopItem = 0;

    Color bgColor = { 20, 20, 30, 255 };
    Color screenBg = { 30, 30, 45, 255 };
    Color panelBg = { 40, 40, 60, 255 };
    Color threatColor = { 220, 50, 50, 255 };
    Color safeColor = { 50, 200, 100, 255 };
    Color textColor = { 240, 240, 240, 255 };
    Color blueColor = { 100, 200, 255, 255 };
    Color yellowColor = { 255, 200, 100, 255 };
    Color darkColor = { 20, 20, 30, 255 };
    Color grayColor = { 100, 100, 100, 255 };
    Color borderColor = { 100, 100, 120, 255 };
    Color titleBarColor = { 60, 60, 80, 255 };
    Color popupBg = { 245, 245, 250, 255 };
    Color overlayColor = { 0, 0, 0, 200 };
    Color goBoxColor = { 30, 30, 40, 255 };
    Color feedbackBg = { 20, 20, 30, 200 };
    Color lightGray = { 200, 200, 200, 255 };
    Color redBtn = { 255, 100, 100, 255 };
    Color greenBtn = { 100, 255, 150, 255 };
    Color yellowBtn = { 255, 200, 100, 255 };
    Color menuBtnColor = { 60, 60, 100, 255 };
    Color menuBtnSelected = { 100, 100, 180, 255 };
    Color comboColor = { 255, 215, 0, 255 };

public:
    CyberDefender() {
        InitWindow(screenWidth, screenHeight, "CyberDefender - Cybersecurity Trainer");
        SetTargetFPS(60);
        srand((unsigned int)time(nullptr));

        // Load font with extended character set
        int fontChars[224];
        int count = 0;
        for (int i = 32; i < 127; i++) fontChars[count++] = i;
        for (int i = 160; i < 256; i++) fontChars[count++] = i;

        font = LoadFontEx("Roboto-Regular.ttf", 40, fontChars, count);
        if (font.texture.id == 0) {
            cout << "WARNING: Using default font" << endl;
            font = GetFontDefault();
        }

        initEvents();
        loadHighScores();
    }

    ~CyberDefender() {
        if (font.texture.id != 0 && font.texture.id != GetFontDefault().texture.id) {
            UnloadFont(font);
        }
        CloseWindow();
    }

    void drawText(const string& text, int x, int y, int size, Color color) {
        DrawTextEx(font, text.c_str(), Vector2{ (float)x, (float)y }, (float)size, 1.0f, color);
    }

    void spawnParticles(int x, int y, Color color, int count = 20) {
        for (int i = 0; i < count; i++) {
            Particle p;
            p.x = (float)x;
            p.y = (float)y;
            p.vx = (float)(rand() % 200 - 100) / 10.0f;
            p.vy = (float)(rand() % 200 - 100) / 10.0f;
            p.life = 1.0f;
            p.color = color;
            particles.push_back(p);
        }
    }

    void updateParticles() {
        for (int i = (int)particles.size() - 1; i >= 0; i--) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].life -= 0.02f;
            particles[i].vy += 0.1f;

            if (particles[i].life <= 0) {
                particles.erase(particles.begin() + i);
            }
        }
    }

    void renderParticles() {
        for (auto& p : particles) {
            Color c = p.color;
            c.a = (unsigned char)(p.life * 255);
            DrawRectangle((int)p.x, (int)p.y, 4, 4, c);
        }
    }

    void initEvents() {
        GameEvent e;

        // Phishing emails
        e = { "URGENT! Account hacked!", "security@bank-verify.ru",
             "Click here to restore access", "phishing", true, "DELETE", -500, 1 };
        eventPool.push_back(e);

        e = { "You won iPhone 17!", "prize@free-gifts.com",
             "Follow link to claim prize", "phishing", true, "DELETE", -300, 1 };
        eventPool.push_back(e);

        e = { "Confirm card details", "noreply@sberbank-online.net",
             "Card will be blocked. Enter CVV", "phishing", true, "DELETE", -1000, 2 };
        eventPool.push_back(e);

        e = { "Password expired", "admin@company-mail.com",
             "Update password immediately", "phishing", true, "DELETE", -400, 1 };
        eventPool.push_back(e);

        e = { "Package delivery failed", "delivery@post-service.net",
             "Pay $5 for redelivery", "phishing", true, "DELETE", -200, 1 };
        eventPool.push_back(e);

        e = { "Netflix subscription ended", "billing@netflix-support.com",
             "Update payment info", "phishing", true, "DELETE", -600, 2 };
        eventPool.push_back(e);

        e = { "IRS Tax Refund", "refund@irs-gov.com",
             "Claim your tax refund now", "phishing", true, "DELETE", -1500, 3 };
        eventPool.push_back(e);

        e = { "Amazon order confirmation", "orders@amazon-verify.com",
             "Confirm your purchase", "phishing", true, "DELETE", -700, 2 };
        eventPool.push_back(e);

        e = { "LinkedIn profile viewed", "notifications@linkedin-alert.com",
             "See who viewed you", "phishing", true, "DELETE", -300, 1 };
        eventPool.push_back(e);

        e = { "Cryptocurrency giveaway", "promo@crypto-free.com",
             "Send 0.1 BTC get 1 BTC!", "phishing", true, "DELETE", -2000, 3 };
        eventPool.push_back(e);

        // Scam calls
        e = { "Incoming call: +7 (999) 123-45-67", "Unknown caller",
             "Bank security. Tell SMS code", "scam_call", true, "HANG UP", -2000, 2 };
        eventPool.push_back(e);

        e = { "Incoming call: Tax Office", "+7 (495) 000-00-00",
             "You have debt. Pay now!", "scam_call", true, "HANG UP", -5000, 3 };
        eventPool.push_back(e);

        e = { "Incoming call: Tech Support", "+1 (800) 555-0199",
             "Virus detected. Give access", "scam_call", true, "HANG UP", -3000, 2 };
        eventPool.push_back(e);

        e = { "Incoming call: Lottery", "+44 (20) 1234-5678",
             "You won! Pay fee to claim", "scam_call", true, "HANG UP", -4000, 3 };
        eventPool.push_back(e);

        e = { "Incoming call: Family", "Unknown (spoofed)",
             "Mom, I need money!", "scam_call", true, "HANG UP", -2500, 2 };
        eventPool.push_back(e);

        // Trojans
        e = { "Downloading: free_game_crack.exe", "Torrent Site",
             "Suspicious file. 45 MB", "trojan", true, "BLOCK", -1000, 2 };
        eventPool.push_back(e);

        e = { "System Update", "windows-update-free.com",
             "Install for speed boost", "trojan", true, "CANCEL", -2000, 2 };
        eventPool.push_back(e);

        e = { "Adobe Flash update", "adobe-flash-update.net",
             "Required for video", "trojan", true, "CANCEL", -1500, 2 };
        eventPool.push_back(e);

        e = { "Antivirus expired", "fake-antivirus.com",
             "Renew or system at risk", "trojan", true, "CANCEL", -2500, 3 };
        eventPool.push_back(e);

        e = { "Browser extension", "search-toolbar.com",
             "Improve browsing", "trojan", true, "CANCEL", -800, 1 };
        eventPool.push_back(e);

        // Legitimate
        e = { "Letter from employer", "hr@company.com",
             "Salary transferred", "legit", false, "OPEN", 5000, 1 };
        eventPool.push_back(e);

        e = { "Bank notification", "notifications@tinkoff.ru",
             "Transaction: 150 rub", "legit", false, "OPEN", 100, 1 };
        eventPool.push_back(e);

        e = { "Windows Update", "Microsoft Update",
             "Security update KB5034441", "legit", false, "INSTALL", 200, 1 };
        eventPool.push_back(e);

        e = { "Steam sale", "noreply@steampowered.com",
             "Summer sale tomorrow", "legit", false, "OPEN", 50, 1 };
        eventPool.push_back(e);

        e = { "GitHub security", "noreply@github.com",
             "New sign-in detected", "legit", false, "REVIEW", 150, 1 };
        eventPool.push_back(e);

        // Suspicious links
        e = { "Messenger message", "Friend (hacked)",
             "Check video! http://bit.ly/3xK9mP", "suspicious_link", true, "IGNORE", -1500, 2 };
        eventPool.push_back(e);

        e = { "SMS: Bank alert", "+79001234567",
             "Card blocked. Verify: http://sber-verify.ru", "suspicious_link", true, "IGNORE", -1200, 2 };
        eventPool.push_back(e);

        e = { "Email: Meeting link", "colleague@work.com",
             "Join: http://zoom-meet.tk/abc", "suspicious_link", true, "VERIFY", -1800, 3 };
        eventPool.push_back(e);
    }

    void loadHighScores() {
        ifstream file("highscores.txt");
        if (file.is_open()) {
            HighScore hs;
            while (file >> hs.name >> hs.score >> hs.level) {
                highScores.push_back(hs);
            }
            file.close();
        }
        sort(highScores.begin(), highScores.end(),
            [](const HighScore& a, const HighScore& b) { return a.score > b.score; });
    }

    void saveHighScore() {
        highScores.push_back({ "Player", player.score, player.level });
        sort(highScores.begin(), highScores.end(),
            [](const HighScore& a, const HighScore& b) { return a.score > b.score; });
        if (highScores.size() > 10) highScores.resize(10);

        ofstream file("highscores.txt");
        if (file.is_open()) {
            for (auto& hs : highScores) {
                file << hs.name << " " << hs.score << " " << hs.level << "\n";
            }
            file.close();
        }
    }

    void spawnEvent() {
        vector<GameEvent> available;
        for (auto& e : eventPool) {
            if (e.difficulty <= player.level || rand() % 3 == 0) {
                available.push_back(e);
            }
        }

        if (available.empty()) available = eventPool;

        int idx = rand() % (int)available.size();
        currentEvent = available[idx];
        eventActive = true;
        showFeedback = false;

        timeLeft = max(5.0f, eventTimeout - (player.level - 1) * 1.5f);
        if (player.hasTimeExtension) timeLeft += 5.0f;
        pauseTimer = 0;
    }

    void handleInput() {
        switch (currentState) {
        case MENU: handleMenuInput(); break;
        case PLAYING: handleGameInput(); break;
        case INSTRUCTIONS: handleInstructionsInput(); break;
        case GAMEOVER: handleGameOverInput(); break;
        case SHOP: handleShopInput(); break;
        case LEADERBOARD: handleLeaderboardInput(); break;
        }
    }

    void handleMenuInput() {
        if (IsKeyPressed(KEY_UP)) {
            selectedMenuItem--;
            if (selectedMenuItem < 0) selectedMenuItem = 4;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            selectedMenuItem++;
            if (selectedMenuItem > 4) selectedMenuItem = 0;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (selectedMenuItem == 0) {
                player = Player();
                currentState = PLAYING;
                spawnEvent();
            }
            else if (selectedMenuItem == 1) {
                currentState = INSTRUCTIONS;
            }
            else if (selectedMenuItem == 2) {
                currentState = LEADERBOARD;
            }
            else if (selectedMenuItem == 3) {
                currentState = SHOP;
            }
            else if (selectedMenuItem == 4) {
                CloseWindow();
            }
        }
    }

    void handleGameInput() {
        if (IsKeyPressed(KEY_ESCAPE)) {
            currentState = MENU;
            return;
        }

        if (!eventActive || showFeedback) return;

        if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_B)) {
            processDecision(true);
        }
        else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_O)) {
            processDecision(false);
        }
        else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_H)) {
            processDecision(true);
        }
    }

    void handleInstructionsInput() {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            currentState = MENU;
        }
    }

    void handleGameOverInput() {
        if (IsKeyPressed(KEY_R)) {
            player = Player();
            currentState = PLAYING;
            spawnEvent();
        }
        else if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) {
            saveHighScore();
            currentState = MENU;
        }
    }

    void handleShopInput() {
        if (IsKeyPressed(KEY_UP)) {
            selectedShopItem--;
            if (selectedShopItem < 0) selectedShopItem = 3;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            selectedShopItem++;
            if (selectedShopItem > 3) selectedShopItem = 0;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            buyItem(selectedShopItem);
        }

        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_B)) {
            currentState = MENU;
        }
    }

    void handleLeaderboardInput() {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            currentState = MENU;
        }
    }

    void buyItem(int item) {
        int cost = 0;

        switch (item) {
        case 0:
            cost = 500;
            if (player.money >= cost && !player.hasAntivirus) {
                player.money -= cost;
                player.hasAntivirus = true;
            }
            break;
        case 1:
            cost = 800;
            if (player.money >= cost && !player.hasFirewall) {
                player.money -= cost;
                player.hasFirewall = true;
            }
            break;
        case 2:
            cost = 600;
            if (player.money >= cost && !player.hasSpamFilter) {
                player.money -= cost;
                player.hasSpamFilter = true;
            }
            break;
        case 3:
            cost = 400;
            if (player.money >= cost && !player.hasTimeExtension) {
                player.money -= cost;
                player.hasTimeExtension = true;
            }
            break;
        }
    }

    void processDecision(bool blocked) {
        showFeedback = true;
        bool correct = (blocked == currentEvent.isThreat);

        if (!correct && player.hasAntivirus && currentEvent.type == "trojan") {
            correct = true;
            feedbackText = "ANTIVIRUS SAVED YOU! " + currentEvent.correctAction;
        }
        if (!correct && player.hasSpamFilter && currentEvent.type == "phishing") {
            correct = true;
            feedbackText = "SPAM FILTER CAUGHT IT! " + currentEvent.correctAction;
        }

        if (correct) {
            player.combo++;
            if (player.combo > player.maxCombo) player.maxCombo = player.combo;

            int comboBonus = player.combo * 10;
            player.score += 100 + comboBonus;
            player.money += currentEvent.moneyChange;
            if (player.money < 0) player.money = 0;

            if (player.combo > 1) {
                feedbackText = "CORRECT! COMBO x" + to_string(player.combo) + " +" + to_string(comboBonus);
            }
            else {
                feedbackText = "CORRECT! " + currentEvent.correctAction;
            }
            feedbackColor = safeColor;
            player.correctAnswers++;
            player.eventsHandled++;

            spawnParticles(600, 400, safeColor, 30);

            if (player.eventsHandled % 5 == 0) {
                player.level++;
                currentState = SHOP;
            }
        }
        else {
            player.combo = 0;
            int livesLost = 1;
            if (player.hasFirewall) livesLost = 0;

            player.lives -= livesLost;
            player.money += currentEvent.moneyChange;
            if (player.money < 0) player.money = 0;
            feedbackText = "WRONG! " + currentEvent.correctAction;
            feedbackColor = threatColor;
            player.wrongAnswers++;

            spawnParticles(600, 400, threatColor, 20);
        }

        if (player.lives <= 0) {
            currentState = GAMEOVER;
            saveHighScore();
        }
        else {
            pauseTimer = 0;
        }
    }

    void update() {
        menuAnimation += GetFrameTime() * 2.0f;
        updateParticles();

        if (currentState != PLAYING) return;

        float dt = GetFrameTime();

        if (showFeedback) {
            pauseTimer += dt;
            if (pauseTimer >= 2.0f) {
                spawnEvent();
            }
            return;
        }

        if (!eventActive) return;

        timeLeft -= dt;
        if (timeLeft <= 0) {
            player.lives--;
            player.money -= 500;
            if (player.money < 0) player.money = 0;
            player.combo = 0;
            feedbackText = "TIME'S UP!";
            feedbackColor = threatColor;
            showFeedback = true;
            pauseTimer = 0;
            player.wrongAnswers++;

            if (player.lives <= 0) {
                currentState = GAMEOVER;
                saveHighScore();
            }
        }
    }

    void render() {
        BeginDrawing();
        ClearBackground(bgColor);

        switch (currentState) {
        case MENU: renderMenu(); break;
        case PLAYING: renderGame(); break;
        case INSTRUCTIONS: renderInstructions(); break;
        case GAMEOVER: renderGameOver(); break;
        case SHOP: renderShop(); break;
        case LEADERBOARD: renderLeaderboard(); break;
        }

        renderParticles();
        EndDrawing();
    }

    void renderMenu() {
        for (int i = 0; i < 50; i++) {
            float x = (float)(i * 30 + (int)(menuAnimation * 10) % screenWidth);
            float y = (float)((i * 50) % screenHeight);
            float alpha = 20 + (float)(i % 5) * 5;
            DrawRectangle((int)x, (int)y, 2, 2, Color{ 100, 100, 255, (unsigned char)alpha });
        }

        drawText("CYBERDEFENDER", screenWidth / 2 - 280, 100, 60, blueColor);
        drawText("Cybersecurity Trainer", screenWidth / 2 - 180, 170, 30, lightGray);
        DrawRectangle(screenWidth / 2 - 300, 210, 600, 2, blueColor);

        string menuItems[] = { "START GAME", "HOW TO PLAY", "LEADERBOARD", "UPGRADE SHOP", "EXIT" };
        int buttonY = 250;

        for (int i = 0; i < 5; i++) {
            Color btnColor = (i == selectedMenuItem) ? menuBtnSelected : menuBtnColor;
            Color txtColor = (i == selectedMenuItem) ? WHITE : lightGray;

            DrawRectangle(screenWidth / 2 - 200, buttonY + i * 70, 400, 55, btnColor);
            DrawRectangleLines(screenWidth / 2 - 200, buttonY + i * 70, 400, 55, blueColor);

            int textWidth = MeasureTextEx(font, menuItems[i].c_str(), 22, 1.0f).x;
            drawText(menuItems[i], screenWidth / 2 - textWidth / 2, buttonY + i * 70 + 13, 22, txtColor);
        }

        drawText("UP/DOWN to navigate, ENTER to select", screenWidth / 2 - 200, 640, 16, grayColor);
        drawText("ESC during game for menu", screenWidth / 2 - 140, 670, 16, grayColor);
        drawText("v2.0 - Enhanced Edition", screenWidth - 200, screenHeight - 30, 14, grayColor);
    }

    void renderShop() {
        DrawRectangle(0, 0, screenWidth, screenHeight, bgColor);

        drawText("UPGRADE SHOP", screenWidth / 2 - 150, 50, 40, yellowColor);
        DrawRectangle(screenWidth / 2 - 300, 100, 600, 2, yellowColor);
        drawText("Balance: " + to_string(player.money) + " rub", screenWidth / 2 - 120, 120, 24, safeColor);

        string items[] = { "ANTIVIRUS - Blocks trojans", "FIREWALL - Saves 1 life",
                         "SPAM FILTER - Catches phishing", "TIME EXTENSION +5 sec" };
        int costs[] = { 500, 800, 600, 400 };
        bool owned[] = { player.hasAntivirus, player.hasFirewall, player.hasSpamFilter, player.hasTimeExtension };

        int y = 180;
        for (int i = 0; i < 4; i++) {
            Color btnColor = menuBtnColor;
            if (i == selectedShopItem) btnColor = menuBtnSelected;
            if (owned[i]) btnColor = Color{ 50, 50, 50, 255 };

            DrawRectangle(200, y + i * 80, 800, 65, btnColor);
            DrawRectangleLines(200, y + i * 80, 800, 65, blueColor);

            string text = items[i] + " - " + to_string(costs[i]) + " rub";
            if (owned[i]) text = items[i] + " - OWNED";
            drawText(text, 220, y + i * 80 + 20, 20, owned[i] ? grayColor : WHITE);
        }

        drawText("UP/DOWN to select, ENTER to buy", screenWidth / 2 - 180, 550, 18, grayColor);
        drawText("Press B or ESC to continue", screenWidth / 2 - 160, 580, 18, grayColor);
    }

    void renderLeaderboard() {
        DrawRectangle(0, 0, screenWidth, screenHeight, bgColor);

        drawText("LEADERBOARD", screenWidth / 2 - 150, 50, 40, blueColor);
        DrawRectangle(screenWidth / 2 - 300, 100, 600, 2, blueColor);

        if (highScores.empty()) {
            drawText("No scores yet. Play a game!", screenWidth / 2 - 150, 300, 24, grayColor);
        }
        else {
            drawText("Rank  Name              Score   Level", 200, 150, 20, yellowColor);
            DrawRectangle(200, 180, 800, 2, yellowColor);

            for (int i = 0; i < (int)highScores.size() && i < 10; i++) {
                string rank = "#" + to_string(i + 1);
                string entry = rank + "    " + highScores[i].name +
                    string(20 - highScores[i].name.length(), ' ') +
                    to_string(highScores[i].score) +
                    string(10, ' ') + to_string(highScores[i].level);
                drawText(entry, 200, 200 + i * 40, 20, textColor);
            }
        }

        drawText("Press ESC or ENTER to return", screenWidth / 2 - 160, 700, 18, grayColor);
    }

    void renderInstructions() {
        DrawRectangle(0, 0, screenWidth, screenHeight, bgColor);

        drawText("HOW TO PLAY", screenWidth / 2 - 150, 50, 40, blueColor);
        DrawRectangle(screenWidth / 2 - 300, 100, 600, 2, blueColor);

        int y = 130;
        int lh = 32;

        drawText("OBJECTIVE:", 100, y, 22, yellowColor); y += lh;
        drawText("Protect computer from cyber threats!", 120, y, 18, textColor); y += lh;

        drawText("CONTROLS:", 100, y, 22, yellowColor); y += lh;
        drawText("[1/B] Block  [2/O] Open  [3/H] Hang up  [ESC] Menu", 120, y, 18, textColor); y += lh;

        drawText("THREAT TYPES:", 100, y, 22, yellowColor); y += lh;
        drawText("[PHISHING] Fake emails  [SCAM] Fraudsters  [TROJAN] Malware", 120, y, 18, threatColor); y += lh;
        drawText("[LINK] Dangerous URLs  [LEGIT] Safe messages", 120, y, 18, safeColor); y += lh;

        drawText("COMBO SYSTEM:", 100, y, 22, yellowColor); y += lh;
        drawText("Consecutive correct = bonus points!", 120, y, 18, comboColor); y += lh;

        drawText("UPGRADES:", 100, y, 22, yellowColor); y += lh;
        drawText("Buy protection in shop between levels", 120, y, 18, textColor); y += lh;

        drawText("TIPS:", 100, y, 22, yellowColor); y += lh;
        drawText("- Check sender email  - Never share passwords  - When in doubt, block!", 120, y, 18, textColor);

        DrawRectangle(screenWidth / 2 - 150, 720, 300, 50, menuBtnColor);
        DrawRectangleLines(screenWidth / 2 - 150, 720, 300, 50, blueColor);
        drawText("BACK TO MENU", screenWidth / 2 - 110, 732, 22, WHITE);
    }

    void renderGame() {
        DrawRectangle(0, 0, screenWidth, 80, panelBg);

        drawText("LIVES: " + to_string(player.lives), 30, 25, 22, threatColor);
        drawText("BALANCE: " + to_string(player.money), 250, 25, 22, safeColor);
        drawText("SCORE: " + to_string(player.score), 500, 25, 22, blueColor);
        drawText("LEVEL: " + to_string(player.level), 720, 25, 22, yellowColor);

        if (player.combo > 1) {
            drawText("COMBO x" + to_string(player.combo), 900, 25, 22, comboColor);
        }

        int upgX = 30;
        if (player.hasAntivirus) { drawText("[AV]", upgX, 55, 14, safeColor); upgX += 50; }
        if (player.hasFirewall) { drawText("[FW]", upgX, 55, 14, safeColor); upgX += 50; }
        if (player.hasSpamFilter) { drawText("[SF]", upgX, 55, 14, safeColor); upgX += 50; }
        if (player.hasTimeExtension) { drawText("[TE]", upgX, 55, 14, safeColor); }

        DrawRectangle(150, 110, 900, 550, screenBg);
        DrawRectangleLines(150, 110, 900, 550, borderColor);
        DrawRectangle(150, 110, 900, 35, titleBarColor);
        drawText("Desktop - CyberOS", 170, 118, 16, WHITE);

        if (eventActive && !showFeedback) {
            Color border = currentEvent.isThreat ? threatColor : safeColor;
            DrawRectangle(190, 170, 820, 420, popupBg);
            DrawRectangleLinesEx(Rectangle{ 190, 170, 820, 420 }, 4, border);

            drawText(currentEvent.title, 210, 190, 22, darkColor);
            drawText("From: " + currentEvent.sender, 210, 230, 16, grayColor);
            drawText(currentEvent.message, 210, 270, 18, Color{ 30, 30, 50, 255 });

            string typeLabel = "";
            if (currentEvent.type == "phishing") typeLabel = "[PHISHING]";
            else if (currentEvent.type == "scam_call") typeLabel = "[SCAM CALL]";
            else if (currentEvent.type == "trojan") typeLabel = "[TROJAN]";
            else if (currentEvent.type == "suspicious_link") typeLabel = "[SUSPICIOUS LINK]";
            else typeLabel = "[LEGITIMATE]";

            drawText(typeLabel, 210, 320, 14, currentEvent.isThreat ? threatColor : safeColor);
            drawText("Difficulty: " + to_string(currentEvent.difficulty), 210, 345, 12, grayColor);

            drawText("Time: " + to_string((int)timeLeft) + " sec", 210, 540, 16,
                timeLeft < 5 ? threatColor : textColor);

            float barWidth = 820 * (timeLeft / eventTimeout);
            DrawRectangle(190, 575, barWidth, 8, timeLeft < 5 ? threatColor : safeColor);
        }

        if (showFeedback) {
            DrawRectangle(190, 400, 820, 100, feedbackBg);
            drawText(feedbackText, 210, 430, 24, feedbackColor);
        }

        DrawRectangle(0, 680, screenWidth, 120, panelBg);
        drawText("ACTIONS:", 50, 700, 20, lightGray);
        drawText("[1/B] BLOCK/DELETE", 50, 735, 18, redBtn);
        drawText("[2/O] OPEN/ALLOW", 450, 735, 18, greenBtn);
        drawText("[3/H] HANG UP CALL", 850, 735, 18, yellowBtn);
        drawText("ESC for menu", screenWidth - 150, 700, 14, grayColor);
    }

    void renderGameOver() {
        DrawRectangle(0, 0, screenWidth, screenHeight, overlayColor);
        DrawRectangle(250, 150, 700, 500, goBoxColor);
        DrawRectangleLinesEx(Rectangle{ 250, 150, 700, 500 }, 4, threatColor);

        drawText("SYSTEM HACKED!", 370, 180, 36, threatColor);
        DrawRectangle(300, 230, 600, 2, threatColor);

        int y = 260;
        int lh = 40;
        drawText("Final Score: " + to_string(player.score), 320, y, 24, textColor); y += lh;
        drawText("Balance: " + to_string(player.money) + " rub", 320, y, 24, textColor); y += lh;
        drawText("Level Reached: " + to_string(player.level), 320, y, 24, textColor); y += lh;
        drawText("Events Handled: " + to_string(player.eventsHandled), 320, y, 24, textColor); y += lh;
        drawText("Correct Answers: " + to_string(player.correctAnswers), 320, y, 24, safeColor); y += lh;
        drawText("Wrong Answers: " + to_string(player.wrongAnswers), 320, y, 24, threatColor); y += lh;

        int accuracy = player.eventsHandled > 0 ?
            (player.correctAnswers * 100) / player.eventsHandled : 0;
        drawText("Accuracy: " + to_string(accuracy) + "%", 320, y, 24,
            accuracy >= 70 ? safeColor : threatColor); y += lh;
        drawText("Max Combo: x" + to_string(player.maxCombo), 320, y, 24, comboColor); y += lh;

        DrawRectangle(300, y + 20, 600, 2, blueColor);
        drawText("Press [R] to restart", 400, y + 40, 20, safeColor);
        drawText("Press [M] or ESC for menu", 380, y + 70, 18, grayColor);
    }

    void run() {
        while (!WindowShouldClose()) {
            handleInput();
            update();
            render();
        }
    }
};

int main() {
    CyberDefender game;
    game.run();
    return 0;
}