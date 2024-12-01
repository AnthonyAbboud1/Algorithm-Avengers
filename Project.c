#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define ROWS 10
#define COLUMNS 10

typedef struct {
    char *name;
    char symbol;
    int size;
    int hits;
} Ship;

typedef struct {
    char playerName[100];
    int radarSweepsUsed;
    int smokeScreensUsed;
    int shipsSunk;
    int artilleryAvailable;
    int torpedoAvailable;
    int nextTurnArtillery;
    int nextTurnTorpedo;
} Player;

typedef struct {
    int row;
    int col;
    int active;
} SmokeScreen;

#define MAX_SMOKE_SCREENS 100
SmokeScreen player1Smokes[MAX_SMOKE_SCREENS];
SmokeScreen player2Smokes[MAX_SMOKE_SCREENS];
int smokeCount1 = 0;
int smokeCount2 = 0;

void gridEmpty(char grid[ROWS][COLUMNS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            grid[i][j] = '-';
        }
    }
}

void displayGrid(char grid[ROWS][COLUMNS], int showShips) {
    printf("\n   ");
    for (char col = 'A'; col < 'A' + COLUMNS; col++) {
        printf("%c ", col);
    }
    printf("\n");

    for (int i = 0; i < ROWS; i++) {
        printf("%2d ", i + 1);
        for (int j = 0; j < COLUMNS; j++) {
            char displayChar = grid[i][j];
            if (!showShips && displayChar != 'o' && displayChar != '*') {
                printf("- ");
            } else {
                printf("%c ", displayChar);
            }
        }
        printf("\n");
    }
}

void displayPlayerGrid(char trackingGrid[ROWS][COLUMNS]) {
    printf("\nYour Grid:\n");
    displayGrid(trackingGrid, 0);
}

void displayOpponentGrid(char grid[ROWS][COLUMNS]) {
    printf("\nOpponent's Grid (Tracking):\n");
    displayGrid(grid, 0);
}

int letterToIndex(char letter) {
    return toupper(letter) - 'A';
}

int placeShip(char grid[ROWS][COLUMNS], Ship *ship, char orientation, int row, int col) {
    if (orientation == 'h') {
        if (col + ship->size > COLUMNS) return 0;
        for (int i = 0; i < ship->size; i++) {
            if (grid[row][col + i] != '-') return 0;
        }
        for (int i = 0; i < ship->size; i++) {
            grid[row][col + i] = ship->symbol;
        }
    } else if (orientation == 'v') {
        if (row + ship->size > ROWS) return 0;
        for (int i = 0; i < ship->size; i++) {
            if (grid[row + i][col] != '-') return 0;
        }
        for (int i = 0; i < ship->size; i++) {
            grid[row + i][col] = ship->symbol;
        }
    } else {
        return 0;
    }
    return 1;
}

int checkIfSunk(Ship *ship) {
    return ship->hits >= ship->size;
}

int allShipsSunk(Ship ships[], int count) {
    for (int i = 0; i < count; i++) {
        if (!checkIfSunk(&ships[i])) return 0;
    }
    return 1;
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int isAreaSmoked(int row, int col, int currentPlayer) {
    SmokeScreen *smokes = currentPlayer == 0 ? player2Smokes : player1Smokes;
    int count = currentPlayer == 0 ? smokeCount2 : smokeCount1;

    for (int s = 0; s < count; s++) {
        if (smokes[s].active) {
            for (int i = row; i <= row + 1; i++) {
                for (int j = col; j <= col + 1; j++) {
                    if (i >= smokes[s].row && i <= smokes[s].row + 1 &&
                        j >= smokes[s].col && j <= smokes[s].col + 1) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

void torpedoAttack(char opponentGrid[ROWS][COLUMNS], char playerTrackingGrid[ROWS][COLUMNS],
                  Ship opponentShips[], int numShips, char target, int isRow, Player *player, Player *opponent) {
    int hit = 0;
    int targetIndex = isRow ? (target - '1') : letterToIndex(target);
    printf("\nTorpedo targeting %s %c...\n", isRow ? "row" : "column", target);

    for (int i = 0; i < (isRow ? COLUMNS : ROWS); i++) {
        int row = isRow ? targetIndex : i;
        int col = isRow ? i : targetIndex;
        char currentCell = opponentGrid[row][col];
        if (currentCell != '-' && currentCell != 'o' && currentCell != '*') {
            hit = 1;
            opponentGrid[row][col] = '*';
            playerTrackingGrid[row][col] = '*';
            for (int s = 0; s < numShips; s++) {
                if (opponentShips[s].symbol == currentCell) {
                    opponentShips[s].hits++;
                    if (checkIfSunk(&opponentShips[s])) {
                        printf("You sunk the %s!\n", opponentShips[s].name);
                        if (allShipsSunk(opponentShips, numShips)) {
                            printf("\nAll ships of %s have been sunk! %s wins!\n", opponent->playerName, player->playerName);
                            exit(0);
                        }
                    }
                    break;
                }
            }
        } else if (playerTrackingGrid[row][col] != '*') {
            playerTrackingGrid[row][col] = 'o';
        }
    }
    if (hit) {
        printf("Torpedo hit confirmed!\n");
    } else {
        printf("Torpedo missed all targets.\n");
    }
    player->torpedoAvailable = 0;
}

void artilleryAttack(char opponentGrid[ROWS][COLUMNS], char playerTrackingGrid[ROWS][COLUMNS],
                     Ship opponentShips[], int numShips, int row, int col, Player *player, Player *opponent) {
    int hit = 0;
    printf("\nArtillery targeting area around %c%d...\n", col + 'A', row + 1);
    for (int i = row; i <= row + 1 && i < ROWS; i++) {
        for (int j = col; j <= col + 1 && j < COLUMNS; j++) {
            if (i < 0 || j < 0) continue;
            char target = opponentGrid[i][j];
            if (target != '-' && target != 'o' && target != '*') {
                hit = 1;
                opponentGrid[i][j] = '*';
                playerTrackingGrid[i][j] = '*';
                for (int s = 0; s < numShips; s++) {
                    if (opponentShips[s].symbol == target) {
                        opponentShips[s].hits++;
                        if (checkIfSunk(&opponentShips[s])) {
                            printf("You sunk the %s!\n", opponentShips[s].name);
                            if (allShipsSunk(opponentShips, numShips)) {
                                printf("\nAll ships of %s have been sunk! %s wins!\n", opponent->playerName, player->playerName);
                                exit(0);
                            }
                        }
                        break;
                    }
                }
            } else if (playerTrackingGrid[i][j] != '*') {
                playerTrackingGrid[i][j] = 'o';
            }
        }
    }
    if (hit) {
        printf("Artillery hit confirmed!\n");
    } else {
        printf("Artillery missed all targets.\n");
    }
    player->artilleryAvailable = 0;
}

void radarSweep(char grid[ROWS][COLUMNS], int row, int col, int currentPlayer) {
    int found = 0;

    if (isAreaSmoked(row, col, currentPlayer)) {
        printf("There are no ships found.\n");
        return;
    }

    for (int i = row; i <= row + 1 && i < ROWS; i++) {
        for (int j = col; j <= col + 1 && j < COLUMNS; j++) {
            if (i < 0 || j < 0) continue;

            if (grid[i][j] != '-' && grid[i][j] != 'o' && grid[i][j] != '*') {
                found = 1;
                break;
            }
        }
        if (found) break;
    }

    if (found) {
        printf("Enemy ships found in the specified area.\n");
    } else {
        printf("No enemy ships found in the specified area.\n");
    }
}

void smokeScreen(int row, int col, int currentPlayer) {
    if (currentPlayer == 0) {
        if (smokeCount1 < MAX_SMOKE_SCREENS) {
            player1Smokes[smokeCount1].row = row;
            player1Smokes[smokeCount1].col = col;
            player1Smokes[smokeCount1].active = 1;
            smokeCount1++;
        }
    } else {
        if (smokeCount2 < MAX_SMOKE_SCREENS) {
            player2Smokes[smokeCount2].row = row;
            player2Smokes[smokeCount2].col = col;
            player2Smokes[smokeCount2].active = 1;
            smokeCount2++;
        }
    }
    printf("Smoke screen deployed successfully.\n");
}

void humanPlayTurn(Player *player, Player *opponent, char opponentGrid[ROWS][COLUMNS], Ship opponentShips[], 
              int numShips, char playerTrackingGrid[ROWS][COLUMNS], int currentPlayer) {
    if (player->nextTurnArtillery) {
        player->artilleryAvailable = 1;
        player->nextTurnArtillery = 0;
    } else {
        player->artilleryAvailable = 0;
    }

    if (player->nextTurnTorpedo) {
        player->torpedoAvailable = 1;
        player->nextTurnTorpedo = 0;
    }

    char command[10], target;
    int row, col;
    printf("\n%s's turn.\n", player->playerName);
    displayOpponentGrid(playerTrackingGrid);
    printf("\nAvailable smoke screens: %d (Must sink ships to earn more)\n", 
           player->shipsSunk - player->smokeScreensUsed);
    if (player->artilleryAvailable) {
        printf("Artillery strike available!\n");
    }
    if (player->torpedoAvailable) {
        printf("Torpedo attack available!\n");
    }
    printf("Enter command (Fire B3, Radar B3, Smoke B3, Artillery B3, or Torpedo B/3): ");

    char input[20];
    fgets(input, sizeof(input), stdin);

    if (strncmp(input, "Torpedo", 7) == 0) {
        if (player->torpedoAvailable) {
            target = input[8];
            int isRow = isdigit(target);
            if ((!isRow && (target < 'A' || target >= 'A' + COLUMNS)) ||
                (isRow && (target < '1' || target >= '1' + ROWS))) {
                printf("Invalid torpedo target.\n");
                return;
            }
            torpedoAttack(opponentGrid, playerTrackingGrid, opponentShips, numShips, target, isRow, player, opponent);
        } else {
            printf("Torpedo not available. Sink three enemy ships to unlock it!\n");
            return;
        }
    } else if (sscanf(input, "%s %c%d", command, &target, &row) == 3) {
        col = letterToIndex(target);
        row -= 1;

        if (row < 0 || row >= ROWS || col < 0 || col >= COLUMNS) {
            printf("Invalid coordinates. Row must be 1-%d and column must be A-%c.\n", 
                   ROWS, 'A' + COLUMNS - 1);
            return;
        }

        if (strcmp(command, "Artillery") == 0) {
            if (player->artilleryAvailable) {
                artilleryAttack(opponentGrid, playerTrackingGrid, opponentShips, numShips, row, col, player, opponent);
            } else {
                printf("Artillery strike not available. Sink an enemy ship to unlock it!\n");
                return;
            }
        } else if (strcmp(command, "Smoke") == 0) {
            if (player->smokeScreensUsed < player->shipsSunk) {
                smokeScreen(row, col, currentPlayer);
                player->smokeScreensUsed++;
            } else {
                printf("No smoke screens available. You need to sink more ships first.\n");
            }
        } else if (strcmp(command, "Radar") == 0) {
            radarSweep(opponentGrid, row, col, currentPlayer);
        } else if (strcmp(command, "Fire") == 0) {
            if (playerTrackingGrid[row][col] == '*' || playerTrackingGrid[row][col] == 'o') {
                printf("You already fired at %c%d. Try a different cell.\n", target, row + 1);
                return;
            }

            char targetCell = opponentGrid[row][col];
            if (targetCell != '-' && targetCell != 'o' && targetCell != '*') {
                printf("Hit at %c%d!\n", target, row + 1);
                opponentGrid[row][col] = '*';
                playerTrackingGrid[row][col] = '*';

                for (int i = 0; i < numShips; i++) {
                    if (opponentShips[i].symbol == targetCell) {
                        opponentShips[i].hits++;
                        if (checkIfSunk(&opponentShips[i])) {
                            printf("You sunk the %s!\n", opponentShips[i].name);
                            player->shipsSunk++;
                            player->nextTurnArtillery = 1;

                            if (player->shipsSunk == 3) {
                                player->nextTurnTorpedo = 1;
                                printf("\nYou have unlocked the Torpedo attack for your next turn!\n");
                            }
                            
                            if (allShipsSunk(opponentShips, numShips)) {
                                printf("\nAll ships of %s have been sunk! %s wins!\n", opponent->playerName, player->playerName);
                                exit(0);
                            }
                        }
                        break;
                    }
                }
            } else {
                printf("Miss at %c%d.\n", target, row + 1);
                playerTrackingGrid[row][col] = 'o';
            }
        }
    } else {
        printf("Invalid command. Use 'Fire B3', 'Radar B3', 'Smoke B3', 'Artillery B3', or 'Torpedo B/3'.\n");
        return;
    }

    printf("\nPress Enter to continue...");
    getchar();
    clearScreen();
}

void askForShipPlacements(char* playerName, char grid[ROWS][COLUMNS], Ship ships[], int count) {
    printf("\n%s, place your ships.\n", playerName);
    printf("Format: [Column][Row] [Orientation]\n");
    printf("Example: B3 h (for horizontal) or B3 v (for vertical)\n\n");
    
    for (int i = 0; i < count; i++) {
        ships[i].hits = 0;
        int shipPlaced = 0;
        char coord[3], orientation;
        int row, col;

        while (!shipPlaced) {
            displayGrid(grid, 1);
            printf("\nPlace your %s (length: %d): ", ships[i].name, ships[i].size);
            
            if (scanf(" %c%d %c", &coord[0], &row, &orientation) != 3) {
                printf("Invalid input format. Try again.\n");
                while (getchar() != '\n');
                continue;
            }
            
            col = letterToIndex(coord[0]);
            row -= 1;
            orientation = tolower(orientation);

            if (row < 0 || row >= ROWS || col < 0 || col >= COLUMNS ||
                (orientation != 'h' && orientation != 'v')) {
                printf("Invalid placement. Check coordinates and orientation.\n");
                continue;
            }

            shipPlaced = placeShip(grid, &ships[i], orientation, row, col);
            if (!shipPlaced) {
                printf("Can't place ship there. Check boundaries and overlapping.\n");
            }
        }
        clearScreen();
    }
}

void placeShipsForBot(char grid[ROWS][COLUMNS], Ship ships[], int numShips) {
    int placed, row, col;
    char orientation;
    for (int i = 0; i < numShips; i++) {
        placed = 0;
        while (!placed) {
            row = rand() % ROWS;
            col = rand() % COLUMNS;
            orientation = (rand() % 2) ? 'h' : 'v';

            if (placeShip(grid, &ships[i], orientation, row, col)) {
                placed = 1;
            }
        }
    }
}
void botPlayTurn(Player *player, Player *opponent, char opponentGrid[ROWS][COLUMNS], Ship opponentShips[], int numShips, char playerTrackingGrid[ROWS][COLUMNS], int difficulty) {
    int row, col, validShot = 0;
    static int lastHitRow = -1, lastHitCol = -1;
    static int searchDirection = 0;
    static int radarUsed = 0;
    static int radarSweepsUsed = 0;
    static int radarFound = 0;
    static int radarCoords[4][2];
    static int radarTargetIndex = 0;
    static int shipsDestroyed = 0;
    static int radarSweepsRemaining = 3;
    static int artilleryUsedLastTurn = 0;

    printf("\n%s's turn (Bot).\n", player->playerName);

    if (difficulty == 0) {
        if (lastHitRow != -1 && lastHitCol != -1) {
            for (int i = 0; i < 4; i++) {
                int r = lastHitRow, c = lastHitCol;
                switch (searchDirection) {
                    case 0: r--; break;
                    case 1: r++; break;
                    case 2: c--; break;
                    case 3: c++; break;
                }
                searchDirection = (searchDirection + 1) % 4;
                if (r >= 0 && r < ROWS && c >= 0 && c < COLUMNS &&
                    playerTrackingGrid[r][c] != '*' && playerTrackingGrid[r][c] != 'o') {
                    row = r;
                    col = c;
                    validShot = 1;
                    break;
                }
            }
            if (!validShot) {
                lastHitRow = -1;
                lastHitCol = -1;
            }
        }
        if (!validShot) {
            for (int r = 0; r < ROWS; r++) {
                for (int c = (r % 2); c < COLUMNS; c += 2) {
                    if (playerTrackingGrid[r][c] != '*' && playerTrackingGrid[r][c] != 'o') {
                        row = r;
                        col = c;
                        validShot = 1;
                        break;
                    }
                }
                if (validShot) break;
            }
        }

        if (!validShot) {
            while (!validShot) {
                row = rand() % ROWS;
                col = rand() % COLUMNS;
                if (playerTrackingGrid[row][col] != '*' && playerTrackingGrid[row][col] != 'o') {
                    validShot = 1;
                }
            }
        }

        char targetCell = opponentGrid[row][col];
        if (targetCell != '-' && targetCell != 'o' && targetCell != '*') {
            printf("Bot hit at %c%d!\n", col + 'A', row + 1);
            opponentGrid[row][col] = '*';
            playerTrackingGrid[row][col] = '*';
            lastHitRow = row;
            lastHitCol = col;

            for (int i = 0; i < numShips; i++) {
                if (opponentShips[i].symbol == targetCell) {
                    opponentShips[i].hits++;
                    if (checkIfSunk(&opponentShips[i])) {
                        printf("Bot sunk the %s!\n", opponentShips[i].name);
                        player->shipsSunk++;
                        lastHitRow = -1;
                        lastHitCol = -1;
                        if (allShipsSunk(opponentShips, numShips)) {
                            printf("\nAll ships of %s have been sunk! %s wins!\n", opponent->playerName, player->playerName);
                            exit(0);
                        }
                    }
                    break;
                }
            }
        } else {
            printf("Bot missed at %c%d.\n", col + 'A', row + 1);
            playerTrackingGrid[row][col] = 'o';
            lastHitRow = -1;
            lastHitCol = -1;
        }
        return;
    }
    if (difficulty == 1) {

        if (player->smokeScreensUsed < player->shipsSunk) {
            row = rand() % ROWS;
            col = rand() % COLUMNS;
            printf("Bot chooses to deploy a smoke screen");
            smokeScreen(row, col, 1);
            player->smokeScreensUsed++;
            return;
        }

        if (!radarUsed) {
            row = rand() % (ROWS - 1);
            col = rand() % (COLUMNS - 1);

            printf("Bot chooses to use radar sweep around %c%d.\n", col + 'A', row + 1);
            radarSweep(opponentGrid, row, col, 1);

            radarCoords[0][0] = row; radarCoords[0][1] = col;
            radarCoords[1][0] = row; radarCoords[1][1] = col + 1;
            radarCoords[2][0] = row + 1; radarCoords[2][1] = col;
            radarCoords[3][0] = row + 1; radarCoords[3][1] = col + 1;

            radarFound = 0;
            for (int i = 0; i < 4; i++) {
                int r = radarCoords[i][0];
                int c = radarCoords[i][1];
                if (opponentGrid[r][c] != '-' && opponentGrid[r][c] != 'o' && opponentGrid[r][c] != '*') {
                    radarFound = 1;
                }
            }
            radarUsed = 1;
            radarTargetIndex = 0;
            return;
        }

        if (radarFound) {
            while (radarTargetIndex < 4) {
                row = radarCoords[radarTargetIndex][0];
                col = radarCoords[radarTargetIndex][1];
                radarTargetIndex++;
                if (playerTrackingGrid[row][col] != '*' && playerTrackingGrid[row][col] != 'o') {
                    validShot = 1;
                    break;
                }
            }
            if (!validShot) {
                radarFound = 0;
            }
        }

        if (!radarFound) {
            if (lastHitRow != -1 && lastHitCol != -1) {
                for (int i = 0; i < 4; i++) {
                    int r = lastHitRow, c = lastHitCol;
                    switch (i) {
                        case 0: r--; break;
                        case 1: r++; break;
                        case 2: c--; break;
                        case 3: c++; break;
                    }
                    if (r >= 0 && r < ROWS && c >= 0 && c < COLUMNS &&
                        playerTrackingGrid[r][c] != '*' && playerTrackingGrid[r][c] != 'o') {
                        row = r;
                        col = c;
                        validShot = 1;
                        break;
                    }
                }
                if (!validShot) {
                    lastHitRow = -1;
                    lastHitCol = -1;
                }
            }

            if (!validShot) {
                while (!validShot) {
                    row = rand() % ROWS;
                    col = rand() % COLUMNS;
                    if (playerTrackingGrid[row][col] != '*' && playerTrackingGrid[row][col] != 'o') {
                        validShot = 1;
                    }
                }
            }
        }

        char targetCell = opponentGrid[row][col];
        if (targetCell != '-' && targetCell != 'o' && targetCell != '*') {
            printf("Bot hit at %c%d!\n", col + 'A', row + 1);
            opponentGrid[row][col] = '*';
            playerTrackingGrid[row][col] = '*';
            lastHitRow = row;
            lastHitCol = col;
            for (int i = 0; i < numShips; i++) {
                if (opponentShips[i].symbol == targetCell) {
                    opponentShips[i].hits++;
                    if (checkIfSunk(&opponentShips[i])) {
                        printf("Bot sunk the %s!\n", opponentShips[i].name);
                        player->shipsSunk++;
                        lastHitRow = -1;
                        lastHitCol = -1;
                        if (allShipsSunk(opponentShips, numShips)) {
                            printf("\nAll ships of %s have been sunk! %s wins!\n", opponent->playerName, player->playerName);
                            exit(0);
                        }
                    }
                    break;
                }
            }
        } else {
            printf("Bot missed at %c%d.\n", col + 'A', row + 1);
            playerTrackingGrid[row][col] = 'o';
            lastHitRow = -1;
            lastHitCol = -1;
        }
        return;
    }
    
    if (difficulty == 2) {

        if (radarSweepsUsed < 3 && !radarFound) {
            row = rand() % (ROWS - 1);
            col = rand() % (COLUMNS - 1);
            printf("Bot chooses to use radar sweep around %c%d.\n", col + 'A', row + 1);
            radarSweep(opponentGrid, row, col, 1);

            radarCoords[0][0] = row; radarCoords[0][1] = col;
            radarCoords[1][0] = row; radarCoords[1][1] = col + 1;
            radarCoords[2][0] = row + 1; radarCoords[2][1] = col;
            radarCoords[3][0] = row + 1; radarCoords[3][1] = col + 1;

            radarFound = 0;
            for (int i = 0; i < 4; i++) {
                int r = radarCoords[i][0];
                int c = radarCoords[i][1];
                if (opponentGrid[r][c] != '-' && opponentGrid[r][c] != 'o' && opponentGrid[r][c] != '*') {
                    radarFound = 1;
                }
            }
            radarSweepsUsed++;
            radarTargetIndex = 0;
            if (radarFound) {
                printf("Radar detected enemy ships! Preparing to target the area.\n");
            } else {
                printf("No ships detected by radar.\n");
            }
            return;
        }
        if (shipsDestroyed < player->shipsSunk) {
            shipsDestroyed = player->shipsSunk;
            row = rand() % (ROWS - 1);
            col = rand() % (COLUMNS - 1);
            printf("Bot chooses to use artillery at %c%d.\n", col + 'A', row + 1);
            artilleryAttack(opponentGrid, playerTrackingGrid, opponentShips, numShips, row, col, player, opponent);
            return;
        }

        int opponentShipsRemaining = 0;
        for (int i = 0; i < numShips; i++) {
            if (!checkIfSunk(&opponentShips[i])) {
                opponentShipsRemaining++;
            }
        }
        if (opponentShipsRemaining == 1 && player->torpedoAvailable) {
            char target;
            int isRow = rand() % 2;
            int index = isRow ? rand() % ROWS : rand() % COLUMNS;
            target = isRow ? ('1' + index) : ('A' + index);
            printf("Bot chooses to use a torpedo targeting %s %c.\n", isRow ? "row" : "column", target);
            torpedoAttack(opponentGrid, playerTrackingGrid, opponentShips, numShips, target, isRow, player, opponent);
            return;
        }

        if (radarFound) {
            while (radarTargetIndex < 4) {
                row = radarCoords[radarTargetIndex][0];
                col = radarCoords[radarTargetIndex][1];
                radarTargetIndex++;
                if (playerTrackingGrid[row][col] != '*' && playerTrackingGrid[row][col] != 'o') {
                    validShot = 1;
                    break;
                }
            }
            if (!validShot) {
                radarFound = 0;
            }
        }

        if (!validShot && lastHitRow != -1 && lastHitCol != -1) {
            for (int i = 0; i < 4; i++) {
                int r = lastHitRow, c = lastHitCol;
                switch (i) {
                    case 0: r--; break;
                    case 1: r++; break;
                    case 2: c--; break;
                    case 3: c++; break;
                }
                if (r >= 0 && r < ROWS && c >= 0 && c < COLUMNS &&
                    playerTrackingGrid[r][c] != '*' && playerTrackingGrid[r][c] != 'o') {
                    row = r;
                    col = c;
                    validShot = 1;
                    break;
                }
            }
            if (!validShot) {
                lastHitRow = -1;
                lastHitCol = -1;
            }
        }
        if (!validShot) {
            while (!validShot) {
                row = rand() % ROWS;
                col = rand() % COLUMNS;
                if (playerTrackingGrid[row][col] != '*' && playerTrackingGrid[row][col] != 'o') {
                    validShot = 1;
                }
            }
        }
        char targetCell = opponentGrid[row][col];
        if (targetCell != '-' && targetCell != 'o' && targetCell != '*') {
            printf("Bot hit at %c%d!\n", col + 'A', row + 1);
            opponentGrid[row][col] = '*';
            playerTrackingGrid[row][col] = '*';
            lastHitRow = row;
            lastHitCol = col;

            for (int i = 0; i < numShips; i++) {
                if (opponentShips[i].symbol == targetCell) {
                    opponentShips[i].hits++;
                    if (checkIfSunk(&opponentShips[i])) {
                        printf("Bot sunk the %s!\n", opponentShips[i].name);
                        player->shipsSunk++;
                        lastHitRow = -1;
                        lastHitCol = -1;
                        if (allShipsSunk(opponentShips, numShips)) {
                            printf("\nAll ships of %s have been sunk! %s wins!\n", opponent->playerName, player->playerName);
                            exit(0);
                        }
                    }
                    break;
                }
            }
        } else {
            printf("Bot missed at %c%d.\n", col + 'A', row + 1);
            playerTrackingGrid[row][col] = 'o';
            lastHitRow = -1;
            lastHitCol = -1;
        }
    }
}

int main() {
    char grid1[ROWS][COLUMNS], grid2[ROWS][COLUMNS];
    char trackingGrid1[ROWS][COLUMNS], trackingGrid2[ROWS][COLUMNS];
    
    Ship ships1[] = {
        {"Carrier", 'C', 5, 0},
        {"Battleship", 'B', 4, 0},
        {"Destroyer", 'D', 3, 0},
        {"Submarine", 'S', 2, 0}
    };

    Ship ships2[] = {
        {"Carrier", 'C', 5, 0},
        {"Battleship", 'B', 4, 0},
        {"Destroyer", 'D', 3, 0},
        {"Submarine", 'S', 2, 0}
    };
    int numShips = sizeof(ships1) / sizeof(ships1[0]);
    Player player1 = {"Player 1", 0, 0, 0, 0, 0, 0, 0};
    Player player2 = {"AI Bot", 0, 0, 0, 0, 0, 0, 0};
    int difficulty;

    srand(time(NULL));
    printf("=== BATTLESHIP GAME ===\n\n");
    printf("Enter Player 1's name: ");
    fgets(player1.playerName, sizeof(player1.playerName), stdin);
    player1.playerName[strcspn(player1.playerName, "\n")] = 0;
    printf("\nChoose difficulty level for the bot:\n");
    printf("0 - Easy (Random moves)\n");
    printf("1 - Medium (Smarter targeting)\n");
    printf("2 - Hard (Hunt-and-target strategy)\n");
    printf("Selection: ");
    scanf("%d", &difficulty);
    while (difficulty < 0 || difficulty > 2) {
        printf("Invalid selection. Please choose 0, 1, or 2: ");
        scanf("%d", &difficulty);
    }
    while (getchar() != '\n');

    gridEmpty(grid1);
    gridEmpty(grid2);
    gridEmpty(trackingGrid1);
    gridEmpty(trackingGrid2);

    clearScreen();
    askForShipPlacements(player1.playerName, grid1, ships1, numShips);
    placeShipsForBot(grid2, ships2, numShips);

    int currentPlayer = rand() % 2;
    printf("\n%s goes first!\n", (currentPlayer == 0) ? player1.playerName : "Bot");
    getchar();
    while (1) {
        clearScreen();
        displayPlayerGrid(currentPlayer == 0 ? trackingGrid1 : trackingGrid2);
        displayOpponentGrid(currentPlayer == 0 ? grid1 : grid2);
        if (currentPlayer == 0) {
            humanPlayTurn(&player1, &player2, grid2, ships2, numShips, trackingGrid1, currentPlayer);
            currentPlayer = 1;
        } else {
            botPlayTurn(&player2, &player1, grid1, ships1, numShips, trackingGrid2, difficulty);
            currentPlayer = 0;
        }
        printf("\nPress Enter to continue...");
        getchar();
    }
    return 0;
}