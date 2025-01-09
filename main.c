#include <stdio.h>
#include <sys/time.h>
#include <ncurses.h>
#include <unistd.h>
#include <string.h>

#define MAX_INPUT_LEN 500

// Log timing data to a CSV file
void logTimingData(const char *filename, char key, long time_diff) {
    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    fprintf(file, "%c,%ld\n", key, time_diff);
    fclose(file);
}

// Run typing test
void runTypingTest(const char *test_string, const char *filename) {
    initscr();
    noecho();
    cbreak();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK); // Green for correct
    init_pair(2, COLOR_RED, COLOR_BLACK);   // Red for incorrect

    struct timeval start, end;
    int input_len = strlen(test_string);
    char input[MAX_INPUT_LEN] = {0};
    int current_pos = 0;

    clear();
    mvprintw(0, 0, "%s", test_string); // Display test string
    refresh();

    gettimeofday(&start, NULL); // Start timing

    while (1) {
        char ch = getch();

        if (ch == 127 || ch == 8) { // Handle backspace
            if (current_pos > 0) {
                current_pos--;
                mvprintw(1, current_pos, " "); // Clear last character
                refresh();
            }
        } else if (ch == 23) { // Handle Ctrl + Backspace (clear current word)
            while (current_pos > 0 && input[current_pos - 1] != ' ') {
                current_pos--;
                mvprintw(1, current_pos, " ");
            }
            refresh();
        } else if (ch == ' ' || ch == '\n') { // Handle space or Enter
            if (ch == '\n' || current_pos == input_len) break; // End test

            input[current_pos++] = ch;
            mvprintw(1, current_pos - 1, "%c", ch);
            refresh();
        } else if (current_pos < input_len) { // Normal character input
            input[current_pos++] = ch;
            if (ch == test_string[current_pos - 1]) {
                attron(COLOR_PAIR(1)); // Correct letters in green
            } else {
                attron(COLOR_PAIR(2)); // Incorrect letters in red
            }
            mvprintw(1, current_pos - 1, "%c", ch);
            attroff(COLOR_PAIR(1) | COLOR_PAIR(2));
            refresh();
        }

        gettimeofday(&end, NULL); // End timing for each key
        long time_diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        logTimingData(filename, ch, time_diff);

        gettimeofday(&start, NULL); // Reset start for next character
    }

    input[current_pos] = '\0'; // Null-terminate input

    clear();
    if (strcmp(input, test_string) == 0) {
        mvprintw(0, 0, "Correct!\n");
    } else {
        mvprintw(0, 0, "Incorrect. Expected: %s\nYou typed: %s\n", test_string, input);
    }
    refresh();
    getch();

    endwin();
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
