# 'make all' for compiling all code in package
all: proxy client

# 'make server' for compiling Server.c
proxy: Proxy.c
	gcc -I . -pthread Proxy.c -o proxy

# 'make client' for compiling Client.c
client: Client.c
	gcc -I . Client.c -o client

# 'make clean' for discarding all previously created object files
clean:
	$(RM) client server
	
	
	





