#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <dirent.h>
#include <stdarg.h>

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

static char *getElement(char *, char *);
int getArraySize(char *);
int getArraySizeByString(char *);
static char *getArrayElement(char *, int);
static char *getArrayString(char *, int);
static char *createCJSON();
static char *createArray();
static char *addItemToCJSON(char *, char *, char *);
static char *addItemToArray(char*, char*);
char *concat(int , ...);


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
        end = start;
        size_t len = 0;
        while(*end != ']')
        {
            ++len;
            ++end;
        }
        ++len;
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

static char *createCJSON()
{
    char *result = (char *)calloc(3, sizeof(char));
    memcpy(result, "{}", 2);
    return result;
}

static char *createArray()
{
    char *result = (char *)calloc(3, sizeof(char));
    memcpy(result, "[]", 2);
    return result;
}

static char *addItemToCJSON(char *string, char *item, char *content)
{

    if(strlen(string) == 2)
    {
        *(string + strlen(string) - 1) = 0;
    }
    else
    {
        *(string + strlen(string) - 1) = ',';
    }
    return concat(7, string, "\"", item, "\"", ":",content, "}");
}

static char *addItemToArray(char *array, char *item)
{
    if(strlen(array) == 2)
    {
        *(array + strlen(array) - 1) = 0;
    }
    else
    {
        *(array + strlen(array) - 1) = ',';
    }
    return concat(3, array, item, "]");
}


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

        char *regJSON = NULL;
        regJSON = createCJSON();
        regJSON = addItemToCJSON(regJSON, "type", "\"Error\"");
        regJSON = addItemToCJSON(regJSON, "content", "\"User already exists.\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, regJSON);

        free(regJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return REG_ALREADY_EXISTS;
    }
    else
    {
        createUser(username, password);

        char *regJSON = NULL;
        regJSON = createCJSON();
        regJSON = addItemToCJSON(regJSON, "type", "\"Successful\"");
        regJSON = addItemToCJSON(regJSON, "content", "\"\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, regJSON);

        free(regJSON);

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

    char *json = NULL;
    json = createCJSON();
    json = addItemToCJSON(json, "username", concat(3, "\"", username, "\""));
    json = addItemToCJSON(json, "password", concat(3, "\"", password, "\""));

    fprintf(user, "%s", json);

    free(json);
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
            char *logJSON = NULL;
            logJSON = createCJSON();
            logJSON = addItemToCJSON(logJSON, "type", "\"Error\"");
            logJSON = addItemToCJSON(logJSON, "content", "\"User already logged in.\"");

            char tmp[BUFFER_MAX] = {};
            strcpy(tmp, logJSON);

            free(logJSON);

            printf("Response sent : %s\n", tmp);
            send(client_socket, tmp, sizeof(tmp), 0);

            return LOGIN_ALREADY_LOGGEDIN;
        }

        if(checkPass(username, password))
        {
            while(!makeAuth(userindex(username)));
            loggedin[userindex(username)] = true;

            char *logJSON = NULL;
            logJSON = createCJSON();
            logJSON = addItemToCJSON(logJSON, "type", "\"AuthToken\"");
            logJSON = addItemToCJSON(logJSON, "content", concat(3, "\"", auth_tokens[userindex(username)], "\""));

            char tmp[BUFFER_MAX] = {};
            strcpy(tmp, logJSON);

            free(logJSON);
            send(client_socket, tmp, sizeof(tmp), 0);
            printf("Response sent : %s\n", tmp);

            return LOGIN_SUCCESSFUL;
        }
        else
        {
            char *logJSON = NULL;
            logJSON = createCJSON();
            logJSON = addItemToCJSON(logJSON, "type", "\"Error\"");
            logJSON = addItemToCJSON(logJSON, "content", "\"Wrong Password.\"");

            char tmp[BUFFER_MAX] = {};
            strcpy(tmp, logJSON);

            free(logJSON);

            printf("Response sent : %s\n", tmp);
            send(client_socket, tmp, sizeof(tmp), 0);

            return LOGIN_WRONG_PASS;
        }
    }
    else
    {
        char *logJSON = NULL;
        logJSON = createCJSON();
        logJSON = addItemToCJSON(logJSON, "type", "\"Error\"");
        logJSON = addItemToCJSON(logJSON, "content", "\"User doesn't exist.\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, logJSON);

        free(logJSON);

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

    char *element = getElement(string, "password");
    if(!strcmp(password, element))
    {
        free(element);
        return true;
    }
    else
    {
        free(element);
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
        char *crchJSON = NULL;
        crchJSON = createCJSON();
        crchJSON = addItemToCJSON(crchJSON, "type", "\"Error\"");
        crchJSON = addItemToCJSON(crchJSON, "content", "\"Channel already exists.\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, crchJSON);

        free(crchJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return CRCH_ALREADY_EXISTS;
    }
    else
    {

        char *json = NULL;
        json = createCJSON();

        char *messagejson;
        messagejson = createCJSON();
        messagejson = addItemToCJSON(messagejson, "sender", "\"server\"");
        messagejson = addItemToCJSON(messagejson, "content", concat(5, "\"",users[userindex_auth(auth)], " created ", name, ".\""));

        char *messagesjson = NULL;
        messagesjson = createArray();
        messagesjson = addItemToArray(messagesjson, messagejson);

        json = addItemToCJSON(json, "messages", messagesjson);
        json = addItemToCJSON(json, "name", concat(3, "\"", name, "\""));

        free(messagejson);
        free(messagesjson);

        FILE *file = fopen(concat(3, "./channels/",name, ".channel.json"), "w");

        fprintf(file, "%s" ,json);
        free(json);
        fclose(file);

        char *crchJSON = NULL;
        crchJSON = createCJSON();
        crchJSON = addItemToCJSON(crchJSON, "type", "\"Successful\"");
        crchJSON = addItemToCJSON(crchJSON, "content", "\"\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, crchJSON);

        free(crchJSON);

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

        char *json = NULL;
        json = createCJSON();

        char *messagejson = NULL;
        messagejson = createCJSON();
        messagejson = addItemToCJSON(messagejson, "sender", "\"server\"");
        messagejson = addItemToCJSON(messagejson, "content", concat(4, "\"",users[userindex_auth(auth)], " joined", ".\""));

        char *messagesjson = NULL;
        messagesjson = getElement(string, "messages");
        messagesjson = addItemToArray(messagesjson, messagejson);

        free(messagejson);

        json = addItemToCJSON(json, "messages", messagesjson);
        json = addItemToCJSON(json, "name", concat(3, "\"", name, "\""));

        free(messagesjson);

        file = fopen(concat(3, "./Channels/",name, ".channel.json"), "w");
        fprintf(file, "%s" ,json);
        free(json);
        fclose(file);

        char *jochJSON = NULL;
        jochJSON = createCJSON();
        jochJSON = addItemToCJSON(jochJSON, "type", "\"Successful\"");
        jochJSON = addItemToCJSON(jochJSON, "content", "\"\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, jochJSON);

        free(jochJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        inchannel[channelindex(name)][userindex_auth(auth)] = true;

        return JOCH_SUCCESSFUL;
    }
    else
    {
        char *jochJSON = NULL;
        jochJSON = createCJSON();
        jochJSON = addItemToCJSON(jochJSON, "type", "\"Error\"");
        jochJSON = addItemToCJSON(jochJSON, "content", "\"Channel doesn't exist.\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, jochJSON);

        free(jochJSON);

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

    char *logJSON = NULL;
    logJSON = createCJSON();
    logJSON = addItemToCJSON(logJSON, "type", "\"Successful\"");
    logJSON = addItemToCJSON(logJSON, "content", "\"\"");

    char tmp[BUFFER_MAX] = {};
    strcpy(tmp, logJSON);

    free(logJSON);

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
        char *msgJSON = NULL;
        msgJSON = createCJSON();
        msgJSON = addItemToCJSON(msgJSON, "type", "\"Error\"");
        msgJSON = addItemToCJSON(msgJSON, "content", "\"Not in a channel.\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, msgJSON);

        free(msgJSON);

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

        char *json = NULL;
        json = createCJSON();

        char *messagejson = NULL;
        messagejson = createCJSON();
        messagejson = addItemToCJSON(messagejson, "sender", concat(3, "\"", sender, "\""));
        messagejson = addItemToCJSON(messagejson, "content", concat(3, "\"", msg, "\""));

        char *messagesjson = NULL;
        messagesjson = getElement(string, "messages");
        messagesjson = addItemToArray(messagesjson, messagejson);

        free(messagejson);

        json = addItemToCJSON(json, "messages", messagesjson);
        json = addItemToCJSON(json, "name", concat(3, "\"", channel, "\""));

        free(messagesjson);

        file = fopen(concat(3, "./Channels/",channel, ".channel.json"), "w");
        fprintf(file, "%s" ,json);
        free(json);
        fclose(file);

        char *msgJSON = NULL;
        msgJSON = createCJSON();
        msgJSON = addItemToCJSON(msgJSON, "type", "\"Successful\"");
        msgJSON = addItemToCJSON(msgJSON, "content", "\"\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, msgJSON);

        free(msgJSON);

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
        char *reJSON = NULL;
        reJSON = createCJSON();
        reJSON = addItemToCJSON(reJSON, "type", "\"Error\"");
        reJSON = addItemToCJSON(reJSON, "content", "\"Not in a channel.\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, reJSON);

        free(reJSON);

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

        char *messagesjsonold = NULL;
        messagesjsonold = getElement(string, "messages");
        printf("%s\n", messagesjsonold);

        size_t sz = getArraySize(messagesjsonold);


        char *jsonnew = NULL;
        jsonnew = createCJSON();

        char *messagesjsonnew = NULL;
        messagesjsonnew = createArray();

        for(size_t i = msgindex[channelindex_auth(auth)][userindex_auth(auth)]; i < sz; ++i)
        {
            char *msgjson = getArrayElement(messagesjsonold, i);
            messagesjsonnew = addItemToArray(messagesjsonnew, msgjson);
            free(msgjson);
        }
        free(messagesjsonold);
        jsonnew = addItemToCJSON(jsonnew, "type", concat(3, "\"", "List", "\""));
        jsonnew = addItemToCJSON(jsonnew, "content", messagesjsonnew);
        free(messagesjsonnew);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, jsonnew);

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
        char *memJSON = NULL;
        memJSON = createCJSON();
        memJSON = addItemToCJSON(memJSON, "type", "\"Error\"");
        memJSON = addItemToCJSON(memJSON, "content", "\"Not in a channel.\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, memJSON);

        free(memJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return CHMEM_NOT_IN_CHANNEL;
    }
    else
    {

        char *json = NULL;
        json = createCJSON();
        json = addItemToCJSON(json, "type", concat(3, "\"", "List", "\""));

        char *content = NULL;
        content = createArray();

        for(int i = 0; i < usercount; ++i)
        {
            if(inchannel[chindex][i] == true)
            {
                content = addItemToArray(content, concat(3, "\"", users[i], "\"" ));
            }
        }
        json = addItemToCJSON(json, "content", content);
        free(content);

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, json);

        free(json);

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
        char *leaveJSON = NULL;
        leaveJSON = createCJSON();
        leaveJSON = addItemToCJSON(leaveJSON, "type", "\"Error\"");
        leaveJSON = addItemToCJSON(leaveJSON, "content", "\"Not in a channel.\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, leaveJSON);

        free(leaveJSON);

        printf("Response sent : %s\n", tmp);
        send(client_socket, tmp, sizeof(tmp), 0);

        return LEAVE_NOT_IN_CHANNEL;
    }
    else
    {
        char name[CHANNEL_MAX] = {};
        strcpy(name, channels[chindex]);

        inchannel[chindex][userindex_auth(auth)] = false;

        FILE *file = fopen(concat(3, "./Channels/",name, ".channel.json"), "r");
        char string[BUFFER_MAX] = {};

        fseek(file, 0L, SEEK_END);
        size_t filesize = ftell(file);
        rewind(file);
        fread(string, 1, filesize, file);
        fclose(file);

        char *json = NULL;
        json = createCJSON();

        char *messagejson = NULL;
        messagejson = createCJSON();
        messagejson = addItemToCJSON(messagejson, "sender", "\"server\"");
        messagejson = addItemToCJSON(messagejson, "content", concat(4, "\"",users[userindex_auth(auth)], " leaved", ".\""));

        char *messagesjson = NULL;
        messagesjson = getElement(string, "messages");
        messagesjson = addItemToArray(messagesjson, messagejson);

        free(messagejson);

        json = addItemToCJSON(json, "messages", messagesjson);
        json = addItemToCJSON(json, "name", concat(3, "\"", name, "\""));

        free(messagesjson);

        file = fopen(concat(3, "./Channels/",name, ".channel.json"), "w");
        fprintf(file, "%s" ,json);
        free(json);
        fclose(file);

        char *leaveJSON = NULL;
        leaveJSON = createCJSON();
        leaveJSON = addItemToCJSON(leaveJSON, "type", "\"Successful\"");
        leaveJSON = addItemToCJSON(leaveJSON, "content", "\"\"");

        char tmp[BUFFER_MAX] = {};
        strcpy(tmp, leaveJSON);

        free(leaveJSON);

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
