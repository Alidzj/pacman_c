#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Color support (ANSI escape codes)
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

// All the elements to be used
#define WIDTH 40
#define HEIGHT 20

// Define a struct for each cell
typedef struct
{
    char type;
    int value;
} Cell;

// Constants for cell types
#define PACMAN 'C'
#define WALL '#'
#define FOOD '.'
#define EMPTY ' '
#define DEMON 'X'
#define ENEMY 'E'
#define BOOST '$'

// Global Variables
int res = 0;
int score = 0;       // Total score, does NOT reset
int level_score = 0; // Score for current level
int pacman_x, pacman_y;
Cell board[HEIGHT][WIDTH];
int food = 0;
int curr = 0;
int level = 1;
int computer_mode = 0;
int stop_thread = 0;
int enemy_speed = 1;
int boost_moves = 0;
int is_boost_active = 0;
pthread_mutex_t board_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for board access

// Function to save the current game state
void saveGame()
{
    FILE *file = fopen("saved_game.bin", "wb");
    if (file == NULL)
    {
        printf("Error saving game!\n");
        return;
    }

    fwrite(&level, sizeof(int), 1, file);
    fwrite(&pacman_x, sizeof(int), 1, file);
    fwrite(&pacman_y, sizeof(int), 1, file);
    fwrite(&score, sizeof(int), 1, file);
    fwrite(&food, sizeof(int), 1, file);
    fwrite(&curr, sizeof(int), 1, file);
    fwrite(&boost_moves, sizeof(int), 1, file);
    fwrite(&is_boost_active, sizeof(int), 1, file); // Save boost status
    fwrite(board, sizeof(Cell), HEIGHT * WIDTH, file);

    fclose(file);
    printf("Game saved successfully!\n");
}

// Function to load the game state
int loadGame()
{
    FILE *file = fopen("saved_game.bin", "rb");
    if (file == NULL)
    {
        printf("No saved game found.\n");
        return 0;
    }

    fread(&level, sizeof(int), 1, file);
    fread(&pacman_x, sizeof(int), 1, file);
    fread(&pacman_y, sizeof(int), 1, file);
    fread(&score, sizeof(int), 1, file);
    fread(&food, sizeof(int), 1, file);
    fread(&curr, sizeof(int), 1, file);
    fread(&boost_moves, sizeof(int), 1, file);
    fread(&is_boost_active, sizeof(int), 1, file); // Load boost status
    fread(board, sizeof(Cell), HEIGHT * WIDTH, file);

    fclose(file);
    printf("Game loaded successfully!\n");
    return 1;
}

// Flood fill algorithm to check if all food is accessible
int is_all_food_accessible(int start_x, int start_y)
{
    Cell temp_board[HEIGHT][WIDTH];
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            temp_board[i][j] = board[i][j];
        }
    }
    int accessible_food = 0;
    int queue_x[WIDTH * HEIGHT];
    int queue_y[WIDTH * HEIGHT];
    int head = 0, tail = 0;

    queue_x[tail] = start_x;
    queue_y[tail] = start_y;
    tail++;

    temp_board[start_y][start_x].type = 'V'; // Mark as visited

    while (head != tail)
    {
        int x = queue_x[head];
        int y = queue_y[head];
        head++;

        // Check adjacent cells
        int dx[] = {0, 0, 1, -1};
        int dy[] = {1, -1, 0, 0};

        for (int i = 0; i < 4; i++)
        {
            int new_x = x + dx[i];
            int new_y = y + dy[i];

            if (new_x >= 0 && new_x < WIDTH && new_y >= 0 && new_y < HEIGHT && temp_board[new_y][new_x].type != WALL && temp_board[new_y][new_x].type != 'V')
            {
                if (temp_board[new_y][new_x].type == FOOD)
                {
                    accessible_food++;
                }
                queue_x[tail] = new_x;
                queue_y[tail] = new_y;
                tail++;
                temp_board[new_y][new_x].type = 'V'; // Mark as visited
            }
        }
    }
    return accessible_food == food;
}

void generate_random_map()
{
    // Fill with empty
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            board[i][j].type = EMPTY;
            board[i][j].value = 0;
        }
    }

    // Add borders
    for (int i = 0; i < HEIGHT; i++)
    {
        board[i][0].type = WALL;
        board[i][WIDTH - 1].type = WALL;
    }
    for (int j = 0; j < WIDTH; j++)
    {
        board[0][j].type = WALL;
        board[HEIGHT - 1][j].type = WALL;
    }

    // Add some guaranteed wall structures for better maze-like appearance
    for (int i = 2; i < HEIGHT - 2; i += 4)
    {
        for (int j = 2; j < WIDTH - 2; j += 4)
        {
            // Create small boxes of walls
            board[i][j].type = WALL;
            board[i + 1][j].type = WALL;
            board[i][j + 1].type = WALL;
            board[i + 1][j + 1].type = WALL;
        }
    }

    // Randomly generate walls with longer structures
    for (int i = 1; i < HEIGHT - 1; i++)
    {
        for (int j = 1; j < WIDTH - 1; j++)
        {
            if (rand() % 10 == 0) // Adjust probability for wall density
            {
                // Create horizontal wall
                int wall_length = rand() % 5 + 2; // Random length 2-6
                for (int k = 0; k < wall_length && j + k < WIDTH - 1; k++)
                {
                    board[i][j + k].type = WALL;
                    board[i][j + k].value = 0;
                }
                j += wall_length - 1; // Skip the wall just created
            }
            else if (rand() % 10 == 0)
            {
                // Create vertical wall
                int wall_length = rand() % 4 + 2; // Random length 2-5
                for (int k = 0; k < wall_length && i + k < HEIGHT - 1; k++)
                {
                    board[i + k][j].type = WALL;
                    board[i + k][j].value = 0;
                }
                i += wall_length - 1; // Skip the wall just created
            }
        }
    }
}

void initialize()
{
    pthread_mutex_lock(&board_mutex); // Lock before modifying the board
    food = 0;
    level_score = 0;     // Reset level score at the start of each level
    is_boost_active = 0; // Deactivate boost at level start
    boost_moves = 0;

    // Generate the map until all food is accessible
    do
    {
        generate_random_map();
        // Place food
        food = 0;
        for (int i = 1; i < HEIGHT - 1; i++)
        {
            for (int j = 1; j < WIDTH - 1; j++)
            {
                if (board[i][j].type == EMPTY)
                {
                    if (rand() % 2 == 0) // Reduce the amount of foods
                    {
                        board[i][j].type = FOOD;
                        board[i][j].value = 1;
                        food++;
                    }
                }
            }
        }

        // Place boosts (approximately 1 boost per level)
        if (rand() % 2 == 0) // 50% chance of boost appearing each level.  adjust for balance
        {
            int i, j;
            do
            {
                i = (rand() % (HEIGHT - 2)) + 1;
                j = (rand() % (WIDTH - 2)) + 1;
            } while (board[i][j].type != EMPTY);
            board[i][j].type = BOOST;
            board[i][j].value = 0;
        }

        pacman_x = WIDTH / 2;
        pacman_y = HEIGHT / 2;
        board[pacman_y][pacman_x].type = PACMAN;
        board[pacman_y][pacman_x].value = 0;
    } while (!is_all_food_accessible(pacman_x, pacman_y));

    // Add demons
    int demonCount = 1 + (level / 3);
    for (int k = 0; k < demonCount; k++)
    {
        int i, j;
        do
        {
            i = (rand() % (HEIGHT - 2)) + 1;
            j = (rand() % (WIDTH - 2)) + 1;
        } while (board[i][j].type != EMPTY);
        board[i][j].type = DEMON;
    }

    // Add enemies
    int enemyCount = 1 + (level / 3);
    for (int k = 0; k < enemyCount; k++)
    {
        int i, j;
        do
        {
            i = (rand() % (HEIGHT - 2)) + 1;
            j = (rand() % (WIDTH - 2)) + 1;
        } while (board[i][j].type != EMPTY);
        board[i][j].type = ENEMY;
    }

    pthread_mutex_unlock(&board_mutex); // Unlock after modifying the board
}

void draw()
{
    pthread_mutex_lock(&board_mutex); // Lock before reading the board

    // Clear screen
    system("cls");

    // Drawing All the elements in the screen
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            switch (board[i][j].type)
            {
            case WALL:
                printf(ANSI_COLOR_BLUE "%c" ANSI_COLOR_RESET, board[i][j].type);
                break;
            case FOOD:
                printf(ANSI_COLOR_YELLOW "%c" ANSI_COLOR_RESET, board[i][j].type);
                break;
            case PACMAN:
                printf(ANSI_COLOR_GREEN "%c" ANSI_COLOR_RESET, board[i][j].type);
                break;
            case DEMON:
                printf(ANSI_COLOR_RED "%c" ANSI_COLOR_RESET, board[i][j].type);
                break;
            case ENEMY:
                printf(ANSI_COLOR_MAGENTA "%c" ANSI_COLOR_RESET, board[i][j].type);
                break;
            case BOOST:
                printf(ANSI_COLOR_CYAN "%c" ANSI_COLOR_RESET, board[i][j].type);
                break;
            default:
                printf("%c", board[i][j].type);
                break;
            }
        }
        printf("\n");
    }
    printf("Score: %d\n", score);             // Print the total score
    printf("Level Score: %d\n", level_score); // Print level score
    printf("Level: %d\n", level);
    printf("Total Food count: %d\n", food);
    printf("Total Food eaten: %d\n", curr);
    printf("Computer mode: %s\n", computer_mode ? "ON" : "OFF");
    if (is_boost_active)
    {
        printf("Boost moves left: %d\n", boost_moves);
    }

    pthread_mutex_unlock(&board_mutex); // Unlock after reading the board
}

// Function enables to move the Cursor
void move(int move_x, int move_y)
{
    int moves = 1;
    if (is_boost_active)
    {
        moves = 2;
    }

    for (int i = 0; i < moves; i++)
    {
        pthread_mutex_lock(&board_mutex); // Lock before modifying the board

        int x = pacman_x + move_x;
        int y = pacman_y + move_y;

        // Boundary check
        if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        {
            pthread_mutex_unlock(&board_mutex);
            return; // Invalid move, early return
        }

        if (board[y][x].type != WALL)
        {
            if (board[y][x].type == FOOD)
            {
                level_score += board[y][x].value; // Add food points to level_score
                score += board[y][x].value;       // Add to total score
                food--;
                curr++;
                board[y][x].type = EMPTY; // Remove food from the board
                board[y][x].value = 0;
            }
            else if (board[y][x].type == BOOST)
            {
                is_boost_active = 1;
                boost_moves = 10; // Activate boost for 10 moves
                board[y][x].type = EMPTY;
                board[y][x].value = 0;
            }
            else if (board[y][x].type == DEMON || board[y][x].type == ENEMY)
            {
                res = 1; // Game over if Pac-Man collides with a demon or enemy
            }

            board[pacman_y][pacman_x].type = EMPTY;
            board[pacman_y][pacman_x].value = 0;
            pacman_x = x;
            pacman_y = y;
            board[pacman_y][pacman_x].type = PACMAN;
            board[pacman_y][pacman_x].value = 0;
        }
        else
        {
            pthread_mutex_unlock(&board_mutex);
            break; // Stop moving if hit a wall
        }
        pthread_mutex_unlock(&board_mutex); // Unlock after modifying the board
    }
    // Decrement boost moves if active

    if (is_boost_active)
    {
        boost_moves--;
        if (boost_moves <= 0)
        {
            is_boost_active = 0;
            boost_moves = 0;
        }
    }
    // Refresh the screen only when there is a change
    draw();
}

// Function to move enemies randomly
void move_enemy(int x, int y)
{
    pthread_mutex_lock(&board_mutex); // Lock before modifying the board

    int direction = rand() % 4; // 0: up, 1: down, 2: left, 3: right
    int new_x = x, new_y = y;

    // Calculate potential new coordinates
    switch (direction)
    {
    case 0:
        new_y--;
        break; // Up
    case 1:
        new_y++;
        break; // Down
    case 2:
        new_x--;
        break; // Left
    case 3:
        new_x++;
        break; // Right
    }

    // Boundary check
    if (new_x < 0 || new_x >= WIDTH || new_y < 0 || new_y >= HEIGHT)
    {
        pthread_mutex_unlock(&board_mutex);
        return; // Invalid move, early return
    }

    // Check if the new position is valid
    if (board[new_y][new_x].type == EMPTY || board[new_y][new_x].type == FOOD || board[new_y][new_x].type == PACMAN) // allow enemies to walk on pacman to kill him
    {
        board[y][x].type = EMPTY;
        board[y][x].value = 0;
        board[new_y][new_x].type = ENEMY;
        board[new_y][new_x].value = 0;
    }
    else if (board[new_y][new_x].type == PACMAN) // Redundant check, but makes logic clear
    {
        // Enemy eats Pac-Man
        res = 1; // Game over
    }

    pthread_mutex_unlock(&board_mutex); // Unlock after modifying the board
}

// Function to handle enemy movements
void *enemy_movement(void *arg)
{
    while (!stop_thread)
    {
        pthread_mutex_lock(&board_mutex);
        for (int i = 1; i < HEIGHT - 1; i++)
        {
            for (int j = 1; j < WIDTH - 1; j++)
            {
                if (board[i][j].type == ENEMY)
                {
                    pthread_mutex_unlock(&board_mutex);
                    move_enemy(j, i);
                    pthread_mutex_lock(&board_mutex);
                }
            }
        }
        pthread_mutex_unlock(&board_mutex);
        draw();
        sleep(enemy_speed); // Wait based on enemy speed
    }
    return NULL;
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

// Function to count remaining food
int count_food()
{
    int count = 0;
    pthread_mutex_lock(&board_mutex);
    for (int i = 1; i < HEIGHT - 1; i++)
    {
        for (int j = 1; j < WIDTH - 1; j++)
        {
            if (board[i][j].type == FOOD)
            {
                count++;
            }
        }
    }
    pthread_mutex_unlock(&board_mutex);
    return count;
}

// Main Function
int main()
{
    char ch;
    int totalFood;
    pthread_t thread_id, enemy_thread_id;

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
                totalFood = food; // correct initialization
                remove("saved_game.bin");
            }
        }
        else
        {
            initialize();
            totalFood = food; // Correct initialization
            remove("saved_game.bin");
        }
    }
    else
    {
        initialize();
        totalFood = food; // correct initialization
    }

    // Create a thread for random movement
    pthread_create(&thread_id, NULL, random_movement, NULL);

    // Create a thread for enemy movement
    pthread_create(&enemy_thread_id, NULL, enemy_movement, NULL);

    // Game instructions
    printf("Use buttons for w(up), a(left), d(right), and s(down)\n");
    printf("Press 'q' to quit or 'p' to save and quit.\n");
    printf("Press 'o' to toggle computer mode.\n");

    while (1)
    {
        if (res == 1)
        {
            system("cls");
            printf("Game Over! Pac-Man was eaten by an enemy!\n Your Score: %d\n", score);
            stop_thread = 1;                     // Stop the random movement thread
            pthread_join(thread_id, NULL);       // Wait for the thread to finish
            pthread_join(enemy_thread_id, NULL); // Wait for the enemy thread to finish
            pthread_mutex_destroy(&board_mutex); // Destroy the mutex before exiting

            return 1;
        }

        food = count_food(); // Update food count

        if (food <= 0)
        {
            level++; // Increment level
            initialize();
            res = 0; // Reset game result
        }

        ch = getch();
        switch (ch)
        {
        case 'w':
            if (!computer_mode)
            {
                move(0, -1);
            }
            break;
        case 's':
            if (!computer_mode)
            {
                move(0, 1);
            }
            break;
        case 'a':
            if (!computer_mode)
            {
                move(-1, 0);
            }
            break;
        case 'd':
            if (!computer_mode)
            {
                move(1, 0);
            }
            break;
        case 'q':
            printf("Game Over! Your Score: %d\n", score);
            stop_thread = 1;                     // Stop the random movement thread
            pthread_join(thread_id, NULL);       // Wait for the thread to finish
            pthread_join(enemy_thread_id, NULL); // Wait for the enemy thread to finish
            pthread_mutex_destroy(&board_mutex); // Destroy the mutex before exiting

            return 0;
        case 'p':
            saveGame();
            printf("Game saved. Exiting...\n");
            stop_thread = 1;                     // Stop the random movement thread
            pthread_join(thread_id, NULL);       // Wait for the thread to finish
            pthread_join(enemy_thread_id, NULL); // Wait for the enemy thread to finish
            pthread_mutex_destroy(&board_mutex); // Destroy the mutex before exiting

            return 0;
        case 'o':
            computer_mode = !computer_mode;
            printf("Computer mode %s\n", computer_mode ? "ON" : "OFF");
            break;
        }
    }

    return 0;
}
