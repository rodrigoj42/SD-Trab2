#include <stdio.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <random>
#include <thread>
#include <string>
#include <dispatch/dispatch.h>

using namespace std;

#define MAX_CONSUMER      10000     // numbers to be consumed before finish the program

dispatch_semaphore_t mtx;                           // mutual exclusion to shared set of buffers      
dispatch_semaphore_t empty;                         // count of empty buffers (all empty to start)
dispatch_semaphore_t full;                          // count of full buffers (none full to start)
int global_index;
int consumed;
double wait_for_empty;
double wait_for_full;
double wait_mutex;

auto start_program = chrono::high_resolution_clock::now();
int buffer_size;
int n_producer;
int n_consumer;

void writeToCSV(double total) {
    ofstream myfile;
    myfile.open("dados.csv", ios_base::app);
    myfile << buffer_size << ',';
    myfile << n_producer << ',';
    myfile << n_consumer << ',';
    myfile << wait_for_empty << ',';
    myfile << wait_for_full << ',';
    myfile << wait_mutex << ',';
    myfile << total;
    myfile << endl;
    myfile.close();
}


bool isPrime(unsigned int n) {
    if (n < 2) return false;
    if (n < 4) return true;
    if (n % 2 == 0) return false;

    const unsigned int iMax = (int)sqrt(n) + 1;
    unsigned int i;
    for (i = 3; i <= iMax; i += 2)
        if (n % i == 0)
            return false;

    return true;
}

int generateRandomNumber() {
    random_device rd;                                    // obtain a random number from hardware
    mt19937 eng(rd());                                   // seed the generator
    uniform_int_distribution<> distr(1, 10000000);     // define the range (1 - 10ˆ7)

    return distr(eng);                                   // generate number
}

void task_producer (vector<int>& buffer) {
    printf("Produtor\n");
    while (true) {
        int value = generateRandomNumber();     // produce new resource
        
        auto start_time = chrono::high_resolution_clock::now();
        dispatch_semaphore_wait(empty, DISPATCH_TIME_FOREVER);
        auto mid_time = chrono::high_resolution_clock::now();
        dispatch_semaphore_wait(mtx,   DISPATCH_TIME_FOREVER);
        auto end_time = chrono::high_resolution_clock::now();

        double empty_ms  = chrono::duration_cast<chrono::microseconds>(mid_time - start_time).count();
        double mutex_ms  = chrono::duration_cast<chrono::microseconds>(end_time -   mid_time).count();

        wait_mutex += mutex_ms;
        wait_for_empty += empty_ms;    

        global_index++;
        buffer[global_index] = value;                      // add resource to an empty buffer
        
        dispatch_semaphore_signal(mtx);
        dispatch_semaphore_signal(full);

        //printf("Producer says %d is in buffer\n", value);
    }
}

void task_consumer (vector<int>& buffer) {
    printf("Consumidor\n");
    while (true) {

        auto start_time = chrono::high_resolution_clock::now();
        dispatch_semaphore_wait(full, DISPATCH_TIME_FOREVER);
        auto mid_time = chrono::high_resolution_clock::now();
        dispatch_semaphore_wait(mtx,  DISPATCH_TIME_FOREVER);
        auto end_time = chrono::high_resolution_clock::now();

        double full_ms  = chrono::duration_cast<chrono::microseconds>(mid_time - start_time).count();
        double mutex_ms = chrono::duration_cast<chrono::microseconds>(end_time -   mid_time).count();

        wait_mutex += mutex_ms;
        wait_for_full += full_ms;

        if (consumed == MAX_CONSUMER) { 
            auto end_program = chrono::high_resolution_clock::now();
            double total = chrono::duration_cast<chrono::microseconds>(end_program - start_program).count();
            writeToCSV(total);    
            terminate();
        }

        int value = buffer[global_index];
        buffer[global_index] = 0;                          // remove resource from a full buffer
        global_index--;
        consumed++;
        
        dispatch_semaphore_signal(mtx);
        dispatch_semaphore_signal(empty);

        if (isPrime(value)) {                   // consume resource
            printf("Consumer says %d is prime\n", value);
        } else {
            printf("Consumer says %d isn't prime\n", value);
        }

    }
}


int main (int argc, char **argv) {
    
    if (argc != 4) {
        cout << "Please check arguments: buffer_size, n_producer, n_consumer" << endl;
        return 0;
    }

    buffer_size = strtol(argv[1], NULL, 10);
    n_producer  = strtol(argv[2], NULL, 10);
    n_consumer  = strtol(argv[3], NULL, 10);

    global_index   = -1;
    consumed       =  0;
    wait_for_empty = .0;
    wait_for_full  = .0;
    wait_mutex     = .0;
    empty          = dispatch_semaphore_create(buffer_size);
    full           = dispatch_semaphore_create(0);
    mtx            = dispatch_semaphore_create(1);

    cout << "Initializing buffer" << endl;

    vector<int> buffer(buffer_size, 0); // initialize vector with buffer_size elements and 0 as value
            
    vector<thread> threads_consumer;
    vector<thread> threads_producer;

    cout << "Creating consumer threads" << endl;

    start_program = chrono::high_resolution_clock::now();
    // Create consumer threads
    for (int t = 0; t < n_consumer; t++) {
        threads_consumer.push_back(thread(task_consumer,ref(buffer)));
    }

    cout << "Creating producer threads" << endl;

    // Create producer threads
    for (int t = 0; t < n_producer; t++) {
        threads_producer.push_back(thread(task_producer,ref(buffer)));
    }

    cout << "Running" << endl;

    // loop again to join the consumer threads
    for (auto& t : threads_consumer) {
        t.join();
    }

    cout << "All consumer threads have stopped" << endl;

    // loop again to join the producer threads
    for (auto& t : threads_producer) {
        t.join();
    }

    cout << "All producer threads have stopped" << endl;

    dispatch_release(mtx);
    dispatch_release(full);
    dispatch_release(empty);

}
