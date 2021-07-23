#!/usr/bin/env python3

# @author Christopher Yarp
# @date 11/02/2020
#
# This script plots telemetry information collected from Laminar processes

import argparse
import collections
import pandas as pd
import numpy as np
import re
import os
import typing
import copy
import matplotlib.pyplot as plt

#Using technique from https://stackoverflow.com/questions/29792189/grouping-constants-in-python
# to create something close to a const struct.  A little werd in python but it is mostly a style question


ReportFieldNamesType = collections.namedtuple('reportFieldNamesType', ['serverCPU', 'clientCPU', 'serverTime', 'clientTime', 'bytesTx', 'bytesRx'])

REPORT_FEILD_NAMES: typing.Final[ReportFieldNamesType] = ReportFieldNamesType(serverCPU='ServerCPU', 
                                                                              clientCPU='ClientCPU', 
                                                                              serverTime='ServerTime', 
                                                                              clientTime='ClientTime', 
                                                                              bytesTx='BytesTx', 
                                                                              bytesRx='BytesRx')

# REPORT_FEILD_TYPES: typing.Final[typing.Dict] = {ReportFieldNamesType.serverCPU : int,
#                                                  ReportFieldNamesType.clientCPU : int,
#                                                  ReportFieldNamesType.serverTime: float,
#                                                  ReportFieldNamesType.clientTime: float,
#                                                  ReportFieldNamesType.bytesTx: int,
#                                                  ReportFieldNamesType.bytesRx: int}

class TestResult:
    def __init__(self, name: str, result: pd.DataFrame):
        self.name = name
        self.result = result

#This function needs to be curried for the pd apply function
def lblFunFactory(formatStr: str, matchFromFilename: typing.List[str], argsFromFilename: typing.List[int], argsFromRpt: typing.List[str]):
    # Assembly the format argument str
    args = []
    for filenameArgInd in argsFromFilename:
        if filenameArgInd >= len(matchFromFilename):
            raise ValueError('Filename Index {} is larger than the number of matches from the filename: {}'.format(filenameArgInd, len(matchFromFilename)))
        
        args.append(str(matchFromFilename[filenameArgInd]))

    def lblFun(row: collections.namedtuple) -> str:
        argsLocal = copy.deepcopy(args)
        #Need to make a deep copy of args to which we can append.  This allows us to call this function more than once, each time starting out with the origional args list
        #If we don't do this, values from previous calls to lblFun will still be in args.
        #In other words, the args list behaves like a private variable of an object to which lblFun is a method.  Its value persists over calls to a given realization of lblFun
        #Getting a new lblFun by calling lblFunFactory basically creates a new object
        #
        #In the functional space, this is a closure with mutable variables
        #See https://towardsdatascience.com/closures-and-decorators-in-python-2551abbc6eb6
        # and https://en.wikipedia.org/wiki/Closure_(computer_programming)

        for argFromRpt in argsFromRpt:
            argsLocal.append(str(getattr(row,argFromRpt)))

        #Don't need to make a copy of formatStr since it is immutable (and not modified by format)
        return formatStr.format(*argsLocal)
    
    return lblFun

def importReport(rptFiles: typing.List[str], filenameRegex: str, lblFormat: str, argsFromFilename: typing.List[int], argsFromRpt: typing.List[str]):
    filenameRe = re.compile(filenameRegex)
    report = pd.DataFrame()

    for file in rptFiles:
        if os.path.isfile(file):
            match = filenameRe.match(file)
            if match is not None:
                #This file should be included in the result set
                matchFromFilename = list(match.groups())

                #Open the File
                # reportPart = pd.read_csv(file, dtype=REPORT_FEILD_TYPES)
                reportPart = pd.read_csv(file)


                #Compute Server Rate
                reportPart['ServerGbps'] = reportPart[REPORT_FEILD_NAMES.bytesTx] / reportPart[REPORT_FEILD_NAMES.serverTime] * 8 / 1.0e9

                #Compute Client Rate
                reportPart['ClientGbps'] = reportPart[REPORT_FEILD_NAMES.bytesRx] / reportPart[REPORT_FEILD_NAMES.clientTime] * 8 / 1.0e9

                #Set entry name
                lblFun = lblFunFactory(lblFormat, matchFromFilename, argsFromFilename, argsFromRpt)
                #Not using DataFrame.apply because it returns a series with a single type.  However, there are different types for the different columns
                #Instead, using map with itertuples which itterates over the rows of the 
                reportPart['label'] = [x for x in map(lblFun, reportPart.itertuples())]

                #Append (use concat because append with an empty dataframe did not work)
                report = pd.concat([report, reportPart], axis=0, ignore_index=True)

    return report

def loadResults(inputDir: str) -> typing.List[TestResult]:
    dirFiles = os.listdir(inputDir)
    dirFiles = sorted(dirFiles)
    dirFiles = [os.path.join(inputDir, f) for f in dirFiles] #We want the path to the files and not just the file name

    results = []
    results.append(TestResult('IntraL3 - Single FIFO', importReport(dirFiles, '.*_intraL3_singleFifo_L3-([0-9]+)_L3CPUA-([0-9]+)_L3CPUB-([0-9]+).csv', 'L3:{0} CPUA:{1} CPUB:{2}', [0], ['ServerCPU', 'ClientCPU']))) 
    results.append(TestResult('IntraL3 - Single L3', importReport(dirFiles, '.*_intraL3_singleL3_L3-([0-9]+).csv', 'L3:{0} CPUA:{1} CPUB:{2}', [0], ['ServerCPU', 'ClientCPU'])))    
    results.append(TestResult('IntraL3 - All Cores Paired', importReport(dirFiles, '.*_intraL3_allL3_startL3-([0-9]+).csv', 'StartL3:{0} CPU{1}->CPU{2}', [0], ['ServerCPU', 'ClientCPU'])))
    results.append(TestResult('InterL3 - Single FIFO', importReport(dirFiles, '.*_interL3_singleFifo_L3A-([0-9]+)_L3B-([0-9]+).csv', 'L3A:{0} L3B:{1} CPU{2}->CPU{3}', [0, 1], ['ServerCPU', 'ClientCPU'])))
    results.append(TestResult('InterL3 - Single L3 Pair (All Cores Paired)', importReport(dirFiles, '.*_interL3_singleL3_L3A-([0-9]+)_L3B-([0-9]+).csv', 'L3A:{0} L3B:{1} CPU{2}->CPU{3}', [0, 1], ['ServerCPU', 'ClientCPU'])))
    results.append(TestResult('InterL3 - All L3s (All Cores Paired)', importReport(dirFiles, '.*_interL3_AllL3_startL3-([0-9]+).csv', 'StartL3:{0} CPU{1}->CPU{2}', [0], ['ServerCPU', 'ClientCPU'])))
    results.append(TestResult('InterL3 - One To Multiple L3', importReport(dirFiles, '.*interL3_OneToMultiple_fromL3-([0-9]+).csv', 'FromL3:{0} CPU{1}->CPU{2}', [0], ['ServerCPU', 'ClientCPU'])))

    return results

def plotResults(results: typing.List[TestResult], ylim, title: str, outputPrefix: str, avgLines: bool):
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
    for (i, result) in enumerate(results):
        #Get Lbls
        currentLbls = result.result['label'].to_list()
        xPos = np.arange(currentLblOffset, currentLblOffset+len(currentLbls)) * width
        barLblXPos = np.append(barLblXPos, xPos)
        currentLblOffset += len(currentLbls)+1
        barLbls.extend(currentLbls)

        color = cmap(i)
        currentBar = ax.bar(xPos, result.result['ServerGbps'], width, label=result.name, color=color)
        averages.append(result.result['ServerGbps'].mean())
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
    plotResults(results, setup_rtn.yLim, setup_rtn.title, setup_rtn.outputFileDir, setup_rtn.avgLines)

if __name__ == '__main__':
    main()

