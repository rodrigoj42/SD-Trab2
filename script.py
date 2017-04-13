from os import system
from time import sleep
producer_consumer = [(1, 1), (1, 2), (1, 4), (1, 8), (1, 16), (2, 1), (4, 1), (8, 1), (16, 1)]
for buffer_size in [2, 8, 32]:
    buffer_size = str(buffer_size)
    for i in range(len(producer_consumer)):
        n_producer = str(producer_consumer[i][0])
        n_consumer = str(producer_consumer[i][1])
        for p in range(10):
            system("./semaphore %s %s %s" % (buffer_size, n_producer, n_consumer))
