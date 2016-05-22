/*
The priority queue is implemented as a binary heap. The heap stores an index into its data array, so it can quickly update the weight of an item already in it.
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <time.h>
#include <math.h>

#include <string.h>
#include <winsock2.h>
#include <process.h>

#define BUFSIZE 100
#define BUFSIZE12 12
#define BUFSIZE4 4

DWORD WINAPI ClientConn(void *arg);
void SendMSG(char* message, int len);
void ErrorHandling(char *message);

int clntNumber = 0;
SOCKET clntSocks[10];
HANDLE hMutex;

#define BILLION 1000000000L;
#define FF 0xff;

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif


struct timezone
{
	int  tz_minuteswest; /* minutes W of Greenwich */
	int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag;

	if (NULL != tv)
	{
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		/*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tmpres /= 10;  /*convert into microseconds*/
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (NULL != tz)
	{
		if (!tzflag)
		{
			_tzset();
			tzflag++;
		}
	}

	return 0;
}

typedef struct {
	int vertex;
	int weight;
} edge_t;

typedef struct {
	edge_t **edges;
	int edges_len;
	int edges_size;
	int dist;
	int prev;
	int visited;
} vertex_t;

typedef struct {
	vertex_t **vertices;
	int vertices_len;
	int vertices_size;
} graph_t;

typedef struct {
	int *data;
	int *prio;
	int *index;
	int len;
	int size;
} heap_t;

void add_vertex(graph_t *g, int i) {
	if (g->vertices_size < i + 1) {
		int size = g->vertices_size * 2 > i ? g->vertices_size * 2 : i + 4;
		g->vertices = realloc(g->vertices, size * sizeof(vertex_t *));
		for (int j = g->vertices_size; j < size; j++)
			g->vertices[j] = NULL;
		g->vertices_size = size;
	}
	if (!g->vertices[i]) {
		g->vertices[i] = calloc(1, sizeof(vertex_t));
		g->vertices_len++;
	}
}

int add_edge(graph_t *g, int a, int b, int w) {
	a = a - 1;
	b = b - 1;
	add_vertex(g, a);
	add_vertex(g, b);
	vertex_t *v = g->vertices[a];
	if (v->edges_len >= v->edges_size) {
		v->edges_size = v->edges_size ? v->edges_size * 2 : 4;
		v->edges = realloc(v->edges, v->edges_size * sizeof(edge_t *));
	}
	int i;
	for (i = 0; i < v->edges_len; i++)
		if (v->edges[i]->vertex == b) break;
	if (i == v->edges_len)
	{
		edge_t *e = calloc(1, sizeof(edge_t));
		e->vertex = b;
		e->weight = w;
		v->edges[v->edges_len++] = e;
		return 1;
	}
	return 0;
}

heap_t *create_heap(int n) {
	heap_t *h = calloc(1, sizeof(heap_t));
	h->data = calloc(n + 1, sizeof(int));
	h->prio = calloc(n + 1, sizeof(int));
	h->index = calloc(n, sizeof(int));
	return h;
}

void push_heap(heap_t *h, int v, int p) {
	int i = h->index[v];
	if (!i) i = ++h->len;
	int j = i / 2;
	while (i > 1) {
		if (h->prio[j] < p)
			break;
		h->data[i] = h->data[j];
		h->prio[i] = h->prio[j];
		h->index[h->data[i]] = i;
		i = j;
		j = j / 2;
	}
	h->data[i] = v;
	h->prio[i] = p;
	h->index[v] = i;
}

int min2(heap_t *h, int i, int j, int k) {
	int m = i;
	if (j <= h->len && h->prio[j] < h->prio[m])
		m = j;
	if (k <= h->len && h->prio[k] < h->prio[m])
		m = k;
	return m;
}

int pop_heap(heap_t *h) {
	int v = h->data[1];
	h->data[1] = h->data[h->len];
	h->prio[1] = h->prio[h->len];
	h->index[h->data[1]] = 1;
	h->len--;
	int i = 1;
	while (1) {
		int j = min2(h, i, 2 * i, 2 * i + 1);
		if (j == i)
			break;
		h->data[i] = h->data[j];
		h->prio[i] = h->prio[j];
		h->index[h->data[i]] = i;
		i = j;
	}
	h->data[i] = h->data[h->len + 1];
	h->prio[i] = h->prio[h->len + 1];
	h->index[h->data[i]] = i;
	return v;
}

void dijkstra(graph_t *g, int a) {
	int i, j;
	a = a - 1;
	for (i = 0; i < g->vertices_len; i++) {
		vertex_t *v = g->vertices[i];
		v->dist = INT_MAX;
		v->prev = 0;
		v->visited = 0;
	}
	vertex_t *v = g->vertices[a];
	v->dist = 0;
	heap_t *h = create_heap(g->vertices_len);
	push_heap(h, a, v->dist);
	while (h->len) {
		i = pop_heap(h);
		v = g->vertices[i];
		v->visited = 1;
		for (j = 0; j < v->edges_len; j++) {
			edge_t *e = v->edges[j];
			vertex_t *u = g->vertices[e->vertex];
			if (!u->visited && v->dist + e->weight <= u->dist) {
				u->prev = i;
				u->dist = v->dist + e->weight;
				push_heap(h, e->vertex, u->dist);
			}
		}
	}
}

void print_path(graph_t *g) {
	int n, j;
	vertex_t *v, *u;

	for (int i = 0; i < g->vertices_len; i++)
	{
		printf("%d: ", i + 1);
		for (int j = 0; j < g->vertices[i]->edges_len; j++)
			printf("(%d, %d), ", g->vertices[i]->edges[j]->vertex + 1, g->vertices[i]->edges[j]->weight);
		printf("\n");
	}

	for (int k = 0; k < g->vertices_len; k++)
	{
		v = g->vertices[k];
		if (v->dist == INT_MAX) {
			printf("%d (%d): no path\n", k + 1, v->dist);
			continue;
		}
		for (n = 1, u = v; u->dist; u = g->vertices[u->prev], n++)
			;
		printf("%d (%d): ", k + 1, v->dist);
		int *path = malloc(n * sizeof(int));
		path[n - 1] = 1 + k;
		for (j = 0, u = v; u->dist; u = g->vertices[u->prev], j++)
			path[n - j - 2] = 1 + u->prev;
		for (j = 0; j < n; j++)
			printf("%d ", path[j]);
		printf("\n");
	}
}

//int datread(int fh, char *buffer, int size)
//{
//	int bytes;
//	if ((bytes = _read(fh, buffer, size)) == -1)
//	{
//		switch (errno)
//		{
//		case EBADF:
//			perror("Bad file descriptor!");  exit(-1);
//			break;
//		case ENOSPC:
//			perror("No space left on device!");  exit(-1);
//			break;
//		case EINVAL:
//			perror("Invalid parameter: buffer was NULL!");  exit(-1);
//			break;
//		default:
//			// An unrelated error occured
//			perror("Unexpected error!");  exit(-1);
//		}
//	}
//	return bytes;
//}
//
//void datwrite(int fh, char *buffer, int size)
//{
//	int bytes;
//	if ((bytes = _write(fh, buffer, size)) == -1 || bytes != size)
//	{
//		if (bytes != size) { perror("less bytes were writen!");  exit(-1); }
//		switch (errno)
//		{
//		case EBADF:
//			perror("Bad file descriptor!");  exit(-1);
//			break;
//		case ENOSPC:
//			perror("No space left on device!");  exit(-1);
//			break;
//		case EINVAL:
//			perror("Invalid parameter: buffer was NULL!");  exit(-1);
//			break;
//		default:
//			// An unrelated error occured
//			perror("Unexpected error!");  exit(-1);
//		}
//	}
//}

void SPA_compute(graph_t *g, int numvert, int startID)
{
	struct timeval start, end;

	gettimeofday(&start, NULL);

	dijkstra(g, startID);

	gettimeofday(&end, NULL);
	printf("computing time = %ld(microsecond)\n", ((end.tv_sec * 1000000 + end.tv_usec)
		- (start.tv_sec * 1000000 + start.tv_usec)));
}

DWORD WINAPI ClientConn(void *arg)
{
	SOCKET clntSock = (SOCKET)arg;
	int strLen = 0;
	char message[BUFSIZE];
	int i;

	while ((strLen = recv(clntSock, message, BUFSIZE, 0)) != 0)
		SendMSG(message, strLen);

	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clntNumber; i++) {   // 클라이언트 연결 종료시
		if (clntSock == clntSocks[i]) {
			for (; i < clntNumber - 1; i++)
				clntSocks[i] = clntSocks[i + 1];
			break;
		}
	}
	clntNumber--;
	ReleaseMutex(hMutex);

	closesocket(clntSock);
	return 0;
}

void SendMSG(char* message, int len)
{
	int i;
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clntNumber; i++)
		send(clntSocks[i], message, len, 0);
	ReleaseMutex(hMutex);
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

char* studuntID = NULL;
char* name = NULL;
char* portNumber = NULL;
char* tmp = NULL;
char buf[100] = { 0 };
char buf2[100] = { 0 };

int str_len;

int main() {
	int numvert = 0, numedge = 0, maxcost = 0, minedge = 0, maxedge = 0, startID = 0, mode, fileID;
	int fh1, fh2;
	int bytes, cnt, r, *costarray;
	int *temp, *edgearray;
	char filename[100], buffer[12];
	graph_t *g;

	WSADATA wsaData;
	SOCKET servSock;
	SOCKET clntSock;
	SOCKET sock;
	char addr[100], name[20], ID[20], msg[BUFSIZE12], message[100] = { 0 };
	int port;
	struct timeval start, end;
	int range, i, j;

	SOCKADDR_IN servAddr;
	SOCKADDR_IN clntAddr;
	int clntAddrSize;

	HANDLE hThread;
	DWORD dwThreadID;
	vertex_t *v;

	while (1)
	{
		printf("mode(1=graph generation, 5=client, 6=online graph server, 9=quit): ");
		scanf("%d", &mode);
		if (mode == 9) break;
		switch (mode)
		{
		case 1: // 1=graph generation
			g = calloc(1, sizeof(graph_t));
			printf("vertex#, startID, maxcost, min_edge#, max_edge#: ");
			scanf("%d %d %d %d %d", &numvert, &startID, &maxcost, &minedge, &maxedge);
			range = maxedge - minedge + 1;
			numedge = 0;
			for (int i = 0; i < numvert; i++)
			{
				int curvert = rand() % range + minedge;
				int upper = i + 1 + maxedge * 2; if (upper > numvert - 1) upper = numvert - 1;
				int buttom = upper - maxedge * 2; if (buttom < 0) buttom = 0;
				int r2 = upper - buttom + 1;
				while (g->vertices_size < i + 1 || !g->vertices[i] || curvert > g->vertices[i]->edges_len)
				{
					int loc = rand() % r2 + buttom;
					if (loc == i) continue;
					int ed = rand() % maxcost + 1;
					numedge += add_edge(g, i + 1, loc + 1, ed);
					numedge += add_edge(g, loc + 1, i + 1, ed);
				}
			}
			SPA_compute(g, numvert, startID);
			print_path(g);

			break;
		case 5: // client
			printf("IP Address, port, 이름, 학번 : ");
			scanf("%s %d %s %d", &addr, &port, &name, &studuntID);

			if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
				ErrorHandling("WSAStartup() error");
			sock = socket(PF_INET, SOCK_STREAM, 0); //IP4V, TCP전송 
			if (sock == -1)
				ErrorHandling("socket() error");
			/* client에 주소 할당 초기화 */
			memset(&clntAddr, 0, sizeof(clntAddr));
			clntAddr.sin_family = AF_INET;
			clntAddr.sin_addr.s_addr = inet_addr(addr);
			clntAddr.sin_port = htons(port);
			/* server connection */
			if ((connect(sock, (SOCKADDR *)&clntAddr, sizeof(clntAddr))) == SOCKET_ERROR)
				ErrorHandling("connect() error");

			memset(msg, 0, sizeof(char) * BUFSIZE12);
			sprintf(msg, "%s(%d)\0", &name, studuntID);
			send(sock, msg, strlen(msg), 0);
			
			memset(msg, 0, sizeof(char) * BUFSIZE12);
			recv(sock, msg, BUFSIZE12, 0);
			
			for (i = 0; i < 4; i++) {
				numvert += (int)msg[i] & FF;
				if (i != 3)
					numvert <<= 8;
			}

			for (i = 4; i < 8; i++) {
				numedge += (int)msg[i] & FF;
				if (i != 7)
					numedge <<= 8;
			}

			for (i = 8; i < 12; i++) {
				startID += (int)msg[i] & FF;
				if (i != 11)
					startID <<= 8;
			}

			g = calloc(1, sizeof(graph_t));

			for (i = 0; i < numedge; i++) {
				int start = 0, end = 0, cost = 0;

				memset(msg, 0, BUFSIZE12);
				recv(sock, msg, BUFSIZE12, 0);

				for (j = 0; j < 4; j++) {
					start += (int)msg[j] & FF;
					if (j != 3)
						start <<= 8;
				}

				for (j = 4; j < 8; j++) {
					end += (int)msg[j] & FF;
					if (j != 7)
						end <<= 8;
				}

				for (j = 8; j < 12; j++) {
					cost += (int)msg[j] & FF;
					if (j != 11)
						cost <<= 8;
				}

				add_edge(g, start + 1, end + 1, cost);
			}

			SPA_compute(g, numvert, startID);
			print_path(g);

			for (int k = 0; k < g->vertices_len; k++) {
				char sender[BUFSIZE4] = { 0 };
				int temp = 0;

				v = g->vertices[k];
				temp = v->dist;

				for (j = 3; j > 0; j--) {
					sender[j] = temp & FF;
					temp >>= 8;
				}

				send(sock, sender, BUFSIZE4, 0);
			}

			memset(message, 0, sizeof(char) * BUFSIZE);
			recv(sock, message, BUFSIZE, 0);
			printf("%s\n", message);

			closesocket(sock);
			break;
		case 6: // online graph server
			/*printf("port : ");
			scanf("%d", &port);

			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
				ErrorHandling("WSAStartup() error!");

			servSock = socket(PF_INET, SOCK_STREAM, 0);
			if (servSock == INVALID_SOCKET)
				ErrorHandling("socket() error");

			memset(&servAddr, 0, sizeof(servAddr));
			servAddr.sin_family = AF_INET;
			servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			servAddr.sin_port = htons(port);

			if (bind(servSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
				ErrorHandling("bind() error");

			if (listen(servSock, 5) == SOCKET_ERROR)
				ErrorHandling("listen() error");

			clntAddrSize = sizeof(clntAddr);
			clntSock = accept(servSock, (SOCKADDR*)&clntAddr, &clntAddrSize);
			if (clntSock == INVALID_SOCKET)
				ErrorHandling("accept() error");

			recv(clntSock, msg, 100, 0);
			printf("%s\n", msg);
			memset(msg, 0, sizeof(char) * sizeof(msg));

			sprintf(buf2, "%d %d %d %d %d", numvert, startID, maxcost, minedge, maxedge);
			printf("%s\n", buf2);
			send(clntSock, buf2, sizeof(buf2), 0);

			closesocket(clntSock);*/
			break;
		default:
			break;
		}
	}
	return 0;
}
