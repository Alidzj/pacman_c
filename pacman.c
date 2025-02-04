// https://www.geeksforgeeks.org/pacman-game-in-c/

// Pacman Game in C language
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>

// All the elements to be used
// Declared here
#define WIDTH 40
#define HEIGHT 20
#define PACMAN 'C'
#define WALL '#'
#define FOOD '.'
#define EMPTY ' '
#define DEMON 'X'

// Global Variables are
// Declared here
int res = 0;
int score = 0;
int pacman_x, pacman_y;
char board[HEIGHT][WIDTH];
int food = 0;
int curr = 0;

// Function to save the current game state to a binary file
void saveGame() {
    FILE *file = fopen("saved_game.bin", "wb"); // Open file in binary write mode
    if (file == NULL) {
        printf("Error saving game!\n");
        return;
    }

    // Save Pacman's position and game variables
    fwrite(&pacman_x, sizeof(int), 1, file);
    fwrite(&pacman_y, sizeof(int), 1, file);
    fwrite(&score, sizeof(int), 1, file);
    fwrite(&food, sizeof(int), 1, file);
    fwrite(&curr, sizeof(int), 1, file);

    // Save the game board
    fwrite(board, sizeof(char), HEIGHT * WIDTH, file);

    fclose(file); // Close the file
    printf("Game saved successfully!\n");
}

// Function to load the game state from a binary file
int loadGame() {
    FILE *file = fopen("saved_game.bin", "rb"); // Open file in binary read mode
    if (file == NULL) {
        printf("No saved game found.\n");
        return 0; // No saved game exists
    }

    // Load Pacman's position and game variables
    fread(&pacman_x, sizeof(int), 1, file);
    fread(&pacman_y, sizeof(int), 1, file);
    fread(&score, sizeof(int), 1, file);
    fread(&food, sizeof(int), 1, file);
    fread(&curr, sizeof(int), 1, file);

    // Load the game board
    fread(board, sizeof(char), HEIGHT * WIDTH, file);

    fclose(file); // Close the file
    printf("Game loaded successfully!\n");
    return 1; // Game loaded successfully
}
void initialize()
{
    // Putting Walls as boundary in the Game
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            if (i == 0 || j == WIDTH - 1 || j == 0 || i == HEIGHT - 1)
            {
                board[i][j] = WALL;
            }
            else
                board[i][j] = EMPTY;
        }
    }

    // Putting Walls inside the Game
    int count = 50;
    while (count != 0)
    {
        int i = (rand() % (HEIGHT + 1));
        int j = (rand() % (WIDTH + 1));

        if (board[i][j] != WALL && board[i][j] != PACMAN)
        {
            board[i][j] = WALL;
            count--;
        }
    }

    int val = 5;
    while (val--)
    {
        int row = (rand() % (HEIGHT + 1));
        for (int j = 3; j < WIDTH - 3; j++)
        {
            if (board[row][j] != WALL && board[row][j] != PACMAN)
            {
                board[row][j] = WALL;
            }
        }
    }

    // Putting Demons in the Game
    count = 10;
    while (count != 0)
    {
        int i = (rand() % (HEIGHT + 1));
        int j = (rand() % (WIDTH + 1));

        if (board[i][j] != WALL && board[i][j] != PACMAN)
        {
            board[i][j] = DEMON;
            count--;
        }
    }

    // Cursor at Center
    pacman_x = WIDTH / 2;
    pacman_y = HEIGHT / 2;
    board[pacman_y][pacman_x] = PACMAN;

    // Points Placed
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            if (i % 2 == 0 && j % 2 == 0 && board[i][j] != WALL && board[i][j] != DEMON && board[i][j] != PACMAN)
            {

                board[i][j] = FOOD;
                food++;
            }
        }
    }
}

void draw()
{
    // Clear screen
    system("cls");

    // Drawing All the elements in the screen
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            printf("%c", board[i][j]);
        }
        printf("\n");
    }
    printf("Score: %d\n", score);
}

// Function enables to move the Cursor
void move(int move_x, int move_y)
{
    int x = pacman_x + move_x;
    int y = pacman_y + move_y;

    if (board[y][x] != WALL)
    {
        if (board[y][x] == FOOD)
        {
            score++;
            food--;
            curr++;
            if (food == 0)
            {
                res = 2;
                return;
            }
        }
        else if (board[y][x] == DEMON)
        {
            res = 1;
        }

        board[pacman_y][pacman_x] = EMPTY;
        pacman_x = x;
        pacman_y = y;
        board[pacman_y][pacman_x] = PACMAN;
    }
}

// Main Function
int main() {
    char ch;
    int totalFood;

    // Check if a saved game file exists
    FILE *file = fopen("saved_game.bin", "rb");
    if (file != NULL) {
        fclose(file);
        printf("A saved game exists. Do you want to continue? (Y/N): ");
        ch = getch();
        if (ch == 'Y' || ch == 'y') {
            if (loadGame()) {
                totalFood = food; // Set total food count
                draw(); // Draw the loaded game state
            } else {
                // If the saved game is corrupted, start a new game
                initialize();
                totalFood = food - 35;
                remove("saved_game.bin"); // Delete the corrupted save file
            }
        } else {
            // If the user chooses to start a new game, delete the saved file
            initialize();
            totalFood = food - 35;
            remove("saved_game.bin"); // Delete the saved file
        }
    } else {
        // If no saved game exists, start a new game
        initialize();
        totalFood = food - 35;
    }

    // Display game instructions
    printf("Use buttons for w(up), a(left), d(right), and s(down)\n");
    printf("Press 'q' to quit or 'p' to save and quit.\n");

    while (1) {
        draw(); // Draw the current game state
        printf("Total Food count: %d\n", totalFood);
        printf("Total Food eaten: %d\n", curr);

        // Check if the game is over (Pacman died)
        if (res == 1) {
            system("cls");
            printf("Game Over! Dead by Demon\n Your Score: %d\n", score);
            return 1;
        }

        // Check if the game is won (all food collected)
        if (res == 2) {
            system("cls");
            printf("You Win! \n Your Score: %d\n", score);
            return 1;
        }

        // Get user input
        ch = getch();

        // Handle user input
        switch (ch) {
            case 'w': // Move up
                move(0, -1);
                break;
            case 's': // Move down
                move(0, 1);
                break;
            case 'a': // Move left
                move(-1, 0);
                break;
            case 'd': // Move right
                move(1, 0);
                break;
            case 'q': // Quit the game
                printf("Game Over! Your Score: %d\n", score);
                return 0;
            case 'p': // Save and quit
                saveGame();
                printf("Game saved. Exiting...\n");
                return 0;
        }
    }

    return 0;
}