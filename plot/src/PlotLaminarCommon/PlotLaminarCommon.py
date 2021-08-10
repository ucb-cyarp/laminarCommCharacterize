#!/usr/bin/env python3

import typing
import collections
import pandas as pd
import re
import os
import warnings
import copy
from enum import Enum

#Using technique from https://stackoverflow.com/questions/29792189/grouping-constants-in-python
# to create something close to a const struct.  A little werd in python but it is mostly a style question

FIFOReportFieldNamesType = collections.namedtuple('fifoReportFieldNamesType', ['serverCPU', 'clientCPU', 'serverTime', 'clientTime', 'bytesTx', 'bytesRx'])
MemoryReportFieldNamesType = collections.namedtuple('memoryReportFieldNamesType', ['cpu', 'memoryTime', 'bytesTransacted', 'arrayLenBytes'])


FIFO_REPORT_FEILD_NAMES: typing.Final[FIFOReportFieldNamesType] = FIFOReportFieldNamesType(serverCPU='ServerCPU', 
                                                                                           clientCPU='ClientCPU', 
                                                                                           serverTime='ServerTime', 
                                                                                           clientTime='ClientTime', 
                                                                                           bytesTx='BytesTx', 
                                                                                           bytesRx='BytesRx')

MEMORY_REPORT_FEILD_NAMES: typing.Final[MemoryReportFieldNamesType] = MemoryReportFieldNamesType(cpu='CPU', 
                                                                                                 memoryTime='MemoryTime', 
                                                                                                 bytesTransacted='BytesTransacted', 
                                                                                                 arrayLenBytes='MemArrayBytes')

# REPORT_FEILD_TYPES: typing.Final[typing.Dict] = {ReportFieldNamesType.serverCPU : int,
#                                                  ReportFieldNamesType.clientCPU : int,
#                                                  ReportFieldNamesType.serverTime: float,
#                                                  ReportFieldNamesType.clientTime: float,
#                                                  ReportFieldNamesType.bytesTx: int,
#                                                  ReportFieldNamesType.bytesRx: int}

REPORTS: typing.Final[typing.List[str]] = [
    'IntraL3 - Single FIFO',
    'IntraL3 - Single L3',
    'IntraL3 - All Cores Paired',
    'InterL3 - Single FIFO',
    'InterL3 - Single L3 Pair (All Cores Paired)',
    'InterL3 - All L3s (All Cores Paired)',
    'InterL3 - One To Multiple L3',
    'Memory - Single Reader',
    'Memory - Multiple Readers Single L3',
    'Memory - Multiple Readers Multiple L3',
    'Memory - Multiple Readers All L3',
    'Memory - Single Writer',
    'Memory - Multiple Writers Single L3',
    'Memory - Multiple Writers Multiple L3',
    'Memory - Multiple Writers All L3'
]

class TestResultType(Enum):
    FIFO = 1
    MEMORY = 2

class TestResult:
    def __init__(self, name: str, result: pd.DataFrame, resultType: TestResultType):
        self.name = name
        self.result = result
        self.type = resultType

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

def importReport(rptFiles: typing.List[str], filenameRegex: str, lblFormat: str, argsFromFilename: typing.List[int], argsFromRpt: typing.List[str], type: TestResultType):
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

                if type == TestResultType.FIFO:
                    #Compute Server Rate
                    reportPart['ServerGbps'] = (reportPart[FIFO_REPORT_FEILD_NAMES.bytesTx] / reportPart[FIFO_REPORT_FEILD_NAMES.serverTime]) * 8 / 1.0e9
                    #Compute Client Rate
                    reportPart['ClientGbps'] = (reportPart[FIFO_REPORT_FEILD_NAMES.bytesRx] / reportPart[FIFO_REPORT_FEILD_NAMES.clientTime]) * 8 / 1.0e9
                elif type == TestResultType.MEMORY:
                    #Compute Memory Rate
                    reportPart['RateGbps'] = (reportPart[MEMORY_REPORT_FEILD_NAMES.bytesTransacted] / reportPart[MEMORY_REPORT_FEILD_NAMES.memoryTime]) * 8 / 1.0e9
                else:
                    raise RuntimeError('Unknown TestResultType: ' + str(type))

                #Set entry name
                lblFun = lblFunFactory(lblFormat, matchFromFilename, argsFromFilename, argsFromRpt)
                #Not using DataFrame.apply because it returns a series with a single type.  However, there are different types for the different columns
                #Instead, using map with itertuples which itterates over the rows of the 
                reportPart['label'] = [x for x in map(lblFun, reportPart.itertuples())]

                #Append (use concat because append with an empty dataframe did not work)
                report = pd.concat([report, reportPart], axis=0, ignore_index=True)

    return report

def loadResultHelper(resultMap: typing.Dict[str, TestResult], 
                     name: str,
                     rptFiles: typing.List[str], 
                     filenameRegex: str, 
                     lblFormat: str, 
                     argsFromFilename: typing.List[int], 
                     argsFromRpt: typing.List[str], 
                     type: TestResultType):
    importedTbl = importReport(rptFiles, filenameRegex, lblFormat, argsFromFilename, argsFromRpt, type)
    if importedTbl is not None:
        if not importedTbl.empty:
            resultMap[name] = TestResult(name, importedTbl, type)

def loadResults(inputDir: str) -> typing.Dict[str, TestResult]:
    dirFiles = os.listdir(inputDir)
    dirFiles = sorted(dirFiles)
    dirFiles = [os.path.join(inputDir, f) for f in dirFiles] #We want the path to the files and not just the file name

    #Add report names to REPORTS constant to plot them.  The order of REPORTS is the default order in which they are plotted
    results = {}
    #Note, in the filename arguments, 0 refers to the value in the first parentiesized group and not the whole matched string (re.Match.groups() returns groups starting from 1 with 0 being the full match - the requested field is the entry in the returned tuple from groups())
    loadResultHelper(results, 'IntraL3 - Single FIFO', dirFiles, '.*_intraL3_singleFifo_L3-([0-9]+)_L3CPUA-([0-9]+)_L3CPUB-([0-9]+).csv', 'L3:{0} CPUA:{1} CPUB:{2}', [0], ['ServerCPU', 'ClientCPU'], TestResultType.FIFO)
    loadResultHelper(results, 'IntraL3 - Single L3', dirFiles, '.*_intraL3_singleL3_L3-([0-9]+).csv', 'L3:{0} CPUA:{1} CPUB:{2}', [0], ['ServerCPU', 'ClientCPU'], TestResultType.FIFO)
    loadResultHelper(results, 'IntraL3 - All Cores Paired', dirFiles, '.*_intraL3_allL3_startL3-([0-9]+).csv', 'StartL3:{0} CPU{1}->CPU{2}', [0], ['ServerCPU', 'ClientCPU'], TestResultType.FIFO)
    loadResultHelper(results, 'InterL3 - Single FIFO', dirFiles, '.*_interL3_singleFifo_L3A-([0-9]+)_L3B-([0-9]+).csv', 'L3A:{0} L3B:{1} CPU{2}->CPU{3}', [0, 1], ['ServerCPU', 'ClientCPU'], TestResultType.FIFO)
    loadResultHelper(results, 'InterL3 - Single L3 Pair (All Cores Paired)', dirFiles, '.*_interL3_singleL3_L3A-([0-9]+)_L3B-([0-9]+).csv', 'L3A:{0} L3B:{1} CPU{2}->CPU{3}', [0, 1], ['ServerCPU', 'ClientCPU'], TestResultType.FIFO)
    loadResultHelper(results, 'InterL3 - All L3s (All Cores Paired)', dirFiles, '.*_interL3_AllL3_startL3-([0-9]+).csv', 'StartL3:{0} CPU{1}->CPU{2}', [0], ['ServerCPU', 'ClientCPU'], TestResultType.FIFO)
    loadResultHelper(results, 'InterL3 - One To Multiple L3', dirFiles, '.*interL3_OneToMultiple_fromL3-([0-9]+).csv', 'FromL3:{0} CPU{1}->CPU{2}', [0], ['ServerCPU', 'ClientCPU'], TestResultType.FIFO)

    loadResultHelper(results, 'Memory - Single Reader', dirFiles, '.*_singleMemoryReader_L3-([0-9]+)_L3CPU-([0-9]+).csv', 'L3:{0} CPU:{1}', [0], ['CPU'], TestResultType.MEMORY)
    loadResultHelper(results, 'Memory - Multiple Readers Single L3', dirFiles, '.*_multipleMemoryReader_L3-([0-9]+).csv', 'L3:{0} CPU:{1}', [0], ['CPU'], TestResultType.MEMORY)
    loadResultHelper(results, 'Memory - Multiple Readers Multiple L3', dirFiles, '.*_multipleMemoryReader_startL3-([0-9]+)_numL3-([0-9]+).csv', 'L3:{0} NUM_L3:{1} CPU:{2}', [0, 1], ['CPU'], TestResultType.MEMORY)
    loadResultHelper(results, 'Memory - Multiple Readers All L3', dirFiles, '.*_multipleMemoryReader_startL3-([0-9]+).csv', 'L3:{0} CPU:{1}', [0], ['CPU'], TestResultType.MEMORY)

    loadResultHelper(results, 'Memory - Single Writer', dirFiles, '.*_singleMemoryWriter_L3-([0-9]+)_L3CPU-([0-9]+).csv', 'L3:{0} CPU:{1}', [0], ['CPU'], TestResultType.MEMORY)
    loadResultHelper(results, 'Memory - Multiple Writers Single L3', dirFiles, '.*_multipleMemoryWriter_L3-([0-9]+).csv', 'L3:{0} CPU:{1}', [0], ['CPU'], TestResultType.MEMORY)
    loadResultHelper(results, 'Memory - Multiple Writers Multiple L3', dirFiles, '.*_multipleMemoryWriter_startL3-([0-9]+)_numL3-([0-9]+).csv', 'L3:{0} NUM_L3:{1} CPU:{2}', [0, 1], ['CPU'], TestResultType.MEMORY)
    loadResultHelper(results, 'Memory - Multiple Writers All L3', dirFiles, '.*_multipleMemoryWriter_startL3-([0-9]+).csv', 'L3:{0} CPU:{1}', [0], ['CPU'], TestResultType.MEMORY)

    return results

# Is equivalent to the harmonic mean of the rates
#Returns rate in Gbps
def getAvgRate(result: TestResult):
    if result.type == TestResultType.FIFO:
        totalBytesTransacted = result.result[FIFO_REPORT_FEILD_NAMES.bytesTx].sum()
        totalTime = result.result[FIFO_REPORT_FEILD_NAMES.serverTime].sum()
    elif result.type == TestResultType.MEMORY:
        totalBytesTransacted = result.result[MEMORY_REPORT_FEILD_NAMES.bytesTransacted].sum()
        totalTime = result.result[MEMORY_REPORT_FEILD_NAMES.memoryTime].sum()
    else:
        raise RuntimeError('Unknown TestResultType: ' + str(result.type))

    return (totalBytesTransacted/totalTime)*8/1.0e9


def getMinRate(result: TestResult):
    if result.type == TestResultType.FIFO:
        return result.result['ServerGbps'].min()
    elif result.type == TestResultType.MEMORY:
        return result.result['RateGbps'].min()
    else:
        raise RuntimeError('Unknown TestResultType: ' + str(result.type))

def getMaxRate(result: TestResult):
    if result.type == TestResultType.FIFO:
        return result.result['ServerGbps'].max()
    elif result.type == TestResultType.MEMORY:
        return result.result['RateGbps'].max()
    else:
        raise RuntimeError('Unknown TestResultType: ' + str(result.type))

def getRates(result: TestResult):
    if result.type == TestResultType.FIFO:
        return result.result['ServerGbps']
    elif result.type == TestResultType.MEMORY:
        return result.result['RateGbps']
    else:
        raise RuntimeError('Unknown TestResultType: ' + str(result.type))

def checkResults(results: typing.Dict[str, TestResult], clientServerTolerance: float):
    problem = False
    for result in results.values():
        if result.type == TestResultType.FIFO:
            ratio = result.result['ServerGbps']/result.result['ClientGbps']
            outOfSpec = (ratio > (1+clientServerTolerance)) | (ratio < (1-clientServerTolerance)) #See https://pandas.pydata.org/pandas-docs/stable/user_guide/indexing.html for why parens are needed

            if outOfSpec.any():
                warnings.warn(f'Server Gbps differs from Client Gbps by > {clientServerTolerance:%} [{result.name}]', UserWarning)
                problem = True
        elif result.type != TestResultType.MEMORY:
            raise RuntimeError('Unknown TestResultType: ' + str(result.type))
    
    return problem