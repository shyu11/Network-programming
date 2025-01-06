#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

extern "C" {
#include "unp.h"
}

#undef min
#undef max

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cmath>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

const int WINDOW_WIDTH = 700;
const int WINDOW_HEIGHT = 750;
sf::RenderWindow window; //(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Watermelon Chess - Lobby");
sf::RenderWindow window2; //(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Watermelon Chess - Checkboard");
bool window1Active = true;
sf::Event event;
sf::Font font;
sf::Text timerText;
float remainingTime = 30.0f;

void handlePlayerButtonClick();
void updateLobbyUI();
void chatBox();
void displayInvitation(const string& );
void rejectNotice(const string& );
void lobbyLoop();
vector<sf::CircleShape> createThickArc(sf::Vector2f, float, float, float, float);
void showPSSNotice();
void winNotice();
void lossNotice();
void tieNotice();
void wrongNotice();
void updateBoard();
void initializeTimer();
void updateTimer(sf::Clock& );
void myTurn();
void checkBoard();
void receiveData();

mutex chatMutex, listMutex, windowMutex, boardMutex;
vector<string> chatMessages;
vector<pair<string, string>> playerList;
vector<int> boardState(21, 0); // 0 = empty, 1 = player 1 piece, 2 = player 2 piece
vector<sf::CircleShape> circleButtons;    // dots

const float RADIUS = 21.f; // Radius of grid points
const float BIG_CIRCLE_RADIUS = 270.f; // Radius of the big circle
const float SMALL_CIRCLE_RADIUS = 90.f; // Radius of the small circle
const sf::Vector2f CENTER(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2); // Center point of the window (21)
const int POINT_COUNT = 60; // Number of points for each arc
const float ARC_THICKNESS = 3.f; // Thickness of the arc (simulated with small circles)

int sockfd;
char id[MAXLINE];
int pssnotice;
char inviter[MAXLINE];
int bNotice;
int nNotice;
int myColor;
int wNotice;
int winNot;
int lossNot;
int tieNot;
int turn;
int selectedPiece;

struct PlayerButton {
    sf::RectangleShape button;
    string command;  // "watch" or "special"
    string playerName;
};

vector<PlayerButton> playerButtons;
void handlePlayerButtonClick() {
    //static sf::Clock clickClock;
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        //clickClock.restart();
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        for (const auto& pb : playerButtons) {
            if (pb.button.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                char sendline[MAXLINE];
                bzero(sendline, MAXLINE);
                sprintf(sendline, "%s\n", pb.command.c_str());
                Writen(sockfd, sendline, strlen(sendline));
                printf("sent: %s", sendline);
                bzero(sendline, MAXLINE);
                sprintf(sendline, "%s\n", pb.playerName.c_str());
                Writen(sockfd, sendline, strlen(sendline));
                printf("sent: %s", sendline);
                break;
            }
        }
    }
}

void updateLobbyUI() {
    playerButtons.clear();
    float y = 120;

    lock_guard<mutex> lock(listMutex);
    int index = 0;
    for (const auto& player : playerList) {
        if (index == 0) {
            sf::Text you("Hi " + player.first + " :D", font, 22);
            you.setFillColor(sf::Color::Black);
            you.setPosition(105, y);
            window.draw(you);
            y += 30;
            sf::Text pList("-- Players --", font, 20);
            pList.setFillColor(sf::Color::Black);
            pList.setPosition(280, y);
            window.draw(pList);
            int yy = y + 15;
            sf::Text line("-------------------------------------------------------------------", font, 20);
            line.setFillColor(sf::Color::Black);
            line.setPosition(100, yy);
            window.draw(line);
        }

        if (index > 0) {
            sf::Text name(player.first + " (" + player.second + ")", font, 20);
            name.setFillColor(sf::Color::Black);
            name.setPosition(120, y);
            window.draw(name);

            sf::RectangleShape button(sf::Vector2f(100, 25));
            button.setFillColor(sf::Color(0, 255, 127));    // spring green
            button.setPosition(450, y);

            string command = player.second == "playing" ? "watch" : "special";
            playerButtons.push_back({button, command, player.first});

            sf::Text buttonText(player.second == "playing" ? "Watch" : "Battle", font, 14);
            buttonText.setFillColor(sf::Color::Black);
            buttonText.setPosition(478, y + 5);

            if (player.second != "watching"){
                window.draw(button);
                window.draw(buttonText);
            }
        }
        y += 36;
        index++;
    }
}

void displayInvitation(const string& inviter) {
    if (!bNotice) {
        return;
    }
    //printf("in battle.\n");
    // 畫背景正方形通知框
    sf::RectangleShape inviteWindow(sf::Vector2f(400, 200)); // 正方形
    inviteWindow.setFillColor(sf::Color(50, 50, 50, 180)); // 半透明的灰色背景
    inviteWindow.setPosition(150, 150); // 設定正方形位置

    // 設定通知文字
    sf::Text inviteText(inviter + " wants to challenge you!", font, 18);
    inviteText.setFillColor(sf::Color::White);
    inviteText.setPosition(200, 200); // 文字位置

    sf::RectangleShape yesButton(sf::Vector2f(80, 40));
    yesButton.setFillColor(sf::Color::Green);
    yesButton.setPosition(230, 270);

    sf::Text yesText("Yes", font, 18);
    yesText.setFillColor(sf::Color::Black);
    yesText.setPosition(255, 280);

    sf::RectangleShape noButton(sf::Vector2f(80, 40));
    noButton.setFillColor(sf::Color::Red);
    noButton.setPosition(400, 270);

    sf::Text noText("No", font, 18);
    noText.setFillColor(sf::Color::Black);
    noText.setPosition(430, 280);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);

        char sendline[MAXLINE];
        if (yesButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
            bzero(sendline, MAXLINE);
            sprintf(sendline, "answer\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            bzero(sendline, MAXLINE);
            sprintf(sendline, "%s\n", inviter.c_str());
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            bzero(sendline, MAXLINE);
            sprintf(sendline, "1\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            bNotice = 0;
        }
        else if (noButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
            bzero(sendline, MAXLINE);
            sprintf(sendline, "answer\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            bzero(sendline, MAXLINE);
            sprintf(sendline, "%s\n", inviter.c_str());
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            bzero(sendline, MAXLINE);
            sprintf(sendline, "0\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            bNotice = 0;
        }
    }

    window.draw(inviteWindow);
    window.draw(inviteText);
    window.draw(yesButton);
    window.draw(yesText);
    window.draw(noButton);
    window.draw(noText);
}

void rejectNotice(const string& inviter) {
    //printf("in pss, notice = %d", notice);
    if (!nNotice) {
        return;
    }

    // 畫背景正方形通知框
    sf::RectangleShape NnotificationBox(sf::Vector2f(300, 100)); // 正方形
    NnotificationBox.setFillColor(sf::Color(50, 50, 50, 200)); // 半透明的灰色背景
    NnotificationBox.setPosition(200, 150); // 設定正方形位置

    // 設定通知文字
    sf::Text Ntitle("Oh no... you're rejected:(", font, 18);
    Ntitle.setFillColor(sf::Color::White);
    Ntitle.setPosition(220, 180); // 文字位置

    // 設定 ok 按鈕
    sf::RectangleShape NokButton(sf::Vector2f(40, 20));
    NokButton.setFillColor(sf::Color::Red);
    NokButton.setPosition(440, 218);
    sf::Text NokText("OK~", font, 12);
    NokText.setFillColor(sf::Color::White);
    NokText.setPosition(450, 220);

    // 畫出正方形背景
    window.draw(NnotificationBox);
    window.draw(Ntitle);        // 畫標題文字
    window.draw(NokButton);   // 畫 ok 按鈕
    window.draw(NokText);
    //printf("draw reject good.\n");

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);

        if (NokButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {   // ok
            printf("press ok.\n");
            nNotice = 0;
            printf("reject notice = %d.\n", nNotice);
        }
    }
}

void lobbyLoop() {
    window.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Watermelon Chess - Lobby");
    sf::Text title("Watermelon Chess!", font, 45);
    title.setPosition(140, 20);
    title.setFillColor(sf::Color::Black);
    sf::Text ftitle("Welcome to the lobby", font, 20);
    ftitle.setPosition(225, 75);
    ftitle.setFillColor(sf::Color::Black);

    sf::CircleShape randomButton(60.f);
    randomButton.setFillColor(sf::Color(238, 0, 0));  // red
    randomButton.setPosition(553, 604);

    sf::Text randomText("Random", font, 20);
    randomText.setFillColor(sf::Color::Black);
    randomText.setPosition(573, 650);

    while (true){
        if (window1Active){
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed){
                    window.close();
                    send(sockfd, NULL, 0, 0);
                    close(sockfd);
                    exit(0);
                    return;
                }

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                    if (randomButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                        char sendline[MAXLINE];
                        bzero(sendline, MAXLINE);
                        sprintf(sendline, "random\n");
                        Writen(sockfd, sendline, strlen(sendline));
                        printf("sent: %s", sendline);
                        pssnotice = 0;
                    }
                }
            }

            window.clear(sf::Color(220, 220, 220));
            window.draw(title);
            window.draw(ftitle);
            window.draw(randomButton);
            window.draw(randomText);

            updateLobbyUI();
            handlePlayerButtonClick();
            displayInvitation(string(inviter));
            rejectNotice(string(inviter));
            winNotice();
            lossNotice();
            tieNotice();
            showPSSNotice();

            window.display();
        }
        else{
            window.close();
            checkBoard();
            break;
        }
    }
    
}

// Grid points
vector<sf::Vector2f> positions = {
    {CENTER.x - 90, CENTER.y - 255},            // 1
    {CENTER.x, CENTER.y - 270},                 // 2
    {CENTER.x + 90, CENTER.y - 255},            // 3
    {CENTER.x, CENTER.y - 180},                 // 4
    {CENTER.x - 255, CENTER.y - 90},            // 5
    {CENTER.x - 270, CENTER.y},                 // 6
    {CENTER.x - 255, CENTER.y + 90},            // 7
    {CENTER.x - 180, CENTER.y},                 // 8
    {CENTER.x - 90, CENTER.y + 255},            // 9
    {CENTER.x, CENTER.y + 270},                 // 10
    {CENTER.x + 90, CENTER.y + 255},            // 11
    {CENTER.x, CENTER.y + 180},                 // 12
    {CENTER.x + 255, CENTER.y + 90},            // 13
    {CENTER.x + 270, CENTER.y},                 // 14
    {CENTER.x + 255, CENTER.y - 90},            // 15
    {CENTER.x + 180, CENTER.y},                 // 16
    {CENTER.x, CENTER.y - 90},                  // 17
    {CENTER.x - 90, CENTER.y},                  // 18
    {CENTER.x, CENTER.y + 90},                  // 19
    {CENTER.x + 90, CENTER.y},                  // 20
    {CENTER.x, CENTER.y},                       // 21
};

// Draw a thick arc using small circles
vector<sf::CircleShape> createThickArc(sf::Vector2f center, float radius, float startAngle, float endAngle, float thickness) {
    vector<sf::CircleShape> thickArc;
    for (int i = 0; i <= POINT_COUNT; ++i) {
        float angle = startAngle + (endAngle - startAngle) * i / POINT_COUNT;
        float radian = angle * 3.14159265358979f / 180.f; // Convert degrees to radians
        sf::Vector2f point(center.x + radius * cos(radian), center.y + radius * sin(radian));

        // Use a small circle to simulate thickness
        sf::CircleShape dot(thickness);
        dot.setFillColor(sf::Color(139, 69, 19));
        dot.setOrigin(thickness, thickness);
        dot.setPosition(point);
        thickArc.push_back(dot);
    }
    return thickArc;
}

void showPSSNotice() {
    //printf("in pss, pssnotice = %d", pssnotice);
    if (!pssnotice) {
        return;
    }

    // 畫背景正方形通知框
    sf::RectangleShape notificationBox(sf::Vector2f(400, 200)); // 正方形
    notificationBox.setFillColor(sf::Color(50, 50, 50, 180)); // 半透明的灰色背景
    notificationBox.setPosition(150, 150); // 設定正方形位置

    // 設定通知文字
    sf::Text psstitle("Paper, Scissors, or Stone?", font, 18);
    psstitle.setFillColor(sf::Color::White);
    psstitle.setPosition(200, 200); // 文字位置

    // 設定 Paper 按鈕
    sf::RectangleShape paperButton(sf::Vector2f(75, 50));
    paperButton.setFillColor(sf::Color::Blue);
    paperButton.setPosition(180, 260);
    sf::Text paperText("Paper", font, 15);
    paperText.setFillColor(sf::Color::White);
    paperText.setPosition(195, 274);

    // 設定 Scissors 按鈕
    sf::RectangleShape scissorsButton(sf::Vector2f(75, 50));
    scissorsButton.setFillColor(sf::Color::Green);
    scissorsButton.setPosition(310, 260);
    sf::Text scissorsText("Scissors", font, 15);
    scissorsText.setFillColor(sf::Color::White);
    scissorsText.setPosition(316, 274);

    // 設定 Stone 按鈕
    sf::RectangleShape stoneButton(sf::Vector2f(75, 50));
    stoneButton.setFillColor(sf::Color::Red);
    stoneButton.setPosition(436, 260);
    sf::Text stoneText("Stone", font, 15);
    stoneText.setFillColor(sf::Color::White);
    stoneText.setPosition(450, 274);

    // 畫出正方形背景
    window.draw(notificationBox);
    window.draw(psstitle);        // 畫標題文字
    window.draw(paperButton);  // 畫 Paper 按鈕
    window.draw(paperText);    // 畫 Paper 文字
    window.draw(scissorsButton); // 畫 Scissors 按鈕
    window.draw(scissorsText);  // 畫 Scissors 文字
    window.draw(stoneButton);   // 畫 Stone 按鈕
    window.draw(stoneText);     // 畫 Stone 文字

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);

        char sendline[MAXLINE];
        if (paperButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {   // choose paper
            bzero(sendline, MAXLINE);
            sprintf(sendline, "pss\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            bzero(sendline, MAXLINE);
            sprintf(sendline, "1\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            pssnotice = 0;
        }
        else if (scissorsButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {   //choose scissors
            bzero(sendline, MAXLINE);
            sprintf(sendline, "pss\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            bzero(sendline, MAXLINE);
            sprintf(sendline, "2\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            pssnotice = 0;
        }
        else if (stoneButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {  // choose stone
            bzero(sendline, MAXLINE);
            sprintf(sendline, "pss\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            bzero(sendline, MAXLINE);
            sprintf(sendline, "3\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            pssnotice = 0;
        }
    }
}

void wrongNotice() {
    //printf("in pss, wNotice = %d", wNotice);
    if (!wNotice) {
        return;
    }

    // 畫背景正方形通知框
    sf::RectangleShape WnotificationBox(sf::Vector2f(175, 87)); // 正方形
    WnotificationBox.setFillColor(sf::Color(50, 50, 50, 200)); // 半透明的灰色背景
    WnotificationBox.setPosition(30, 30); // 設定正方形位置

    // 設定通知文字
    sf::Text Wtitle("Wrong step!", font, 18);
    Wtitle.setFillColor(sf::Color::White);
    Wtitle.setPosition(55, 55); // 文字位置

    // 設定 ok 按鈕
    sf::RectangleShape okButton(sf::Vector2f(40, 20));
    okButton.setFillColor(sf::Color::Red);
    okButton.setPosition(150, 90);
    sf::Text okText("OK~", font, 12);
    okText.setFillColor(sf::Color::Black);
    okText.setPosition(158, 93);

    // 畫出正方形背景
    window2.draw(WnotificationBox);
    window2.draw(Wtitle);        // 畫標題文字
    window2.draw(okButton);   // 畫 ok 按鈕
    window2.draw(okText);
    printf("draw wrong good.\n");

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window2);

        if (okButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {   // ok
            printf("press ok.\n");
            wNotice = 0;
            printf("wrong notice = %d.\n", wNotice);
            checkBoard();
        }
    }
}

void winNotice() {
    //printf("in pss, notice = %d", notice);
    if (!winNot) {
        return;
    }

    // 畫背景正方形通知框
    sf::RectangleShape WinnotificationBox(sf::Vector2f(300, 100)); // 正方形
    WinnotificationBox.setFillColor(sf::Color(106, 90, 50)); // 半透明的灰色背景
    WinnotificationBox.setPosition(200, 150); // 設定正方形位置

    // 設定通知文字
    sf::Text winTitle("Congratulations!", font, 18);
    winTitle.setFillColor(sf::Color::Black);
    winTitle.setPosition(220, 170); // 文字位置
    sf::Text winTitle2("You won the game!", font, 18);
    winTitle2.setFillColor(sf::Color::Black);
    winTitle2.setPosition(220, 190); // 文字位置

    // 設定 ok 按鈕
    sf::RectangleShape winOKButton(sf::Vector2f(40, 20));
    winOKButton.setFillColor(sf::Color::Red);
    winOKButton.setPosition(440, 218);
    sf::Text winOKText("OK~", font, 12);
    winOKText.setFillColor(sf::Color::White);
    winOKText.setPosition(450, 220);

    // 畫出正方形背景
    window.draw(WinnotificationBox);
    window.draw(winTitle);        // 畫標題文字
    window.draw(winTitle2);
    window.draw(winOKButton);   // 畫 ok 按鈕
    window.draw(winOKText);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);

        if (winOKButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {   // ok
            printf("press ok.\n");
            winNot = 0;
            printf("won notice = %d.\n", winNot);
        }
    }
}

void tieNotice() {
    //printf("in pss, notice = %d", notice);
    if (!tieNot) {
        return;
    }

    // 畫背景正方形通知框
    sf::RectangleShape TienotificationBox(sf::Vector2f(300, 100)); // 正方形
    TienotificationBox.setFillColor(sf::Color(106, 90, 50)); // 半透明的灰色背景
    TienotificationBox.setPosition(200, 150); // 設定正方形位置

    // 設定通知文字
    sf::Text tieTitle("It is a tie!", font, 18);
    tieTitle.setFillColor(sf::Color::Black);
    tieTitle.setPosition(220, 180); // 文字位置

    // 設定 ok 按鈕
    sf::RectangleShape tieOKButton(sf::Vector2f(40, 20));
    tieOKButton.setFillColor(sf::Color::Red);
    tieOKButton.setPosition(440, 218);
    sf::Text tieOKText("OK~", font, 12);
    tieOKText.setFillColor(sf::Color::White);
    tieOKText.setPosition(450, 220);

    // 畫出正方形背景
    window.draw(TienotificationBox);
    window.draw(tieTitle);        // 畫標題文字
    window.draw(tieOKButton);   // 畫 ok 按鈕
    window.draw(tieOKText);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);

        if (tieOKButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {   // ok
            printf("press ok.\n");
            tieNot = 0;
            printf("tie notice = %d.\n", tieNot);
        }
    }
}

void lossNotice() {
    //printf("in pss, notice = %d", notice);
    if (!lossNot) {
        return;
    }

    // 畫背景正方形通知框
    sf::RectangleShape LossnotificationBox(sf::Vector2f(300, 100)); // 正方形
    LossnotificationBox.setFillColor(sf::Color(106, 90, 50)); // 半透明的灰色背景
    LossnotificationBox.setPosition(200, 150); // 設定正方形位置

    // 設定通知文字
    sf::Text lossTitle("You loss the game:((", font, 18);
    lossTitle.setFillColor(sf::Color::Black);
    lossTitle.setPosition(220, 180); // 文字位置

    // 設定 ok 按鈕
    sf::RectangleShape lossOKButton(sf::Vector2f(40, 20));
    lossOKButton.setFillColor(sf::Color::Red);
    lossOKButton.setPosition(440, 218);
    sf::Text lossOKText("OK~", font, 12);
    lossOKText.setFillColor(sf::Color::White);
    lossOKText.setPosition(450, 220);

    // 畫出正方形背景
    window.draw(LossnotificationBox);
    window.draw(lossTitle);        // 畫標題文字
    window.draw(lossOKButton);   // 畫 ok 按鈕
    window.draw(lossOKText);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);

        if (lossOKButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {   // ok
            printf("press ok.\n");
            lossNot = 0;
            printf("loss notice = %d.\n", lossNot);
        }
    }
}

// Initialization function to set up the timer
void initializeTimer() {
    timerText.setFont(font);
    timerText.setString("30"); // Initial text
    timerText.setCharacterSize(20); // Size of the text
    timerText.setFillColor(sf::Color::White);

    // Position the timer in the middle of the window (adjust as needed)
    timerText.setPosition(330, 10);
}

// Update function to manage the timer
void updateTimer(sf::Clock& clock) {
    remainingTime -= clock.restart().asSeconds(); // Decrease remaining time

    if (remainingTime <= 0.0f) {
        remainingTime = 0.0f;
        printf("Time's up!\n");
        //myTurnEnded();
    }

    // Update the timer display
    timerText.setString(std::to_string(static_cast<int>(remainingTime)));
}

void myTurn() {
    if (!turn) {
        return;
    }
    //printf("in myturn\n");
    //sf::Clock clock;
    //initializeTimer();
    //updateTimer(clock);
    //window.draw(timerText);
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window2);
        int minSize = min({positions.size(), circleButtons.size(), boardState.size()});
        for (int i = 0; i < minSize; ++i) {
            if (circleButtons[i].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                if (selectedPiece == -1 && boardState[i] == myColor) { // Select a piece
                    selectedPiece = i;
                    // circleButtons[i].setFillColor(sf::Color::Green);
                    // window2.draw(circleButtons[i]);
                    break;
                }
                if (selectedPiece != -1 && boardState[i] == 0) { // Move to an empty spot
                    // circleButtons[i].setFillColor(sf::Color::Green);
                    // window2.draw(circleButtons[i]);
                    char sendline[MAXLINE];
                    bzero(sendline, MAXLINE);
                    sprintf(sendline, "move\n");
                    Writen(sockfd, sendline, strlen(sendline));
                    printf("sent: %s", sendline);

                    bzero(sendline, MAXLINE);   // a
                    sprintf(sendline, "%d\n", selectedPiece + 1);
                    Writen(sockfd, sendline, strlen(sendline));
                    printf("sent: %s", sendline);
                    bzero(sendline, MAXLINE);   // b
                    sprintf(sendline, "%d\n", i + 1);
                    Writen(sockfd, sendline, strlen(sendline));
                    printf("sent: %s", sendline);

                    selectedPiece = -1;
                    turn = 0;
                    break;
                }
            }
        }
    }
}

void checkBoard() {
    //sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Watermelon Chess - Checkboard");
    window2.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Watermelon Chess - Checkboard");
    //window2.setTitle("Watermelon Chess - Checkboard");
    window2.clear(sf::Color(245, 245, 220)); // Beige background
    //window2.display();

    sf::Text myBoardTitle("-----" + string(id) + "'s Board-----", font, 25);
    myBoardTitle.setFillColor(sf::Color::Black);
    myBoardTitle.setPosition(220, 30);

    sf::Text WmyBoardTitle("(watching)", font, 18);
    WmyBoardTitle.setFillColor(sf::Color::Black);
    WmyBoardTitle.setPosition(220, 40);

    sf::Text myTurntitle("Your turn~", font, 15);
    myTurntitle.setFillColor(sf::Color::Black);
    myTurntitle.setPosition(600, 720);

    // Big circle
    sf::CircleShape bigCircle(BIG_CIRCLE_RADIUS);
    bigCircle.setFillColor(sf::Color::Transparent);
    bigCircle.setOutlineColor(sf::Color(139, 69, 19));
    bigCircle.setOutlineThickness(4.5f); // Match arc thickness
    bigCircle.setOrigin(BIG_CIRCLE_RADIUS, BIG_CIRCLE_RADIUS);
    bigCircle.setPosition(CENTER);

    // Small circle
    sf::CircleShape smallCircle(SMALL_CIRCLE_RADIUS);
    smallCircle.setFillColor(sf::Color::Transparent);
    smallCircle.setOutlineColor(sf::Color(139, 69, 19));
    smallCircle.setOutlineThickness(4.5f); // Match arc thickness
    smallCircle.setOrigin(SMALL_CIRCLE_RADIUS, SMALL_CIRCLE_RADIUS);
    smallCircle.setPosition(CENTER);

    // Horizontal and vertical lines
    sf::RectangleShape horizontalLine(sf::Vector2f(BIG_CIRCLE_RADIUS * 2, 4.f));
    horizontalLine.setFillColor(sf::Color(139, 69, 19));
    horizontalLine.setPosition(CENTER.x - BIG_CIRCLE_RADIUS, CENTER.y);

    sf::RectangleShape verticalLine(sf::Vector2f(BIG_CIRCLE_RADIUS * 2, 4.f));
    verticalLine.setFillColor(sf::Color(139, 69, 19));
    verticalLine.setPosition(CENTER.x, CENTER.y - BIG_CIRCLE_RADIUS);
    verticalLine.setRotation(90.f);

    // Dot (small circles)
    for (const auto& pos : positions) {
        sf::CircleShape circleButton(RADIUS);
        circleButton.setFillColor(sf::Color(0, 0, 0));
        circleButton.setOrigin(RADIUS, RADIUS);
        circleButton.setPosition(pos);
        circleButtons.push_back(circleButton);
    }

    // Four quarter-circle arcs (thick)
    vector<vector<sf::CircleShape>> thickArcs;
    thickArcs.push_back(createThickArc({CENTER.x - BIG_CIRCLE_RADIUS, CENTER.y}, SMALL_CIRCLE_RADIUS, 270, 450, ARC_THICKNESS)); // Left
    thickArcs.push_back(createThickArc({CENTER.x + BIG_CIRCLE_RADIUS, CENTER.y}, SMALL_CIRCLE_RADIUS, 90, 270, ARC_THICKNESS));   // Right
    thickArcs.push_back(createThickArc({CENTER.x, CENTER.y - BIG_CIRCLE_RADIUS}, SMALL_CIRCLE_RADIUS, 0, 180, ARC_THICKNESS));   // Top
    thickArcs.push_back(createThickArc({CENTER.x, CENTER.y + BIG_CIRCLE_RADIUS}, SMALL_CIRCLE_RADIUS, 180, 360, ARC_THICKNESS)); // Bottom

    while (true){
        if (!window1Active){
            while (window2.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    char sendline[MAXLINE];
                    bzero(sendline, MAXLINE);
                    sprintf(sendline, "surrender\n");
                    Writen(sockfd, sendline, strlen(sendline));
                    printf("sent: %s", sendline);
                    window2.close();
                    window1Active = true;
                    lobbyLoop();
                    cout << "Switched to Window 1!" << endl;
                    return;
                }
            }
            window2.draw(myBoardTitle);
            window2.draw(bigCircle); // Draw big circle lines
            window2.draw(smallCircle);   // Draw small circle lines
            window2.draw(horizontalLine);    // Draw horizontal lines
            window2.draw(verticalLine);  // Draw vertical lines
            if (turn){
                window2.draw(myTurntitle);
            }

            for (const auto& thickArc : thickArcs) {    // Draw quarter-circle arcs (thick)
                for (const auto& arc : thickArc) {
                    window2.draw(arc);
                }
            }
            int minSize = min({positions.size(), circleButtons.size(), boardState.size()});
            for (int i = 0; i < minSize; ++i) {
                if (boardState[i] == 0) {
                    circleButtons[i].setFillColor(sf::Color::Red);
                }
                else if (boardState[i] == 1) {
                    circleButtons[i].setFillColor(sf::Color::Black);
                }
                else if (boardState[i] == 2) {
                    circleButtons[i].setFillColor(sf::Color::White);
                }
                else printf("wow\n");
                window2.draw(circleButtons[i]);
            }
            
            window2.display();
            myTurn();
            wrongNotice();
        }
        else {
            window2.close();
            lobbyLoop();
            break;
        }
    }
}

void receiveData() {
    char recvline[MAXLINE];
    int n;
    while ((n = readline(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = '\0';
        recvline[strcspn(recvline, "\n")] = '\0';
        printf("recv: %s\n", recvline);

        if (strcmp(recvline, "update") == 0) {
            lock_guard<mutex> lock(listMutex);
            //playerList.clear();

            while (true) {
                char name[MAXLINE], status[MAXLINE];
                if ((n = readline(sockfd, name, MAXLINE)) <= 0)
                    break;
                name[n] = '\0';
                name[strcspn(name, "\n")] = '\0';
                printf("name: %s", name);

                if ((n = readline(sockfd, status, MAXLINE)) <= 0)
                    break;
                status[n] = '\0';
                status[strcspn(status, "\n")] = '\0';
                printf("\tstatus: %s\n", status);

                if (strcmp(status, "leave") == 0){
                    auto it = remove_if(playerList.begin(), playerList.end(), [&name](const pair<string, string>& player) {
                        return player.first == name;
                    });
                    if (it != playerList.end()) {
                        playerList.erase(it, playerList.end());
                        cout << name << " is deleted\n";
                    }
                    break;
                }
                else if (strcmp(status, "online") == 0){
                    auto it = find_if(playerList.begin(), playerList.end(), [&name](const pair<string, string>& player) {
                        return player.first == name;
                    });
                    if (it != playerList.end()) {
                        it->first = name;
                        it->second = status;
                        cout << name << " is changed to online\n";
                    }
                    else {
                        playerList.emplace_back(name, status);
                        printf("player added.\n");
                    }
                    break;
                }
                else if (strcmp(status, "playing") == 0){
                    auto it = find_if(playerList.begin(), playerList.end(), [&name](const pair<string, string>& player) {
                        return player.first == name;
                    });
                    if (it != playerList.end()) {
                        it->first = name;
                        it->second = status;
                        cout << "updated " << it->first << " to " << it->second << endl;
                    }
                    break;
                }
                else if (strcmp(status, "watching") == 0){
                    auto it = find_if(playerList.begin(), playerList.end(), [&name](const pair<string, string>& player) {
                        return player.first == name;
                    });
                    if (it != playerList.end()) {
                        it->first = name;
                        it->second = status;
                        cout << "updated " << it->first << " to " << it->second << endl;
                    }
                    break;
                }
            }
        }
        else if (strcmp(recvline, "invite") == 0) {
            if ((n = readline(sockfd, inviter, MAXLINE)) > 0) {
                inviter[n] = '\0';
                inviter[strcspn(inviter, "\n")] = '\0';
                printf("recv: %s\n", inviter);
                bNotice = 1;
                printf("battle notice = %d.\n", bNotice);
            }
        }
        else if (strcmp(recvline, "inviteno") == 0) {
            char buffer[MAXLINE];
            if ((n = readline(sockfd, buffer, MAXLINE)) > 0) {
                buffer[n] = '\0';
                buffer[strcspn(buffer, "\n")] = '\0';
                nNotice = 1;
            }
        }
        else if (strcmp(recvline, "pss") == 0) {
            pssnotice = 1;
            printf("pssnotice = %d.\n", pssnotice);
        }
        else if (strcmp(recvline, "winpss") == 0) {
            printf("pssnotice = %d.\n", pssnotice);
            window1Active = false;
            cout << "Switched to Window 2 (CheckBoard)!" << endl;
            char color[MAXLINE];
            if ((n = readline(sockfd, color, MAXLINE)) <= 0)
                    break;
            color[n] = '\0';
            color[strcspn(color, "\n")] = '\0';
            printf("color: %s\n", color);

            if (strcmp(color, id) == 0){
                myColor = 1;    // black
                printf("My color: %d.\n", myColor);
            }
            else {
                myColor = 2;    // white
                printf("My color: %d.\n", myColor);
            }
        }
        else if (strcmp(recvline, "wrong") == 0) {
            wNotice = 1;
            printf("wrong notice = %d.\n", wNotice);
        }
        else if (strcmp(recvline, "board") == 0) {
            
            char buffer[MAXLINE];
            if ((n = readline(sockfd, buffer, MAXLINE)) > 0) {  // Update board state
                buffer[n] = '\0';
                buffer[strcspn(buffer, "\n")] = '\0';
                printf("recv: %s\n", buffer);
                unique_lock<mutex> boardLock(boardMutex);
                for (int i = 0; i < 21; ++i)
                    boardState[i] = buffer[i] - '0';
                boardLock.unlock();

                char isTurn[MAXLINE];
                if ((n = readline(sockfd, isTurn, MAXLINE)) <= 0)
                    break;
                isTurn[n] = '\0';
                isTurn[strcspn(isTurn, "\n")] = '\0';
                printf("isTurn?: %s\n", isTurn);

                if (strcmp(isTurn, "turn") == 0){
                    printf("My turn.\n");
                    selectedPiece = -1;
                    turn = 1;
                    myTurn();
                }
                else if (strcmp(isTurn, "noturn") == 0){
                    printf("Their turn.\n");
                    turn = 0;
                }
            }
        }
        else if (strcmp(recvline, "win") == 0) {
            char sendline[MAXLINE];
            bzero(sendline, MAXLINE);
            sprintf(sendline, "surrender\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            window1Active = true;
            cout << "Switched to Window 1 (Lobby)!" << endl;
            winNot = 1;
        }
        else if (strcmp(recvline, "lose") == 0) {
            char sendline[MAXLINE];
            bzero(sendline, MAXLINE);
            sprintf(sendline, "surrender\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            window1Active = true;
            cout << "Switched to Window 1 (Lobby)!" << endl;
            lossNot = 1;
        }
        else if (strcmp(recvline, "tie") == 0) {
            char sendline[MAXLINE];
            bzero(sendline, MAXLINE);
            sprintf(sendline, "surrender\n");
            Writen(sockfd, sendline, strlen(sendline));
            printf("sent: %s", sendline);
            window1Active = true;
            cout << "Switched to Window 1 (Lobby)!" << endl;
            tieNot = 1;
        }
    }
}

void xchg_data(FILE *fp, int sockfd)
{
    int       maxfdp1, stdineof, peer_exit, n;
    fd_set    rset;
    char      sendline[MAXLINE], recvline[MAXLINE];

    Writen(sockfd, id, strlen(id));
    printf("sent: %s\n", id);
    stdineof = 0;
    peer_exit = 0;
    pssnotice = 0;
    bNotice = 0;
    nNotice = 0;
    wNotice = 0;
    winNot = 0;
    lossNot = 0;
    tieNot = 0;
    turn = 0;

    for ( ; ; ) {
        FD_ZERO(&rset);
        maxfdp1 = 0;
        if (stdineof == 0) {
            FD_SET(fileno(fp), &rset);
            maxfdp1 = fileno(fp);
        };
        if (peer_exit == 0) {
            FD_SET(sockfd, &rset);
            if (sockfd > maxfdp1)
                maxfdp1 = sockfd;
        };
        maxfdp1++;
        Select(maxfdp1, &rset, NULL, NULL, NULL);
        if (FD_ISSET(sockfd, &rset)) {  // socket is readable
            n = readline(sockfd, recvline, MAXLINE);
            
            if (n == 0) {
                    if (stdineof == 1)
                    return;         // normal termination
                   else {
                    printf("(End of input from the peer!)");
                    peer_exit = 1;
                    return;
                };
            }
            else if (n > 0) {
                recvline[n] = '\0';
                printf("\x1B[0;36m%s\x1B[0m", recvline);
                recvline[strcspn(recvline, "\n")] = '\0';
                //recvline[n] = '\0';
                printf("recv: %s\n", recvline);

                if (strcmp(recvline, "full") == 0) {
                    cerr << "Lobby is full. Please wait and try again.\n";
                    close(sockfd);
                    return;
                }
                else if (strcmp(recvline, "connect") == 0) {
                    printf("Welcome to the Watermelon Chess Game!\n");
                    thread receiver(receiveData);
                    lobbyLoop();
                    receiver.join();
                    //continue;
                }
                else if (strcmp(recvline, "rename") == 0) {
                    printf("Your name has already been taken. Please rename.\n");
                    close(sockfd);
                    return;
                }
                //printf("%s", recvline);
                //fflush(stdout);
            }
            else { // n < 0
                printf("(server down)");
                return;
            };
        }
    }
};

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "usage: tcpcli <IPaddress> <ID>" << endl;
        return 1;
    }

    struct sockaddr_in servaddr;
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT + 5);
    Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    strcpy(id, argv[2]);

    Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
        cerr << "Font loading failed!" << endl;
        return -1;
    }
    
    xchg_data(stdin, sockfd);

    close(sockfd);
    exit(0);
}
