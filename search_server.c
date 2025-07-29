#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <ctype.h>

#define BUFFER_SIZE 256
#define LIST_SIZE 256
#define ASCII 37

// 검색어 Trie 생성
struct trie_node root;

// TrieNode Struct 생성
struct trie_node {
	struct trie_node* children_trie_node[ASCII];		// 0 ~ 25 : 알파벳, 26 ~ 35 : 숫자
	int search_term_frequency;
};

// search_term Struct 생성
struct search_term_struct {
	char search_term[BUFFER_SIZE];
	int search_term_frequency;
};

// search_term을 trie에 insert
void insert_search_term_trie(char* search_term, int search_frequency) {
	// search_term(검색어)의 자리수 구하기
	size_t search_term_size = strlen(search_term);
	
	// trie 채우기
	struct trie_node* current_node = &root;
	for (int i = 0; i < search_term_size; i ++) {
		
		// trie에 넣을 문자 target_char 선언
		char target_char = search_term[i];
		
		// 대문자일 경우 소문자로 만들기
		target_char = tolower(target_char);
		
		// target_char이 들어갈 children_trie의 index 선언
		unsigned short index = 0;
		
		// target_char이 알파벳인 경우
		if (target_char >= 'a' && target_char <= 'z') {
			index = target_char - 'a';
		}
		// target_char이 SP(' ')인 경우
		else if (target_char == ' ') {
			index = 36;
		}
		// target_char이 숫자일 경우
		else {
			index = target_char - '0' + 26;
		}
		
		// trie_node가 존재하지 않는 경우, trie_node 새로 생성
		if (current_node->children_trie_node[index] == NULL) {
			current_node->children_trie_node[index] = (struct trie_node*) malloc(sizeof(struct trie_node));
		}
		
		// target_char이 마지막 문자일 경우 search_term_frequency값 넣기
		if (i == search_term_size - 1) {
			current_node->children_trie_node[index]->search_term_frequency = search_frequency;
		}
		else {
			current_node->children_trie_node[index]->search_term_frequency = 0;
		}
		
		// cerrent_node update
		current_node = current_node->children_trie_node[index];
	}
}

// 재귀적으로 하위 node를 찾아 하위 node가 leaf node일 경우 하위 node로 이동, 현재 node가 file에 등록된 search_term인 경우 search_term_list에 추가
void save_search_term_including_word_node(struct trie_node* node, char* search_term, char* word, struct search_term_struct search_term_list[LIST_SIZE], int* list_count) {
	
	// 현재 node가 file에 등록된 search_term인 경우 + word가 빈 문자열이 아닌 경우
	if (node->search_term_frequency != 0 && strlen(word) != 0) {
		
		// 현재 node가 word를 포함하고 있을 경우
		if (strstr(search_term, word) != NULL) {
			
			// list_count 업데이트, search_term_list에 search_term 저장
			strcpy(search_term_list[*list_count].search_term, search_term);
			search_term_list[*list_count].search_term_frequency = node->search_term_frequency;
			*list_count = *list_count + 1;
		}
	}
	
	char search_term_added[BUFFER_SIZE] = "";
	// 하위 node 탐색
	for (int i = 0; i < ASCII; i ++) {
		
		// search_term_added 초기화
		strcpy(search_term_added, search_term);
		
		// 하위 node가 존재하지 않는 경우
		if (node->children_trie_node[i] == NULL) {
			continue;
		}
		// 하위 node가 존재하는 경우
		else {
			
			// 하위 node의 문자를 search_term에 추가
			char add_str[2] = "";
			
			// 추가되는 문자가 알파벳인 경우
			if (i >= 0 && i <= 25) {
				add_str[0] = i + 97;
			}
			// 추가되는 문자가 SP(' ')인 경우
			else if (i == 36) {
				add_str[0] = ' ';
			}
			// 추가되는 문자가 숫자인 경우
			else {
				add_str[0] = i - 26 + 48;
			}
			
			strcat(search_term_added, add_str);
			
			save_search_term_including_word_node(node->children_trie_node[i], search_term_added, word, search_term_list, list_count);
		}
	}
}

// 검색어 파일 읽고 trie에 추가하기
void read_search_file_and_create_trie(char* filename) {
	FILE* fp = fopen(filename, "r");
	while(feof(fp) == 0) {
		char buffer[BUFFER_SIZE] = "";
		char* search_term = "";
		int search_frequency = 0;
		
		// 검색어 파일 한 줄씩 읽어오기
		fgets(buffer, BUFFER_SIZE, fp);
		
		// 검색어와 검색 횟수를 나누는 delimiter는 '\t'
		search_term = strtok(buffer, "\t");
		search_frequency = atoi(strtok(NULL, "\t"));
		
		// insert_search_term_trie 호출
		insert_search_term_trie(search_term, search_frequency);
	}
}

// client에게 search_term_list를 전송하는 함수
void send_search_term_list_to_client(struct search_term_struct search_term_list[LIST_SIZE], int list_count, int client_sock) {
	
	// list_count(list의 개수) 전송
	write(client_sock, &list_count, sizeof(list_count));
	
	// search_term_list 전송
	for (int i = 0; i < list_count; i ++) {
		
		// search_term의 크기 전송
		size_t search_term_size = strlen(search_term_list[i].search_term)+1;
		write(client_sock, &search_term_size, sizeof(search_term_size));
		
		// search_term 전송
		write(client_sock, search_term_list[i].search_term, search_term_size);
	}
}

// quick_sort에서 사용되는 swap 함수
void swap(struct search_term_struct* a, struct search_term_struct* b) {
	struct search_term_struct temp = *a;
	*a = *b;
	*b = temp;
}

// search_term_list 정렬
void quick_sort(struct search_term_struct search_term_list[LIST_SIZE], int start, int end) {
	if (start >= end) {
		return;
	}
	
	int key = start, i = start + 1, j = end, temp;
	
	while (i <= j) {
		while (i <= end && search_term_list[i].search_term_frequency >= search_term_list[key].search_term_frequency) {
			i++;
		}
		while (j > start && search_term_list[j].search_term_frequency <= search_term_list[key].search_term_frequency) {
			j--;
		}
		
		if (i > j) {
			swap(&search_term_list[key], &search_term_list[j]);
		}
		else {
			swap(&search_term_list[i], &search_term_list[j]);
		}
	}
	
	quick_sort(search_term_list, start, j - 1);
	quick_sort(search_term_list, j + 1, end); 
}

// thread_handler
void* thread_handler(void* arg) {
	int client_sock = *((int*)arg);
	
	printf("client_sock : %d\n", client_sock);
	
	struct search_term_struct search_term_list[LIST_SIZE];
	int list_count = 0;
	char word[BUFFER_SIZE] = "";
	
	while (true) {
		size_t word_size = 0, received_byte = 0, read_byte = 0;
		
		// client로부터 word 크기 받기
		read_byte = read(client_sock, &word_size, sizeof(word_size));
		if (read_byte == 0) {
			printf("socket %d closed\n", client_sock);
			pthread_exit(NULL);
		}
		
		// word 초기화
		for (int i = 0; i < BUFFER_SIZE; i ++) {
			word[i] = 0;
		}
		
		// client로부터 word 받기
		while(received_byte < word_size && word_size != 0) {
		
			// client로부터 word 받기
			read_byte = read(client_sock, word, word_size);
			if (read_byte == 0) {
				printf("socket %d closed\n", client_sock);
				pthread_exit(NULL);
			}
			received_byte += read_byte;
		}
		
		// search_term_list, list_count 초기화
		list_count = 0;
		for (int i = 0; i < LIST_SIZE; i ++) {
			search_term_list[i].search_term_frequency = 0;
			for (int j = 0; j < BUFFER_SIZE; j ++) {
				search_term_list[i].search_term[j] = 0;
			}
		}
		
		// search_term_list에 search_term 받기
		save_search_term_including_word_node(&root, "", word, search_term_list, &list_count);
		
		// search_term_list를 search_term_frequency를 기준으로 정렬
		quick_sort(search_term_list, 0, list_count);
		
		// search_term_list의 요소가 10개 미만일 경우
		if (list_count < 10) {
			send_search_term_list_to_client(search_term_list, list_count, client_sock);
		}
		// search_term_list의 요소가 10개 이상일 경우 10개만 출력
		else {
			send_search_term_list_to_client(search_term_list, 10, client_sock);
		}
	}
	
	pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
	int server_sock, client_sock;
	struct sockaddr_in server_addr, client_addr;
	int server_addr_size = sizeof(server_addr), client_addr_size = sizeof(client_addr);
	
	// input 확인
	if (argc != 3) {
		printf("Usage : %s <Port> <File_name>\n", argv[0]);
		exit(-1);
	}
	
	// socket()
	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock == -1) {
		printf("[Error] socket()\n");
		exit(-1);
	}
	
	// server_sock 세팅
	memset(&server_addr, 0, server_addr_size);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("203.252.112.31");
	server_addr.sin_port = htons(atoi(argv[1]));
	
	// client_sock 세팅
	memset(&client_addr, 0, client_addr_size);
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = 0;
	client_addr.sin_port = 0;
	
	// bind()
	if (bind(server_sock, (struct sockaddr*) &server_addr, server_addr_size) == -1) {
		printf("[Error] bind()\n");
		exit(-1);
	}
	
	// listen()
	if (listen(server_sock, 10) == -1) {
		printf("[Error] listen()\n");
		exit(-1);
	}
	
	// 검색어 파일 읽고 trie에 추가하기
	read_search_file_and_create_trie(argv[2]);
	
	// 반복적으로 client의 요청을 받고 해당 client와의 통신을 처리하는 thread 생성
	pthread_t thread_id;
	while(true) {
		
		// accept()
		client_sock = accept(server_sock, (struct sockaddr*) &client_addr, &client_addr_size);
		if (client_sock == -1) {
			printf("[Error] accept()\n");
			exit(-1);
		}
		
		// pthread_create()
		if (pthread_create(&thread_id, NULL, thread_handler, (void*) &client_sock) != 0) {
			printf("[Error] pthread_create()\n");
			exit(-1);
		}
		pthread_detach(thread_id);
	}
	
	close(server_sock);
	
	return 0;
}
