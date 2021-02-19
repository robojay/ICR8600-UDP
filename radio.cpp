#include "framework.h"
#include "ICR8600-UDP.h"

#pragma comment(lib, "ws2_32.lib")

//using namespace System;
//using namespace System::ComponentModel;
//using namespace System::Collections;
//using namespace System::Windows::Forms;
//using namespace System::Data;
//using namespace System::Drawing;
//using namespace System::Net::Sockets;
//using namespace System::Text;

pfnInitHW InitHW;
pfnShowGUI ShowGUI;
pfnHideGUI HideGUI;
pfnSetCallback SetCallback;
pfnOpenHW OpenHW;
pfnCloseHW CloseHW;
pfnStartHW StartHW;
pfnStartHW64 StartHW64;
pfnSetHWLO64 SetHWLO64;
pfnStopHW StopHW;
pfnExtIoGetActualSrateIdx ExtIoGetActualSrateIdx;
pfnExtIoGetSetting ExtIoGetSetting;

//LPCWSTR dllName = L".\\ExtIO_Buffer.dll";
LPCWSTR dllName = L".\\ExtIO_ICR8600.dll";
HINSTANCE extIO;

BOOL radioOpened = FALSE;
BOOL hasGui = FALSE;
BOOL hasStartHW64 = FALSE;

BOOL radioStreaming = FALSE;
BOOL udpStreaming = FALSE;
BOOL threadShutdown = FALSE;

SOCKET udpSocket;
sockaddr_in udpDestination;

// WinSock
WSAData wsData;

std::thread udpThread;
std::condition_variable bufferUpdate;

int iqPerCallback = 0;
unsigned int bufferOverruns = 0;
int sRateIndex = 0;
const int sRateIndex_5120_kHz = 8;
const int bitRateIndex = 4;
BOOL sample24bit = FALSE;
BOOL sendSequenceNumber = TRUE;
BOOL sendFloat = FALSE;

unsigned int udpHead = 0;
unsigned int udpTail = 0;
const unsigned int udpTotalBuffers = 4096;
const unsigned int udpDataSize = 1024;
uint64_t udpSequenceNumber = 0;

#pragma pack(1)
struct UdpBufferEntry {
    uint64_t sequence;
    uint8_t data[udpDataSize];
};

UdpBufferEntry udpBuffer[udpTotalBuffers];

BOOL isBitDepth24() {
    static char dummy[1024];
    static char value[1024];
    int result = ExtIoGetSetting(bitRateIndex, dummy, value);
    // very crude...
    if (value[0] == '1') {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

void udpStreamThreadFunction() {
    std::mutex mtx;
    std::unique_lock<std::mutex> lck(mtx);

    udpStreaming = TRUE;
    while (!threadShutdown) {
        bufferUpdate.wait_for(lck, std::chrono::seconds(1));
        while (udpTail != udpHead) {
            if (sendSequenceNumber) {
                sendto(udpSocket, (const char*)&udpBuffer[udpTail], udpDataSize + 8, 0, (sockaddr*)&udpDestination, sizeof(udpDestination));
            }
            else {
                sendto(udpSocket, (const char*)&udpBuffer[udpTail].data, udpDataSize, 0, (sockaddr*)&udpDestination, sizeof(udpDestination));
            }
            // catch error?

            unsigned int tempTail = udpTail + 1;
            if (tempTail >= udpTotalBuffers) {
                tempTail = 0;
            }
            udpTail = tempTail;
        }
    }
    udpStreaming = FALSE;
}

int extIOCallback(int cnt, int status, float IQoffs, void* IQdata) 
{
    int retVal = 0;

    if (radioStreaming && (cnt > 0)) {
        // Data
        if (udpStreaming) {
            // lots of assumption on data size, etc. right now!!!
            // assuming 512 IQ pairs per callback

            if (sample24bit) {
                // handle the 24bit values
                // will stuff these as 32bit values
                // data is in little endian format
                // can stuff last byte as zero/sign extended and retain proper signed value
                // III0 QQQ0
                // assuming 512 pairs, which results in 512 * 8 bytes = 4096 bytes to send
                // do that in 4 packets of 1024 bytes
                uint8_t* iqPtr = (uint8_t*)IQdata;
                for (unsigned int i = 0; i < 4; i++) {
                    udpBuffer[udpHead].sequence = udpSequenceNumber++;
                    // need to do this the hard way
                    for (unsigned int dataIndex = 0; dataIndex < 1024; dataIndex += 4) {
                        int32_t* p = (int32_t*)iqPtr;
                        int32_t dInt32 = *p;  // this will have invalid high byte
                        dInt32 = dInt32 & 0x00ffffff;
                        if ((dInt32 & 0x00800000) == 0x00800000) {
                            dInt32 = dInt32 | 0xff000000;
                        }

                        if (sendFloat) {
                            float dFloat = (float)dInt32 / (float)8387967.0;
                            memcpy(&udpBuffer[udpHead].data[dataIndex], (uint8_t*)&dFloat, 4);
                        }
                        else {
                            memcpy(&udpBuffer[udpHead].data[dataIndex], (uint8_t*)&dInt32, 4);
                        }

                        iqPtr += 3;

                    }
                    unsigned int tempHead = udpHead + 1;
                    if (tempHead >= udpTotalBuffers) {
                        tempHead = 0;
                    }
                    if (tempHead == udpTail) {
                        bufferOverruns++;
                        //if (crazyDebug) std::cout << "o";
                        //else std::cerr << "[ExtIO_Buffer] Buffer overrun" << std::endl;
                        tempHead = udpHead;
                    }
                    else {
                        //if (crazyDebug) std::cout << "b";
                    }
                    udpHead = tempHead;
                    bufferUpdate.notify_one();
                }
            }
            else {
                if (sendFloat) {
                    // convert the 16bit ints to 32 bit floats and send
                    // assuming 512 pairs, which results in 512 * 8 bytes = 4096 bytes to send
                    // do that in 4 packets of 1024 bytes
                    int16_t* iqPtr = (int16_t*)IQdata;
                    for (unsigned int i = 0; i < 4; i++) {
                        udpBuffer[udpHead].sequence = udpSequenceNumber++;
                        // need to do this the hard way
                        for (unsigned int dataIndex = 0; dataIndex < 1024; dataIndex += 4) {
                            float dFloat = (float)*iqPtr++;
                            dFloat = dFloat / (float)32767.0;
                            memcpy(&udpBuffer[udpHead].data[dataIndex], (uint8_t*)&dFloat, 4);
                        }
                        unsigned int tempHead = udpHead + 1;
                        if (tempHead >= udpTotalBuffers) {
                            tempHead = 0;
                        }
                        if (tempHead == udpTail) {
                            bufferOverruns++;
                            //if (crazyDebug) std::cout << "o";
                            //else std::cerr << "[ExtIO_Buffer] Buffer overrun" << std::endl;
                            tempHead = udpHead;
                        }
                        else {
                            //if (crazyDebug) std::cout << "b";
                        }
                        udpHead = tempHead;
                        bufferUpdate.notify_one();
                    }
                }
                else {
                    // handle the 16bit values
                    // 512 * 4 bytes = 2048 bytes to send
                    // will do this in 2 packets of 1024 bytes
                    for (unsigned int i = 0; i < 2; i++) {
                        udpBuffer[udpHead].sequence = udpSequenceNumber++;
                        uint8_t* iqPtr = (uint8_t*)IQdata;
                        memcpy(&udpBuffer[udpHead].data, &iqPtr[i * udpDataSize], udpDataSize);

                        unsigned int tempHead = udpHead + 1;
                        if (tempHead >= udpTotalBuffers) {
                            tempHead = 0;
                        }
                        if (tempHead == udpTail) {
                            bufferOverruns++;
                            //if (crazyDebug) std::cout << "o";
                            //else std::cerr << "[ExtIO_Buffer] Buffer overrun" << std::endl;
                            tempHead = udpHead;
                        }
                        else {
                            //if (crazyDebug) std::cout << "b";
                        }
                        udpHead = tempHead;
                        bufferUpdate.notify_one();
                    }
                }
            }
        }
        retVal = 0;
    }
    else if (cnt == -1 ){
        // Info
        switch (status) {
            case extHw_Changed_SampleRate:
            case extHw_SampleFmt_IQ_INT16:
            case extHw_SampleFmt_IQ_INT24:
                // update sample rate and more importantly, bit depth
                // trust, well, ok, don't trust, verify...

                // see note about bit depth in radioStartStream()
                sRateIndex = ExtIoGetActualSrateIdx();
                if (sRateIndex == sRateIndex_5120_kHz) {
                    sample24bit = FALSE;
                }
                else {
                    sample24bit = isBitDepth24();
                }
                break;
            default:
                break;
        }

        // ignoring lots of stuff here... TODO!!!!

        retVal = 0;

    }
    else {
        // ???
        retVal = 0;
    }
    return retVal;
}

BOOL radioOpen() 
{
    extIO = LoadLibrary(dllName);

    if (extIO) 
    {
        InitHW = (pfnInitHW)GetProcAddress(extIO, "InitHW");

        ShowGUI = (pfnShowGUI)GetProcAddress(extIO, "ShowGUI");
        HideGUI = (pfnHideGUI)GetProcAddress(extIO, "HideGUI");
        if ((ShowGUI != NULL) && (HideGUI != NULL))
        {
            hasGui = TRUE;
        }

        SetCallback = (pfnSetCallback)GetProcAddress(extIO, "SetCallback");
        OpenHW = (pfnOpenHW)GetProcAddress(extIO, "OpenHW");
        CloseHW = (pfnCloseHW)GetProcAddress(extIO, "CloseHW");

        StartHW = (pfnStartHW)GetProcAddress(extIO, "StartHW");

        StartHW64 = (pfnStartHW64)GetProcAddress(extIO, "StartHW64");
        if (StartHW64 != NULL)
        {
            hasStartHW64 = TRUE;
        }

        StopHW = (pfnStopHW)GetProcAddress(extIO, "StopHW");

        SetHWLO64 = (pfnSetHWLO64)GetProcAddress(extIO, "SetHWLO64");

        ExtIoGetActualSrateIdx = (pfnExtIoGetActualSrateIdx)GetProcAddress(extIO, "ExtIoGetActualSrateIdx");
        ExtIoGetSetting = (pfnExtIoGetSetting)GetProcAddress(extIO, "ExtIoGetSetting");
    }
    else 
    {
        return FALSE;
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
        return FALSE;
    }

    char hwName[128];
    char hwModel[128];
    int hwType;

    if (!InitHW(hwName, hwModel, hwType)) 
    {
        FreeLibrary(extIO);
        return FALSE;
    }

    SetCallback(extIOCallback);
    OpenHW();

    radioOpened = true;
    return TRUE;
}

BOOL radioClose(void) 
{
    if (radioOpened) 
    {
        //StopHW();
        CloseHW();    
        WSACleanup();
        FreeLibrary(extIO);
    }
    return TRUE;
}

void radioShowGui(void)
{
    if (hasGui)
        ShowGUI();
}

void radioHideGui(void)
{
    if (hasGui)
        HideGUI();
}

BOOL radioStartStream(char *ipAddress, USHORT udpPort, int64_t frequency, BOOL seqNumEnabled, BOOL sendAsFloat) {
    radioStreaming = FALSE;
    udpStreaming = FALSE;

    udpHead = 0;
    udpTail = 0;

    iqPerCallback = StartHW64(frequency);
    // Evidently only the first call to StartHW64 sets the local oscillator ... ???
    SetHWLO64(frequency);

    // we don't really know the bit depth selected
    // at 5.12MHz, 16bit only is valid
    // however, if the user selects a lower sampling rate
    // and 24bit depth, then switches back to 5.12MHz
    // the bit depth returned by GetSetting() still indicates
    // 24bits
    // we'll force it here...
    // and do the same in the callback function if sample rate change is detected
    sRateIndex = ExtIoGetActualSrateIdx();
    if (sRateIndex == sRateIndex_5120_kHz) {
        sample24bit = FALSE;
    }
    else {
        sample24bit = isBitDepth24();
    }
    
    sendFloat = sendAsFloat;

    threadShutdown = FALSE;

    udpDestination.sin_family = AF_INET;
    inet_pton(AF_INET, ipAddress, &udpDestination.sin_addr.s_addr);
    udpDestination.sin_port = htons(udpPort);
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    //char testMsg[] = "Testing 1 2 3";
    //sendto(udpSocket, testMsg, strlen(testMsg), 0, (sockaddr*)&udpDestination, sizeof(udpDestination));
    //closesocket(udpSocket);

    sendSequenceNumber = seqNumEnabled;

    udpThread = std::thread(udpStreamThreadFunction);
    
    radioStreaming = TRUE;
    return TRUE;
    
}

void radioStopStream() {
    if (radioStreaming) {
        StopHW();
        radioStreaming = FALSE;
        threadShutdown = TRUE;
        udpThread.join();
        closesocket(udpSocket);
    }
}
