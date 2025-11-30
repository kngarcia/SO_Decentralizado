/* user/app_ml_demo.c
 * Application 3: ML Demo - Distributed machine learning inference
 * 
 * Features:
 * - Linear regression model for temperature prediction
 * - Distributed inference (can receive data from peers)
 * - Simple visualization via serial output
 */

#include <stdint.h>

#define SYS_EXIT       1
#define SYS_LOG        3
#define SYS_SOCKET     15
#define SYS_BIND       16
#define SYS_SENDTO     22
#define SYS_RECVFROM   23

#define SOCK_DGRAM     2
#define AF_INET        2

typedef struct {
    uint8_t addr[4];
} ip_addr_t;

typedef struct {
    uint16_t family;
    uint16_t port;
    ip_addr_t addr;
} sockaddr_t;

/* ML protocol */
#define ML_CMD_INFERENCE_REQUEST  1
#define ML_CMD_INFERENCE_RESULT   2
#define ML_CMD_MODEL_SYNC         3

typedef struct {
    uint8_t cmd;
    uint8_t model_id;
    float features[4];  /* Input features */
    float result;       /* Prediction result */
} ml_packet_t;

static inline long syscall(long num, long a1, long a2, long a3, long a4, long a5) {
    long ret;
    register long r10 asm("r10") = a4;
    register long r8 asm("r8") = a5;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static void print(const char *msg) {
    syscall(SYS_LOG, (long)msg, 0, 0, 0, 0);
}

static void print_float(float val) {
    int integer_part = (int)val;
    int decimal_part = (int)((val - integer_part) * 100);
    if (decimal_part < 0) decimal_part = -decimal_part;
    
    char buf[16];
    int idx = 0;
    
    /* Handle negative */
    if (integer_part < 0) {
        buf[idx++] = '-';
        integer_part = -integer_part;
    }
    
    /* Integer part */
    if (integer_part == 0) {
        buf[idx++] = '0';
    } else {
        int div = 1000;
        int started = 0;
        while (div > 0) {
            int digit = integer_part / div;
            if (digit > 0 || started) {
                buf[idx++] = '0' + digit;
                started = 1;
            }
            integer_part %= div;
            div /= 10;
        }
    }
    
    /* Decimal point */
    buf[idx++] = '.';
    
    /* Decimal part */
    buf[idx++] = '0' + (decimal_part / 10);
    buf[idx++] = '0' + (decimal_part % 10);
    buf[idx] = '\0';
    
    print(buf);
}

/* Simple linear regression: y = w0 + w1*x1 + w2*x2 + ... */
static float predict(float *features, int num_features, float *weights) {
    float result = weights[0];  /* Bias */
    for (int i = 0; i < num_features; i++) {
        result += weights[i + 1] * features[i];
    }
    return result;
}

void _start(void) {
    print("=== ML Demo Application ===\n");
    print("Distributed Machine Learning Inference Service\n");
    
    /* Pre-trained model for temperature prediction
     * Model: Temperature = 20.0 + 0.5*humidity + 0.3*pressure - 0.2*wind_speed
     */
    float weights[4] = {20.0f, 0.5f, 0.3f, -0.2f};
    int num_features = 3;
    
    print("Model: Temperature = 20.0 + 0.5*humidity + 0.3*pressure - 0.2*wind\n");
    print("Weights: [20.0, 0.5, 0.3, -0.2]\n");
    
    /* Create UDP socket for distributed inference */
    int sockfd = syscall(SYS_SOCKET, AF_INET, SOCK_DGRAM, 0, 0, 0);
    if (sockfd < 0) {
        print("ERROR: Failed to create socket\n");
        syscall(SYS_EXIT, 1, 0, 0, 0, 0);
    }
    
    print("Socket created\n");
    
    /* Bind to port 7777 (ML inference port) */
    sockaddr_t addr;
    addr.family = AF_INET;
    addr.port = 7777;
    addr.addr.addr[0] = 0;
    addr.addr.addr[1] = 0;
    addr.addr.addr[2] = 0;
    addr.addr.addr[3] = 0;
    
    if (syscall(SYS_BIND, sockfd, (long)&addr, sizeof(addr), 0, 0) < 0) {
        print("ERROR: Failed to bind socket\n");
        syscall(SYS_EXIT, 1, 0, 0, 0, 0);
    }
    
    print("Bound to port 7777\n");
    
    /* Test local inference with sample data */
    print("\n--- Local Inference Test ---\n");
    
    float test_cases[3][3] = {
        {60.0f, 1013.0f, 10.0f},  /* Humidity, Pressure, Wind Speed */
        {80.0f, 1000.0f, 5.0f},
        {50.0f, 1020.0f, 15.0f}
    };
    
    for (int i = 0; i < 3; i++) {
        print("Test ");
        char num[4] = {'0' + (i+1), ':', ' ', 0};
        print(num);
        print("humidity=");
        print_float(test_cases[i][0]);
        print(", pressure=");
        print_float(test_cases[i][1]);
        print(", wind=");
        print_float(test_cases[i][2]);
        print("\n");
        
        float prediction = predict(test_cases[i], num_features, weights);
        print("  Predicted temperature: ");
        print_float(prediction);
        print(" °C\n");
    }
    
    /* Listen for distributed inference requests */
    print("\n--- Listening for Inference Requests ---\n");
    
    ml_packet_t recv_pkt;
    sockaddr_t src_addr;
    uint32_t addrlen = sizeof(src_addr);
    
    for (int i = 0; i < 5; i++) {
        long received = syscall(SYS_RECVFROM, sockfd, (long)&recv_pkt,
                               sizeof(recv_pkt), 0, (long)&src_addr);
        
        if (received > 0) {
            if (recv_pkt.cmd == ML_CMD_INFERENCE_REQUEST) {
                print("Inference request received\n");
                
                /* Perform inference */
                float result = predict(recv_pkt.features, num_features, weights);
                
                print("Input: [");
                for (int j = 0; j < num_features; j++) {
                    print_float(recv_pkt.features[j]);
                    if (j < num_features - 1) print(", ");
                }
                print("]\n");
                print("Prediction: ");
                print_float(result);
                print("\n");
                
                /* Send result back */
                ml_packet_t response;
                response.cmd = ML_CMD_INFERENCE_RESULT;
                response.model_id = recv_pkt.model_id;
                response.result = result;
                
                sockaddr_t dest_addr = src_addr;
                syscall(SYS_SENDTO, sockfd, (long)&response,
                       sizeof(response), 0, (long)&dest_addr);
                
                print("Result sent\n");
            }
        }
        
        /* Delay */
        for (volatile int j = 0; j < 10000000; j++);
    }
    
    print("\nML service shutting down\n");
    print("Total inferences: 3 local + distributed\n");
    syscall(SYS_EXIT, 0, 0, 0, 0, 0);
    
    for (;;);
}
