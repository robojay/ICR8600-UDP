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
pfnStopHW StopHW;

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

BOOL sendSequenceNumber = TRUE;

unsigned int udpHead = 0;
unsigned int udpTail = 0;
const unsigned int udpTotalBuffers = 64;
const unsigned int udpDataSize = 1024;
uint64_t udpSequenceNumber = 0;

#pragma pack(1)
struct UdpBufferEntry {
    uint64_t sequence;
    uint8_t data[udpDataSize];
};

UdpBufferEntry udpBuffer[udpTotalBuffers];

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
    if (radioStreaming && (cnt > 0)) {
        // Data
        if (udpStreaming) {
            // lots of assumption on data size, etc. right now!!!
            // assuming 512 IQ pairs per callback, 16bit unsigned ints
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
        return 0;
    }
    else {
        // Info

        // ** it is possible format changes while running ***
        // TODO: handle this...
        // following status codes to change sampleformat at runtime
        //            , extHw_SampleFmt_IQ_UINT8 = 126   // change sample format to unsigned 8 bit INT (Realtek RTL2832U)
        //                , extHw_SampleFmt_IQ_INT16 = 127   //           -"-           signed 16 bit INT
        //                , extHw_SampleFmt_IQ_INT24 = 128   //           -"-           signed 24 bit INT
        //                , extHw_SampleFmt_IQ_INT32 = 129   //           -"-           signed 32 bit INT
        //                , extHw_SampleFmt_IQ_FLT32 = 130   //           -"-           signed 16 bit FLOAT

        int retVal;

        // ignoring lots of stuff here... TODO!!!!

        retVal = 0;

        return retVal;
    }
    return 0;
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

BOOL radioStartStream(char *ipAddress, USHORT udpPort, int64_t frequency, BOOL seqNumEnabled) {
    radioStreaming = FALSE;
    udpStreaming = FALSE;

    udpHead = 0;
    udpTail = 0;

    iqPerCallback = StartHW64(frequency);

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
