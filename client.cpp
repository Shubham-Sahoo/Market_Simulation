#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include "matching_engine.h"

using namespace std;

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

class RandomData {
    private:
        int price_low;
        int price_high;
        int quantity_low;
        int quantity_high;
        int order_id;
        string order_dir;
    public:
        RandomData(int order_id = 0, int price_l = 101, int price_h = 102, int quantity_l = 15, int quantity_h = 20) 
        {
            this->price_low = price_l;
            this->price_high = price_h;
            this->quantity_low = quantity_l;
            this->quantity_high = quantity_h;
            this->order_id = order_id;
            this->order_dir = "buy";
        }
        Order get_data(int seed) {
            // srand(seed);
            double p = (double)rand() / RAND_MAX;
            if (p <= 0.5) 
            {
                this->order_dir = "buy";
            } 
            else 
            {
                this->order_dir = "sell";
            }

            double price = (double)rand() / RAND_MAX * (this->price_high - this->price_low) + this->price_low;
            int quantity = (int)((double)rand() / RAND_MAX * (this->quantity_high - this->quantity_low) + this->quantity_low);
            return Order(this->order_id, this->order_dir, price, quantity);
        }
};

string get_order(int count)
{
    RandomData r;
    auto order_packet = r.get_data(count);
    order_packet.id = count;
    string data_packet = to_string(order_packet.id);
    data_packet.append(",");
    data_packet.append(order_packet.direction);
    data_packet.append(",");
    data_packet.append(to_string(order_packet.price));
    data_packet.append(",");
    data_packet.append(to_string(order_packet.quantity));
    data_packet.append(",");

    return data_packet;
}
void parse_trade(char *databuf)
{   
    string trade_data(databuf);
    string delimiter = ",";
    size_t pos;
    double price;
    int quantity;

    while(trade_data.find(delimiter) != string::npos)
    {
        pos = trade_data.find(delimiter);
        price = stof(trade_data.substr(0, pos));
        trade_data.erase(0, pos + delimiter.length());

        pos = trade_data.find(delimiter);
        quantity = stoi(trade_data.substr(0, pos));
        trade_data.erase(0, pos + delimiter.length());

        cout<<"Trade Price : "<<price<<", quantity : "<<quantity<<"\n"; 
    }
    
}

int __cdecl main(int argc, char **argv) 
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    


    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;
    
    // Validate the parameters
    if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }
    int count = 0;
    do
    {
        string data_packet;
        data_packet = get_order(count);

        const char *sendbuf = data_packet.data();

        // Send an initial buffer
        iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );

        if (iResult == SOCKET_ERROR) 
        {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }
        //printf("Bytes Sent: %ld\n", iResult);
        count++;

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 )
        {   
            parse_trade(recvbuf);
            //printf("Bytes received: %d\n", iResult);
        }
            
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    }while(count < 100);


    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}