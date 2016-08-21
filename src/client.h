
typedef struct Client Client;

struct Client {
    int fd;
};

void client_init(Client *c);

int client_start(Client *c);
