static const char *Net_Label(AppState *app)
{
    return app->net.is_server ? "SERVER" : "CLIENT";
}

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
                Net_Label(app),
                (int)dgram->buflen,
                SDLNet_GetAddressString(dgram->addr),
                (int)dgram->port);

        if (!app->net.is_server)
        {
            if ((dgram->port != app->net.client.server_port) ||
                (SDLNet_CompareAddresses(dgram->addr, app->net.client.server_address) != 0))
            {
                SDL_Log("%s: Ignoring message from non-server address %s:%d",
                        Net_Label(app),
                        SDLNet_GetAddressString(dgram->addr), (int)dgram->port);
                goto datagram_cleanup;
            }
        }

        SDL_Log("%s MSG: %.*s",
                Net_Label(app),
                (int)dgram->buflen, (char *)dgram->buf);

        if (app->net.is_server)
        {
            if (!Net_FindUser(app, dgram->addr))
            {
                SDL_Log("%s: saving user with port: %d",
                        Net_Label(app), (int)dgram->port);
                Net_AddUser(app, dgram->addr, dgram->port);
            }
        }

        datagram_cleanup:
        SDLNet_DestroyDatagram(dgram);
    }

    if (!app->net.is_server && ((app->frame_id % 6400) == 0))
    {
        char send_buf[] = "I'm a client and I like sending messages.";
        bool send_res = SDLNet_SendDatagram(app->net.socket,
                                            app->net.client.server_address,
                                            app->net.client.server_port,
                                            send_buf, sizeof(send_buf));
        SDL_Log("%s: SendDatagram result: %s",
                Net_Label(app), send_res ? "success" : "fail");
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
            SDL_Log("%s: SendDatagram result: %s",
                    Net_Label(app), send_res ? "success" : "fail");
        }
    }
}

static void Net_Init(AppState *app)
{
    SDL_Log(app->net.is_server ? "Launching as server" : "Launching as client");

    if (!app->net.is_server)
    {
        const char *hostname = "localhost";
        SDL_Log("%s: Resolving server hostname '%s' ...",
                Net_Label(app), hostname);
        app->net.client.server_address = SDLNet_ResolveHostname(hostname);
        app->net.client.server_port = NET_DEFAULT_SEVER_PORT;
        if (app->net.client.server_address)
        {
            if (SDLNet_WaitUntilResolved(app->net.client.server_address, -1) < 0)
            {
                SDLNet_UnrefAddress(app->net.client.server_address);
                app->net.client.server_address = 0;
            }
        }

        if (!app->net.client.server_address)
        {
            app->net.err = true;
            SDL_Log("%s: Failed to resolve server hostname '%s'",
                    Net_Label(app), hostname);
        }
    }

    Uint16 port = (app->net.is_server ? NET_DEFAULT_SEVER_PORT : 0);
    app->net.socket = SDLNet_CreateDatagramSocket(0, port);
    if (!app->net.socket)
    {
        app->net.err = true;
        SDL_Log("%s: Failed to create socket",
                Net_Label(app));
    }
}

static void Net_Deinit()
{
    Assert(!"@todo");
}
