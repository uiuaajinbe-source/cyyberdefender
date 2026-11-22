#include "raylib.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

enum GameState { MENU, PLAYING, INSTRUCTIONS, GAMEOVER, SHOP, LEADERBOARD };
enum Theme { THEME_HOME, THEME_OFFICE, THEME_MOBILE };

struct GameEvent {
    string title, sender, message, url, type, correctAction;
    bool isThreat;
    int moneyChange, difficulty;
};

struct Player {
    int lives = 3, money = 1000, score = 0, level = 1;
    int eventsHandled = 0, correctAnswers = 0, wrongAnswers = 0;
    int combo = 0, maxCombo = 0;
    bool hasAntivirus = false, hasFirewall = false, hasSpamFilter = false, hasTimeExtension = false;
};

struct HighScore { string name = "Player"; int score = 0; int level = 1; };
struct Particle { float x, y, vx, vy, life; Color color; };

class CyberDefender {
private:
    const int screenWidth = 1200, screenHeight = 800;
    GameState currentState = MENU;
    Theme currentTheme = THEME_HOME;
    Player player;
    vector<GameEvent> eventPool;
    vector<HighScore> highScores;
    vector<Particle> particles;
    GameEvent currentEvent;

    Font font;
    Sound soundCorrect, soundWrong, soundNotify, soundClick;

    bool eventActive = false, showFeedback = false;
    string feedbackText = "";
    Color feedbackColor = WHITE;

    float eventTimeout = 15.0f, timeLeft = 15.0f, pauseTimer = 0, menuAnimation = 0;
    int selectedMenuItem = 0, selectedShopItem = 0;

    Color bgHome = { 20,20,30,255 }, bgOffice = { 30,35,40,255 }, bgMobile = { 40,20,40,255 };
    Color screenBg = { 30,30,45,255 }, panelBg = { 40,40,60,255 };
    Color threatColor = { 220,50,50,255 }, safeColor = { 50,200,100,255 }, textColor = { 240,240,240,255 };
    Color blueColor = { 100,200,255,255 }, yellowColor = { 255,200,100,255 }, darkColor = { 20,20,30,255 };
    Color grayColor = { 100,100,100,255 }, borderColor = { 100,100,120,255 }, titleBarColor = { 60,60,80,255 };
    Color popupBg = { 245,245,250,255 }, overlayColor = { 0,0,0,200 }, goBoxColor = { 30,30,40,255 };
    Color feedbackBg = { 20,20,30,200 }, lightGray = { 200,200,200,255 };
    Color redBtn = { 255,100,100,255 }, greenBtn = { 100,255,150,255 }, yellowBtn = { 255,200,100,255 };
    Color menuBtnColor = { 60,60,100,255 }, menuBtnSelected = { 100,100,180,255 }, comboColor = { 255,215,0,255 };

public:
    CyberDefender() {
        InitWindow(screenWidth, screenHeight, "CyberDefender - Cybersecurity Trainer");
        SetTargetFPS(60);
        srand((unsigned int)time(nullptr));

        // Load Consolas font (guaranteed to have English letters)
        vector<int> cps;
        for (int i = 32; i <= 126; i++) cps.push_back(i);
        font = LoadFontEx("C:/Windows/Fonts/consolas.ttf", 40, cps.data(), (int)cps.size());
        if (font.texture.id == 0) {
            cout << "WARNING: Consolas not found, using default font" << endl;
            font = GetFontDefault();
        }
        else {
            cout << "Font loaded successfully!" << endl;
        }

        soundCorrect = LoadSound("correct.wav");
        soundWrong = LoadSound("wrong.wav");
        soundNotify = LoadSound("notify.wav");
        soundClick = LoadSound("click.wav");

        initEvents();
        loadHighScores();
    }

    ~CyberDefender() {
        if (font.texture.id != 0 && font.texture.id != GetFontDefault().texture.id) UnloadFont(font);
        UnloadSound(soundCorrect); UnloadSound(soundWrong);
        UnloadSound(soundNotify);  UnloadSound(soundClick);
        CloseWindow();
    }

    void playSoundSafe(Sound& s) { if (s.stream.buffer != NULL) PlaySound(s); }

    void drawText(const string& t, int x, int y, int size, Color c) {
        DrawTextEx(font, t.c_str(), Vector2{ (float)x,(float)y }, (float)size, 1.0f, c);
    }

    int drawTextWrapped(const string& text, int x, int y, int size, int maxW, Color c, int maxLines = 3) {
        int lineH = size + 4;
        string cur;
        int curY = y;
        int lines = 0;
        size_t i = 0;
        while (i < text.size()) {
            size_t sp = text.find(' ', i);
            string word = (sp == string::npos) ? text.substr(i) : text.substr(i, sp - i);
            string test = cur.empty() ? word : cur + " " + word;
            if (MeasureTextEx(font, test.c_str(), (float)size, 1.0f).x > maxW && !cur.empty()) {
                drawText(cur, x, curY, size, c);
                curY += lineH; lines++;
                cur = word;
                if (lines >= maxLines) return curY;
            }
            else {
                cur = test;
            }
            if (sp == string::npos) break;
            i = sp + 1;
        }
        if (!cur.empty()) { drawText(cur, x, curY, size, c); curY += lineH; }
        return curY;
    }

    void spawnParticles(int x, int y, Color c, int n = 20) {
        for (int i = 0; i < n; i++) {
            Particle p; p.x = (float)x; p.y = (float)y;
            p.vx = (float)(rand() % 200 - 100) / 10.0f; p.vy = (float)(rand() % 200 - 100) / 10.0f;
            p.life = 1.0f; p.color = c; particles.push_back(p);
        }
    }
    void updateParticles() {
        for (int i = (int)particles.size() - 1;i >= 0;i--) {
            particles[i].x += particles[i].vx; particles[i].y += particles[i].vy;
            particles[i].life -= 0.02f; particles[i].vy += 0.1f;
            if (particles[i].life <= 0) particles.erase(particles.begin() + i);
        }
    }
    void renderParticles() {
        for (auto& p : particles) {
            Color c = p.color; c.a = (unsigned char)(p.life * 255);
            DrawRectangle((int)p.x, (int)p.y, 4, 4, c);
        }
    }

    void updateTheme() {
        if (player.level <= 3) currentTheme = THEME_HOME;
        else if (player.level <= 6) currentTheme = THEME_OFFICE;
        else currentTheme = THEME_MOBILE;
    }
    string themeName() { return currentTheme == THEME_HOME ? "Home Network" : (currentTheme == THEME_OFFICE ? "Corporate Office" : "Mobile Device"); }
    Color themeBg() { return currentTheme == THEME_HOME ? bgHome : (currentTheme == THEME_OFFICE ? bgOffice : bgMobile); }

    void addEvent(const string& t, const string& s, const string& m, const string& u,
        const string& ty, bool th, const string& ca, int mc, int df) {
        GameEvent e; e.title = t; e.sender = s; e.message = m; e.url = u; e.type = ty;
        e.isThreat = th; e.correctAction = ca; e.moneyChange = mc; e.difficulty = df;
        eventPool.push_back(e);
    }

    void initEvents() {
        // HOME THEME
        addEvent("URGENT! Account hacked!", "security@bank-verify.ru", "Click here to restore access to your account", "", "phishing", true, "DELETE", -500, 1);
        addEvent("You won iPhone 17!", "prize@free-gifts.com", "Follow the link to claim your prize now", "", "phishing", true, "DELETE", -300, 1);
        addEvent("Confirm your card details", "noreply@sberbank-online.net", "Your card will be blocked. Enter CVV code to verify", "", "phishing", true, "DELETE", -1000, 2);
        addEvent("Password expired", "admin@company-mail.com", "Update your password immediately or lose access", "", "phishing", true, "DELETE", -400, 1);
        addEvent("Package delivery failed", "delivery@post-service.net", "Pay $5 for redelivery of your package", "", "phishing", true, "DELETE", -200, 1);
        addEvent("Netflix subscription ended", "billing@netflix-support.com", "Update payment info to continue watching", "", "phishing", true, "DELETE", -600, 2);
        addEvent("IRS Tax Refund Available", "refund@irs-gov.com", "Claim your tax refund of $1,200 now", "", "phishing", true, "DELETE", -1500, 3);
        addEvent("Incoming call: +7 (999) 123-45-67", "Unknown caller", "Hello! This is bank security. Please tell us the SMS code", "", "scam_call", true, "HANG UP", -2000, 2);
        addEvent("Incoming call: Tax Office", "+7 (495) 000-00-00", "You have unpaid taxes. Pay now or accounts will be frozen", "", "scam_call", true, "HANG UP", -5000, 3);
        addEvent("Letter from employer", "hr@company.com", "Your monthly salary has been transferred to your account", "", "legit", false, "OPEN", 5000, 1);
        addEvent("Bank notification", "notifications@tinkoff.ru", "Card transaction confirmed. Amount: 150 rub", "", "legit", false, "OPEN", 100, 1);

        // OFFICE THEME
        addEvent("CEO Urgent Request", "ceo@yourcompany-internal.net", "I am in a meeting. Buy $5000 in gift cards immediately and send codes", "", "phishing", true, "DELETE", -5000, 3);
        addEvent("HR Policy Update", "hr@yourcompany.com", "Please review the updated remote work policy attached", "", "legit", false, "OPEN", 200, 1);
        addEvent("Downloading: invoice_crack.exe", "Torrent Site", "Suspicious file detected. Size: 12 MB", "", "trojan", true, "BLOCK", -1500, 2);
        addEvent("System Update Available", "windows-update-free.com", "Install this critical update to speed up your PC by 300%", "", "trojan", true, "CANCEL", -2000, 2);
        addEvent("Antivirus License Expired", "fake-antivirus.com", "Your system is at risk! Renew license immediately for $99", "", "trojan", true, "CANCEL", -2500, 3);

        // MOBILE THEME + URL CHECK
        addEvent("SMS: Bank Alert", "+79001234567", "Your card is blocked. Verify identity immediately:", "http://sber-verify-secure.ru/confirm", "url_check", true, "BLOCK", -1200, 2);
        addEvent("Message from Mom", "Mom (account may be hacked)", "Hi, I changed my number. Can you send me 5000 rubles urgently?", "", "scam_call", true, "IGNORE", -2500, 2);
        addEvent("App Update Required", "system-android-update.com", "Your OS is outdated. Click to install security patch", "", "trojan", true, "CANCEL", -2000, 3);
        addEvent("Two-Factor Authentication", "noreply@google.com", "Your new verification code is: 849201. Do not share this code", "", "legit", false, "IGNORE", 100, 1);
        addEvent("Delivery Notification", "noreply@dhl-express.net", "Your package is delayed. Pay $3 fee:", "http://dhl-tracking-fee.com/pay", "url_check", true, "BLOCK", -800, 2);
        addEvent("LinkedIn Connection", "noreply@linkedin.com", "John Doe wants to connect with you on LinkedIn", "", "legit", false, "OPEN", 50, 1);
    }

    void loadHighScores() {
        ifstream f("highscores.txt");
        if (f.is_open()) { HighScore hs; while (f >> hs.name >> hs.score >> hs.level) highScores.push_back(hs); f.close(); }
        sort(highScores.begin(), highScores.end(), [](const HighScore& a, const HighScore& b) {return a.score > b.score;});
    }
    void saveHighScore() {
        highScores.push_back({ "Player",player.score,player.level });
        sort(highScores.begin(), highScores.end(), [](const HighScore& a, const HighScore& b) {return a.score > b.score;});
        if (highScores.size() > 10) highScores.resize(10);
        ofstream f("highscores.txt");
        if (f.is_open()) { for (auto& hs : highScores) f << hs.name << " " << hs.score << " " << hs.level << "\n"; f.close(); }
    }

    void spawnEvent() {
        updateTheme();
        vector<GameEvent> av;
        for (auto& e : eventPool) if (e.difficulty <= player.level || rand() % 3 == 0) av.push_back(e);
        if (av.empty()) av = eventPool;
        currentEvent = av[rand() % av.size()];
        eventActive = true; showFeedback = false;
        playSoundSafe(soundNotify);
        timeLeft = max(5.0f, eventTimeout - (player.level - 1) * 1.5f);
        if (player.hasTimeExtension) timeLeft += 5.0f;
        pauseTimer = 0;
    }

    void handleInput() {
        if (currentState == MENU) {
            if (IsKeyPressed(KEY_UP)) { selectedMenuItem--; if (selectedMenuItem < 0)selectedMenuItem = 4; }
            if (IsKeyPressed(KEY_DOWN)) { selectedMenuItem++; if (selectedMenuItem > 4)selectedMenuItem = 0; }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                playSoundSafe(soundClick);
                if (selectedMenuItem == 0) { player = Player();currentState = PLAYING;spawnEvent(); }
                else if (selectedMenuItem == 1)currentState = INSTRUCTIONS;
                else if (selectedMenuItem == 2)currentState = LEADERBOARD;
                else if (selectedMenuItem == 3)currentState = SHOP;
                else if (selectedMenuItem == 4)CloseWindow();
            }
        }
        else if (currentState == PLAYING) {
            if (IsKeyPressed(KEY_ESCAPE)) { currentState = MENU;return; }
            if (!eventActive || showFeedback)return;
            if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_B))processDecision(true);
            else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_O))processDecision(false);
            else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_H))processDecision(true);
        }
        else if (currentState == INSTRUCTIONS || currentState == LEADERBOARD) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))currentState = MENU;
        }
        else if (currentState == GAMEOVER) {
            if (IsKeyPressed(KEY_R)) { player = Player();currentState = PLAYING;spawnEvent(); }
            else if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) { saveHighScore();currentState = MENU; }
        }
        else if (currentState == SHOP) {
            if (IsKeyPressed(KEY_UP)) { selectedShopItem--; if (selectedShopItem < 0)selectedShopItem = 3; }
            if (IsKeyPressed(KEY_DOWN)) { selectedShopItem++; if (selectedShopItem > 3)selectedShopItem = 0; }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                playSoundSafe(soundClick);
                int cost = selectedShopItem == 0 ? 500 : selectedShopItem == 1 ? 800 : selectedShopItem == 2 ? 600 : 400;
                if (player.money >= cost) {
                    if (selectedShopItem == 0 && !player.hasAntivirus) { player.money -= cost;player.hasAntivirus = true; }
                    else if (selectedShopItem == 1 && !player.hasFirewall) { player.money -= cost;player.hasFirewall = true; }
                    else if (selectedShopItem == 2 && !player.hasSpamFilter) { player.money -= cost;player.hasSpamFilter = true; }
                    else if (selectedShopItem == 3 && !player.hasTimeExtension) { player.money -= cost;player.hasTimeExtension = true; }
                }
            }
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_B))currentState = MENU;
        }
    }

    string getExplanation() {
        if (currentEvent.type == "phishing")return"Check the sender email address carefully.";
        if (currentEvent.type == "scam_call")return"Never share personal info over the phone.";
        if (currentEvent.type == "trojan")return"Only download software from official sources.";
        if (currentEvent.type == "url_check")return"Always verify URLs before clicking.";
        return"Be more careful next time!";
    }

    void processDecision(bool blocked) {
        showFeedback = true;
        bool correct = (blocked == currentEvent.isThreat);
        if (!correct && player.hasAntivirus && currentEvent.type == "trojan") { correct = true;feedbackText = "ANTIVIRUS PROTECTED YOU!"; }
        if (!correct && player.hasSpamFilter && currentEvent.type == "phishing") { correct = true;feedbackText = "SPAM FILTER CAUGHT IT!"; }

        if (correct) {
            playSoundSafe(soundCorrect);
            player.combo++; if (player.combo > player.maxCombo)player.maxCombo = player.combo;
            int bonus = player.combo * 10;
            player.score += 100 + bonus;
            player.money += currentEvent.moneyChange; if (player.money < 0)player.money = 0;
            feedbackText = (player.combo > 1) ? "CORRECT! COMBO x" + to_string(player.combo) : "CORRECT! Good decision.";
            feedbackColor = safeColor;
            player.correctAnswers++; player.eventsHandled++;
            spawnParticles(600, 400, safeColor, 30);
            if (player.eventsHandled % 5 == 0) { player.level++;currentState = SHOP;return; }
        }
        else {
            playSoundSafe(soundWrong);
            player.combo = 0;
            player.lives -= player.hasFirewall ? 0 : 1;
            player.money += currentEvent.moneyChange; if (player.money < 0)player.money = 0;
            feedbackText = "WRONG! " + getExplanation();
            feedbackColor = threatColor;
            player.wrongAnswers++;
            spawnParticles(600, 400, threatColor, 20);
        }
        if (player.lives <= 0) { currentState = GAMEOVER;saveHighScore(); }
    }

    void update() {
        menuAnimation += GetFrameTime() * 2.0f;
        updateParticles();
        if (currentState != PLAYING)return;
        float dt = GetFrameTime();
        if (showFeedback) {
            pauseTimer += dt;
            if (pauseTimer >= 1.5f) { showFeedback = false; spawnEvent(); }
            return;
        }
        if (!eventActive)return;
        timeLeft -= dt;
        if (timeLeft <= 0) {
            playSoundSafe(soundWrong);
            player.lives--; player.money -= 500; if (player.money < 0)player.money = 0;
            player.combo = 0;
            feedbackText = "TIME'S UP! You hesitated too long.";
            feedbackColor = threatColor; showFeedback = true; pauseTimer = 0; player.wrongAnswers++;
            if (player.lives <= 0) { currentState = GAMEOVER;saveHighScore(); }
        }
    }

    void render() {
        BeginDrawing();
        ClearBackground((currentState == PLAYING) ? themeBg() : bgHome);
        switch (currentState) {
        case MENU:renderMenu();break;
        case PLAYING:renderGame();break;
        case INSTRUCTIONS:renderInstructions();break;
        case GAMEOVER:renderGameOver();break;
        case SHOP:renderShop();break;
        case LEADERBOARD:renderLeaderboard();break;
        }
        renderParticles();
        EndDrawing();
    }

    void renderMenu() {
        for (int i = 0;i < 50;i++) {
            float x = (float)(i * 30 + (int)(menuAnimation * 10) % screenWidth);
            float y = (float)((i * 50) % screenHeight);
            DrawRectangle((int)x, (int)y, 2, 2, Color{ 100,100,255,(unsigned char)(20 + (i % 5) * 5) });
        }
        drawText("CYBERDEFENDER", screenWidth / 2 - 280, 100, 60, blueColor);
        drawText("Cybersecurity Trainer v3.0", screenWidth / 2 - 180, 170, 28, lightGray);
        DrawRectangle(screenWidth / 2 - 300, 210, 600, 2, blueColor);
        string items[] = { "START GAME","HOW TO PLAY","LEADERBOARD","UPGRADE SHOP","EXIT" };
        int by = 260;
        for (int i = 0;i < 5;i++) {
            Color bc = (i == selectedMenuItem) ? menuBtnSelected : menuBtnColor;
            Color tc = (i == selectedMenuItem) ? WHITE : lightGray;
            DrawRectangle(screenWidth / 2 - 200, by + i * 70, 400, 55, bc);
            DrawRectangleLines(screenWidth / 2 - 200, by + i * 70, 400, 55, blueColor);
            int w = MeasureTextEx(font, items[i].c_str(), 22, 1.0f).x;
            drawText(items[i], screenWidth / 2 - w / 2, by + i * 70 + 13, 22, tc);
        }
        drawText("UP/DOWN to navigate, ENTER to select", screenWidth / 2 - 200, 650, 16, grayColor);
    }

    void renderGame() {
        DrawRectangle(0, 0, screenWidth, 80, panelBg);
        drawText("LIVES: " + to_string(player.lives), 30, 25, 22, threatColor);
        drawText("BALANCE: " + to_string(player.money), 250, 25, 22, safeColor);
        drawText("SCORE: " + to_string(player.score), 500, 25, 22, blueColor);
        drawText("LEVEL: " + to_string(player.level) + " (" + themeName() + ")", 720, 25, 20, yellowColor);
        if (player.combo > 1)drawText("COMBO x" + to_string(player.combo), 950, 25, 22, comboColor);

        DrawRectangle(150, 110, 900, 550, screenBg);
        DrawRectangleLines(150, 110, 900, 550, borderColor);
        DrawRectangle(150, 110, 900, 35, titleBarColor);
        drawText("Desktop - " + themeName(), 170, 118, 16, WHITE);

        if (eventActive && !showFeedback) {
            DrawRectangle(190, 170, 820, 420, popupBg);
            DrawRectangleLinesEx(Rectangle{ 190,170,820,420 }, 4, borderColor);
            int y = drawTextWrapped(currentEvent.title, 210, 190, 22, 780, darkColor, 2);
            drawText("From: " + currentEvent.sender, 210, y + 6, 16, grayColor);
            drawTextWrapped(currentEvent.message, 210, y + 32, 18, 780, Color{ 30,30,50,255 }, 2);

            if (currentEvent.type == "url_check" && !currentEvent.url.empty()) {
                DrawRectangle(210, 420, 780, 50, Color{ 255,240,240,255 });
                DrawRectangleLines(210, 420, 780, 50, threatColor);
                drawText("LINK PREVIEW: " + currentEvent.url, 220, 435, 18, threatColor);
            }
            else {
                drawText("Analyze carefully and make your decision", 210, 420, 14, grayColor);
            }

            drawText("Time: " + to_string((int)timeLeft) + " sec", 210, 540, 16, timeLeft < 5 ? threatColor : textColor);
            float bw = 820 * (timeLeft / eventTimeout);
            DrawRectangle(190, 575, bw, 8, timeLeft < 5 ? threatColor : safeColor);
        }

        if (showFeedback) {
            DrawRectangle(190, 400, 820, 100, feedbackBg);
            drawTextWrapped(feedbackText, 210, 425, 22, 780, feedbackColor, 2);
        }

        DrawRectangle(0, 680, screenWidth, 120, panelBg);
        drawText("ACTIONS:", 50, 700, 20, lightGray);
        drawText("[1/B] BLOCK/DELETE", 50, 735, 18, redBtn);
        drawText("[2/O] OPEN/ALLOW", 450, 735, 18, greenBtn);
        drawText("[3/H] HANG UP", 850, 735, 18, yellowBtn);
    }

    void renderShop() {
        DrawRectangle(0, 0, screenWidth, screenHeight, themeBg());
        drawText("UPGRADE SHOP", screenWidth / 2 - 150, 50, 40, yellowColor);
        DrawRectangle(screenWidth / 2 - 300, 100, 600, 2, yellowColor);
        drawText("Balance: " + to_string(player.money) + " rub", screenWidth / 2 - 120, 120, 24, safeColor);
        string items[] = { "ANTIVIRUS - Blocks trojans","FIREWALL - Saves 1 life","SPAM FILTER - Catches phishing","TIME EXTENSION +5 sec" };
        int costs[] = { 500,800,600,400 };
        bool owned[] = { player.hasAntivirus,player.hasFirewall,player.hasSpamFilter,player.hasTimeExtension };
        int y = 180;
        for (int i = 0;i < 4;i++) {
            Color bc = owned[i] ? Color{ 50,50,50,255 } : ((i == selectedShopItem) ? menuBtnSelected : menuBtnColor);
            DrawRectangle(200, y + i * 80, 800, 65, bc);
            DrawRectangleLines(200, y + i * 80, 800, 65, blueColor);
            string t = items[i] + " - " + to_string(costs[i]) + " rub";
            if (owned[i])t = items[i] + " - INSTALLED";
            drawText(t, 220, y + i * 80 + 20, 20, owned[i] ? grayColor : WHITE);
        }
        drawText("UP/DOWN to select, ENTER to buy. ESC to return", screenWidth / 2 - 220, 550, 18, grayColor);
    }

    void renderLeaderboard() {
        DrawRectangle(0, 0, screenWidth, screenHeight, bgHome);
        drawText("LEADERBOARD", screenWidth / 2 - 150, 50, 40, blueColor);
        DrawRectangle(screenWidth / 2 - 300, 100, 600, 2, blueColor);
        if (highScores.empty()) drawText("No scores yet. Play a game!", screenWidth / 2 - 150, 300, 24, grayColor);
        else {
            drawText("Rank  Name              Score   Level", 200, 150, 20, yellowColor);
            for (int i = 0;i < (int)highScores.size() && i < 10;i++) {
                string e = "#" + to_string(i + 1) + "    " + highScores[i].name + string(20 - highScores[i].name.length(), ' ') + to_string(highScores[i].score) + string(10, ' ') + to_string(highScores[i].level);
                drawText(e, 200, 200 + i * 40, 20, textColor);
            }
        }
        drawText("Press ESC or ENTER to return", screenWidth / 2 - 160, 700, 18, grayColor);
    }

    void renderInstructions() {
        DrawRectangle(0, 0, screenWidth, screenHeight, bgHome);
        drawText("HOW TO PLAY", screenWidth / 2 - 150, 40, 40, blueColor);
        DrawRectangle(screenWidth / 2 - 300, 90, 600, 2, blueColor);
        int y = 120, lh = 30;
        drawText("OBJECTIVE: Protect your computer from cyber threats!", 100, y, 20, textColor);y += lh;
        drawText("THEMES: Game progresses from Home -> Office -> Mobile.", 100, y, 18, textColor);y += lh;
        drawText("CONTROLS: [1/B] Block  [2/O] Open  [3/H] Hang up  [ESC] Menu", 100, y, 18, textColor);y += lh;
        drawText("URL CHECK: Special events show a link. Analyze it carefully!", 100, y, 18, yellowColor);y += lh + 10;
        drawText("RED FLAGS:", 100, y, 22, threatColor);y += lh;
        drawText("- Urgency and threats  - Requests for money/codes  - Strange addresses", 100, y, 18, threatColor);y += lh;
        drawText("- Links with typos (paypa1.com)  - Calls asking for SMS codes", 100, y, 18, threatColor);y += lh + 10;
        drawText("TIPS:", 100, y, 22, safeColor);y += lh;
        drawText("- 90% of phishing creates false urgency.", 100, y, 18, textColor);y += lh;
        drawText("- Banks NEVER ask for SMS codes over the phone.", 100, y, 18, textColor);y += lh;
        drawText("- Cracked software is the #1 source of Trojans.", 100, y, 18, textColor);y += lh;
        drawText("- HTTPS does not guarantee a site is legitimate.", 100, y, 18, textColor);
        DrawRectangle(screenWidth / 2 - 150, 720, 300, 50, menuBtnColor);
        DrawRectangleLines(screenWidth / 2 - 150, 720, 300, 50, blueColor);
        drawText("BACK TO MENU", screenWidth / 2 - 110, 732, 22, WHITE);
    }

    void renderGameOver() {
        DrawRectangle(0, 0, screenWidth, screenHeight, overlayColor);
        DrawRectangle(250, 150, 700, 500, goBoxColor);
        DrawRectangleLinesEx(Rectangle{ 250,150,700,500 }, 4, threatColor);
        drawText("SYSTEM HACKED!", 370, 180, 36, threatColor);
        DrawRectangle(300, 230, 600, 2, threatColor);
        int y = 260, lh = 40;
        drawText("Final Score: " + to_string(player.score), 320, y, 24, textColor);y += lh;
        drawText("Level Reached: " + to_string(player.level) + " (" + themeName() + ")", 320, y, 24, textColor);y += lh;
        drawText("Accuracy: " + to_string(player.eventsHandled > 0 ? (player.correctAnswers * 100) / player.eventsHandled : 0) + "%", 320, y, 24, safeColor);y += lh;
        drawText("Max Combo: x" + to_string(player.maxCombo), 320, y, 24, comboColor);y += lh;
        drawText("Press [R] to restart", 400, y + 20, 20, safeColor);
        drawText("Press [M] or ESC for menu", 380, y + 50, 18, grayColor);
    }

    void run() {
        while (!WindowShouldClose()) { handleInput(); update(); render(); }
    }
};

int main() { CyberDefender game; game.run(); return 0; }