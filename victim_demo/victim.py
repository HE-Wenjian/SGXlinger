
from datetime import datetime
from time import sleep
import gc


def mem_nonintensive():
    sleep(0.2)


def mem_intensive():
    d = {}
    i = 0
    gc.collect()
    for i in range(0, 110000):
        d[i] = 'A' * (3000 + i % 9)
    return d


if __name__ == '__main__':
    for x in range(5):
        print(str(datetime.now()))
        mem_nonintensive()
        mem_intensive()
