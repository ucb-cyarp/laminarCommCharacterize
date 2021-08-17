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
import pandas

from PlotLaminarCommon.PlotLaminarCommon import *

SweepRtnType = collections.namedtuple('Sweep', ['blockSizesBytes', 'results'])

DUTY_CYCLE_FIELD_NAMES = ['ServerDutyCycle', 'ClientDutyCycle', 'MemoryDutyCycle']

pointAlpha = 0.1
avgLineWidth = 1

def setup():
    #Parse CLI Arguments for Config File Location
    parser = argparse.ArgumentParser(description='Plots Results form Laminar Comm Characterize Sweep')
    parser.add_argument('--input-dir', type=str, required=True, help='The directory in which the resuts of the Laminar Comm Characterize sweep are located')
    parser.add_argument('--output-file-prefix', type=str, required=False, help='If supplied, plots will be written to files with this given prefix')
    parser.add_argument('--ylim', required=False, type=float, nargs=2, help='The y limits (low, high).  If supplied, overrides the automatic y limit')
    parser.add_argument('--title', required=False, type=str, help='Title for the graphs.  If not supplied, the project name will be used')
    parser.add_argument('--plot-pts', required=False, action='store_true', help='Plot raw data points')

    args = parser.parse_args()

    # print(args)
    inputDir = args.input_dir
    outputFileDir = args.output_file_prefix

    if(not os.path.isdir(inputDir)):
        raise ValueError("input-dir must be a directory")

    #Print the CLI options for debugging
    print('Input Dir: ' + inputDir)

    RtnType = collections.namedtuple('SetupRtn', ['inputDir', 'outputFileDir', 'yLim', 'title', 'plotPts'])

    yLim = args.ylim

    if yLim is not None:
        if yLim[0] >= yLim[1]:
            raise ValueError('y Limit should be supplied with the lowest number first')

    title = args.title
    if title is None:
        title = 'Laiminar Comm Characterize Sweep Results'

    rtn_val = RtnType(inputDir=inputDir, outputFileDir=outputFileDir, yLim=yLim, title=title, plotPts=args.plot_pts)
    return rtn_val

#Returns a named tuple of the points in the sweep, ordered by the b
def loadSweep(dir: str):

    filenameRe = re.compile('blkSizeBytes([0-9]+)')

    dirFiles = os.listdir(dir)

    resultMap = {}

    for fileName in dirFiles:
        dirPath = os.path.join(dir, fileName)
        if os.path.isdir(dirPath):
            match = filenameRe.match(fileName)
            if match is not None:
                blkSize = int(match.group(1))
                if blkSize in resultMap.keys():
                    raise RuntimeError('Duplicate blocksize detected: {blkSize}')
                #Load the results for the directory
                result = loadResults(dirPath)
                #Check the result
                checkResults(result, 0.02)
                resultMap[blkSize] = result
                print(f'Loaded and Checked {fileName}')

    #Sort the discovered block sizes
    blockSizesBytes = sorted(resultMap.keys())
    #Get the corresponding results in the correct order
    results = [resultMap[blkSize] for blkSize in blockSizesBytes]
    
    return SweepRtnType(blockSizesBytes=blockSizesBytes, results=results)

def plotSweep(results: SweepRtnType, ylim, title: str, outputPrefix: str, plotPts):
    blkSizesBytes = results.blockSizesBytes #ordered list
    sweepResults = results.results #ordered list corresponding to blkSizesBytes

    #Modified from numpy example https://matplotlib.org/stable/gallery/lines_bars_and_markers/barchart.html
    fig, ax = plt.subplots()
    lines = []
    scatter = []
    colors = []

    cmap = plt.get_cmap('tab20')

    #Also make a dataframe to export to a CSV file for easy analysis
    tbl = pd.DataFrame()
    tbl['blkSizeBytes'] = blkSizesBytes

    #Plot Points First (if Applicable)
    if plotPts:
        for (i, testName) in enumerate(REPORTS): #Use the order in REPORTS (via a generator)
            if sweepResults:
                if testName in sweepResults[0]:

                    color = cmap(i)

                    #Plot the raw points in the collected test runs.  Each point is typically the rate expereienced by a given FIFO
                    pointXPos = np.empty(shape=(0))
                    pointYPos = np.empty(shape=(0))

                    for idx, resultPt in enumerate(sweepResults):
                        yVals = getRates(resultPt[testName]).to_numpy()
                        xVals = np.full(yVals.shape, blkSizesBytes[idx]) #The x value (block size) of all of these points is the same

                        pointXPos = np.append(pointXPos, xVals)
                        pointYPos = np.append(pointYPos, yVals)

                    currentScatter = ax.scatter(pointXPos, pointYPos, color=color, label=None, alpha=pointAlpha,  linewidths=0)
                    scatter.append(currentScatter)

    #Plot Average Lines
    for (i, testName) in enumerate(REPORTS): #Use the order in REPORTS (via a generator)
        if sweepResults:
                if testName in sweepResults[0]:

                    color = cmap(i)

                    #We have a list of datapoints, for each block size
                    #Need to extract the particular test case we are interested in as well as take the average
                    avgGbps = [getAvgRate(resultPt[testName]) for resultPt in sweepResults]
                    tbl[testName] = avgGbps
                    
                    currentLine = ax.plot(blkSizesBytes, avgGbps, label=testName, color=color, linewidth=avgLineWidth)
                    lines.append(currentLine)
                    colors.append(color)

    if ylim:
        ax.set_ylim(ylim)
    ax.set_ylabel('Rate (Gbps)')
    ax.set_xlabel('Block Size (bytes)')
    ax.set_title(title)
    ax.legend(fontsize=8)

    # fig.tight_layout()

    suffix = '_comm_sweep'
    if plotPts:
        suffix += '_pts'
    
    if outputPrefix:
        fig.savefig(outputPrefix+suffix+'.pdf', format='pdf')
        tbl.to_csv(outputPrefix+suffix+'.csv', index=False)

    plt.close(fig)

def plotSweepPtsSeperatePlots(results: SweepRtnType, ylim, title: str, outputPrefix: str):
    blkSizesBytes = results.blockSizesBytes #ordered list
    sweepResults = results.results #ordered list corresponding to blkSizesBytes

    #Modified from numpy example https://matplotlib.org/stable/gallery/lines_bars_and_markers/barchart.html
    figs = []
    axs = []

    lines = []
    scatter = []
    colors = []

    yLimMin = None
    yLimMax = None

    cmap = plt.get_cmap('tab20')

    #Also make a dataframe to export to a CSV file for easy analysis
    tbl = pd.DataFrame()
    tbl['blkSizeBytes'] = blkSizesBytes

    #Plot Points First
    for (i, testName) in enumerate(REPORTS): #Use the order in REPORTS (via a generator)
        if sweepResults:
            if testName in sweepResults[0]:
                fig, ax = plt.subplots()

                color = cmap(i)

                pointXPos = np.empty(shape=(0))
                pointYPos = np.empty(shape=(0))

                #Plot the raw points in the collected test runs.  Each point is typically the rate expereienced by a given FIFO
                for idx, resultPt in enumerate(sweepResults):
                    yVals = getRates(resultPt[testName]).to_numpy()
                    xVals = np.full(yVals.shape, blkSizesBytes[idx]) #The x value (block size) of all of these points is the same

                    pointXPos = np.append(pointXPos, xVals)
                    pointYPos = np.append(pointYPos, yVals)

                currentScatter = ax.scatter(pointXPos, pointYPos, color=color, label=testName, alpha=pointAlpha,  linewidths=0)
                scatter.append(currentScatter)

                #Plot the Average Line
                avgGbps = [getAvgRate(resultPt[testName]) for resultPt in sweepResults]
                tbl[testName] = avgGbps
                
                currentLine = ax.plot(blkSizesBytes, avgGbps, label=testName+' - Harmonic Avg.', color=(0, 0, 0, 1), linewidth=avgLineWidth) #For seperate plots, plot the average line in black
                lines.append(currentLine)
                colors.append(color)

                ax.set_ylabel('Rate (Gbps)')
                ax.set_xlabel('Block Size (bytes)')
                ax.set_title(title)
                ax.legend(fontsize=8)

                # fig.tight_layout()

                if ylim:
                    ax.set_ylim(ylim)
                else:
                    yLims = ax.get_ylim()
                    if yLimMin is None:
                        yLimMin = yLims[0]
                    else:
                        yLimMin = min(yLimMin, yLims[0])

                    if yLimMax is None:
                        yLimMax = yLims[1]
                    else:
                        yLimMax = max(yLimMax, yLims[1])

                figs.append(fig)
                axs.append(ax)

    #Set the y limits of the subplots to all be the same (if ylim provided, it is set earlier)
    if not ylim:    
        for ax in axs:
            ax.set_ylim(yLimMin, yLimMax)

    if outputPrefix:
        for i, fig in enumerate(figs):
            suffix = '_comm_sweep_pts' + '_subplt'+str(i)
            fig.savefig(outputPrefix+suffix+'.pdf', format='pdf')

    for fig in figs:
        plt.close(fig)

def plotDutyCycleSweepPtsSeperatePlots(results: SweepRtnType, title: str, outputPrefix: str):
    blkSizesBytes = results.blockSizesBytes #ordered list
    sweepResults = results.results #ordered list corresponding to blkSizesBytes

    #Modified from numpy example https://matplotlib.org/stable/gallery/lines_bars_and_markers/barchart.html
    figs = []
    axs = []

    lines = []
    scatter = []
    colors = []

    yLimMin = None
    yLimMax = None

    filenames = []

    cmap = plt.get_cmap('tab20')

    #Also make a dataframe to export to a CSV file for easy analysis
    tbl = pd.DataFrame()
    tbl['blkSizeBytes'] = blkSizesBytes

    #Plot Points First
    for (i, testName) in enumerate(REPORTS): #Use the order in REPORTS (via a generator)
        if sweepResults:
            if testName in sweepResults[0]:
                for dutyCycleFieldName in DUTY_CYCLE_FIELD_NAMES:
                    if dutyCycleFieldName in (sweepResults[0])[testName].result.columns:
                        fig, ax = plt.subplots()

                        color = cmap(i)

                        pointXPos = np.empty(shape=(0))
                        pointYPos = np.empty(shape=(0))

                        #Plot the raw points in the collected test runs.  Each point is typically the rate expereienced by a given FIFO
                        for idx, resultPt in enumerate(sweepResults):
                            yVals = (resultPt[testName].result)[dutyCycleFieldName]
                            xVals = np.full(yVals.shape, blkSizesBytes[idx]) #The x value (block size) of all of these points is the same

                            pointXPos = np.append(pointXPos, xVals)
                            pointYPos = np.append(pointYPos, yVals)

                        scatterLbl = testName + ' - ' + dutyCycleFieldName
                        currentScatter = ax.scatter(pointXPos, pointYPos, color=color, label=scatterLbl, alpha=pointAlpha,  linewidths=0)
                        scatter.append(currentScatter)

                        #Plot the Average Line
                        avgDutyCycle = [((resultPt[testName].result)[dutyCycleFieldName]).mean() for resultPt in sweepResults]
                        tbl[testName] = avgDutyCycle
                        
                        currentLine = ax.plot(blkSizesBytes, avgDutyCycle, label=testName+' - Avg.', color=(0, 0, 0, 1), linewidth=avgLineWidth) #For seperate plots, plot the average line in black
                        lines.append(currentLine)
                        colors.append(color)

                        ax.set_ylabel('Duty Cycle')
                        ax.set_xlabel('Block Size (bytes)')
                        ax.set_title(title)
                        ax.legend(fontsize=8)

                        # fig.tight_layout()

                        ax.set_ylim(0, 1.1)
                        figs.append(fig)
                        axs.append(ax)
                        filenames.append(outputPrefix + '_comm_sweep_pts' + '_subplt'+str(i) + '_dutyCycle_' + dutyCycleFieldName + '.pdf')

    if outputPrefix:
        for i, fig in enumerate(figs):
            fig.savefig(filenames[i], format='pdf')

    for fig in figs:
        plt.close(fig)

def exportDutyCycleInfo(results: SweepRtnType, outputPrefix: str):
    blkSizesBytes = results.blockSizesBytes #ordered list
    sweepResults = results.results #ordered list corresponding to blkSizesBytes

    #Also make a dataframe to export to a CSV file for easy analysis
    tbl = pd.DataFrame()
    tbl['blkSizeBytes'] = blkSizesBytes

    foundDutyCycle = False

    #Plot Average Lines
    for (i, testName) in enumerate(REPORTS): #Use the order in REPORTS (via a generator)
        if sweepResults:
                if testName in sweepResults[0]:

                    #We have a list of datapoints, for each block size
                    #Need to extract the particular test case we are interested in as well as take the average
                    avgGbps = [getAvgRate(resultPt[testName]) for resultPt in sweepResults]
                    tbl[testName + ' - Rate (Gbps)'] = avgGbps

                    for dutyCycleFieldName in DUTY_CYCLE_FIELD_NAMES:
                        if dutyCycleFieldName in (sweepResults[0])[testName].result.columns:
                            avgDutyCycle = [((resultPt[testName].result)[dutyCycleFieldName]).mean() for resultPt in sweepResults]
                            tbl[testName + ' - ' + dutyCycleFieldName] = avgDutyCycle
                            foundDutyCycle = True
                    
    suffix = '_comm_sweep_duty_cycle'
    
    if outputPrefix and foundDutyCycle:
        tbl.to_csv(outputPrefix+suffix+'.csv', index=False)

def main():
    setup_rtn = setup()

    sweep = loadSweep(setup_rtn.inputDir)
    plotSweep(sweep, setup_rtn.yLim, setup_rtn.title, setup_rtn.outputFileDir, setup_rtn.plotPts)

    if setup_rtn.plotPts:
        #Also plot the version with just averages
        plotSweep(sweep, setup_rtn.yLim, setup_rtn.title, setup_rtn.outputFileDir, False)
        plotSweepPtsSeperatePlots(sweep, setup_rtn.yLim, setup_rtn.title, setup_rtn.outputFileDir)

    plotDutyCycleSweepPtsSeperatePlots(sweep, setup_rtn.title, setup_rtn.outputFileDir)
    exportDutyCycleInfo(sweep, setup_rtn.outputFileDir)

if __name__ == '__main__':
    main()

