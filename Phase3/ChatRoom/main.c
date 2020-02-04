#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <winsock2.h>

#define PORT 12345
#define BUFFER_MAX 15000
#define USER_MAX 50
#define PASS_MAX 50
#define SELECTED_MAX 20
#define CHANNEL_MAX 50
#define MSG_MAX 10000
#define SA struct sockaddr
#define IP_ADDRESS "127.0.0.1"

static char *getElement(char *, char *);
int getArraySize(char *);
int getArraySizeByString(char *);
static char *getArrayElement(char *, int);
static char *getArrayString(char *, int);
void make_socket();
void account_menu();
void channel_menu();
char *concat(int , ...);

int client_socket, server_socket;
struct sockaddr_in servaddr, cliaddr;

static char *getElement(char buffer[], char element[])
{
    char *start = NULL;
    char *end = NULL;
    char *result = NULL;

    if((start = strstr(buffer, element)) != NULL) {
        start += strlen(element) + 1;
        for(int i = 0;; ++i)
        {
            if(*(start + i) == '\"' || *(start + i) == '[')
            {
                start += i;
                break;
            }
        }
    }

    if(*start == '\"')
    {
        ++start;
        end = start;
        size_t len = 0;
        while(*end != '\"')
        {
            ++len;
            ++end;
        }
        result = (char *)calloc(len + 1, sizeof(char));
        memcpy(result, start, len);
        return result;
    }

    if(*start == '[')
    {
        ++start;
        end = start;
        size_t len = 0;
        while(*end != ']')
        {
            ++len;
            ++end;
        }
        result = (char *)calloc(len + 1, sizeof(char));
        memcpy(result, start, len);
        return result;
    }
}

int getArraySize(char buffer[])
{
    int size = 0;
    for(int i = 0; i < strlen(buffer); ++i)
    {
        if(buffer[i] == '{')
            {
                ++size;
            }
    }
    return size;
}

int getArraySizeByString(char buffer[])
{
    int size = 0;
    for(int i = 0; i < strlen(buffer); ++i)
    {
        if(buffer[i] == '\"')
            {
                ++size;
            }
    }
    return size/2;
}

static char *getArrayElement(char buffer[], int element)
{
    char *start = NULL;
    char *end = NULL;
    char *result = NULL;

    int brackets_s = 0;
    int brackets_e = 0;
    int i;
    int j;
    for(i = 0; i < strlen(buffer); ++i)
    {
        if(buffer[i] == '{')
        {
            ++brackets_s;
        }
        if(brackets_s == element + 1)
        {
            start = buffer + i;
            break;
        }
    }

    for(j = 0; i < strlen(buffer); ++j)
    {
        if(buffer[j] == '}')
        {
            ++brackets_e;
        }
        if(brackets_e == element + 1)
        {
            end = buffer + j;
            break;
        }
    }
    size_t len = j - i + 1;
    result = (char *)calloc(len + 1, sizeof(char));
    memcpy(result, start, len);
    return result;
}

static char *getArrayString(char buffer[], int element)
{
    char *start = NULL;
    char *end = NULL;
    char *result = NULL;

    int count = 0;
    int i;
    int j;
    for(i = 0; i < strlen(buffer); ++i)
    {
        if(buffer[i] == '\"')
        {
            ++count;
        }
        if((count + 1) / 2 == element + 1)
        {
            start = buffer + (i + 1);
            break;
        }
    }
    count = 0;
    for(j = 0; j < strlen(buffer); ++j)
    {
        if(buffer[j] == '\"')
        {
            ++count;
        }
        if(count / 2 == element + 1)
        {
            end = buffer + (j - 1);
            break;
        }
    }
    size_t len = j - i - 1;
    result = (char *)calloc(len + 1, sizeof(char));
    memcpy(result, start, len);
    return result;
}

void make_socket(){

    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

	// Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        // Tell the user that we could not find a usable Winsock DLL.
        exit(0);
    }

	// Create and verify socket
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket == -1) {
		exit(0);
	}

	// Assign IP and port
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	servaddr.sin_port = htons(PORT);
}

void account_menu(){

    while(1){
        puts("Account Menu:");
        puts("1: Register");
        puts("2: Login");
        puts("3: Exit");

        char selected[SELECTED_MAX];
        memset(selected, 0, SELECTED_MAX);

        for(int n = 0;; ++n){
            selected[n] = getchar();
            if(selected[n] == '\n'){
                selected[n] = '\0';
                break;
            }
        }

        char buffer[BUFFER_MAX];

        char username[USER_MAX];
        char password[PASS_MAX];

        memset(username, 0, USER_MAX);
        memset(password, 0, PASS_MAX);

        if(!strcmp(selected, "1")){

            make_socket();

            //getting the username and password here
            printf("Enter a username : (containing no spaces. MAXIMUM number of characters : %d)\n", USER_MAX);
            for(int n = 0;; ++n){
                username[n] = getchar();
                if(username[n] == '\n'){
                    username[n] = '\0';
                    break;
                }
            }
            printf("Enter a password : (containing no spaces. MAXIMUM number of characters : %d)\n", PASS_MAX);
            for(int n = 0;; ++n){
                password[n] = getchar();
                if(password[n] == '\n'){
                    password[n] = '\0';
                    break;
                }
            }

            memset(buffer, 0, sizeof(char *));
            //utility function for merging strings
            char *buffer_temp = concat(5, "register ", username, ", ", password, "\n");

            for(int i = 0; *(buffer_temp + i) != 0; ++i)
                buffer[i] = *(buffer_temp + i);

            // Connect the client socket to server socket
            if (connect(client_socket, (SA*)&servaddr, sizeof(servaddr)) != 0) {
                exit(0);
            }

            send(client_socket, buffer, sizeof(buffer), 0);
            memset(buffer, 0, sizeof(char *));

            recv(client_socket, buffer, sizeof(buffer), 0);

            closesocket(server_socket);

            if(!strcmp(getElement(buffer, "type"), "Successful"))
                printf("Account successfully created.\n");
            else
                printf("Error: %s\n", getElement(buffer, "content"));


        }else if(!strcmp(selected, "2")){

            make_socket();
            //getting the username and password here
            puts("Enter your username : ");
            for(int n = 0;; ++n){
                username[n] = getchar();
                if(username[n] == '\n'){
                    username[n] = '\0';
                    break;
                }
            }
            puts("Enter your password : ");
            for(int n = 0;; ++n){
                password[n] = getchar();
                if(password[n] == '\n'){
                    password[n] = '\0';
                    break;
                }
            }

            memset(buffer, 0, sizeof(char *));
            //utility function for merging strings
            char *buffer_temp = concat(5, "login ", username, ", ", password, "\n");

            for(int i = 0; *(buffer_temp + i) != 0; ++i)
                buffer[i] = *(buffer_temp + i);

            // Connect the client socket to server socket
            if (connect(client_socket, (SA*)&servaddr, sizeof(servaddr)) != 0) {
                exit(0);
            }

            send(client_socket, buffer, sizeof(buffer), 0);
            memset(buffer, 0, sizeof(char *));

            recv(client_socket, buffer, sizeof(buffer), 0);

            closesocket(client_socket);

            if(!strcmp(getElement(buffer, "type"), "AuthToken"))
                channel_menu(getElement(buffer, "content"));
            else
                printf("Error: %s\n", getElement(buffer, "content"));

        }else if(!strcmp(selected, "3")){
            puts("Exiting..");
            return;
        }else
            puts("That item is not on the list.");
    }
}


void channel_menu(char auth[]){
    while(1){

        puts("Successfully logged in.");
        puts("1: Create Channel");
        puts("2: Join Channel");
        puts("3: logout");

        char selected[SELECTED_MAX];
        memset(selected, 0, SELECTED_MAX);

        for(int n = 0;; ++n){
            selected[n] = getchar();
            if(selected[n] == '\n'){
                selected[n] = '\0';
                break;
            }
        }

        char buffer[BUFFER_MAX];
        char channel[CHANNEL_MAX];
        memset(channel, 0, CHANNEL_MAX);

        if(!strcmp(selected, "1")){

            make_socket();

            printf("Enter channel name : (containing no spaces. MAXIMUM number of characters : %d\n", CHANNEL_MAX);
            for(int n = 0;; ++n){
                channel[n] = getchar();
                if(channel[n] == '\n'){
                    channel[n] = '\0';
                    break;
                }
            }

            memset(buffer, 0, sizeof(char *));
            //utility function for merging strings
            char *buffer_temp = concat(5, "create channel ", channel, ", ", auth, "\n");

            for(int i = 0; *(buffer_temp + i) != 0; ++i)
                buffer[i] = *(buffer_temp + i);

            // Connect the client socket to server socket
            if (connect(client_socket, (SA*)&servaddr, sizeof(servaddr)) != 0) {
                exit(0);
            }

            send(client_socket, buffer, sizeof(buffer), 0);
            memset(buffer, 0, sizeof(char *));

            recv(client_socket, buffer, sizeof(buffer), 0);

            closesocket(server_socket);

            if(!strcmp(getElement(buffer, "type"), "Successful"))
                chat_menu(auth);
            else
                printf("Error: %s\n", getElement(buffer, "content"));

        }else if(!strcmp(selected, "2")){

            make_socket();

            puts("Enter channel name : ");
            for(int n = 0;; ++n){
                channel[n] = getchar();
                if(channel[n] == '\n'){
                    channel[n] = '\0';
                    break;
                }
            }

            memset(buffer, 0, sizeof(char *));
            //utility function for merging strings
            char *buffer_temp = concat(5, "join channel ", channel, ", ", auth, "\n");

            for(int i = 0; *(buffer_temp + i) != 0; ++i)
                buffer[i] = *(buffer_temp + i);

            // Connect the client socket to server socket
            if (connect(client_socket, (SA*)&servaddr, sizeof(servaddr)) != 0) {
                exit(0);
            }

            send(client_socket, buffer, sizeof(buffer), 0);
            memset(buffer, 0, sizeof(char *));

            recv(client_socket, buffer, sizeof(buffer), 0);

            closesocket(server_socket);

            if(!strcmp(getElement(buffer, "type"), "Successful"))
                chat_menu(auth);
            else
                printf("Error: %s\n", getElement(buffer, "content"));

        }else if(!strcmp(selected, "3")){

            make_socket();

            memset(buffer, 0, sizeof(char *));
            //utility function for merging strings
            char *buffer_temp = concat(3, "logout ", auth, "\n");

            for(int i = 0; *(buffer_temp + i) != 0; ++i)
                buffer[i] = *(buffer_temp + i);

            // Connect the client socket to server socket
            if (connect(client_socket, (SA*)&servaddr, sizeof(servaddr)) != 0) {
                exit(0);
            }

            send(client_socket, buffer, sizeof(buffer), 0);
            memset(buffer, 0, sizeof(char *));

            recv(client_socket, buffer, sizeof(buffer), 0);

            closesocket(client_socket);

            if(!strcmp(getElement(buffer, "type"), "Successful"))
                puts("Logging out..");

            return;
        }else{
            puts("That item is not on the list.");
        }

    }
}

void chat_menu(char auth[]){

    while(1){
        puts("1: Send Message");
        puts("2: Refresh");
        puts("3: Channel Members");
        puts("4: Leave Channel");

        char selected[SELECTED_MAX];
        memset(selected, 0, SELECTED_MAX);

        for(int n = 0;; ++n){
            selected[n] = getchar();
            if(selected[n] == '\n'){
                selected[n] = '\0';
                break;
            }
        }

        char buffer[BUFFER_MAX];
        char message[MSG_MAX];
        memset(message, 0, MSG_MAX);

        if(!strcmp(selected, "1")){

            make_socket();

            printf("Say something : ");
            for(int n = 0;; ++n){
                message[n] = getchar();
                if(message[n] == '\n'){
                    message[n] = '\0';
                    break;
                }
            }

            memset(buffer, 0, sizeof(char *));
            //utility function for merging strings
            char *buffer_temp = concat(5, "send ", message, ", ", auth, "\n");

            for(int i = 0; *(buffer_temp + i) != 0; ++i)
                buffer[i] = *(buffer_temp + i);

            // Connect the client socket to server socket
            if (connect(client_socket, (SA*)&servaddr, sizeof(servaddr)) != 0) {
                exit(0);
            }

            send(client_socket, buffer, sizeof(buffer), 0);
            memset(buffer, 0, sizeof(char *));

            recv(client_socket, buffer, sizeof(buffer), 0);

            memset(buffer, 0, sizeof(buffer));

            closesocket(server_socket);

        }else if(!strcmp(selected, "2")){

            make_socket();

            memset(buffer, 0, sizeof(char *));
            //utility function for merging strings
            char *buffer_temp = concat(3, "refresh ", auth, "\n");

            for(int i = 0; *(buffer_temp + i) != 0; ++i)
                buffer[i] = *(buffer_temp + i);

            // Connect the client socket to server socket
            if (connect(client_socket, (SA*)&servaddr, sizeof(servaddr)) != 0) {
                exit(0);
            }

            send(client_socket, buffer, sizeof(buffer), 0);
            memset(buffer, 0, sizeof(char *));

            recv(client_socket, buffer, sizeof(buffer), 0);
            closesocket(client_socket);


            puts("Refreshing..");
            if(!strcmp(getElement(buffer, "type"), "List")){

                char *list = getElement(buffer, "content");
                int size = getArraySize(list);

                for(int i = 0; i < size; ++i){

                    char *element = getArrayElement(list, i);
                    char *sender = getElement(element, "sender");
                    char *content = getElement(element, "content");

                    printf("%s : %s\n", sender, content);
                }
            }


        }else if(!strcmp(selected, "3")){

            make_socket();

            memset(buffer, 0, sizeof(char *));
            //utility function for merging strings
            char *buffer_temp = concat(3, "channel members ", auth, "\n");

            for(int i = 0; *(buffer_temp + i) != 0; ++i)
                buffer[i] = *(buffer_temp + i);

            // Connect the client socket to server socket
            if (connect(client_socket, (SA*)&servaddr, sizeof(servaddr)) != 0) {
                exit(0);
            }

            send(client_socket, buffer, sizeof(buffer), 0);
            memset(buffer, 0, sizeof(char *));

            recv(client_socket, buffer, sizeof(buffer), 0);

            closesocket(client_socket);

            puts("Channel members : ");
            if(!strcmp(getElement(buffer, "type"), "List")){

                char *list = getElement(buffer, "content");
                int size = getArraySizeByString(list);

                for(int i = 0; i < size; ++i){

                    char *element = getArrayString(list, i);

                    printf("%s\n", element);
                }
            }

        }else if(!strcmp(selected, "4")){

            make_socket();

            memset(buffer, 0, sizeof(char *));
            //utility function for merging strings
            char *buffer_temp = concat(3, "leave ", auth, "\n");

            for(int i = 0; *(buffer_temp + i) != 0; ++i)
                buffer[i] = *(buffer_temp + i);

            // Connect the client socket to server socket
            if (connect(client_socket, (SA*)&servaddr, sizeof(servaddr)) != 0) {
                exit(0);
            }

            send(client_socket, buffer, sizeof(buffer), 0);
            memset(buffer, 0, sizeof(char *));

            recv(client_socket, buffer, sizeof(buffer), 0);

            closesocket(client_socket);

            if(!strcmp(getElement(buffer, "type"), "Successful"))
                puts("Leaving this channel..");

            return;

        }else{
            puts("That item is not on the list.");
        }
    }
}

char *concat(int count, ...)
{
    va_list ap;
    int i;

    // Find required length to store merged string
    int len = 1; // room for NULL
    va_start(ap, count);
    for(i=0 ; i<count ; i++)
        len += strlen(va_arg(ap, char*));
    va_end(ap);

    // Allocate memory to concat strings
    char *merged = calloc(sizeof(char),len);
    int null_pos = 0;

    // Actually concatenate strings
    va_start(ap, count);
    for(i=0 ; i<count ; i++)
    {
        char *s = va_arg(ap, char*);
        strcpy(merged+null_pos, s);
        null_pos += strlen(s);
    }
    va_end(ap);

    return merged;
}

int main(){

    account_menu();
    return 0;
}
