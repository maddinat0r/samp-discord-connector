/**********************************************************************/
/* This is a simple FilterScript can handle the Coming Discord Plugin */
/**********************************************************************/

#include <a_samp>
#include <discord-connector>

new DCC_Channel:general;

public OnFilterScriptInit()
{
    DCC_Connect("https://discordapp.com/invite/qFq3gVw");
    general = DCC_FindChannelByName("general");
    print(">> Discord FilterScript has been Loaded.");
    return 1;
}

public OnFilterScriptExit()
{
    print(">> Discord FilterScript has been UnLoaded.");
    return 1;
}

public OnPlayerText(playerid, text[])
{
    new str[50], pname[24];
    GetPlayerName(playerid, pname, 24);
    format(str, sizeof(str), "[%d] %s: %s", playerid, pname, text);
    DCC_SendChannelMessage(general, str);
    return 1;
}

DCCMD:say(user[], params[])
{
    if (!isnull(params))
    {
        new msg[128];
        format(msg, sizeof(msg), "*(%s) on Discord:%s", user, params);
        DCC_SendChannelMessage(general, msg);
        format(msg, sizeof(msg), "*Discord(%s): %s", user, params);
        SendClientMessageToAll(-1, msg);
    }
    return 1;
}

DCCMD:players(user[], params[]) 
{
    new count, PlayerNames[512], string[256];
    for(new i=0; i<=MAX_PLAYERS; i++)
    {
        if(IsPlayerConnected(i))
        {
            if(count == 0)
            {
                new PlayerName1[MAX_PLAYER_NAME];
                GetPlayerName(i, PlayerName1, sizeof(PlayerName1));
                format(PlayerNames, sizeof(PlayerNames),"%s", PlayerName1);
                count++;
            }
            else
            {
                new PlayerName1[MAX_PLAYER_NAME];
                GetPlayerName(i, PlayerName1, sizeof(PlayerName1));
                format(PlayerNames, sizeof(PlayerNames),"%s, %s", PlayerNames, PlayerName1);
                count++;
            }
        }
        else { if(count == 0) format(PlayerNames, sizeof(PlayerNames), "No Players Online!"); }
    }

    new counter = 0;
    for(new i=0; i<=MAX_PLAYERS; i++)
    {
        if(IsPlayerConnected(i)) counter++;
    }
    format(string, 256, "Connected Players[%d]: %s", counter, PlayerNames);
    DCC_SendChannelMessage(general, string);
    return true;
}

DCCMD:pm(user[], params[])
{
    new string[128], str1[128], ppid, pname[MAX_PLAYER_NAME];
    if(sscanf(params, "us[100]", ppid, str1))
    {
        DCC_SendChannelMessage(general, "Usage: !pm [ID] [message]");
        return 1;
    }
    if(!IsPlayerConnected(ppid)) return DCC_SendChannelMessage(general, "Invalid Player ID.");
    {
        GetPlayerName(ppid, pname, sizeof(pname));
        format(string, sizeof(string), "PM To: %s(ID %d): %s", pname, ppid, str1);
        DCC_SendChannelMessage(general, string);
        format(str1, sizeof(str1), "*PM From [Discord]%s : %s", user, str1);
        SendClientMessage(ppid, COLOR_ORANGE, str1);
    }
    return 1;
}

DCCMD:cmds(user[], params[])
{
    DCC_SendChannelMessage(general, "Discord Commands: @players @say @pm");
    return 1;
}



