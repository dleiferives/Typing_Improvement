#include <stdio.h>
#include <sys/time.h>
#include <ncurses.h>
#include <unistd.h>
#include <string.h>

#define MAX_INPUT_LEN 500

void initializeScreen();
void finalizeScreen();
void initializeColors();
void handleBackspace(int *current_pos, const char *test_string);
void handleCtrlBackspace(int *current_pos, const char *test_string);
void logTimingData(const char *filename, char key, long time_diff);
void processCharacter(int *current_pos, char ch, const char *test_string);
void runTypingTest(const char *test_string, const char *filename);

void initializeScreen() {
    initscr();
    noecho();
    cbreak();
    initializeColors();
}

void finalizeScreen() {
    endwin();
}

void initializeColors() {
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK); // Green for correct
    init_pair(2, COLOR_RED, COLOR_BLACK);   // Red for incorrect
}

void handleBackspace(int *current_pos, const char *test_string) {
    if (*current_pos > 0) {
        (*current_pos)--;
        mvprintw(0, *current_pos, "%c", test_string[*current_pos]); // Reset color to default
        refresh();
    }
}

void handleCtrlBackspace(int *current_pos, const char *test_string) {
    while (*current_pos > 0 && test_string[*current_pos - 1] != ' ') {
        (*current_pos)--;
        mvprintw(0, *current_pos, "%c", test_string[*current_pos]);
    }
    refresh();
}

void logTimingData(const char *filename, char key, long time_diff) {
    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    fprintf(file, "%c,%ld\n", key, time_diff);
    fclose(file);
}

void processCharacter(int *current_pos, char ch, const char *test_string) {
    if (ch == test_string[*current_pos]) {
        attron(COLOR_PAIR(1)); // Correct letters in green
    } else {
        attron(COLOR_PAIR(2)); // Incorrect letters in red
    }
    mvprintw(0, *current_pos, "%c", test_string[*current_pos]);
    attroff(COLOR_PAIR(1) | COLOR_PAIR(2));
    (*current_pos)++;
    refresh();
}

void runTypingTest(const char *test_string, const char *filename) {
    initializeScreen();

    struct timeval start, end;
    int input_len = strlen(test_string);
    int current_pos = 0;

    clear();
    mvprintw(0, 0, "%s", test_string); // Display test string
    refresh();

    gettimeofday(&start, NULL); // Start timing

    while (1) {
        char ch = getch();

        if (ch == 127 || ch == 8) { // Handle backspace
            handleBackspace(&current_pos, test_string);
        } else if (ch == 23) { // Handle Ctrl + Backspace
            handleCtrlBackspace(&current_pos, test_string);
        } else if (ch == ' ' || ch == '\n') { // Handle space or Enter
            if (ch == '\n' || current_pos == input_len) break;
            if (test_string[current_pos] == ' ') {
                processCharacter(&current_pos, ch, test_string);
            } else {
                processCharacter(&current_pos, ch, test_string); // Show incorrect space in red
            }
        } else if (current_pos < input_len) { // Normal character input
            processCharacter(&current_pos, ch, test_string);
        }

        gettimeofday(&end, NULL); // End timing for each key
        long time_diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        logTimingData(filename, ch, time_diff);

        gettimeofday(&start, NULL); // Reset start for next character
    }

    finalizeScreen();
}

int main() {
    const char *filename = "timing_data.csv";
    const char *test_string = "hello world test";

    FILE *file = fopen(filename, "w");
    if (file) {
        fprintf(file, "Key,Time (microseconds)\n");
        fclose(file);
    }

    runTypingTest(test_string, filename);

    printf("Exiting program.\n");
    return 0;
}
