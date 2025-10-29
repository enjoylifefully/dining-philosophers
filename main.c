#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

constexpr size_t N = 5;

enum state { STATE_THINKING, STATE_HUNGRY, STATE_EATING };
typedef enum state State;

static inline size_t left(size_t i) {
    return (i + N - 1) % N;
}

static inline size_t right(size_t i) {
    return (i + 1) % N;
}

State g_state[N];
pthread_mutex_t g_state_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_can_eat[N];

static void sleep_ms(uint32_t ms) {
    struct timespec ts;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;

    while (nanosleep(&ts, &ts) == -1) {}
}

uint32_t rand_inclusive(uint32_t min, uint32_t max) {
    if (max <= min) {
        return min;
    }

    return min + arc4random_uniform(max - min + 1);
}

void test(size_t i) {
    if (g_state[i] == STATE_HUNGRY && g_state[left(i)] != STATE_EATING &&
        g_state[right(i)] != STATE_EATING) {
        g_state[i] = STATE_EATING;

        pthread_cond_signal(&g_can_eat[i]);
    }
}

void think(size_t i) {
    size_t duration = rand_inclusive(2000, 3000);

    printf("%3zu está pensando (%zums)\n", i, duration);

    usleep((useconds_t)duration * 1000);
}

void take_forks(size_t i) {
    pthread_mutex_lock(&g_state_mutex);

    g_state[i] = STATE_HUNGRY;

    printf("%3zu está com fome\n", i);
    test(i);

    while (g_state[i] != STATE_EATING) {
        pthread_cond_wait(&g_can_eat[i], &g_state_mutex);
    }

    pthread_mutex_unlock(&g_state_mutex);
}

void eat(size_t i) {
    uint32_t duration = rand_inclusive(2000, 3000);

    printf("%3zu está comendo (%ums)\n", i, duration);

    usleep((useconds_t)(duration * 1000));
}

void put_forks(size_t i) {
    pthread_mutex_lock(&g_state_mutex);

    g_state[i] = STATE_THINKING;

    test(left(i));
    test(right(i));

    pthread_mutex_unlock(&g_state_mutex);
}

void* philosopher(void* id) {
    size_t i = *(size_t*)id;

    while (true) {
        think(i);
        take_forks(i);
        eat(i);
        put_forks(i);
    }
}

int main() {
    for (size_t i = 0; i < N; ++i) {
        g_state[i] = STATE_THINKING;

        pthread_cond_init(&g_can_eat[i], NULL);
    }

    pthread_t threads[N];
    size_t ids[N];

    for (size_t i = 0; i < N; ++i) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, philosopher, &ids[i]);
    }

    for (size_t i = 0; i < N; ++i) {
        pthread_join(threads[i], NULL);
    }

    for (size_t i = 0; i < N; ++i) {
        pthread_cond_destroy(&g_can_eat[i]);
    }

    pthread_mutex_destroy(&g_state_mutex);

    return 0;
}
