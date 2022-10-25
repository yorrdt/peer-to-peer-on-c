#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

char userName[255];
int PORT;

void sendMessagesToPeer();
void getMessageFromPeer(int server_fd);
void* client_thread(void* server_fd);

int main (int argc, char** argv) 
{
	/*
	printf("Enter name: ");
	scanf("%s", userName);

	printf("Enter port: ");
	scanf("%d", &PORT);
	*/

	int server_fd;
	struct sockaddr_in server_address;

	// Getting socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("[PeerError]: error in calling socket()\n");
        getchar();
        exit(1);
    }

    // Attaching socket
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    // Print the server socket address and port
    char server_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(server_address.sin_addr), server_ip, INET_ADDRSTRLEN);

    printf("[PeerMessage]: %s:%d\n", server_ip, (int)ntohs(server_address.sin_port));
	

	if (bind(server_fd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("[PeerError]: bind() call failed\n");
		close(server_fd);
		getchar();
        exit(1);
    }

    if (listen(server_fd, 5) == -1) {
		perror("[PeerError]: listen() call failed\n");
		close(server_fd);
		getchar();
        exit(1);
	}


	pthread_t thread_id;
	pthread_create(&thread_id, NULL, &client_thread, &server_fd);

	while(1) sendMessagesToPeer(); // getch loop

	shutdown(server_fd, SHUT_RDWR);
	close(server_fd);

	return 0;
}

void sendMessagesToPeer() // like client
{
	char buffer[1024] = {0};
	int PORT_server;

	printf("Enter the port to send message: ");
	scanf("%d", &PORT_server);

	int remote_peer_socket;
	struct sockaddr_in remote_peer_address;

	if ((remote_peer_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("[PeerError]: error in calling socket() --> sendMessages()\n");
        getchar();
        exit(1);
    }

    remote_peer_address.sin_family = AF_INET;
    remote_peer_address.sin_port = htons(PORT_server);
    remote_peer_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (connect(remote_peer_socket, (struct sockaddr*)&remote_peer_address, sizeof(remote_peer_address)) == -1) {
		perror("[PeerError]: connection error --> sendMessages()\n");
		close(remote_peer_socket);
		getchar();
		exit(1);
	}

	//fgets(buffer, sizeof(buffer), stdin);
	char message[1024] = {0};
	scanf("%s", message);
	sprintf(buffer, "%s (port:%d) says: %s", userName, PORT, message);
	send(remote_peer_socket, buffer, sizeof(buffer), 0);

	close(remote_peer_socket);
}

void* client_thread(void* server_fd) // reveive messages from another peer (client)
{
	int serv_fd = *((int*)server_fd);
	while (1) 
	{
		sleep(2);
		getMessageFromPeer(serv_fd);
	}
}

void getMessageFromPeer(int server_fd)
{
	struct sockaddr_in client_address;
	char buffer[1024] = {0};
	int client_address_length = sizeof(client_address);
	fd_set current_sockets, ready_sockets;

	FD_ZERO(&current_sockets);
	FD_SET(server_fd, &current_sockets);

	int count = 0;
	while (1) 
	{
		count++;
		ready_sockets = current_sockets;

		if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0)
		{
			perror("[PeerError]: error when select() --> getMessageFromPeer()\n");
			getchar();
			exit(1);
		}

		for (int i = 0; i < FD_SETSIZE; i++)
		{
			if (FD_ISSET(i, &ready_sockets))
			{
				if (i == server_fd)
				{
					int client_socket;
					if ((client_socket = accept(server_fd, (struct sockaddr*)&client_address, &client_address_length)) < 0)
					{
						perror("[PeerError]: accept() call failed --> getMessageFromPeer()\n");
						close(client_socket);
						getchar();
						exit(1);
					}
					FD_SET(client_socket, &current_sockets);
				}
				else 
				{
					recv(i, buffer, sizeof(buffer), 0);
					printf("\n%s\n", buffer);
					FD_CLR(i, &current_sockets);
				}
			}
		}
		if (count == (FD_SETSIZE * 2)) break; 
	}
}