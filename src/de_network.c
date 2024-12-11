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
    }

    if (!app->net.is_server && ((app->frame_id % 1600) == 0))
    {
        char send_buf[] = "hello, how are you?";
        bool send_res = SDLNet_SendDatagram(app->net.socket,
                                            app->net.client.address,
                                            NET_DEFAULT_SEVER_PORT,
                                            send_buf, sizeof(send_buf));
        SDL_Log("CLIENT SendDatagram result: %s", send_res ? "success" : "fail");
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
