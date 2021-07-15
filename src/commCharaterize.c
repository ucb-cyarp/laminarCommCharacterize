#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "laminarFifoRunner.h"

#define RYZEN_3970_NOSMT

#ifdef RYZEN_3970_NOSMT
    #define CORES_PER_L3 (4)
    #define L3_S (8)
    #define START_L3 (2)

    const int CORE_MAP[L3_S][CORES_PER_L3] = {{ 0, 17, 18, 19}, 
                                              { 1,  2,  3,  4}, 
                                              { 5,  6,  7,  8}, 
                                              { 9, 10, 11, 12}, 
                                              {13, 14, 15, 16}, 
                                              {20, 21, 22, 23}, 
                                              {24, 25, 26, 27}, 
                                              {28, 29, 30, 31}};
#endif

#ifdef EPYC_7002_NOSMT
    #define CORES_PER_L3 (4)
    #define L3_S (16)
    #define START_L3 (2)

    const int CORE_MAP[L3_S][CORES_PER_L3] = {{ 0,  1,  2,  3}, 
                                              { 4,  5,  6,  7}, 
                                              { 8,  9, 10, 11}, 
                                              {12, 13, 14, 15}, 
                                              {16, 17, 18, 19}, 
                                              {20, 21, 22, 23}, 
                                              {24, 25, 26, 27}, 
                                              {28, 29, 30, 31}, 
                                              {32, 33, 34, 35}, 
                                              {36, 37, 38, 39}, 
                                              {40, 41, 42, 43}, 
                                              {44, 45, 46, 47}, 
                                              {48, 49, 50, 51}, 
                                              {52, 53, 54, 55}, 
                                              {56, 57, 58, 59}, 
                                              {60, 61, 62, 63}};
#endif

/**
 * Note: This function allocates a new string which should be freed after use
 */
char* genReportName(const char* reportPrefix, const char* reportSuffix){
    int reportNameLen = strlen(reportPrefix) + strlen(reportSuffix);
    char* reportName = malloc(reportNameLen*sizeof(char));
    strcpy(reportName, reportPrefix);
    strcat(reportName, reportSuffix);
    return reportName;
}

/**
 * Single pair of cores in an L3 paired up
 * 
 * Multiple pairings of the 0th CPU are tried to determine if cores within an L3 are uniform
 */
void runIntraL3SingleFifo(char* reportPrefix){
    static_assert(CORES_PER_L3>1, "Intra-L3 Test Requires >1 Core Per L3");

    for(int i = 1; i<CORES_PER_L3; i++){
        char reportNameSuffix[80];
        snprintf(reportNameSuffix, 80, "_intraL3_singleFifo_L3-%d_L3CPUA-%d_L3CPUB%d.csv", START_L3, 0, i);
        char* reportName = genReportName(reportPrefix, reportNameSuffix);
        
        int serverCPUs[1] = {CORE_MAP[START_L3][0]};
        int clientCPUs[1] = {CORE_MAP[START_L3][i]};
        runLaminarFifoBench(serverCPUs, clientCPUs, 1, reportName);
        free(reportName);
    }
}

/**
 * Single pair of cores in different L3s paired up
 * 
 * Multiple pairings of the 0th CPUs in different L3s are tried to determine if cores within an L3 are uniform
 */
void runInterL3SingleFifo(char* reportPrefix){
    static_assert(START_L3+1<L3_S, "Inter-L3 Test Requires >1 L3s to be Tested");

    for(int i = 1; i<L3_S-START_L3; i++){
        char reportNameSuffix[80];
        snprintf(reportNameSuffix, 80, "_intraL3_singleFifo_L3A-%d_L3B-%d.csv", START_L3, START_L3+i);
        char* reportName = genReportName(reportPrefix, reportNameSuffix);

        int serverCPUs[1] = {CORE_MAP[START_L3][0]};
        int clientCPUs[1] = {CORE_MAP[START_L3+i][0]};
        runLaminarFifoBench(serverCPUs, clientCPUs, 1, reportName);
        free(reportName);
    }   
}

/**
 * All cores in a single L3 paired up
 */
void runIntraL3SingleL3(char* reportPrefix){
    static_assert(CORES_PER_L3>1, "Intra-L3 Test Requires >1 Core Per L3");
    char reportNameSuffix[80];
    snprintf(reportNameSuffix, 80, "_intraL3_singleL3_L3-%d.csv", START_L3);
    char* reportName = genReportName(reportPrefix, reportNameSuffix);

    int numFifos = CORES_PER_L3/2; //Round Down
    int serverCPUs[numFifos];
    int clientCPUs[numFifos];

    for(int i = 0; i<numFifos; i++){
        serverCPUs[i] = CORE_MAP[START_L3][i*2];
        clientCPUs[i] = CORE_MAP[START_L3][i*2+1];
    }

    runLaminarFifoBench(serverCPUs, clientCPUs, numFifos, reportName);

    free(reportName);
}

/**
 * All cores in an L3 paired up.  All L3s after START_L3 participating
 */
/**
 * All cores in a single L3 paired up
 */
void runIntraL3AllL3(char* reportPrefix){
    static_assert(CORES_PER_L3>1, "Intra-L3 Test Requires >1 Core Per L3");
    char* reportName = genReportName(reportPrefix, "_intraL3_allL3.csv");

    int numFifosPer = CORES_PER_L3/2; //Round Down
    int numL3s = L3_S-START_L3;
    int numFifos = numFifosPer*numL3s;
    int serverCPUs[numFifos];
    int clientCPUs[numFifos];

    for(int l3 = 0; l3<numL3s; l3++){
        for(int l3Fifo = 0; l3Fifo<numFifosPer; l3Fifo++){
            int ind = l3*numFifosPer+l3Fifo;
            serverCPUs[ind] = CORE_MAP[START_L3+l3][l3Fifo*2];
            clientCPUs[ind] = CORE_MAP[START_L3+l3][l3Fifo*2+1];
        }
    }

    runLaminarFifoBench(serverCPUs, clientCPUs, numFifos, reportName);

    free(reportName);
}

/**
 * All cores in 1 L3 paired to cores in another L3
 */
void runInterL3SingleL3(char* reportPrefix){
    static_assert(START_L3+1<L3_S, "Inter-L3 Test Requires >1 L3s to be Tested");
    char* reportName = genReportName(reportPrefix, "_interL3_singleL3.csv");

    int numFifos = CORES_PER_L3;
    int serverCPUs[numFifos];
    int clientCPUs[numFifos];

    for(int i = 0; i<CORES_PER_L3; i++){
        serverCPUs[i] = CORE_MAP[START_L3  ][i];
        clientCPUs[i] = CORE_MAP[START_L3+1][i];
    }

    runLaminarFifoBench(serverCPUs, clientCPUs, numFifos, reportName);

    free(reportName);
}

/**
 * All cores in 1 L3 paired to cores in another L3
 * 
 * All L3s after START_L3 Participating
 */
void runInterL3AllL3(char* reportPrefix){
    static_assert(START_L3+1<L3_S, "Inter-L3 Test Requires >1 L3s to be Tested");
    char* reportName = genReportName(reportPrefix, "_interL3_AllL3.csv");

    int numL3Pairs = (L3_S-START_L3)/2; //Round Down
    int numFifos = CORES_PER_L3*numL3Pairs;

    int serverCPUs[numFifos];
    int clientCPUs[numFifos];


    for(int l3 = 0; l3<numL3Pairs; l3++){
        for(int fifo = 0; fifo<CORES_PER_L3; fifo++){
            int ind = l3*CORES_PER_L3*2+fifo;
            serverCPUs[ind] = CORE_MAP[START_L3+l3*2  ][fifo];
            clientCPUs[ind] = CORE_MAP[START_L3+l3*2+1][fifo];
        }
    }

    runLaminarFifoBench(serverCPUs, clientCPUs, numFifos, reportName);

    free(reportName);
}

/**
 * All cores in 1 L3 paired to different cores in other L3s (not the same L3)
 */
void runInterL3OneToMultiple(char* reportPrefix){
    static_assert(START_L3+CORES_PER_L3<L3_S, "Inter-L3 Test Requires >1 L3s to be Tested");
    char* reportName = genReportName(reportPrefix, "_interL3_OneToMultiple.csv");

    int numFifos = CORES_PER_L3*2;
    int serverCPUs[numFifos];
    int clientCPUs[numFifos];

    for(int i = 0; i<CORES_PER_L3; i++){
        serverCPUs[i] = CORE_MAP[START_L3  ][i];
        clientCPUs[i] = CORE_MAP[START_L3+i][0];
    }

    runLaminarFifoBench(serverCPUs, clientCPUs, numFifos, reportName);

    free(reportName);
}

int main(int argc, char *argv[]){
    
    if(argc != 2){
        fprintf(stderr, "Error: Supply a filename prefix for the report files\n");
    }
    char* filenamePrefix = argv[1];

    static_assert(START_L3<L3_S, "START_L3 must be < L3_S");

    runIntraL3SingleFifo(filenamePrefix);
    runInterL3SingleFifo(filenamePrefix);
    runIntraL3SingleL3(filenamePrefix);
    runIntraL3AllL3(filenamePrefix);
    runInterL3SingleL3(filenamePrefix);
    runInterL3AllL3(filenamePrefix);
    runInterL3OneToMultiple(filenamePrefix);

    return 0;
}