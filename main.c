#include <stdio.h>
#include <sys/time.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_LEN 500
#define KEY_SEQUENCE_BASE_SIZE 10

void initializeScreen();
void finalizeScreen();
void initializeColors();
void handleBackspace(int *current_pos, const char *test_string);
void handleCtrlBackspace(int *current_pos, const char *test_string);
int processCharacter(int *current_pos, char ch, const char *test_string);
int runTypingTest(const char *test_string, const char *filename, int word_count);
void calculateWPM(struct timeval start, struct timeval end, int word_count);



typedef struct {
	int key;
	long time;
} KeyPress_t;


typedef struct {
	int size;
	int capacity;
	KeyPress_t *keys;
} KeySequence_t;


KeySequence_t *KeySequence_init(void) {
    KeySequence_t *result = (KeySequence_t *)malloc(sizeof(KeySequence_t));
    if (!result) {
        fprintf(stderr, "Failed to allocate memory for KeySequence.\n");
        exit(EXIT_FAILURE);
    }
    result->size = 0;
    result->capacity = KEY_SEQUENCE_BASE_SIZE;
    result->keys = (KeyPress_t *)malloc(sizeof(KeyPress_t) * KEY_SEQUENCE_BASE_SIZE);
    if (!result->keys) {
        fprintf(stderr, "Failed to allocate memory for KeySequence keys.\n");
        free(result);
        exit(EXIT_FAILURE);
    }
    return result;
}

void KeySequence_add(KeySequence_t *sequence, int key, long time) {
    if (!sequence) {
        fprintf(stderr, "KeySequence is NULL.\n");
        return;
    }

    if (sequence->size + 1 >= sequence->capacity) {
        sequence->capacity *= 2;
        KeyPress_t *new_keys = (KeyPress_t *)realloc(sequence->keys, sizeof(KeyPress_t) * sequence->capacity);
        if (!new_keys) {
            fprintf(stderr, "Failed to allocate memory while resizing KeySequence keys.\n");
            free(sequence->keys);
            free(sequence);
            exit(EXIT_FAILURE);
        }
        sequence->keys = new_keys;
    }

    sequence->keys[sequence->size].key = key;
    sequence->keys[sequence->size].time = time;
    sequence->size++;
}

void KeySequence_free(KeySequence_t *sequence) {
    if (sequence) {
        free(sequence->keys);
        sequence->keys = NULL;
        free(sequence);
    }
}


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

void logTimingData(KeySequence_t *sequence, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
	fprintf(file, "Key,Time (microseconds)\n");
    for(int i = 0; i<sequence->size; i++){
	    fprintf(file, "%x,%ld\n", sequence->keys[i].key, sequence->keys[i].time);
    }
    fclose(file);
}

int processCharacter(int *current_pos, char ch, const char *test_string) {
	int result = 0;
    if (ch == test_string[*current_pos]) {
        attron(COLOR_PAIR(1)); // Correct letters in green
    } else {
	   result = 1;
        attron(COLOR_PAIR(2)); // Incorrect letters in red
    }
    mvprintw(0, *current_pos, "%c", test_string[*current_pos]);
    attroff(COLOR_PAIR(1) | COLOR_PAIR(2));
    (*current_pos)++;
    refresh();
    return result;
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

    KeySequence_t *sequence = KeySequence_init();

    while (1) {
        int ch = getch();

        if (ch == 127 || ch == 8) { // Handle backspace
            handleBackspace(&current_pos, test_string);
        } else if (ch == 23) { // Handle Ctrl + Backspace
            handleCtrlBackspace(&current_pos, test_string);
        } else if (ch == ' ') {
            if (test_string[current_pos] == ' ') {
                processCharacter(&current_pos, ch, test_string);
            } else {
                processCharacter(&current_pos, ch, test_string); // Show incorrect space in red
            }
        } else if (current_pos < input_len) { // Normal character input
            processCharacter(&current_pos, ch, test_string);
        } 
	if (current_pos >= input_len) {

		gettimeofday(&end, NULL); // End timing for each key
		long time_diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
		KeySequence_add(sequence,ch,time_diff);
            break;
            //int result = processCharacter(&current_pos, ch, test_string);
            //if(!result) break;
        }



        gettimeofday(&end, NULL); // End timing for each key
        long time_diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
	KeySequence_add(sequence,ch,time_diff);

        gettimeofday(&start, NULL); // Reset start for next character
    }

    gettimeofday(&end, NULL); // End timing for entire test
    calculateWPM(wpm_start, end, word_count);
    logTimingData(sequence, filename);
    KeySequence_free(sequence);
    sequence = NULL;

    return getch(); // Wait for user to see the result

}
#define BUFFER_SIZE 256
int read_file_to_buffer(const char* filename, char* buffer, size_t buffer_size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Unable to open file %s\n", filename);
        return -1;
    }

    size_t bytes_read = fread(buffer, 1, buffer_size - 1, file);
    buffer[bytes_read] = '\0';  // Null-terminate the string

    fclose(file);
    return bytes_read;
}


int main() {
    const char *filename = "timing_data.csv";

    FILE *file = fopen(filename, "w");
    if (file) {
        fprintf(file, "Key,Time (microseconds)\n");
        fclose(file);
    }
	char *test_string = "hello world test";
	char buffer[BUFFER_SIZE];

        int word_count = 3;

    while (1) {
        int result = runTypingTest(test_string, filename, word_count);
	system("python3 main.py");
	read_file_to_buffer("wordcount.tmp",buffer,BUFFER_SIZE);
	word_count = atoi(buffer);
	read_file_to_buffer("wordlist.tmp",buffer,BUFFER_SIZE);
	test_string = (char *)&buffer[0];

        printf("Press Enter to start a new string or any other key to exit.\n");
        if (result == 'q') {
		finalizeScreen();
            break;
        }
    }

    printf("Exiting program.\n");
    return 0;
}
