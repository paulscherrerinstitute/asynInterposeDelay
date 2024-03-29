/*asynInterposeDelay.c */
/***********************************************************************/

/* Interpose for devices where each written char needs a delay
 * before sending the next char.
 * 
 * Author: Dirk Zimoch
 */

#include <cantProceed.h>
#include <epicsStdio.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <iocsh.h>

#include "asynDriver.h"
#include "asynOctet.h"
#include "asynOption.h"
#include <epicsExport.h>

typedef struct interposePvt {
    asynInterface octet;
    asynOctet     *pasynOctetDrv;
    void          *octetPvt;
    asynInterface option;
    asynOption    *pasynOptionDrv;
    void          *optionPvt;
    double        delay;
    unsigned long chunksize;
} interposePvt;

/* asynOctet methods */
static asynStatus writeIt(void *ppvt, asynUser *pasynUser,
    const char *data, size_t numchars, size_t *nbytesTransfered)
{
    interposePvt *pvt = (interposePvt *)ppvt;
    size_t n;
    size_t transfered = 0;
    double delay = pvt->delay;
    unsigned long chunksize = pvt->chunksize;
    asynStatus status = asynSuccess;

    while (numchars) {
        /* write one chunk at a time */
        if (numchars < chunksize)
            chunksize = numchars;
        status = pvt->pasynOctetDrv->write(pvt->octetPvt,
            pasynUser, data, chunksize, &n);
        if (status != asynSuccess) break;
        /* delay */
        epicsThreadSleep(delay);
        transfered+=n;
        numchars-=n;
        data+=n;
    }
    *nbytesTransfered = transfered;
    return status;
}

static asynStatus readIt(void *ppvt, asynUser *pasynUser,
    char *data, size_t maxchars, size_t *nbytesTransfered, int *eomReason)
{
    interposePvt *pvt = (interposePvt *)ppvt;

    return pvt->pasynOctetDrv->read(pvt->octetPvt,
        pasynUser, data, maxchars, nbytesTransfered, eomReason);
}

static asynStatus flushIt(void *ppvt, asynUser *pasynUser)
{
    interposePvt *pvt = (interposePvt *)ppvt;

    return pvt->pasynOctetDrv->flush(pvt->octetPvt, pasynUser);
}

static asynStatus registerInterruptUser(void *ppvt, asynUser *pasynUser,
    interruptCallbackOctet callback, void *userPvt, void **registrarPvt)
{
    interposePvt *pvt = (interposePvt *)ppvt;

    return pvt->pasynOctetDrv->registerInterruptUser(
        pvt->octetPvt,
        pasynUser, callback, userPvt, registrarPvt);
}

static asynStatus cancelInterruptUser(void *drvPvt, asynUser *pasynUser,
    void *registrarPvt)
{
    interposePvt *pvt = (interposePvt *)drvPvt;

    return pvt->pasynOctetDrv->cancelInterruptUser(
        pvt->octetPvt, pasynUser, registrarPvt);
}

static asynStatus setInputEos(void *ppvt, asynUser *pasynUser,
    const char *eos, int eoslen)
{
    interposePvt *pvt = (interposePvt *)ppvt;

    return pvt->pasynOctetDrv->setInputEos(pvt->octetPvt,
        pasynUser, eos, eoslen);
}

static asynStatus getInputEos(void *ppvt, asynUser *pasynUser,
    char *eos, int eossize, int *eoslen)
{
    interposePvt *pvt = (interposePvt *)ppvt;

    return pvt->pasynOctetDrv->getInputEos(pvt->octetPvt,
        pasynUser, eos, eossize, eoslen);
}

static asynStatus setOutputEos(void *ppvt, asynUser *pasynUser,
    const char *eos, int eoslen)
{
    interposePvt *pvt = (interposePvt *)ppvt;

    return pvt->pasynOctetDrv->setOutputEos(pvt->octetPvt,
        pasynUser, eos, eoslen);
}

static asynStatus getOutputEos(void *ppvt, asynUser *pasynUser,
    char *eos, int eossize, int *eoslen)
{
    interposePvt *pvt = (interposePvt *)ppvt;

    return pvt->pasynOctetDrv->getOutputEos(pvt->octetPvt,
        pasynUser, eos, eossize, eoslen);
}

static asynOctet octet = {
    writeIt, readIt, flushIt,
    registerInterruptUser, cancelInterruptUser,
    setInputEos, getInputEos, setOutputEos, getOutputEos
};

/* asynOption methods */

static asynStatus
getOption(void *ppvt, asynUser *pasynUser,
                              const char *key, char *val, int valSize)
{
    interposePvt *pvt = (interposePvt *)ppvt;
    if (epicsStrCaseCmp(key, "delay") == 0) {
        epicsSnprintf(val, valSize, "%g", pvt->delay);
        return asynSuccess;
    }
    if (epicsStrCaseCmp(key, "chunksize") == 0) {
        epicsSnprintf(val, valSize, "%lu", pvt->chunksize);
        return asynSuccess;
    }
    if (pvt->pasynOptionDrv)
        return pvt->pasynOptionDrv->getOption(pvt->optionPvt,
            pasynUser, key, val, valSize);
    epicsSnprintf(pasynUser->errorMessage,pasynUser->errorMessageSize,
        "Unknown option \"%s\"", key);
    return asynError;
}
static asynStatus
setOption(void *ppvt, asynUser *pasynUser, const char *key, const char *val)
{
    interposePvt *pvt = (interposePvt *)ppvt;
    if (epicsStrCaseCmp(key, "delay") == 0) {
        if (sscanf(val, "%lf", &pvt->delay) != 1) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "Bad number %s", val);
            return asynError;
        }
        return asynSuccess;
    }
    if (epicsStrCaseCmp(key, "chunksize") == 0) {
        if (sscanf(val, "%lu", &pvt->chunksize) != 1) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "Bad unsigned number %s", val);
            return asynError;
        }
        if (pvt->chunksize == 0) pvt->chunksize = 1;
        return asynSuccess;
    }
    if (pvt->pasynOptionDrv)
        return pvt->pasynOptionDrv->setOption(pvt->optionPvt,
            pasynUser, key, val);
    epicsSnprintf(pasynUser->errorMessage,pasynUser->errorMessageSize,
        "Unknown option \"%s\"", key);
    return asynError;
}

static asynOption option = {
    setOption, getOption
};

epicsShareFunc int
asynInterposeDelay(const char *portName, int addr, double delay, unsigned long chunksize)
{
    interposePvt *pvt;
    asynStatus status;
    asynInterface *poctetasynInterface;
    asynInterface *poptionasynInterface;

    pvt = callocMustSucceed(1, sizeof(interposePvt), "asynInterposeDelay");
    pvt->octet.interfaceType = asynOctetType;
    pvt->octet.pinterface = &octet;
    pvt->octet.drvPvt = pvt;
    status = pasynManager->interposeInterface(portName, addr,
        &pvt->octet, &poctetasynInterface);
    if ((status!=asynSuccess) || !poctetasynInterface) {
        printf("%s interposeInterface asynOctetType failed.\n", portName);
        free(pvt);
        return -1;
    }
    pvt->pasynOctetDrv = (asynOctet *)poctetasynInterface->pinterface;
    pvt->octetPvt = poctetasynInterface->drvPvt;

    pvt->option.interfaceType = asynOptionType;
    pvt->option.pinterface = &option;
    pvt->option.drvPvt = pvt;
    status = pasynManager->interposeInterface(portName, addr,
        &pvt->option, &poptionasynInterface);
    if ((status!=asynSuccess) || !poptionasynInterface) {
        status = pasynManager->registerInterface(portName,&pvt->option);
        if(status != asynSuccess) {
            printf("drvAsynSerialPortConfigure: Can't interpose or register option.\n");
        }
    } else {
        pvt->pasynOptionDrv = (asynOption *)poptionasynInterface->pinterface;
    }
    pvt->delay = delay;
    pvt->chunksize = chunksize ? chunksize : 1;
    return 0;
}

/* register asynInterposeDelay*/
static const iocshFuncDef asynInterposeDelayFuncDef =
    {"asynInterposeDelay", 4, (const iocshArg *[]) {
    &(iocshArg) { "portName", iocshArgString },
    &(iocshArg) {  "addr", iocshArgInt },
    &(iocshArg) {  "delay", iocshArgDouble },
    &(iocshArg) {  "chunksize", iocshArgInt },
}};

static void asynInterposeDelayCallFunc(const iocshArgBuf *args)
{
    asynInterposeDelay(args[0].sval, args[1].ival, args[2].dval, args[3].ival);
}

static void asynInterposeDelayRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&asynInterposeDelayFuncDef, asynInterposeDelayCallFunc);
    }
}
epicsExportRegistrar(asynInterposeDelayRegister);
