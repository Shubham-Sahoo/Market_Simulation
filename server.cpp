#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "./matching_engine.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

using namespace std;

class Compare1 {
public:
    bool operator()(Order x, Order y)
    {
        if (x.price < y.price) 
        {   
            return true;
        }
        else if(x.price == y.price)
        {
            if(x.id > y.id)
            {
                return true;
            }
        }
        return false;
    }
};

class Compare2 {
public:
    bool operator()(Order x, Order y)
    {
        if (x.price > y.price) 
        {   
            return true;
        }
        else if(x.price == y.price)
        {
            if(x.id > y.id)
            {
                return true;
            }
        }
        return false;
    }
};


class OrderBook 
{

public:
    OrderBook()
    {
    }

    int __len__() {
        return this->bids.size() + this->asks.size();
    }

    void add(Order order) {
        if (order.direction == "buy") {
            this->bids.insert(order);
        }
        else if (order.direction == "sell") {
            this->asks.insert(order);
        }
    }

    void remove(Order order) {
        if (order.direction == "buy") 
        {
            this->bids.erase(this->bids.find(order));
        }
        else if (order.direction == "sell") 
        {
            this->asks.erase(this->asks.find(order));
        }
    }    

    double best_ask() {
        if (this->asks.size() > 0) {
            return this->asks.begin()->price;
        }
        return 1000000;
    }

    double best_bid() {
        if (this->bids.size() > 0) {
            return this->bids.begin()->price;
        }
        return -1;
    }

    set<Order, Compare1> bids;
    set<Order, Compare2> asks;
};



class MatchingEngine 
{

public:
    MatchingEngine()
    {
    }

    void process(Order order) 
    {
        this->match(order);
    }

    std::vector<Trade> get_trades() 
    {
        std::vector<Trade> v;
        while (!(this->trades).empty())
        {
            v.push_back((this->trades).front());
            (this->trades).pop_front();
        }
        return v;
    }

private:
    void match(Order order) 
    {
        if (order.direction == "buy" && order.price >= this->orderbook.best_ask())
        {
            // Buy order crossed the spread
            int filled = 0;
            std::vector<Order> consumed_asks;
            for (auto i = this->orderbook.asks.begin(); i != this->orderbook.asks.end(); i++) 
            {
                Order ask = (*i);

                if (ask.price > order.price) 
                {
                    break; // Price of ask is too high, stop filling order
                } 
                else if (filled == order.quantity) 
                {
                    break; // Order was filled
                }

                if (filled + ask.quantity <= order.quantity) 
                { // order not yet filled, ask will be consumed whole
                    filled += ask.quantity;
                    Trade trade(ask.price, ask.quantity);
                    this->trades.insert(this->trades.begin(), trade);
                    consumed_asks.push_back(ask);
                } 
                else if (filled + ask.quantity > order.quantity) 
                { // order is filled, ask will be consumed partially
                    int volume = order.quantity - filled;
                    filled += volume;
                    Trade trade(ask.price, volume);
                    this->trades.insert(this->trades.begin(), trade);
                    ask.quantity -= volume;
                }
            }

            // Place any remaining volume in LOB
            if (filled < order.quantity) 
            {
                this->orderbook.add(Order(order.id, "buy", order.price, order.quantity - filled));
            }

            // Remove asks used for filling order
            for (Order ask : consumed_asks) 
            {
                this->orderbook.remove(ask);
            }
        }

        else if (order.direction == "sell" && order.price <= this->orderbook.best_bid()) 
        {
            // Sell order crossed the spread
            int filled = 0;
            std::vector<Order> consumed_bids;
            for (auto i = this->orderbook.bids.begin(); i != this->orderbook.bids.end(); i++) 
            {
                Order bid = (*i);

                if (bid.price < order.price) {
                    break; // Price of bid is too low, stop filling order
                }
                if (filled == order.quantity) {
                    break; // Order was filled
                }

                if (filled + bid.quantity <= order.quantity) { // order not yet filled, bid will be consumed whole
                    filled += bid.quantity;
                    Trade trade(bid.price, bid.quantity);
                    this->trades.insert(this->trades.begin(), trade);
                    consumed_bids.push_back(bid);
                }
                else if (filled + bid.quantity > order.quantity) { // order is filled, bid will be consumed partially
                    int volume = order.quantity - filled;
                    filled += volume;
                    Trade trade(bid.price, volume);
                    this->trades.insert(this->trades.begin(), trade);
                    bid.quantity -= volume;
                }
            }

            // Place any remaining volume in LOB
            if (filled < order.quantity) {
                this->orderbook.add(Order(order.id, "sell", order.price, order.quantity - filled));
            }

            // Remove bids used for filling order
            for (Order bid : consumed_bids) {
                this->orderbook.remove(bid);
            }
        }
        
        else 
        {
            // Order did not cross the spread, place in order book
            this->orderbook.add(order);
        }
    }

    // void run()
    // {
    //     while(this->queue.size() > 0)
    //     {
    //         if(this->queue.size() > 0)
    //         {
    //             Order order = this->queue.front();
    //             this->queue.pop_front();
    //             this->match(order);
    //         }
    //     }
    // }

    std::deque<Trade> trades;
    OrderBook orderbook;
    std::deque<Order> queue;

};

Order parse_buffer(char *recvbuf)
{   
    string data(recvbuf);
    string delimiter = ",";
    Order o;

    size_t pos = data.find(delimiter);
    o.id = stoi(data.substr(0, pos));
    data.erase(0, pos + delimiter.length());
    
    pos = data.find(delimiter);
    o.direction = data.substr(0, pos);
    data.erase(0, pos + delimiter.length());

    pos = data.find(delimiter);
    o.price = stof(data.substr(0, pos));
    data.erase(0, pos + delimiter.length());

    pos = data.find(delimiter);
    o.quantity = stoi(data.substr(0, pos));
    data.erase(0, pos + delimiter.length());

    o.show_order(o);

    return o;
}

void parse_trade(vector<Trade> *t, string *databuf)
{
    *databuf = "";
    string delimiter = ",";

    if(t->size() > 0)
    {
        for(auto i=0; i<t->size(); i++)
        { 
            databuf->append(to_string((*t)[i].price));
            databuf->append(",");
            databuf->append(to_string((*t)[i].quantity));
            databuf->append(",");
        }
    }
}


int __cdecl main(void) 
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char *recvbuf = new char[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);
    MatchingEngine m;
    // Receive until the peer shuts down the connection
    do {

        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            auto order = parse_buffer(recvbuf);
            m.process(order);
            auto trade = m.get_trades();
            
            string ch;
            parse_trade(&trade, &ch);
            memset(recvbuf, '\0', recvbuflen);
            strcpy(recvbuf, ch.data());
            //printf("Bytes received: %d\n", iResult);

        // Echo the buffer back to the sender
            iSendResult = send( ClientSocket, recvbuf, iResult, 0 );
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            //printf("Bytes sent: %d\n", iSendResult);
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else  {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    } while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}