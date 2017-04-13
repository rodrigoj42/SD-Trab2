#include <stdio.h>
#include <iostream>
#include <fstream>
#include <random>
#include <thread>
#include <atomic>

using namespace std;

int acumulator;

class SpinLock {
    atomic_flag locked = ATOMIC_FLAG_INIT ;
    
    public:
    void acquire() {
        while (locked.test_and_set(memory_order_acquire)) { ; }
    }
    void release() {
        locked.clear(memory_order_release);
    }
} spinlock;

void task (int begin, int end, vector<char>& numbers) {
    int sum = 0;
    while (begin <= end) {
        sum += numbers[begin];
        begin++;
    }
    spinlock.acquire();
    acumulator += sum;
    spinlock.release();
}

void generateRandomNumbers(vector<char>& numbers) {
    random_device rd;                               // obtain a random number from hardware
    mt19937 eng(rd());                              // seed the generator
    uniform_int_distribution<> distr(-100, 100);    // define the range

    for (int i = 0; i < numbers.size(); i++) {
        numbers[i] = distr(eng);                    // generate number and save
    }
}

void writeToCSV(int n_value, int k_value, double elapsed_microsecs) {
    ofstream myfile;
    myfile.open("dados.csv", ios_base::app);
    myfile << n_value << ','; 
    myfile << k_value << ',';
    myfile << elapsed_microsecs;
    myfile << endl;
    myfile.close();
}


int main (int argc, char **argv) {
    
    int n_value;
    int k_value;

    // array of n_values
    vector<int> n_values = {10'000'000, 100'000'000, 1'000'000'000};

    // array of k_values
    vector<int> k_values = {1,2,4,8,16,32,64,128,256};

    // for 10 

    for (int pass = 0; pass < 10; pass++) {

        // for n_values

        for (int a = 0; a < n_values.size(); a++) {

           n_value = n_values[a];
         
           acumulator = 0;

           vector<char> random_numbers(n_value);          
           generateRandomNumbers(random_numbers);

           // for k_values

           for (int b = 0; b < k_values.size(); b++) {

               k_value = k_values[b];

               // ================
               // STARTING PROCESS
               // ================


               int size_slice = n_value / k_value;
               int size_rest = n_value % k_value;


               int begin = 0;
               int end = size_slice-1;

               vector<thread> threads;

               auto start_time = chrono::high_resolution_clock::now();

               for (int t = 0; t < k_value; t++) {
                   if (t == k_value-1) {
                       end += size_rest;
                   }

                   threads.push_back(thread(task, begin, end, ref(random_numbers)));

                   begin += size_slice;
                   end += size_slice;
               }

               // loop again to join the threads
               for (auto& t : threads) {
                   t.join();
               }

               auto end_time = chrono::high_resolution_clock::now();

               double elapsed_microsecs = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
               writeToCSV(n_value, k_value, elapsed_microsecs);

               // =================
               // FINISHING PROCESS
               // =================

               cout << "Total of acumulator = " << acumulator << endl;
               acumulator = 0;

               
           }
        }
    }
}

