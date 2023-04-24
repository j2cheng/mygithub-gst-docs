//
// Created by builduser on 4/21/23.
//
//
//#include "ARM_Hdcp_Errors.h"
//#include "ARM_Hdcp_Engine.h"
//#include "ARM_Hdcp_Provisioning.h"
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
//#include <stdint.h>
//#include <stdbool.h>
//#include <inttypes.h>

#define RESULT_SUCCESS      0
#define RESULT_FAIL         1

int Hdcp_prov_init()
{
    const char*	cekFile      = "/data/tmp/data/Provisioning/HDCP2_Cek.dat";
    const char*	dataFile     = "/data/tmp/data/Provisioning/HDCP2_Rcv_2_2_Set1.dat";
    const char* cfgFile      = "/data/tmp/config/Android/DxHDCP.cfg";
    FILE*       tmpFile      = NULL;
    uint8_t     cekData[16];
    uint8_t*    provData     = NULL;
    size_t      dataFileSize = 0;
    int         ret          = RESULT_SUCCESS;
    int         engine_finalize_ret = RESULT_SUCCESS;
    size_t      bytesRead    = 0;
    struct stat statBuf;
//    uint32_t    hdcpError    = DX_SUCCESS;
    bool        bIsEngineInitialized = false;

    printf("\nRunning test provisioning sample application...\n");

    ret = stat(dataFile, &statBuf);
    if (ret != RESULT_SUCCESS) {
        ret = errno;
        printf("stat data file %s failed, errno: %d\n", dataFile, ret);
    }

    if (ret == RESULT_SUCCESS)
    {
        dataFileSize = (size_t)statBuf.st_size;
        provData = (uint8_t *)malloc(dataFileSize);
        if (provData == NULL) {
            ret = RESULT_FAIL;
            printf("Failed to allocate memory for provisioning data, size: %zu\n", dataFileSize);
        }
    }
    if (ret == RESULT_SUCCESS) {
        tmpFile = fopen(dataFile, "rb");
        if (tmpFile == NULL) {
            ret = errno;
            printf("Failed to open data file %s, errno: %d\n", dataFile, ret);
        }
    }
    if (ret == RESULT_SUCCESS) {
        bytesRead = fread(provData, 1, dataFileSize, tmpFile);
        fclose(tmpFile);
        tmpFile = NULL;
        if (bytesRead != dataFileSize) {
            ret = RESULT_FAIL;
            printf("Failed to read provisioning data from %s file. File size: %zu, actual bytes read size: %zu\n",
                   dataFile, dataFileSize, bytesRead);
        }
    }
    if (ret == RESULT_SUCCESS) {
        tmpFile = fopen(cekFile, "rb");
        if (tmpFile == NULL) {
            ret = errno;
            printf("Failed to open CEK file, errno: %d\n", ret);
        }
    }
    if (ret == RESULT_SUCCESS) {
        bytesRead = fread(cekData, 1, sizeof(cekData), tmpFile);
        fclose(tmpFile);
        tmpFile = NULL;
        if (bytesRead != sizeof(cekData)) {
            ret = RESULT_FAIL;
            printf("Failed to read CEK data from the %s file. File size: %zu, bytes read size: %zu\n",
                   cekFile, sizeof(cekData), bytesRead);
        }
    }

    return 0;
}