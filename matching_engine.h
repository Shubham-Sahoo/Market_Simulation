#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include <bits/stdc++.h>

using namespace std;

class Order 
{
public:
    Order(int id = 0, string direction = "buy", double price = 100, int quantity = 10)   // Constructor
    {
        this->id = id;
        transform(direction.begin(), direction.end(), direction.begin(), ::tolower);
        this->direction = direction;
        this->price = price;
        this->quantity = quantity;
    }

    void show_order(Order order)
    {
        cout<<"Order id : "<<order.id<<"\n";
        cout<<"Order Direction : "<<order.direction<<"\n";
        cout<<"Order Price : "<<order.price<<"\n";
        cout<<"Order Quantity : "<<order.quantity<<"\n";
    }

    int id;
    string direction;
    double price;
    int quantity;
};

class Trade {
public:
    Trade(double price, int quantity) 
    {
        this->price = price;
        this->quantity = quantity;
    }

    double price;
    int quantity;
};


#endif