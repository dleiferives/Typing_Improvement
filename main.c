#include <stdio.h>
#include <sys/time.h>
#include <ncurses.h>
#include <unistd.h>
#include <string.h>

#define MAX_WORDS 100
#define MAX_WORD_LEN 50

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
void runTypingTest(const char *words[], int word_count, const char *filename) {
    initscr();
    noecho();
    cbreak();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK); // Green for correct
    init_pair(2, COLOR_RED, COLOR_BLACK);   // Red for incorrect

    struct timeval start, end;

    for (int i = 0; i < word_count; ++i) {
        clear();
        int len = strlen(words[i]);
        char input[MAX_WORD_LEN] = {0};
        int correct = 1;

        mvprintw(0, 0, "%s", words[i]); // Display word
        refresh();

        gettimeofday(&start, NULL); // Start timing

        for (int j = 0; j < len; ++j) {
            char ch = getch();

            if (ch == words[i][j]) {
                attron(COLOR_PAIR(1)); // Correct letters in green
            } else {
                attron(COLOR_PAIR(2)); // Incorrect letters in red
                correct = 0;
            }

            mvprintw(0, j, "%c", ch); // Update word display with color
            attroff(COLOR_PAIR(1) | COLOR_PAIR(2));
            refresh();

            input[j] = ch;

            gettimeofday(&end, NULL); // End timing
            long time_diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
            logTimingData(filename, ch, time_diff);

            gettimeofday(&start, NULL); // Reset start for next character
        }

        input[len] = '\0';
        if (strcmp(input, words[i]) == 0) {
            mvprintw(1, 0, "Correct!\n");
        } else {
            mvprintw(1, 0, "Incorrect. Expected: %s\n", words[i]);
        }
        refresh();
        getch(); // Wait for user input to proceed
    }

    endwin();
}

int main() {
    const char *filename = "timing_data.csv";
    const char *test_words[MAX_WORDS] = {"hello", "world", "test"};
    int test_word_count = 3;

    FILE *file = fopen(filename, "w");
    if (file) {
        fprintf(file, "Key,Time (microseconds)\n");
        fclose(file);
    }

    runTypingTest(test_words, test_word_count, filename);

    printf("Exiting program.\n");
    return 0;
}
