#pragma once

#include "resource.h"

#include "LC_ExtIO_Types.h"

int radioOpen(void);
int radioClose(void);
void radioShowGui(void);
void radioHideGui(void);
BOOL radioStartStream(char* ipAddress, USHORT udpPort, int64_t frequency, BOOL seqNumEnabled);
void radioStopStream(void);

