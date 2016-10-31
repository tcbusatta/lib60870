#include "iec60870_master.h"
#include "iec60870_common.h"
#include "hal_time.h"
#include "hal_thread.h"

#include <stdio.h>

static void
connectionHandler (void* parameter, T104Connection connection, bool closed)
{
    if (closed)
        printf("Connection closed\n");
    else
        printf("Connection established\n");
}

static bool
asduReceivedHandler (void* parameter, ASDU asdu)
{
    printf("RECVD ASDU type: %s(%i) elements: %i\n",
            TypeID_toString(ASDU_getTypeID(asdu)),
            ASDU_getTypeID(asdu),
            ASDU_getNumberOfElements(asdu));

    if (ASDU_getTypeID(asdu) == M_ME_TE_1) {

        printf("  measured scaled values with CP56Time2a timestamp:\n");

        int i;

        for (i = 0; i < ASDU_getNumberOfElements(asdu); i++) {

            MeasuredValueScaledWithCP56Time2a io =
                    (MeasuredValueScaledWithCP56Time2a) ASDU_getElement(asdu, i);

            printf("    IOA: %i value: %i\n",
                    InformationObject_getObjectAddress((InformationObject) io),
                    MeasuredValueScaled_getValue((MeasuredValueScaled) io)
            );

            MeasuredValueScaledWithCP56Time2a_destroy(io);
        }
    }
    else if (ASDU_getTypeID(asdu) == M_SP_NA_1) {
        printf("  single point information:\n");

        int i;

        for (i = 0; i < ASDU_getNumberOfElements(asdu); i++) {

            SinglePointInformation io =
                    (SinglePointInformation) ASDU_getElement(asdu, i);

            printf("    IOA: %i value: %i\n",
                    InformationObject_getObjectAddress((InformationObject) io),
                    SinglePointInformation_getValue((SinglePointInformation) io)
            );

            SinglePointInformation_destroy(io);
        }
    }

    return true;
}

int
main(int argc, char** argv)
{
    T104Connection con = T104Connection_create("localhost", IEC_60870_5_104_DEFAULT_PORT);

    T104Connection_setConnectionHandler(con, connectionHandler, NULL);

    if (T104Connection_connect(con)) {
        printf("Connected!\n");

        T104Connection_setASDUReceivedHandler(con, asduReceivedHandler, NULL);

        T104Connection_sendStartDT(con);

        Thread_sleep(5000);

        T104Connection_sendInterrogationCommand(con, ACTIVATION, 1, IEC60870_QOI_STATION);

        Thread_sleep(5000);

        InformationObject sc = (InformationObject)
                SingleCommand_create(NULL, 5000, true, false, 0);

        printf("Send control command C_SC_NA_1\n");
        T104Connection_sendControlCommand(con, C_SC_NA_1, ACTIVATION, 1, sc);

        InformationObject_destroy(sc);

        /* Send clock synchronization command */
        struct sCP56Time2a newTime;

        CP56Time2a_setFromMsTimestamp(&newTime, Hal_getTimeInMs());

        printf("Send time sync command\n");
        T104Connection_sendClockSyncCommand(con, 1, &newTime);

        Thread_sleep(1000);
    }
    else
        printf("Connect failed!\n");

    T104Connection_destroy(con);

    Thread_sleep(1000);

    printf("exit\n");
}


