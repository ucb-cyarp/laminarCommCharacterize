#!/usr/bin/env python3

# @author Christopher Yarp
#
# This script plots telemetry information collected from Laminar processes

import argparse
import collections
import numpy as np
import os
import typing
import matplotlib.pyplot as plt

from PlotLaminarCommon.PlotLaminarCommon import *

def plotResults(results: typing.Dict[str, TestResult], ylim, title: str, outputPrefix: str, avgLines: bool):
    bars = []
    barLbls = []
    barLblXPos = []
    lines = []
    lineLbls = []
    averages = []
    centerXPos = [] #The center of the bars in the grouped series
    colors = []

    cmap = plt.get_cmap('tab10')

    width = 1

    #Modified from numpy example https://matplotlib.org/stable/gallery/lines_bars_and_markers/barchart.html
    fig, ax = plt.subplots()

    currentLblOffset = 0
    for (i, result) in enumerate((results[test] for test in REPORTS)): #Use the order in REPORTS (via a generator)
        #Get Lbls
        currentLbls = result.result['label'].to_list()
        xPos = np.arange(currentLblOffset, currentLblOffset+len(currentLbls)) * width
        barLblXPos = np.append(barLblXPos, xPos)
        currentLblOffset += len(currentLbls)+1
        barLbls.extend(currentLbls)

        color = cmap(i)
        currentBar = ax.bar(xPos, result.result['ServerGbps'], width, label=result.name, color=color)
        averages.append(result.result[REPORT_FEILD_NAMES.bytesTx].sum()/result.result[REPORT_FEILD_NAMES.serverTime].sum()*8/1.0e9) #Convert from bytes/sec to Gbps
        bars.append(currentBar)
        colors.append(color)
        centerXPos.append(xPos.mean())

    currentXlim = ax.get_xlim() #Need this if adding avg lines

    if avgLines:
    #plotting avg line
        xmin = currentXlim[0]
        xmax = currentXlim[1]
        for (color, avg, centerXPos) in zip(colors, averages, centerXPos):
            #plot line
            currentLine = ax.hlines(y=avg, xmin=xmin, xmax=xmax, linestyles='--', color=color, linewidth=1)
            lines.append(currentLine)

            #plot lbl
            currentLbl = ax.text(s=f'{avg:.1f}', x=centerXPos, y=avg, ha='center', va='center', color='k', fontsize='8', bbox={'boxstyle': 'Square, pad=0.1', 'edgecolor': 'None', 'facecolor': (1, 1, 1, 0.75)})
            lineLbls.append(currentLbl)

    if ylim:
        plt.ylim(ylim)
    ax.set_ylabel('Rate (Gbps)')
    ax.set_title(title)
    ax.set_xticks(barLblXPos)
    ax.set_xticklabels(barLbls, rotation='vertical', fontsize=2)
    ax.legend(fontsize=8)
    ax.set_xlim(currentXlim) #Need to reset xlim after these extra plot functions, otherwise the horiz lines are left floating (horiz margin)

    fig.tight_layout()
    
    if outputPrefix:
        plt.savefig(outputPrefix+'_comm.pdf', format='pdf')

def setup():
    #Parse CLI Arguments for Config File Location
    parser = argparse.ArgumentParser(description='Plots Results form Laminar Comm Characterize')
    parser.add_argument('--input-dir', type=str, required=True, help='The directory in which the resuts of Laminar Comm Characterize are located')
    parser.add_argument('--output-file-prefix', type=str, required=False, help='If supplied, plots will be written to files with this given prefix')
    parser.add_argument('--ylim', required=False, type=float, nargs=2, help='The y limits (low, high).  If supplied, overrides the automatic y limit')
    parser.add_argument('--title', required=False, type=str, help='Title for the graphs.  If not supplied, the project name will be used')
    parser.add_argument('--avg-lines', required=False, action='store_true', help='Plot average lines for the different tests')

    args = parser.parse_args()

    # print(args)
    inputDir = args.input_dir
    outputFileDir = args.output_file_prefix

    if(not os.path.isdir(inputDir)):
        raise ValueError("input-dir must be a directory")

    #Print the CLI options for debugging
    print('Input Dir: ' + inputDir)

    RtnType = collections.namedtuple('SetupRtn', ['inputDir', 'outputFileDir', 'yLim', 'title', 'avgLines'])

    yLim = args.ylim

    if yLim is not None:
        if yLim[0] >= yLim[1]:
            raise ValueError('y Limit should be supplied with the lowest number first')

    title = args.title
    if title is None:
        title = 'Laiminar Comm Characterize Results'

    rtn_val = RtnType(inputDir=inputDir, outputFileDir=outputFileDir, yLim=yLim, title=title, avgLines=args.avg_lines)
    return rtn_val

def main():
    setup_rtn = setup()

    results = loadResults(setup_rtn.inputDir)
    checkResults(results, 0.01)
    plotResults(results, setup_rtn.yLim, setup_rtn.title, setup_rtn.outputFileDir, setup_rtn.avgLines)

if __name__ == '__main__':
    main()

