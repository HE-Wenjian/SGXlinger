
from datetime import datetime
from time import sleep
import gc


def mem_nonintensive():
    sleep(0.2)
    
def mem_intensive():
    d = {}
    i = 0;
    for i in range(0, 130000):
        d[i] = 'A' * (3000 + i % 9)
    return d

if __name__ == '__main__':
    trash = []
    for x in range(5):
        print(str(datetime.now()))
        del trash
        gc.collect()
        mem_nonintensive()
        trash=mem_intensive()
        
