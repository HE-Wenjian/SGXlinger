#!/usr/bin/python3

import sys, os
import matplotlib.pyplot as plt


def read_data(filename):
    if not os.path.exists(filename):
        print("[WARN] Cannot find: "+filename)
        return [], []

    dataX, dataY=[],[]
    with open(filename,'r') as f:
        for line in f:
            if ">" in line:
                d=line.strip().split(' ')
                dataX.append(int(d[0]))
                dataY.append(int(d[-1]))

    return dataX, dataY

def draw_figure(dataX, dataY,FigTitle=None):

    plt.plot(dataX,dataY,marker='.')

    plt.title(FigTitle) if FigTitle else None
    plt.subplots_adjust(top=0.95,bottom=0.05,left=0.05,right=0.97)
    plt.gcf().canvas.set_window_title(FigTitle) if FigTitle else None
    plt.show()

def usage():
    print("Usage:\n" +
        sys.argv[0] + " <datafile>\n" + 
        "Draw a graph of interrupt latency.")
    exit()

if __name__ == '__main__':
    filename=""
    if len(sys.argv)==2:
        filename=sys.argv[1]
    else:
        usage()

    dataX, dataY = read_data(filename)
    draw_figure(dataX, dataY, FigTitle="file: "+filename)
