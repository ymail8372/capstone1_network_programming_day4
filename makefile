CC = gcc

search_server.exe : search_server.c
	$(CC) search_server.c -o search_server.exe
	
search_client.exe : search_client.c
	$(CC) search_client.c -o search_client.exe