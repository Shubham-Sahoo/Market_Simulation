#include <bits/stdc++.h>
#include <queue>

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


class RandomData {
    private:
        int price_low;
        int price_high;
        int quantity_low;
        int quantity_high;
        int order_id;
        string order_dir;
    public:
        RandomData(int order_id = 0, int price_l = 100, int price_h = 110, int quantity_l = 10, int quantity_h = 20) 
        {
            this->price_low = price_l;
            this->price_high = price_h;
            this->quantity_low = quantity_l;
            this->quantity_high = quantity_h;
            this->order_id = order_id;
            this->order_dir = "buy";
        }
        Order get_data() {
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
            double quantity = (double)rand() / RAND_MAX * (this->quantity_high - this->quantity_low) + this->quantity_low;
            return Order(this->order_id, this->order_dir, price, quantity);
        }
};


int main()
{
    MatchingEngine m;
    Order o[2];
    RandomData r;

    for(int i=0; i<10; i++)
    {   
        auto x = r.get_data();
        x.id = i;
        cout<<x.id<<"\t"<<x.price<<"\t"<<x.quantity<<"\t"<<x.direction<<"\n";
        m.process(x);

    }
    
    auto z = m.get_trades();

    for(int i = 0; i<z.size(); i++)
    {
        cout<<z[i].price<<endl;
    }
    
}