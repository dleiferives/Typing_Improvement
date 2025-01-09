
#include <stdio.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

// Disable buffered input
void setBufferedInput(int enable) {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    if (!enable) {
        t.c_lflag &= ~(ICANON | ECHO);
    } else {
        t.c_lflag |= (ICANON | ECHO);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

int main() {
    struct timeval start, end;
    char c;
    long time_diff;

    printf("Typing improvement program. Press keys to see timing information. Press 'q' to quit.\n");

    setBufferedInput(0); // Disable buffered input

    gettimeofday(&start, NULL); // Initialize start time

    while (1) {
        c = getchar(); // Read a character

        if (c == 'q') { // Exit condition
            break;
        }

        gettimeofday(&end, NULL); // Get current time

        time_diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);

        printf("Key '%c' pressed. Time since last key: %ld microseconds\n", c, time_diff);

        start = end; // Update start time
    }

    setBufferedInput(1); // Re-enable buffered input
    printf("Exiting program.\n");

    return 0;
}

