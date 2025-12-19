#define _WIN32_WINNT 0x0600

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iomanip>
#include <string>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

std::string GetTcpState(DWORD state) {
    switch (state) {
        case MIB_TCP_STATE_CLOSED: return "CLOSED";
        case MIB_TCP_STATE_LISTEN: return "LISTENING";
        case MIB_TCP_STATE_SYN_SENT: return "SYN_SENT";
        case MIB_TCP_STATE_SYN_RCVD: return "SYN_RCVD";
        case MIB_TCP_STATE_ESTAB: return "ESTABLISHED";
        case MIB_TCP_STATE_FIN_WAIT1: return "FIN_WAIT1";
        case MIB_TCP_STATE_FIN_WAIT2: return "FIN_WAIT2";
        case MIB_TCP_STATE_CLOSE_WAIT: return "CLOSE_WAIT";
        case MIB_TCP_STATE_CLOSING: return "CLOSING";
        case MIB_TCP_STATE_LAST_ACK: return "LAST_ACK";
        case MIB_TCP_STATE_TIME_WAIT: return "TIME_WAIT";
        case MIB_TCP_STATE_DELETE_TCB: return "DELETE_TCB";
        default: return "UNKNOWN";
    }
}

std::string IpToString(DWORD ipAddress) {
    struct in_addr ipAddr;
    ipAddr.S_un.S_addr = ipAddress;
    char str[INET_ADDRSTRLEN];
    if (InetNtopA(AF_INET, &ipAddr, str, INET_ADDRSTRLEN)) {
        return std::string(str);
    }
    return "Invalid IP";
}

void ShowTcpTable() {
    PMIB_TCPTABLE2 pTcpTable;
    ULONG ulSize = 0;
    GetTcpTable2(NULL, &ulSize, TRUE);
    pTcpTable = (MIB_TCPTABLE2 *) malloc(ulSize);
    if (pTcpTable == NULL) return;
    if (GetTcpTable2(pTcpTable, &ulSize, TRUE) == NO_ERROR) {
        for (int i = 0; i < (int) pTcpTable->dwNumEntries; i++) {
            std::string localIp = IpToString(pTcpTable->table[i].dwLocalAddr);
            unsigned short localPort = ntohs((u_short) pTcpTable->table[i].dwLocalPort);
            std::string localFull = localIp + ":" + std::to_string(localPort);

            std::string remoteIp = IpToString(pTcpTable->table[i].dwRemoteAddr);
            unsigned short remotePort = ntohs((u_short) pTcpTable->table[i].dwRemotePort);
            std::string remoteFull = (remotePort == 0) ? "*:*" : (remoteIp + ":" + std::to_string(remotePort));

            std::string state = GetTcpState(pTcpTable->table[i].dwState);
            DWORD pid = pTcpTable->table[i].dwOwningPid;

            std::cout << std::left
                    << std::setw(7) << "TCP"
                    << std::setw(22) << localFull
                    << std::setw(22) << remoteFull
                    << std::setw(15) << state
                    << std::setw(10) << pid << "\n";
        }
    }
    free(pTcpTable);
}

void ShowUdpTable() {
    PMIB_UDPTABLE_OWNER_PID pUdpTable;
    ULONG ulSize = 0;
    GetExtendedUdpTable(NULL, &ulSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0);

    // 2. Выделяем память
    pUdpTable = (PMIB_UDPTABLE_OWNER_PID) malloc(ulSize);
    if (pUdpTable == NULL) return;
    if (GetExtendedUdpTable(pUdpTable, &ulSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR) {
        for (int i = 0; i < (int) pUdpTable->dwNumEntries; i++) {
            std::string localIp = IpToString(pUdpTable->table[i].dwLocalAddr);
            unsigned short localPort = ntohs((u_short) pUdpTable->table[i].dwLocalPort);
            std::string localFull = localIp + ":" + std::to_string(localPort);
            std::string remoteFull = "*:*";
            std::string state = "---";
            DWORD pid = pUdpTable->table[i].dwOwningPid;

            std::cout << std::left
                    << std::setw(7) << "UDP"
                    << std::setw(22) << localFull
                    << std::setw(22) << remoteFull
                    << std::setw(15) << state
                    << std::setw(10) << pid << "\n";
        }
    }
    free(pUdpTable);
}

int main() {
    SetConsoleOutputCP(65001);

    // Инициализация
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock init failed.\n";
        return 1;
    }

    std::cout << "==============================================================================\n";
    std::cout << "                        NETWORK MONITOR                       \n";
    std::cout << "==============================================================================\n";

    std::cout << std::left
            << std::setw(7) << "Proto"
            << std::setw(22) << "Local Address"
            << std::setw(22) << "Remote Address"
            << std::setw(15) << "State"
            << std::setw(10) << "PID" << "\n";
    std::cout << "------------------------------------------------------------------------------\n";

    // Сначала выводим TCP
    ShowTcpTable();

    // Затем выводим UDP
    ShowUdpTable();

    WSACleanup();
    system("pause");
    return 0;
}
