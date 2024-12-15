typedef struct
{
    Uint64 magic_value;
    Uint64 hash; // of all values post first 16 bytes
} Net_BufHeader;

static const char *Net_Label(AppState *app)
{
    return app->net.is_server ? "SERVER" : "CLIENT";
}

static Uint8 *Net_BufAlloc(AppState *app, Uint32 size)
{
    Uint8 *result = app->net.buf + app->net.buf_used;
    {
        // return ptr to start of the buffer
        // on overflow
        Uint8 *end = (app->net.buf + sizeof(app->net.buf));
        if (end < (result + size))
        {
            Assert(false);
            result = app->net.buf;
            app->net.buf_err = true;
        }
    }
    app->net.buf_used += size;
    return result;
}

static Uint8 *Net_BufMemcpy(AppState *app, void *data, Uint32 size)
{
    Uint8 *result = Net_BufAlloc(app, size);
    memcpy(result, data, size);
    return result;
}

static void Net_BufSend(AppState *app, Net_User destination)
{
    bool send_res = SDLNet_SendDatagram(app->net.socket,
                                        destination.address,
                                        destination.port,
                                        app->net.buf, app->net.buf_used);
    SDL_Log("%s: Sending buffer of size %d to %s:%d; %s",
            Net_Label(app), app->net.buf_used,
            SDLNet_GetAddressString(destination.address),
            (int)destination.port,
            send_res ? "success" : "fail");
}

static void Net_BufSendFlush(AppState *app)
{
    if (app->net.is_server)
    {
        ForU32(i, app->net.user_count)
        {
            Net_BufSend(app, app->net.users[i]);
        }
    }
    else
    {
        Net_BufSend(app, app->net.server_user);
    }

    app->net.buf_used = 0;
}

static bool Net_ConsumeMsg(S8 *msg, void *dest, Uint64 size)
{
    Uint64 copy_size = Min(size, msg->size);
    bool err = copy_size != size;

    if (err)
        memset(dest, 0, size); // fill dest with zeroes

    memcpy(dest, msg->str, copy_size);
    *msg = S8_Skip(*msg, size);

    return err;
}

static Net_User *Net_FindUser(AppState *app, SDLNet_Address *address)
{
    ForU32(i, app->net.user_count)
    {
        if (app->net.users[i].address == address)
            return app->net.users + i;
    }
    return 0;
}

static Net_User *Net_AddUser(AppState *app, SDLNet_Address *address, Uint16 port)
{
    if (app->net.user_count < ArrayCount(app->net.users))
    {
        Net_User *user = app->net.users + app->net.user_count;
        app->net.user_count += 1;
        user->address = SDLNet_RefAddress(address);
        user->port = port;
        return user;
    }
    return 0;
}

static bool Net_UserMatch(Net_User a, Net_User b)
{
    return (a.port == b.port &&
            SDLNet_CompareAddresses(a.address, b.address) != 0);
}

static bool Net_UserMatchAddrPort(Net_User a, SDLNet_Address *address, Uint16 port)
{
    return (a.port == port &&
            SDLNet_CompareAddresses(a.address, address) != 0);
}

static void Net_SendData(AppState *app)
{
    bool is_server = app->net.is_server;
    bool is_client = !app->net.is_server;

    Net_BufHeader header = {};
    header.magic_value = NET_MAGIC_VALUE;
    Uint8 *buf_header = Net_BufAlloc(app, sizeof(Net_BufHeader));

    if (is_server)
    {
        ForArray(i, app->network_ids)
        {
            Object *obj = Object_Network(app, i);
            if (Object_IsZero(app, obj))
                continue;

            Tick_Command cmd = {};
            cmd.tick_id = app->tick_id;
            cmd.kind = Tick_Cmd_NetworkObj;
            cmd.network_slot = i;
            Net_BufMemcpy(app, &cmd, sizeof(cmd));

            Net_BufMemcpy(app, obj, sizeof(*obj));
        }
    }

    if (is_client)
    {
        // empty msg
    }

    S8 msg = S8_Make(app->net.buf, app->net.buf_used);
    msg = S8_Skip(msg, sizeof(header));
    header.hash = S8_Hash(0, msg);
    memcpy(buf_header, &header, sizeof(header));
    Net_BufSendFlush(app);
}

static void Net_ReceiveData(AppState *app)
{
    bool is_server = app->net.is_server;
    bool is_client = !app->net.is_server;

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

        if (is_client)
        {
            if (Net_UserMatchAddrPort(app->net.server_user, dgram->addr, dgram->port))
            {
                SDL_Log("%s: dgram rejected - received from non-server address %s:%d",
                        Net_Label(app),
                        SDLNet_GetAddressString(dgram->addr), (int)dgram->port);
                goto datagram_cleanup;
            }
        }

        S8 msg = S8_Make(dgram->buf, dgram->buflen);

        // validate header
        {
            if (msg.size < sizeof(Net_BufHeader))
            {
                SDL_Log("%s: dgram rejected - too small for BufHeader",
                        Net_Label(app));
                goto datagram_cleanup;
            }

            Net_BufHeader header;
            memcpy(&header, msg.str, sizeof(header));
            msg = S8_Skip(msg, sizeof(header));

            if (header.magic_value != NET_MAGIC_VALUE)
            {
                SDL_Log("%s: dgram rejected - invalid magic value %llu",
                        Net_Label(app), header.magic_value);
                goto datagram_cleanup;
            }

            Uint64 msg_hash = S8_Hash(0, msg);
            if (header.hash != msg_hash)
            {
                SDL_Log("%s: dgram rejected - dgram hash (%llu) != calculated hash (%llu)",
                        Net_Label(app), header.hash, msg_hash);
                goto datagram_cleanup;
            }
        }

        if (is_server)
        {
            if (!Net_FindUser(app, dgram->addr))
            {
                SDL_Log("%s: saving user with port: %d",
                        Net_Label(app), (int)dgram->port);
                Net_AddUser(app, dgram->addr, dgram->port);
            }
        }

        if (is_client)
        {
            if (msg.size)
            {
                Tick_Command cmd;
                Net_ConsumeMsg(&msg, &cmd, sizeof(cmd));

                if (cmd.network_slot >= ArrayCount(app->network_ids))
                {
                    SDL_Log("%s: Network slot overflow: %d",
                            Net_Label(app), (int)cmd.network_slot);
                    goto datagram_cleanup;
                }

                if (cmd.kind != Tick_Cmd_NetworkObj)
                {
                    SDL_Log("%s: Unsupported cmd kind: %d",
                            Net_Label(app), (int)cmd.kind);
                    goto datagram_cleanup;
                }

                if (cmd.kind == Tick_Cmd_NetworkObj)
                {
                    Tick_NetworkObj msg_obj;
                    Net_ConsumeMsg(&msg, &msg_obj, sizeof(msg_obj));

                    if (!app->network_ids[cmd.network_slot])
                    {
                        app->network_ids[cmd.network_slot] =
                            Object_IdFromPointer(app, Object_Create(app, 0));
                    }

                    Object *obj = Object_Network(app, cmd.network_slot);
                    *obj = msg_obj.object;
                }
            }
        }

        datagram_cleanup:
        SDLNet_DestroyDatagram(dgram);
    }
}

static void Net_Iterate(AppState *app)
{
    if (app->net.err) return;

    // hacky temporary network activity rate-limitting
    {
        static Uint64 last_timestamp = 0;
        if (app->frame_time < last_timestamp + 1000)
            return;

        last_timestamp = app->frame_time;
    }

    Net_ReceiveData(app);
    Net_SendData(app);
}

static void Net_Init(AppState *app)
{
    bool is_server = app->net.is_server;
    bool is_client = !app->net.is_server;

    SDL_Log(is_server ? "Launching as server" : "Launching as client");

    if (is_client)
    {
        const char *hostname = "localhost";
        SDL_Log("%s: Resolving server hostname '%s' ...",
                Net_Label(app), hostname);
        app->net.server_user.address = SDLNet_ResolveHostname(hostname);
        app->net.server_user.port = NET_DEFAULT_SEVER_PORT;
        if (app->net.server_user.address)
        {
            if (SDLNet_WaitUntilResolved(app->net.server_user.address, -1) < 0)
            {
                SDLNet_UnrefAddress(app->net.server_user.address);
                app->net.server_user.address = 0;
            }
        }

        if (!app->net.server_user.address)
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
    else
    {
        SDL_Log("%s: Created socket",
                Net_Label(app));
    }
}

static void Net_Deinit()
{
    Assert(!"@todo");
}
