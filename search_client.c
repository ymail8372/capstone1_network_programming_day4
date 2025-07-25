#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>

#define BUFFER_SIZE 256

#define COLOR_GREEN "\033[38;2;0;255;0m"
#define COLOR_ORANGE "\033[38;2;255;128;0m"
#define COLOR_RESET "\033[0m"

int main(int argc, char* argv[]) {
	int sock;
	struct sockaddr_in server_addr;
	int server_addr_size = sizeof(server_addr);
	
	// input 확인
	if (argc != 3) {
		printf("[Error] Usage : %s <IP> <Port>\n", argv[0]);
		exit(-1);
	}
	
	
	// socket()
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printf("[Error] socket()\n");
		exit(-1);
	}
	
	// server_sock 세팅
	memset(&server_addr, 0, server_addr_size);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	server_addr.sin_port = htons(atoi(argv[2]));
	
	// connect()
	if (connect(sock, (struct sockaddr*) &server_addr, server_addr_size) == -1) {
		printf("[Error] connect\n");
		exit(-1);
	}
	
	// terminal 속성 변경
	struct termios new_termios;
	tcgetattr(0, &new_termios);
	
	new_termios.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(0, TCSANOW, &new_termios);
	
	// 입력 받기
	size_t word_size = 0;
	char c = 0;
	char word[BUFFER_SIZE] = "";
	while(true) {
		int list_count = 0;
		char search_term[BUFFER_SIZE] = "";
		
		c = getchar();
		
		// 입력이 있는 경우
		if (c != 0) {
			// consol 초기화
			system("clear");
			
			// backspace를 입력한 경우
			if (c == 127) {
				
				// word가 비어있는데 backspace를 입력한 경우
				if (word_size > 0) {
					word[strlen(word)-1] = 0;
					word_size -= 1;
				}
			}
			else {
				word[strlen(word)] = c;
				word_size += 1;
			}
			
			printf("%sSearch Word : %s%s\n", COLOR_GREEN, COLOR_RESET, word);
			printf("-------------------------------------------\n");
			
			if (strlen(word) != 0) {
				// server에게 word_size 전송
				write(sock, &word_size, sizeof(word_size));
				
				// server에게 word 전송
				write(sock, word, word_size);
				
				// server로부터 list_count 수신
				read(sock, &list_count, sizeof(list_count));
				//printf("list_count : %d\n", list_count);
				
				// server로부터 search_term 수신
				for (int i = 0; i < list_count; i ++) {
					size_t total_size = 0, received_byte = 0, read_byte = 0;
					
					// search_term의 크기 수신
					read(sock, &total_size, sizeof(total_size));
					//printf("total_size : %ld\n", total_size);
					
					// search_term 수신
					while (received_byte < total_size) {
						read_byte = read(sock, search_term, total_size - received_byte);
						received_byte += read_byte;
					}
					
					// search_term 출력
					char* search_term_colored = strstr(search_term, word);
					
					int index = 0;
					while (true) {
						if (search_term[index] == 0) {
							break;
						}
						
						if (&search_term[index] == search_term_colored) {
							printf("%s%s%s", COLOR_ORANGE, word, COLOR_RESET);
							index += strlen(word);
							
							if (search_term[index] == 0) {
								break;
							}
						}
						
						printf("%c", search_term[index]);
						
						index += 1;
					}
					printf("\n");
				}
			}
		}
		
		// c 초기화
		c = 0;
	}
	
	close(sock);
	
	return 0;
}
