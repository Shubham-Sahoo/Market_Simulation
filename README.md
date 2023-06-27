# Market_Simulation

To execute the Trading we have to run both the client and the server side code.

### Server Code
```
g++ server.cpp -o server -lws2_32
.\server.exe
```

### Client Code
```
g++ .\client.cpp -o client -lws2_32
.\client.exe 100 localhost
```
We can change the number of orders using the first argument, currently it is given "100" in this example.
Also, we can change the seed of the random function inside the client code.

