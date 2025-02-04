#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

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
int level = 1;         // Current level
int computer_mode = 0; // 0 for manual mode, 1 for computer mode
int stop_thread = 0;   // Flag to stop the random movement thread

// Function to save the current game state to a binary file
void saveGame()
{
    FILE *file = fopen("saved_game.bin", "wb");
    if (file == NULL)
    {
        printf("Error saving game!\n");
        return;
    }

    // Save game state
    fwrite(&level, sizeof(int), 1, file); // Save current level
    fwrite(&pacman_x, sizeof(int), 1, file);
    fwrite(&pacman_y, sizeof(int), 1, file);
    fwrite(&score, sizeof(int), 1, file);
    fwrite(&food, sizeof(int), 1, file);
    fwrite(&curr, sizeof(int), 1, file);

    // Save the board
    fwrite(board, sizeof(char), HEIGHT * WIDTH, file);

    fclose(file);
    printf("Game saved successfully!\n");
}

// Function to load the game state from a binary file
int loadGame()
{
    FILE *file = fopen("saved_game.bin", "rb");
    if (file == NULL)
    {
        printf("No saved game found.\n");
        return 0;
    }

    // Load game state
    fread(&level, sizeof(int), 1, file); // Load current level
    fread(&pacman_x, sizeof(int), 1, file);
    fread(&pacman_y, sizeof(int), 1, file);
    fread(&score, sizeof(int), 1, file);
    fread(&food, sizeof(int), 1, file);
    fread(&curr, sizeof(int), 1, file);

    // Load the board
    fread(board, sizeof(char), HEIGHT * WIDTH, file);

    fclose(file);
    printf("Game loaded successfully!\n");
    return 1;
}

void initialize()
{
    // Reset the board
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            if (i == 0 || j == WIDTH - 1 || j == 0 || i == HEIGHT - 1)
            {
                board[i][j] = WALL; // Boundary walls
            }
            else
            {
                board[i][j] = EMPTY;
            }
        }
    }

    // Add walls based on the level
    int wallCount = 50 + (level * 5); // Increase walls with level
    while (wallCount != 0)
    {
        int i = (rand() % (HEIGHT - 2)) + 1; // Avoid boundary walls
        int j = (rand() % (WIDTH - 2)) + 1;

        if (board[i][j] != WALL && board[i][j] != PACMAN)
        {
            board[i][j] = WALL;
            wallCount--;
        }
    }

    // Add demons based on the level
    int demonCount = 10 + (level * 2); // Increase demons with level
    while (demonCount != 0)
    {
        int i = (rand() % (HEIGHT - 2)) + 1;
        int j = (rand() % (WIDTH - 2)) + 1;

        if (board[i][j] != WALL && board[i][j] != PACMAN)
        {
            board[i][j] = DEMON;
            demonCount--;
        }
    }

    // Place Pacman at the center
    pacman_x = WIDTH / 2;
    pacman_y = HEIGHT / 2;
    board[pacman_y][pacman_x] = PACMAN;

    // Place food
    food = 0;
    for (int i = 1; i < HEIGHT - 1; i++)
    {
        for (int j = 1; j < WIDTH - 1; j++)
        {
            if (board[i][j] == EMPTY)
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
    printf("Level: %d\n", level);
    printf("Total Food count: %d\n", food);
    printf("Total Food eaten: %d\n", curr);
    printf("Computer mode: %s\n", computer_mode ? "ON" : "OFF");
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
                level++;      // Increment level
                initialize(); // Generate new level
                res = 0;      // Reset game result
            }
        }
        else if (board[y][x] == DEMON)
        {
            res = 1; // Game over
        }

        board[pacman_y][pacman_x] = EMPTY;
        pacman_x = x;
        pacman_y = y;
        board[pacman_y][pacman_x] = PACMAN;

        // Refresh the screen only when there is a change
        draw();
    }
}

// Function to handle random movements in computer mode
void *random_movement(void *arg)
{
    while (!stop_thread)
    {
        if (computer_mode)
        {
            int direction = rand() % 4; // 0: up, 1: down, 2: left, 3: right
            switch (direction)
            {
            case 0:
                move(0, -1);
                break;
            case 1:
                move(0, 1);
                break;
            case 2:
                move(-1, 0);
                break;
            case 3:
                move(1, 0);
                break;
            }
        }
        sleep(1); // Wait for 1 second
    }
    return NULL;
}

// Main Function
int main()
{
    char ch;
    int totalFood;
    pthread_t thread_id;

    // Initialize random seed
    srand(time(NULL));

    // Check if a saved game exists
    FILE *file = fopen("saved_game.bin", "rb");
    if (file != NULL)
    {
        fclose(file);
        printf("A saved game exists. Do you want to continue? (Y/N): ");
        ch = getch();
        if (ch == 'Y' || ch == 'y')
        {
            if (loadGame())
            {
                totalFood = food;
                draw();
            }
            else
            {
                initialize();
                totalFood = food - 35;
                remove("saved_game.bin");
            }
        }
        else
        {
            initialize();
            totalFood = food - 35;
            remove("saved_game.bin");
        }
    }
    else
    {
        initialize();
        totalFood = food - 35;
    }

    // Create a thread for random movement
    pthread_create(&thread_id, NULL, random_movement, NULL);

    // Game instructions
    printf("Use buttons for w(up), a(left), d(right), and s(down)\n");
    printf("Press 'q' to quit or 'p' to save and quit.\n");
    printf("Press 'o' to toggle computer mode.\n");

    while (1)
    {
        if (res == 1)
        {
            system("cls");
            printf("Game Over! Dead by Demon\n Your Score: %d\n", score);
            stop_thread = 1;               // Stop the random movement thread
            pthread_join(thread_id, NULL); // Wait for the thread to finish
            return 1;
        }

        if (res == 2)
        {
            system("cls");
            printf("You Win! \n Your Score: %d\n", score);
            stop_thread = 1;               // Stop the random movement thread
            pthread_join(thread_id, NULL); // Wait for the thread to finish
            return 1;
        }

        ch = getch();
        switch (ch)
        {
        case 'w':
            if (!computer_mode)
                move(0, -1);
            break;
        case 's':
            if (!computer_mode)
                move(0, 1);
            break;
        case 'a':
            if (!computer_mode)
                move(-1, 0);
            break;
        case 'd':
            if (!computer_mode)
                move(1, 0);
            break;
        case 'q':
            printf("Game Over! Your Score: %d\n", score);
            stop_thread = 1;               // Stop the random movement thread
            pthread_join(thread_id, NULL); // Wait for the thread to finish
            return 0;
        case 'p':
            saveGame();
            printf("Game saved. Exiting...\n");
            stop_thread = 1;               // Stop the random movement thread
            pthread_join(thread_id, NULL); // Wait for the thread to finish
            return 0;
        case 'o':
            computer_mode = !computer_mode;
            printf("Computer mode %s\n", computer_mode ? "ON" : "OFF");
            break;
        }
    }

    return 0;
}
