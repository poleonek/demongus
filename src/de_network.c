static NetUser *Net_FindUser(AppState *app, SDLNet_Address *address)
{
    ForU32(i, app->net.server.user_count)
    {
        if (app->net.server.users[i].address == address)
            return app->net.server.users + i;
    }
    return 0;
}

static NetUser *Net_AddUser(AppState *app, SDLNet_Address *address, Uint16 port)
{
    if (app->net.server.user_count < ArrayCount(app->net.server.users))
    {
        NetUser *user = app->net.server.users + app->net.server.user_count;
        app->net.server.user_count += 1;
        user->address = SDLNet_RefAddress(address);
        user->port = port;
        return user;
    }
    return 0;
}

static void Net_Iterate(AppState *app)
{
    if (app->net.err) return;

    for (;;)
    {
        SDLNet_Datagram *dgram = 0;
        int receive = SDLNet_ReceiveDatagram(app->net.socket, &dgram);
        if (!receive) break;
        if (!dgram) break;

        SDL_Log("%s: got %d-byte datagram from %s:%d",
                app->net.is_server ? "SERVER" : "CLIENT",
                (int)dgram->buflen,
                SDLNet_GetAddressString(dgram->addr),
                (int)dgram->port);

        SDL_Log("MESSAGE: %.*s", (int)dgram->buflen, (char *)dgram->buf);

        if (app->net.is_server)
        {
            if (!Net_FindUser(app, dgram->addr))
            {
                SDL_Log("SERVER: saving user with port: %d", (int)dgram->port);
                Net_AddUser(app, dgram->addr, dgram->port);
            }
        }

        // cleanup
        SDLNet_DestroyDatagram(dgram);
    }

    if (!app->net.is_server && ((app->frame_id % 6400) == 0))
    {
        char send_buf[] = "I'm a client and I like sending messages.";
        bool send_res = SDLNet_SendDatagram(app->net.socket,
                                            app->net.client.address,
                                            NET_DEFAULT_SEVER_PORT,
                                            send_buf, sizeof(send_buf));
        SDL_Log("CLIENT SendDatagram result: %s", send_res ? "success" : "fail");
    }

    if (app->net.is_server && ((app->frame_id % 14000) == 123))
    {
        char send_buf[] = "Hello, I'm a server.";
        ForU32(i, app->net.server.user_count)
        {
            NetUser *user = app->net.server.users + i;

            bool send_res = SDLNet_SendDatagram(app->net.socket,
                                                user->address,
                                                user->port,
                                                send_buf, sizeof(send_buf));
            SDL_Log("SERVER SendDatagram result: %s", send_res ? "success" : "fail");
        }
    }
}

static void Net_Init(AppState *app)
{
    SDL_Log(app->net.is_server ? "Launching as server" : "Launching as client");

    if (!app->net.is_server)
    {
        const char *hostname = "localhost";
        SDL_Log("CLIENT: Resolving server hostname '%s' ...", hostname);
        app->net.client.address = SDLNet_ResolveHostname(hostname);
        if (app->net.client.address)
        {
            if (SDLNet_WaitUntilResolved(app->net.client.address, -1) < 0)
            {
                SDLNet_UnrefAddress(app->net.client.address);
                app->net.client.address = 0;
            }
        }

        if (!app->net.client.address)
        {
            app->net.err = true;
            SDL_Log("CLIENT: Failed to resolve server hostname '%s'", hostname);
        }
    }

    Uint16 port = (app->net.is_server ? NET_DEFAULT_SEVER_PORT : 0);
    app->net.socket = SDLNet_CreateDatagramSocket(0, port);
    if (!app->net.socket)
    {
        app->net.err = true;
        SDL_Log("NETWORK: Failed to create socket");
    }
}

static void Net_Deinit()
{
    Assert(!"@todo");
}
