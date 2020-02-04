#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <dirent.h>
#include <stdarg.h>

#include "cJSON.c"
#include "cJSON.h"

#define AUTH_LENGTH 32
#define PORT 12345
#define BUFFER_MAX 15000
#define USER_MAX 50
#define PASS_MAX 50
#define SELECTED_MAX 20
#define CHANNEL_MAX 50
#define USER_COUNT 100
#define CHANNEL_COUNT 100
#define MSG_MAX 10000
#define SA struct sockaddr
#define IP_ADDRESS "127.0.0.1"

int server_socket, client_socket;
struct sockaddr_in server, client;

char users[USER_COUNT][USER_MAX] = {};
char channels[CHANNEL_COUNT][CHANNEL_MAX] = {};
int usercount = 0;
int channelcount = 0;
bool loggedin[USER_COUNT] = {};
bool inchannel[CHANNEL_COUNT][USER_COUNT] = {};
int msgindex[CHANNEL_COUNT][USER_COUNT] = {};
char auth_tokens[USER_COUNT][AUTH_LENGTH + 1] = {};

void readfiles();
void recieveRequest();
void makeSocket();
bool fileExists(DIR *, char *);
void makeServerSocket();
int reg(char *);
int login(char *);
int create_channel(char *);
int join_channel(char *);
int logout(char *);
int send_msg(char *);
int refresh(char *);
int channel_members(char *);
int leave(char *);
bool userExists(char *);
bool channelExists(char *);
int userindex(char *);
void createUser(char *, char *);
char *concat(int , ...);
bool checkPass(char *, char *);
bool makeAuth(int);
int userindex_auth(char *);
bool authValidity(char *);
int channelindex_auth(char *);

//codes
enum regcodes{REG_SUCCESSFUL = 0, REG_ALREADY_EXISTS = 1};
enum logcodes{LOGIN_SUCCESSFUL = 0, LOGIN_ALREADY_LOGGEDIN = 1, LOGIN_WRONG_PASS = 2, LOGIN_NOT_EXISTS = 3};
enum crchcodes{CRCH_SUCCESSFUL = 0, CRCH_ALREADY_EXISTS = 1};
enum jochcodes{JOCH_SUCCESSFUL = 0, JOCH_NOT_EXISTS = 1};
enum logoutcodes{LOGOUT_SUCCESSFUL = 0, LOGOUT_FAILED = 1};
enum msgcodes{MSG_NOT_IN_CHANNEL = -2, MSG_SUCCESSFUL = 0};
enum recodes{RE_NOT_IN_CHANNEL = -2, RE_SUCCESSFUL = 0};
enum chmemcodes{CHMEM_NOT_IN_CHANNEL = -2, CHMEM_SUCCESSFUL = 0};
enum leavecodes{LEAVE_NOT_IN_CHANNEL = -2, LEAVE_SUCCESSFUL = 0};
const int AUTH_INVALID = -1;

void readFiles()
{
    struct dirent *de;  // Pointer for directory entry

    // opendir() returns a pointer of DIR type.
    DIR *dr = opendir(".");

    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        printf("Could not open current directory" );
        return 0;
    }

    if (!fileExists(dr, "Resources"))
    {
        if(!mkdir("Resources"))
        {
            printf("%s\n", "Resources folder created.");
        }
        else
        {
            printf("%s\n", "Resources folder creation failed.");
        }
    }
    rewinddir(dr);

    chdir("./Resources");
    dr = opendir(".");

    if (!fileExists(dr, "Users"))
    {
        if(!mkdir("Users"))
        {
            printf("%s\n", "Users folder created.");
        }
        else
        {
            printf("%s\n", "Users folder creation failed.");
        }
    }
    rewinddir(dr);

    if (!fileExists(dr, "Channels"))
    {
        if(!mkdir("Channels"))
        {
            printf("%s\n", "Channels folder created.");
        }
        else
        {
            printf("%s\n", "Channels folder creation failed.");
        }
    }
    rewinddir(dr);

    chdir("./Users");
    dr = opendir(".");
    while((de = readdir(dr)) != NULL)
    {
        if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
        {
            continue;
        }
        strcpy(users[usercount], de->d_name);
        for(int i = strlen(users[usercount]) - 10; users[usercount][i] != 0; ++i)
        {
            users[usercount][i] = 0;
        }
        printf("User %s successfully registered.\n", users[usercount]);
        ++usercount;
    }
    rewinddir(dr);

    chdir("../Channels");
    dr = opendir(".");
    while((de = readdir(dr)) != NULL)
    {
        if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
        {
            continue;
        }
        strcpy(channels[channelcount], de->d_name);
        for(int i = strlen(channels[channelcount]) - 13; channels[channelcount][i] != 0; ++i)
        {
            channels[channelcount][i] = 0;
        }
        printf("Channel %s successfully created.\n", channels[channelcount]);
        ++channelcount;
    }
    chdir("..");
    closedir(dr);

    printf("Information read.\n");
}

bool fileExists(DIR *dr, char filename[])
{
    struct dirent *de;

    while ((de = readdir(dr)) != NULL)
    {
        if(!strcmp(de->d_name, filename))
        {
            return true;
        }
    }
    return false;
}

void makeServerSocket()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        exit(0);
    }

    // Create and verify socket
    server_socket = socket(AF_INET, SOCK_STREAM, 6);
	if (server_socket == INVALID_SOCKET)
        wprintf(L"socket function failed with error = %d\n", WSAGetLastError() );
    else
        printf("Socket successfully created..\n");

    // Assign IP and port
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    // Bind newly created socket to given IP and verify
    if ((bind(server_socket, (SA *)&server, sizeof(server))) != 0)
    {
        printf("Socket binding failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully bound..\n");

    // Now server is ready to listen and verify
    if ((listen(server_socket, 5)) != 0)
    {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening on port %d\n", PORT);

}

void recieveRequest()
{
    makeSocket();

    char buffer[BUFFER_MAX];
    memset(buffer, 0, sizeof(char *));
    recv(client_socket, buffer, sizeof(buffer), 0);
    printf("Request from /%s : %s\n", IP_ADDRESS, buffer);

    if(!strncmp(buffer, "register", strlen("register")))
    {
        int code = reg(buffer);
    }
    else if(!strncmp(buffer, "login", strlen("login")))
    {
        int code = login(buffer);
    }
    else if(!strncmp(buffer, "create channel", strlen("create channel")))
    {
        int code = create_channel(buffer);
    }
    else if(!strncmp(buffer, "join channel", strlen("join channel")))
    {
        int code = join_channel(buffer);
    }
    else if(!strncmp(buffer, "logout", strlen("logout")))
    {
        int code = logout(buffer);
    }
    else if(!strncmp(buffer, "send", strlen("send")))
    {
        int code = send_msg(buffer);
    }
    else if(!strncmp(buffer, "refresh", strlen("refresh")))
    {
        int code = refresh(buffer);
    }
    else if(!strncmp(buffer, "channel members", strlen("channel members")))
    {
        int code = channel_members(buffer);
    }
    else if(!strncmp(buffer, "leave", strlen("leave")))
    {
        int code = leave(buffer);
    }

    closesocket(client_socket);
}

int reg(char buffer[])
{
    char username[USER_MAX];
    char password[PASS_MAX];
    sscanf(buffer, "%*s%s%s", username, password);
    username[strlen(username) - 1] = 0;

    if(userExists(username))
    {

        cJSON *regJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        regJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Error");
        cJSON_AddItemToObject(regJSON, "type", type);
        content = cJSON_CreateString("User already exists.");
        cJSON_AddItemToObject(regJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(regJSON));

        cJSON_Delete(regJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return REG_ALREADY_EXISTS;
    }
    else
    {
        createUser(username, password);

        cJSON *regJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        regJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Successful");
        cJSON_AddItemToObject(regJSON, "type", type);
        content = cJSON_CreateString("");
        cJSON_AddItemToObject(regJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(regJSON));

        cJSON_Delete(regJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return REG_SUCCESSFUL;
    }

    printf("%s %s\n", username, password);
}

bool userExists(char username[])
{
    for(int i = 0; i < usercount; ++i)
    {
        if(!strcmp(username, users[i]))
        {
            return true;
        }
    }
    return false;
}

int userindex(char username[])
{
    for(int i = 0; i < usercount; ++i)
    {
        if(!strcmp(username, users[i]))
        {
            return i;
        }
    }
}

int userindex_auth(char auth[])
{
    for(int i = 0; i < USER_COUNT; ++i)
    {
        if(!strcmp(auth, auth_tokens[i]))
        {
            return i;
        }
    }
}

void createUser(char username[], char password[])
{
    FILE *user = fopen(concat(3, "./Users/", username, ".user.json"), "w");
    cJSON *json = NULL;
    cJSON *userjson = NULL;
    cJSON *passjson = NULL;
    json = cJSON_CreateObject();
    userjson = cJSON_CreateString(username);
    passjson = cJSON_CreateString(password);
    cJSON_AddItemToObject(json, "username", userjson);
    cJSON_AddItemToObject(json, "password", passjson);
    char *string = cJSON_Print(json);

    cJSON_Delete(json);

    fprintf(user, "%s", string);

    free(string);
    fclose(user);

    strcpy(users[usercount], username);
    ++usercount;

}

int login(char buffer[])
{
    char username[USER_MAX];
    char password[PASS_MAX];
    sscanf(buffer, "%*s%s%s", username, password);
    username[strlen(username) - 1] = 0;

    if(userExists(username))
    {
        if(loggedin[userindex(username)])
        {
            cJSON *logJSON = NULL;
            cJSON *type = NULL;
            cJSON *content = NULL;
            logJSON = cJSON_CreateObject();
            type = cJSON_CreateString("Error");
            cJSON_AddItemToObject(logJSON, "type", type);
            content = cJSON_CreateString("User already logged in.");
            cJSON_AddItemToObject(logJSON, "content", content);

            char tmp[BUFFER_MAX] = {};
            strcpy(tmp, cJSON_Print(logJSON));

            cJSON_Delete(logJSON);

            printf("Response sent : %s\n", tmp);
            send(client_socket, tmp, sizeof(tmp), 0);

            return LOGIN_ALREADY_LOGGEDIN;
        }

        if(checkPass(username, password))
        {
            while(!makeAuth(userindex(username)));
            loggedin[userindex(username)] = true;

            cJSON *logJSON = NULL;
            cJSON *type = NULL;
            cJSON *content = NULL;
            logJSON = cJSON_CreateObject();
            type = cJSON_CreateString("AuthToken");
            cJSON_AddItemToObject(logJSON, "type", type);
            content = cJSON_CreateString(auth_tokens[userindex(username)]);
            cJSON_AddItemToObject(logJSON, "content", content);

            char tmp[BUFFER_MAX] = {};
            strcpy(tmp, cJSON_Print(logJSON));

            cJSON_Delete(logJSON);

            send(client_socket, tmp, sizeof(tmp), 0);
            printf("Response sent : %s\n", tmp);

            return LOGIN_SUCCESSFUL;
        }
        else
        {
            cJSON *logJSON = NULL;
            cJSON *type = NULL;
            cJSON *content = NULL;
            logJSON = cJSON_CreateObject();
            type = cJSON_CreateString("Error");
            cJSON_AddItemToObject(logJSON, "type", type);
            content = cJSON_CreateString("Wrong password.");
            cJSON_AddItemToObject(logJSON, "content", content);

            char tmp[BUFFER_MAX] = {};
            strcpy(tmp, cJSON_Print(logJSON));

            cJSON_Delete(logJSON);

            printf("Response sent : %s\n", tmp);
            send(client_socket, tmp, sizeof(tmp), 0);

            return LOGIN_WRONG_PASS;
        }
    }
    else
    {
        cJSON *logJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        logJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Error");
        cJSON_AddItemToObject(logJSON, "type", type);
        content = cJSON_CreateString("User doesn't exist.");
        cJSON_AddItemToObject(logJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(logJSON));

        cJSON_Delete(logJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return LOGIN_NOT_EXISTS;
    }
}

bool checkPass(char username[], char password[])
{
    FILE *file = fopen(concat(3, "./Users/",username, ".user.json"), "r");
    char string[BUFFER_MAX] = {};

    fseek(file, 0L, SEEK_END);
    size_t filesize = ftell(file);
    rewind(file);
    fread(string, 1, filesize, file);
    fclose(file);

    cJSON *json = NULL;
    json = cJSON_Parse(string);

    if(!strcmp(password, cJSON_GetObjectItemCaseSensitive(json, "password")->valuestring))
    {
        cJSON_Delete(json);

        return true;
    }
    else
    {
        cJSON_Delete(json);

        return false;
    }
}

bool makeAuth(int index)
{
    char auth[AUTH_LENGTH + 1] = {};
    for(int i = 0; i < AUTH_LENGTH; ++i)
    {
        int rnd = (rand() % 54);
        if(rnd < 26)
        {
            rnd +=65;
        }
        else if(rnd >= 26 && rnd < 52)
        {
            rnd += 71;
        }
        else if(rnd == 52)
        {
            rnd = 45;
        }
        else if(rnd == 53)
        {
            rnd = 95;
        }
        auth[i] = (char) rnd;
    }
    for(int i = 0; i < USER_COUNT; ++i)
    {
        if(!strcmp(auth, auth_tokens[i]))
        {
            return false;
        }
    }
    strcpy(auth_tokens[index], auth);

    return true;
}

int create_channel(char buffer[])
{
    char name[CHANNEL_MAX] = {};
    char auth[AUTH_LENGTH + 1] = {};
    sscanf(buffer, "%*s%*s%s%s", name, auth);
    name[strlen(name) - 1] = 0;

    if(!authValidity(auth))
    {
        return AUTH_INVALID;
    }

    if(channelExists(name))
    {
        cJSON *crchJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        crchJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Error");
        cJSON_AddItemToObject(crchJSON, "type", type);
        content = cJSON_CreateString("Channel already exists.");
        cJSON_AddItemToObject(crchJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(crchJSON));

        cJSON_Delete(crchJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return CRCH_ALREADY_EXISTS;
    }
    else
    {
        FILE *file = fopen(concat(3, "./channels/",name, ".channel.json"), "w");

        cJSON *json = NULL;
        cJSON *messagesjson = NULL;
        cJSON *messagejson = NULL;
        cJSON *namejson = NULL;
        cJSON *senderjson = NULL;
        cJSON *contentjson = NULL;
        json = cJSON_CreateObject();
        messagesjson = cJSON_CreateArray();
        messagejson = cJSON_CreateObject();
        senderjson = cJSON_CreateString("server");
        cJSON_AddItemToObject(messagejson, "sender", senderjson);
        contentjson = cJSON_CreateString(concat(4, users[userindex_auth(auth)], " created ", name, "."));
        cJSON_AddItemToObject(messagejson, "content", contentjson);
        cJSON_AddItemToArray(messagesjson, messagejson);
        cJSON_AddItemToObject(json, "messages", messagesjson);

        namejson = cJSON_CreateString(name);
        cJSON_AddItemToObject(json, "name", namejson);
        char *string = cJSON_Print(json);

        cJSON_Delete(json);

        fprintf(file, "%s" ,string);
        printf("%s\n", string);
        free(string);
        fclose(file);

        cJSON *crchJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        crchJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Successful");
        cJSON_AddItemToObject(crchJSON, "type", type);
        content = cJSON_CreateString("");
        cJSON_AddItemToObject(crchJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(crchJSON));

        cJSON_Delete(crchJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        strcpy(channels[channelcount], name);
        inchannel[channelcount][userindex_auth(auth)] = true;
        ++channelcount;

        return CRCH_SUCCESSFUL;
    }

}

bool channelExists(char channel[])
{
    for(int i = 0; i < channelcount; ++i)
    {
        if(!strcmp(channel, channels[i]))
        {
            return true;
        }
    }
    return false;
}

int channelindex(char channel[])
{
    for(int i = 0; i < channelcount; ++i)
    {
        if(!strcmp(channel, channels[i]))
        {
            return i;
        }
    }
}

bool authValidity(char auth[])
{
    for(int i = 0; i < usercount; ++i)
    {
        if(!strcmp(auth, auth_tokens[i]) && strcmp(auth, ""))
        {
            return true;
        }
    }
    return false;
}

int join_channel(char buffer[])
{
    char name[CHANNEL_MAX] = {};
    char auth[AUTH_LENGTH + 1] = {};
    sscanf(buffer, "%*s%*s%s%s", name, auth);
    name[strlen(name) - 1] = 0;

    if(!authValidity(auth))
    {
        return AUTH_INVALID;
    }

    if(channelExists(name))
    {
        FILE *file = fopen(concat(3, "./Channels/",name, ".channel.json"), "r");
        char string[BUFFER_MAX] = {};

        fseek(file, 0L, SEEK_END);
        size_t filesize = ftell(file);
        rewind(file);
        fread(string, 1, filesize, file);
        fclose(file);

        cJSON *json = NULL;
        cJSON *messagesjson = NULL;
        cJSON *messagejson = NULL;
        cJSON *senderjson = NULL;
        cJSON *contentjson = NULL;

        json = cJSON_Parse(string);
        messagesjson = cJSON_GetObjectItemCaseSensitive(json, "messages");
        messagejson = cJSON_CreateObject();
        contentjson = cJSON_CreateString(concat(2, users[userindex_auth(auth)], " joined."));
        senderjson = cJSON_CreateString("server");
        cJSON_AddItemToObject(messagejson, "sender", senderjson);
        cJSON_AddItemToObject(messagejson, "content", contentjson);
        cJSON_AddItemToArray(messagesjson, messagejson);

        strcpy(string, cJSON_Print(json));

        cJSON_Delete(json);

        file = fopen(concat(3, "./Channels/",name, ".channel.json"), "w");
        fprintf(file, "%s" ,string);
        fclose(file);

        cJSON *jochJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        jochJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Successful");
        cJSON_AddItemToObject(jochJSON, "type", type);
        content = cJSON_CreateString("");
        cJSON_AddItemToObject(jochJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(jochJSON));

        cJSON_Delete(jochJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        inchannel[channelindex(name)][userindex_auth(auth)] = true;

        return JOCH_SUCCESSFUL;
    }
    else
    {
        cJSON *jochJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        jochJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Error");
        cJSON_AddItemToObject(jochJSON, "type", type);
        content = cJSON_CreateString("Channel doesn't exist.");
        cJSON_AddItemToObject(jochJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(jochJSON));

        cJSON_Delete(jochJSON);

        printf("Response sent : %s\n", tmp);

        send(client_socket, tmp, sizeof(tmp), 0);

        return JOCH_NOT_EXISTS;
    }

}

int logout(char buffer[])
{
    char auth[AUTH_LENGTH + 1] = {};
    sscanf(buffer, "%*s%s", auth);

    if(!authValidity(auth))
    {
        return AUTH_INVALID;
    }

    int index = userindex_auth(auth);
    memset(auth_tokens[index], 0, sizeof(char **));

    if(loggedin[index] == false)
    {
        return LOGOUT_FAILED;
    }

    loggedin[index] = false;

    cJSON *logJSON = NULL;
    cJSON *type = NULL;
    cJSON *content = NULL;
    logJSON = cJSON_CreateObject();
    type = cJSON_CreateString("Successful");
    cJSON_AddItemToObject(logJSON, "type", type);
    content = cJSON_CreateString("");
    cJSON_AddItemToObject(logJSON, "content", content);

    char tmp[BUFFER_MAX] = {};
    strcpy(tmp, cJSON_Print(logJSON));

    cJSON_Delete(logJSON);

    printf("Response sent : %s\n", tmp);
    send(client_socket, tmp, sizeof(tmp), 0);

    return LOGOUT_SUCCESSFUL;

}

int send_msg(char buffer[])
{
    char auth[AUTH_LENGTH + 1] = {};
    char msg[BUFFER_MAX] = {};
    strcpy(auth, buffer + strlen(buffer) - AUTH_LENGTH - 1);
    auth[AUTH_LENGTH] = 0;
    memset(buffer + strlen(buffer) - AUTH_LENGTH - 3, 0, sizeof(char *));
    strcpy(msg, buffer + 5);

    if(!authValidity(auth))
    {
        return AUTH_INVALID;
    }

    char channel[CHANNEL_MAX] = {};
    if(channelindex_auth(auth) == -1)
    {
        cJSON *msgJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        msgJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Error");
        cJSON_AddItemToObject(msgJSON, "type", type);
        content = cJSON_CreateString("Not in channel.");
        cJSON_AddItemToObject(msgJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(msgJSON));

        cJSON_Delete(msgJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return MSG_NOT_IN_CHANNEL;
    }
    else
    {
        strcpy(channel, channels[channelindex_auth(auth)]);
        char sender[USER_MAX] = {};
        strcpy(sender, users[userindex_auth(auth)]);

        FILE *file = fopen(concat(3, "./Channels/",channel, ".channel.json"), "r");
        char string[BUFFER_MAX] = {};

        fseek(file, 0L, SEEK_END);
        size_t filesize = ftell(file);
        rewind(file);
        fread(string, 1, filesize, file);
        fclose(file);

        cJSON *json = NULL;
        cJSON *messagesjson = NULL;
        cJSON *messagejson = NULL;
        cJSON *senderjson = NULL;
        cJSON *contentjson = NULL;

        json = cJSON_Parse(string);
        messagesjson = cJSON_GetObjectItemCaseSensitive(json, "messages");
        messagejson = cJSON_CreateObject();
        contentjson = cJSON_CreateString(msg);
        senderjson = cJSON_CreateString(sender);
        cJSON_AddItemToObject(messagejson, "sender", senderjson);
        cJSON_AddItemToObject(messagejson, "content", contentjson);
        cJSON_AddItemToArray(messagesjson, messagejson);

        strcpy(string, cJSON_Print(json));

        cJSON_Delete(json);

        file = fopen(concat(3, "./Channels/",channel, ".channel.json"), "w");
        fprintf(file, "%s" ,string);
        fclose(file);

        cJSON *msgJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        msgJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Successful");
        cJSON_AddItemToObject(msgJSON, "type", type);
        content = cJSON_CreateString("");
        cJSON_AddItemToObject(msgJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(msgJSON));

        cJSON_Delete(msgJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return MSG_SUCCESSFUL;
    }
}

int channelindex_auth(char auth[])
{
    int usrindex = userindex_auth(auth);
    for(int i = 0; i < channelcount; ++i)
    {
        if(inchannel[i][usrindex])
        {
            return i;
        }
    }
    return -1;
}

int refresh(char buffer[])
{
    char auth[AUTH_LENGTH + 1] = {};
    sscanf(buffer, "%*s%s", auth);

    if(!authValidity(auth))
    {
        return AUTH_INVALID;
    }

    if(channelindex_auth(auth) == -1)
    {
        cJSON *reJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        reJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Error");
        cJSON_AddItemToObject(reJSON, "type", type);
        content = cJSON_CreateString("Not in channel.");
        cJSON_AddItemToObject(reJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(reJSON));

        cJSON_Delete(reJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return RE_NOT_IN_CHANNEL;
    }
    else
    {
        char channel[CHANNEL_MAX] = {};
        strcpy(channel, channels[channelindex_auth(auth)]);

        FILE *file = fopen(concat(3, "./Channels/",channel, ".channel.json"), "r");
        char string[BUFFER_MAX] = {};

        fseek(file, 0L, SEEK_END);
        size_t filesize = ftell(file);
        rewind(file);
        fread(string, 1, filesize, file);
        fclose(file);

        cJSON *jsonold = NULL;
        cJSON *messagesjsonold = NULL;
        jsonold = cJSON_Parse(string);
        messagesjsonold = cJSON_GetObjectItemCaseSensitive(jsonold, "messages");

        size_t sz = cJSON_GetArraySize(messagesjsonold);

        cJSON *jsonnew = NULL;
        cJSON *typejsonnew = NULL;
        cJSON *messagesjsonnew = NULL;
        jsonnew = cJSON_CreateObject();
        typejsonnew = cJSON_CreateString("List");
        cJSON_AddItemToObject(jsonnew, "type", typejsonnew);
        messagesjsonnew = cJSON_CreateArray();
        for(size_t i = msgindex[channelindex_auth(auth)][userindex_auth(auth)]; i < sz; ++i)
        {
            cJSON *msgjsonold = NULL;
            cJSON *senderjsonold = NULL;
            cJSON *contentjsonold = NULL;
            cJSON *msgjsonnew = NULL;
            cJSON *senderjsonnew = NULL;
            cJSON *contentjsonnew = NULL;
            msgjsonold = cJSON_GetArrayItem(messagesjsonold, i);
            senderjsonold = cJSON_GetObjectItemCaseSensitive(msgjsonold, "sender");
            contentjsonold = cJSON_GetObjectItemCaseSensitive(msgjsonold, "content");
            msgjsonnew = cJSON_CreateObject();
            senderjsonnew = cJSON_CreateString(senderjsonold->valuestring);
            contentjsonnew = cJSON_CreateString(contentjsonold->valuestring);
            cJSON_AddItemToObject(msgjsonnew, "sender", senderjsonnew);
            cJSON_AddItemToObject(msgjsonnew, "content", contentjsonnew);

            printf("%s\n", cJSON_GetObjectItemCaseSensitive(msgjsonnew, "content")->valuestring);
            cJSON_AddItemToArray(messagesjsonnew, msgjsonnew);
        }

        cJSON_AddItemToObject(jsonnew, "content", messagesjsonnew);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(jsonnew));

        cJSON_Delete(jsonold);
        cJSON_Delete(jsonnew);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        msgindex[channelindex_auth(auth)][userindex_auth(auth)] = sz;

        return RE_SUCCESSFUL;
    }

}

int channel_members(char buffer[])
{
    char auth[AUTH_LENGTH + 1] = {};
    sscanf(buffer, "%*s%*s%s", auth);

    if(!authValidity(auth))
    {
        return AUTH_INVALID;
    }

    int chindex = channelindex_auth(auth);

    if(chindex == -1)
    {
        cJSON *memJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        memJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Error");
        cJSON_AddItemToObject(memJSON, "type", type);
        content = cJSON_CreateString("Not in channel.");
        cJSON_AddItemToObject(memJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(memJSON));

        cJSON_Delete(memJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return CHMEM_NOT_IN_CHANNEL;
    }
    else
    {
        cJSON *json = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        json = cJSON_CreateObject();
        type = cJSON_CreateString("List");
        content = cJSON_CreateArray();

        for(int i = 0; i < usercount; ++i)
        {
            if(inchannel[chindex][i] == true)
            {
                cJSON *user = NULL;
                user = cJSON_CreateString(users[i]);
                cJSON_AddItemToArray(content, user);
            }
        }
        cJSON_AddItemToObject(json, "type", type);
        cJSON_AddItemToObject(json, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(json));

        cJSON_Delete(json);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return CHMEM_SUCCESSFUL;
    }

}

int leave(char buffer[])
{
    char auth[AUTH_LENGTH + 1] = {};
    sscanf(buffer, "%*s%s", auth);

    if(!authValidity(auth))
    {
        return AUTH_INVALID;
    }

    int chindex = channelindex_auth(auth);

    if(chindex == -1)
    {
        cJSON *leaveJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        leaveJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Error");
        cJSON_AddItemToObject(leaveJSON, "type", type);
        content = cJSON_CreateString("Not in channel.");
        cJSON_AddItemToObject(leaveJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_CreateString(leaveJSON));

        cJSON_Delete(leaveJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return LEAVE_NOT_IN_CHANNEL;
    }
    else
    {
        inchannel[chindex][userindex_auth(auth)] = false;

        FILE *file = fopen(concat(3, "./channels/",channels[chindex], ".channel.json"), "r");
        char string[BUFFER_MAX] = {};

        fseek(file, 0L, SEEK_END);
        size_t filesize = ftell(file);
        rewind(file);
        fread(string, 1, filesize, file);
        fclose(file);

        cJSON *json = NULL;
        cJSON *messagesjson = NULL;
        cJSON *messagejson = NULL;
        cJSON *senderjson = NULL;
        cJSON *contentjson = NULL;

        json = cJSON_Parse(string);
        messagesjson = cJSON_GetObjectItemCaseSensitive(json, "messages");
        messagejson = cJSON_CreateObject();
        contentjson = cJSON_CreateString(concat(2, users[userindex_auth(auth)], " left."));
        senderjson = cJSON_CreateString("server");
        cJSON_AddItemToObject(messagejson, "sender", senderjson);
        cJSON_AddItemToObject(messagejson, "content", contentjson);
        cJSON_AddItemToArray(messagesjson, messagejson);

        strcpy(string, cJSON_Print(json));

        cJSON_Delete(json);

        file = fopen(concat(3, "./Channels/",channels[chindex], ".channel.json"), "w");
        fprintf(file, "%s" ,string);
        fclose(file);

        cJSON *leaveJSON = NULL;
        cJSON *type = NULL;
        cJSON *content = NULL;
        leaveJSON = cJSON_CreateObject();
        type = cJSON_CreateString("Successful");
        cJSON_AddItemToObject(leaveJSON, "type", type);
        content = cJSON_CreateString("");
        cJSON_AddItemToObject(leaveJSON, "content", content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, cJSON_Print(leaveJSON));

        cJSON_Delete(leaveJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return LEAVE_SUCCESSFUL;
    }

}

void makeSocket()
{
    // Accept the data packet from client and verify
    int len = sizeof(client);
    client_socket = accept(server_socket, (SA *)&client, &len);
    if (client_socket < 0)
    {
        printf("Server accceptance failed...\n");
        exit(0);
    }
    else
        printf("Server acccepted the client..\n");
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


// Driver function
int main()
{
    srand(time(0));
    printf("Reading information..\n");
    readFiles();
    makeServerSocket();
    while(true){
        recieveRequest();
    }
}
