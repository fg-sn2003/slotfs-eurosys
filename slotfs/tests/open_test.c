#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "test.h"

#define NUM_THREADS 16
#define NUM_ITERATIONS 10000000

void* stat_thread(void* arg) {
    char path[100];
    int sequence[5];
    struct stat st;

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        // Generate a random sequence
        for (int j = 0; j < 5; j++) {
            sequence[j] = (rand() % 3) + 1;
        }

        // Access directories based on the sequence
        snprintf(path, sizeof(path), "\\testdir/level1_subdir%d/level2_subdir%d/level3_subdir%d/level4_subdir%d",
                 sequence[0], sequence[1], sequence[2], sequence[3]);
        // printf("path: %s\n", path);
        EXPECT(stat(path, &st) == 0, "stat failed");
        return NULL;
        // Print a message every 100,000 iterations
        if (i % 100000 == 0) {
            printf("Thread %ld: Completed %d iterations\n", (long)pthread_self(), i);
        }
    }

    return NULL;
}

int create_directories(const char* base_path, int level) {
    char path[100];
    for (int j = 1; j <= 3; j++) {
        snprintf(path, sizeof(path), "%s/level%d_subdir%d", base_path, level, j);
        int result = mkdir(path, 0755);
        EXPECT(result == 0, "mkdir failed");

        // Recursively create subdirectories for the next level
        create_directories(path, level + 1);
    }
    return 0;
}

int concurrent_stat() {
    pthread_t threads[NUM_THREADS];
    char path[100];

    // Create directory structure
    mkdir("\\testdir", 0755);
    create_directories("\\testdir", 1);

    printf("Directories created\n");    
    // Seed the random number generator
    srand(time(NULL));

    // Create threads to perform stat on directories
    for (int i = 0; i < NUM_THREADS; i++) {
        int result = pthread_create(&threads[i], NULL, stat_thread, NULL);
        EXPECT(result == 0, "pthread_create failed");
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("concurrent_stat completed successfully\n");
    return 0;
}

int basic_opentest() {
    int fd;
    int result;
    char filepath[50];

    // Create multiple levels of directories
    result = mkdir("\\testdir", 0755);
    EXPECT(result == 0, "mkdir failed");

    result = mkdir("\\testdir/subdir1", 0755);
    EXPECT(result == 0, "mkdir failed");

    result = mkdir("\\testdir/subdir2", 0755);
    EXPECT(result == 0, "mkdir failed");

    // Create multiple files in the directories using a loop
    for (int i = 1; i <= 100; i++) {
        snprintf(filepath, sizeof(filepath), "\\testdir/subdir1/testfile%d.txt", i);
        fd = open(filepath, O_CREAT | O_WRONLY, 0644);
        EXPECT(fd >= 0, "open failed");
        close(fd);

        snprintf(filepath, sizeof(filepath), "\\testdir/subdir2/testfile%d.txt", i);
        fd = open(filepath, O_CREAT | O_WRONLY, 0644);
        EXPECT(fd >= 0, "open failed");
        close(fd);
    }

    // Verify the files were created
    for (int i = 1; i <= 100; i++) {
        snprintf(filepath, sizeof(filepath), "\\testdir/subdir1/testfile%d.txt", i);
        fd = open(filepath, O_RDONLY);
        EXPECT(fd >= 0, "open for read failed");
        close(fd);

        snprintf(filepath, sizeof(filepath), "\\testdir/subdir2/testfile%d.txt", i);
        fd = open(filepath, O_RDONLY);
        EXPECT(fd >= 0, "open for read failed");
        close(fd);
    }

    printf("opentest completed successfully\n");
    return 0;
}

int main() {
    // basic_opentest();
    concurrent_stat();
    printf("open test passed\n");
    return 0;
}