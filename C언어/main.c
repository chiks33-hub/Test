#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <time.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define EMPTY 0
#define BLOCK 1

// 게임 보드 배열
int board[BOARD_HEIGHT][BOARD_WIDTH] = { 0 };

// 7가지 테트리스 블록 (4x4 배열)
const int tetromino[7][4][4] = {
    {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}}, // I
    {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}}, // O
    {{0,0,0,0}, {0,1,0,0}, {1,1,1,0}, {0,0,0,0}}, // T
    {{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}}, // S
    {{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}}, // Z
    {{0,0,0,0}, {1,0,0,0}, {1,1,1,0}, {0,0,0,0}}, // J
    {{0,0,0,0}, {0,0,1,0}, {1,1,1,0}, {0,0,0,0}}  // L
};

int currentPiece[4][4];
int pieceX, pieceY;
int score = 0;

// ==========================================
// 1. 터미널 제어 함수 (키보드 실시간 입력)
// ==========================================
struct termios orig_termios;

void reset_terminal_mode() {
    tcsetattr(0, TCSANOW, &orig_termios);
    printf("\033[?25h"); // 커서 다시 표시
}

void set_conio_terminal_mode() {
    struct termios new_termios;
    tcgetattr(0, &orig_termios);
    new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO); // 엔터 없이 입력, 에코 끄기
    tcsetattr(0, TCSANOW, &new_termios);
    printf("\033[?25l"); // 화면 깜빡임 방지를 위해 커서 숨기기
}

int kbhit() {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int getch() {
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) return r;
    else return c;
}

// ==========================================
// 2. 게임 로직 함수
// ==========================================
void spawnPiece() {
    int shape = rand() % 7;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            currentPiece[i][j] = tetromino[shape][i][j];
        }
    }
    pieceX = BOARD_WIDTH / 2 - 2;
    pieceY = 0;
}

int checkCollision(int tempX, int tempY, int tempPiece[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (tempPiece[i][j]) {
                int boardX = tempX + j;
                int boardY = tempY + i;
                // 벽이나 바닥 충돌
                if (boardX < 0 || boardX >= BOARD_WIDTH || boardY >= BOARD_HEIGHT) return 1;
                // 기존 블록과 충돌
                if (boardY >= 0 && board[boardY][boardX]) return 1;
            }
        }
    }
    return 0;
}

void rotatePiece() {
    int temp[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[i][j] = currentPiece[3 - j][i]; // 90도 회전 공식
        }
    }
    if (!checkCollision(pieceX, pieceY, temp)) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                currentPiece[i][j] = temp[i][j];
            }
        }
    }
}

void mergePiece() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (currentPiece[i][j]) {
                if (pieceY + i < 0) {
                    reset_terminal_mode();
                    printf("\n=== GAME OVER ===\n최종 점수: %d\n", score);
                    exit(0);
                }
                board[pieceY + i][pieceX + j] = BLOCK;
            }
        }
    }
}

void clearLines() {
    for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
        int full = 1;
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (!board[i][j]) { full = 0; break; }
        }
        if (full) {
            score += 100;
            for (int k = i; k > 0; k--) {
                for (int j = 0; j < BOARD_WIDTH; j++) {
                    board[k][j] = board[k - 1][j];
                }
            }
            for (int j = 0; j < BOARD_WIDTH; j++) board[0][j] = 0;
            i++; // 현재 줄을 다시 검사
        }
    }
}

void drawBoard() {
    printf("\033[H"); // 콘솔 지우기 대신 커서를 맨 위로 이동 (깜빡임 방지)
    printf("=== C TETRIS ===\n");
    printf("Score: %d\n", score);
    printf("----------------------\n");

    for (int i = 0; i < BOARD_HEIGHT; i++) {
        printf("|");
        for (int j = 0; j < BOARD_WIDTH; j++) {
            int isPiece = 0;
            if (i >= pieceY && i < pieceY + 4 && j >= pieceX && j < pieceX + 4) {
                if (currentPiece[i - pieceY][j - pieceX]) isPiece = 1;
            }

            if (isPiece || board[i][j]) printf("[]");
            else printf(" .");
        }
        printf("|\n");
    }
    printf("----------------------\n");
    printf("조작: [W]회전 [A]좌 [D]우 [S]하강 [Q]종료\n");
}

// ==========================================
// 3. 메인 게임 루프
// ==========================================
int main() {
    srand(time(NULL));
    set_conio_terminal_mode();
    printf("\033[2J"); // 최초 화면 지우기

    spawnPiece();
    int dropTimer = 0;

    while (1) {
        drawBoard();

        // 사용자 입력 처리
        if (kbhit()) {
            char key = getch();
            if (key == 'a' || key == 'A') {
                if (!checkCollision(pieceX - 1, pieceY, currentPiece)) pieceX--;
            }
            else if (key == 'd' || key == 'D') {
                if (!checkCollision(pieceX + 1, pieceY, currentPiece)) pieceX++;
            }
            else if (key == 's' || key == 'S') {
                if (!checkCollision(pieceX, pieceY + 1, currentPiece)) pieceY++;
            }
            else if (key == 'w' || key == 'W') {
                rotatePiece();
            }
            else if (key == 'q' || key == 'Q') {
                break;
            }
        }

        // 블록 자동 하강 로직 (약 0.5초마다)
        dropTimer++;
        if (dropTimer > 20) {
            if (!checkCollision(pieceX, pieceY + 1, currentPiece)) {
                pieceY++;
            }
            else {
                mergePiece();
                clearLines();
                spawnPiece();
            }
            dropTimer = 0;
        }

        usleep(25000); // 0.025초 대기 (프레임 속도 조절)
    }

    reset_terminal_mode();
    printf("\n게임을 종료합니다.\n");
    return 0;
}