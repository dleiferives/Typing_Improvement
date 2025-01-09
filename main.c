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
int runTypingTest(const char *test_string, const char *filename, int word_count);
void calculateWPM(struct timeval start, struct timeval end, int word_count);

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

void calculateWPM(struct timeval start, struct timeval end, int word_count) {
    long elapsed_time = ((end.tv_sec - start.tv_sec) * 1000000) + (end.tv_usec - start.tv_usec);
    double elapsed_minutes = elapsed_time / 60000000.0;
    double wpm = word_count / elapsed_minutes;
    mvprintw(2, 0, "Words Per Minute: %.2f", wpm);
    refresh();
}

int runTypingTest(const char *test_string, const char *filename, int word_count) {
    initializeScreen();

    struct timeval wpm_start, start, end;
    int input_len = strlen(test_string);
    int current_pos = 0;

    clear();
    mvprintw(0, 0, "%s", test_string); // Display test string
    refresh();

    gettimeofday(&start, NULL); // Start timing
    gettimeofday(&wpm_start, NULL); // Start timing

    while (1) {
        char ch = getch();

        if (ch == 127 || ch == 8) { // Handle backspace
            handleBackspace(&current_pos, test_string);
        } else if (ch == 23) { // Handle Ctrl + Backspace
            handleCtrlBackspace(&current_pos, test_string);
        } else if (ch == '\n') { // Handle Enter
            if (current_pos == input_len) break;
        } else if (ch == ' ') {
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

    gettimeofday(&end, NULL); // End timing for entire test
    calculateWPM(wpm_start, end, word_count);

    return getch(); // Wait for user to see the result

}

int main() {
    const char *filename = "timing_data.csv";

    FILE *file = fopen(filename, "w");
    if (file) {
        fprintf(file, "Key,Time (microseconds)\n");
        fclose(file);
    }

    while (1) {
        const char *test_string = "hello world test";
        int word_count = 3;
        int result = runTypingTest(test_string, filename, word_count);

        printf("Press Enter to start a new string or any other key to exit.\n");
        if (result != '\n') {

			finalizeScreen();
            break;
        }
    }

    printf("Exiting program.\n");
    return 0;
}
