//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "duke3d.h"
#include "demo.h"
#include "screens.h"
#include "renderlayer.h"
#include "cmdline.h"

int32_t g_commandSetup = 0;
int32_t g_noSetup = 0;
int32_t g_noAutoLoad = 0;
int32_t g_noSound = 0;
int32_t g_noMusic = 0;
const char *CommandMap = NULL;
const char *CommandName = NULL;
int32_t g_forceWeaponChoice = 0;
int32_t g_fakeMultiMode = 0;

void G_ShowParameterHelp(void)
{
    static char const s[] = "Usage: " APPBASENAME " [files] [options]\n"
        "Example: " APPBASENAME " -usecwd -cfg myconfig.cfg -map nukeland.map\n\n"
        "Files can be of type [grp|zip|map|con|def]\n"
        "\n"
        "-cfg [file.cfg]\t\tUse an alternate configuration file\n"
#ifdef HAVE_CLIPSHAPE_FEATURE
        "-clipmap [file.map]\tLoad an additional clipping map for use with clipshape\n"
#endif
        "-connect [host]\t\tConnect to a multiplayer game\n"
        "-c#\t\tMultiplayer mode #, 1 = DM, 2 = Co-op, 3 = DM(no spawn)\n"
        "-d [file.edm or #]\tPlay a demo\n"
        "-g [file.grp]\tLoad additional game data\n"
        "-h [file.def]\tLoad an alternate definitions file\n"
        "-j [dir]\t\tAdd a directory to " APPNAME "'s search list\n"
        "-l#\t\tStart game on level #, see -v\n"
        "-map [file.map]\tLoad an external map file\n"
        "-mh [file.def]\tInclude an additional definitions module\n"
        "-mx [file.con]\tInclude an additional CON script module\n"
        "-m\t\tDisable enemies\n"
        "-noffire\t\tDisable friendly fire\n"
#ifndef EDUKE32_STANDALONE
        "-nam\t\tRun in NAM compatibility mode\n"
        "-napalm\t\tRun in NAPALM compatibility mode\n"
#endif
#if defined VW_ENABLED
        "-nosteam\t\tDisable Steam support\n"
#endif
        "-rts [file.rts]\tLoad a custom Remote Ridicule sound bank\n"
        "-r\t\tRecord demo\n"
        "-s#\t\tStart game on skill level #\n"
        "-server\t\tStart a multiplayer server\n"
#ifdef STARTUP_SETUP_WINDOW
        "-setup/nosetup\tEnable or disable startup window\n"
#endif
        "-t#\t\tRespawn mode: 1 = enemies, 2 = weapons, 3 = items, x = all\n"
        "-usecwd\t\tRead data and configuration from current directory\n"
        "-u#########\tUser's favorite weapon order (default: 3425689071)\n"
        "-v#\t\tStart game on episode #, see -l\n"
#ifndef EDUKE32_STANDALONE
        "-ww2gi\t\tRun in WWII GI compatibility mode\n"
#endif
        "-x [game.con]\tLoad custom CON script\n"
        "-#\t\tLoad and run a game from slot # (0-9)\n"
        //              "\n-?/--help\tDisplay this help message and exit\n"
        "\nSee " APPBASENAME " -debughelp for additional parameters for debugging"
        ;
#ifdef WM_MSGBOX_WINDOW
    //Bsnprintf(tempbuf, sizeof(tempbuf), HEAD2 " %s", s_buildRev);
    //wm_msgbox(tempbuf, s);
#endif
    LOG_F(INFO, s);
}

void G_ShowDebugHelp(void)
{
    static char const s[] = "Usage: " APPBASENAME " [files] [options]\n"
        "\n"
#if 0
        "-a\t\tUse fake player AI (fake multiplayer only)\n"
#endif
        "-cachesize #\tSet cache size in kB\n"
        "-game_dir [dir]\tSpecify game data directory\n"
        "-gamegrp   \tSelect main grp file\n"
        "-name [name]\tPlayer name in multiplayer\n"
        "-noautoload\tDisable loading from autoload directory\n"
#if defined RENDERTYPEWIN
        "-nodinput\t\tDisable DirectInput (joystick) support\n"
#endif
        "-nologo\t\tSkip intro anim\n"
        "-ns\t\tDisable sound\n"
        "-nm\t\tDisable music\n"
        "-q#\t\tFake multiplayer with # players\n"
        "-z#/-condebug\tEnable line-by-line CON compile debugging at level #\n"
        "-conversion YYYYMMDD\tSelects CON script version for compatibility with older mods\n"
        "-rotatesprite-no-widescreen\tStretch screen drawing from scripts to fullscreen\n"
        ;
#ifdef WM_MSGBOX_WINDOW
    Bsnprintf(tempbuf, sizeof(tempbuf), HEAD2 " %s", s_buildRev);
    wm_msgbox(tempbuf, s);
#else
    LOG_F(INFO, s);
#endif
}

static void G_AddDemo(const char* param)
{
    Bstrncpyz(tempbuf, param, sizeof(tempbuf));
    char * colon = (char *) Bstrchr(tempbuf, ':');
    int32_t framespertic=-1, numrepeats=1;

    if (colon && colon != tempbuf)
    {
        // -d<filename>:<num>[,<num>]
        // profiling options
        *(colon++) = 0;
        Bsscanf(colon, "%d,%d", &framespertic, &numrepeats);
    }

    Demo_SetFirst(tempbuf);

    if (framespertic < 0)
    {
        LOG_F(INFO, "Playing demo %s", g_firstDemoFile);
    }
    else
    {
        framespertic = clamp(framespertic, 0, 8)+1;
        // TODO: repeat count and gathering statistics.
        LOG_F(INFO, "Profile demo %s, %d frames/gametic, repeated 1x.", g_firstDemoFile,
            framespertic-1);
        Demo_PlayFirst(framespertic, 1);
        g_noLogo = 1;
    }
}

void G_CheckCommandLine(int32_t argc, char const * const * argv)
{
    const char *c, *k;

    ud.m_respawn_items = 0;
    ud.m_respawn_monsters = 0;
    ud.m_respawn_inventory = 0;
    ud.warp_on = 0;
    ud.m_ffire = 1;
    ud.m_player_skill = ud.player_skill = 2;
    g_player[0].wchoice[0] = 3;
    g_player[0].wchoice[1] = 4;
    g_player[0].wchoice[2] = 5;
    g_player[0].wchoice[3] = 7;
    g_player[0].wchoice[4] = 8;
    g_player[0].wchoice[5] = 6;
    g_player[0].wchoice[6] = 0;
    g_player[0].wchoice[7] = 2;
    g_player[0].wchoice[8] = 9;
    g_player[0].wchoice[9] = 1;
    Bsprintf(ud.wchoice, "3457860291");

#ifdef HAVE_CLIPSHAPE_FEATURE
    // pre-form the default 10 clipmaps
    for (int j = '0'; j<='9'; ++j)
    {
        char clipshape[16] = "_clipshape0.map";

        clipshape[10] = j;
        g_clipMapFiles.append(Xstrdup(clipshape));
    }
#endif

    if (argc > 1)
    {
        size_t constexpr size = ARRAY_SIZE(tempbuf);
        size_t bytesWritten = Bsnprintf(tempbuf, size, "Application parameters:");
        for (int i = 1; i < argc; ++i)
            bytesWritten += Bsnprintf(tempbuf + bytesWritten, size - bytesWritten, " %s", argv[i]);
        LOG_F(INFO, "%s", tempbuf);

        int i = 1;
        do
        {
            const char *const oc = argv[i];
            int32_t shortopt = 0, ignored_short_opt = 0;

            c = oc;

            if ((*c == '-')
#ifdef _WIN32
                || (*c == '/')
#endif
                )
            {
                shortopt = 0;

                if (!Bstrcasecmp(c+1, "?") || !Bstrcasecmp(c+1, "help") || !Bstrcasecmp(c+1, "-help"))
                {
                    G_ShowParameterHelp();
                    Bexit(EXIT_SUCCESS);
                }
                if (!Bstrcasecmp(c+1, "addon"))
                {
                    if (argc > i+1)
                    {
                        g_addonNum = Batoi(argv[i+1]);

                        if (g_addonNum > ADDON_NONE && g_addonNum < NUMADDONS)
                            g_noSetup = 1;
                        else g_addonNum = ADDON_NONE;

                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "debughelp") || !Bstrcasecmp(c+1, "-debughelp"))
                {
                    G_ShowDebugHelp();
                    Bexit(EXIT_SUCCESS);
                }
                if (!Bstrcasecmp(c+1, "grp") || !Bstrcasecmp(c+1, "g"))
                {
                    if (argc > i+1)
                    {
                        G_AddGroup(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "game_dir"))
                {
                    if (argc > i+1)
                    {
                        Bstrncpyz(g_modDir, argv[i+1], sizeof(g_modDir));
                        G_AddPath(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "cfg"))
                {
                    if (argc > i+1)
                    {
                        Bstrcpy(g_setupFileName, argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "gamegrp"))
                {
                    if (argc > i+1)
                    {
                        clearGrpNamePtr();
                        g_grpNamePtr = dup_filename(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
#ifndef EDUKE32_STANDALONE
                if (!Bstrcasecmp(c+1, "nam"))
                {
                    g_gameType = GAMEFLAG_NAM;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "napalm"))
                {
                    g_gameType = GAMEFLAG_NAM|GAMEFLAG_NAPALM;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "ww2gi"))
                {
                    g_gameType = GAMEFLAG_WW2GI;
                    i++;
                    continue;
                }
#endif
                if (!Bstrcasecmp(c+1, "setup"))
                {
                    g_commandSetup = TRUE;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "nosetup"))
                {
                    g_noSetup = 1;
                    g_commandSetup = 0;
                    i++;
                    continue;
                }
#ifdef _WIN32
                if (!Bstrcasecmp(c+1, "nodinput"))
                {
                    VLOG_F(LOG_INPUT, "DirectInput (controller) support disabled");
                    g_controllerSupportFlags |= CONTROLLER_NO_DINPUT;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "noxinput"))
                {
                    VLOG_F(LOG_INPUT, "XInput (controller) support disabled");
                    g_controllerSupportFlags |= CONTROLLER_NO_XINPUT;
                    i++;
                    continue;
                }
#endif
                if (!Bstrcasecmp(c+1, "nocontroller"))
                {
                    VLOG_F(LOG_INPUT, "Controller support disabled.");
                    g_controllerSupportFlags |= CONTROLLER_DISABLED;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "noautoload"))
                {
                    LOG_F(INFO, "Autoload disabled");
                    g_noAutoLoad = 1;
                    i++;
                    continue;
                }
#ifndef NETCODE_DISABLE
                if (!Bstrcasecmp(c+1, "net"))
                {
                    G_GameExit(APPNAME " no longer supports legacy networking.\n\n"
                        "If using YANG or other launchers that only support legacy netplay, download NetDuke32. "
                        "Otherwise, run the following:\n\n"
                        APPBASENAME " -server\n\n"
                        "Other clients can then connect by typing \"connect [host]\" in the console.\n\n"
                        APPNAME " will now close.");
                }
#endif
                if (!Bstrcasecmp(c+1, "port"))
                {
                    if (argc > i+1)
                    {
                        g_netPort = Batoi(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
#ifndef NETCODE_DISABLE
                if (!Bstrcasecmp(c+1, "server"))
                {
                    g_networkMode = NET_SERVER;
                    g_noSetup = g_noLogo = TRUE;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "dedicated"))
                {
                    g_networkMode = NET_DEDICATED_SERVER;
                    g_noSetup = g_noLogo = TRUE;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "connect"))
                {
                    if (argc > i+1)
                    {
                        Net_Connect(argv[i+1]);
                        g_noSetup = g_noLogo = TRUE;
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "password"))
                {
                    if (argc > i+1)
                    {
                        Bstrncpyz(g_netPassword, argv[i+1], sizeof(g_netPassword));
                        i++;
                    }
                    i++;
                    continue;
                }
#endif
                if (!Bstrcasecmp(c+1, "name"))
                {
                    if (argc > i+1)
                    {
                        CommandName = argv[i+1];
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "map"))
                {
                    if (argc > i+1)
                    {
                        CommandMap = argv[i+1];
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "noffire"))
                {
                    ud.m_ffire = 0;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "rts"))
                {
                    if (argc > i+1)
                    {
                        Xfree(g_rtsNamePtr);
                        g_rtsNamePtr = dup_filename(argv[i+1]);
                        LOG_F(INFO, "Using RTS file %s", g_rtsNamePtr);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "x"))
                {
                    if (argc > i+1)
                    {
                        G_AddCon(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "mx"))
                {
                    if (argc > i+1)
                    {
                        G_AddConModule(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "h"))
                {
                    if (argc > i+1)
                    {
                        G_AddDef(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "mh"))
                {
                    if (argc > i+1)
                    {
                        G_AddDefModule(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "j"))
                {
                    if (argc > i+1)
                    {
                        G_AddPath(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "d"))
                {
                    if (argc > i+1)
                    {
                        G_AddDemo(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
#ifdef HAVE_CLIPSHAPE_FEATURE
                if (!Bstrcasecmp(c+1, "clipmap"))
                {
                    if (argc > i+1)
                    {
                        G_AddClipMap(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
#endif
                if (!Bstrcasecmp(c+1, "condebug"))
                {
                    g_scriptDebug = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "nologo") || !Bstrcasecmp(c+1, "quick"))
                {
                    g_noLogo = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "rotatesprite-no-widescreen"))
                {
                    g_rotatespriteNoWidescreen = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "usecwd"))
                {
                    g_useCwd = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "cachesize"))
                {
                    if (argc > i+1)
                    {
                        uint32_t j = Batol(argv[i+1]);
                        MAXCACHE1DSIZE = j<<10;
                        LOG_F(INFO, "Cache size: %dkB", j);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "noinstancechecking"))
                {
                    i++;
                    continue;
                }
#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
                if (!Bstrcasecmp(c+1, "forcegl"))
                {
                    forcegl = 1;
                    i++;
                    continue;
                }
#endif
#ifdef VW_ENABLED
                if (!Bstrcasecmp(c+1, "nosteam"))
                {
                    i++;
                    continue;
                }
#endif
                // the following two dummy entries allow us to serve as a drop-in replacement for NAM on Steam
                if (!Bstrcasecmp(c+1, "noconsole"))
                {
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1, "conf"))
                {
                    if (argc > i+1)
                        i++;
                    i++;
                    continue;
                }
            }

            if ((*c == '-')
#ifdef _WIN32
                || (*c == '/')
#endif
                )
            {
                shortopt = 1;

                c++;
                switch (Btolower(*c))
                {
                case 'a':
                    ud.playerai = 1;
                    LOG_F(INFO, "Other player \"AI.\"");
                    break;
                case 'c':
                    c++;
                    ud.m_coop = 0;
                    while ((*c >= '0')&&(*c <= '9'))
                    {
                        ud.m_coop *= 10;
                        ud.m_coop += *c - '0';
                        c++;
                    }
                    ud.m_coop--;
                    break;
                case 'd':
                {
                    c++;
                    if (*c)
                        G_AddDemo(c);
                    break;
                }
                case 'g':
                    c++;
                    if (*c)
                        G_AddGroup(c);
                    break;
                case 'h':
                    c++;
                    if (*c)
                        G_AddDef(c);
                    break;
                case 'j':
                    c++;
                    if (*c)
                        G_AddPath(c);
                    break;
                case 'l':
                    // NOTE: Overlaid with -Lopts=... options for Lunatic, hence the check.
                    if (Bisdigit(c[1]))
                    {
                        ud.warp_on = 1;
                        c++;
                        ud.m_level_number = ud.level_number = ((unsigned) (Batoi(c)-1))%MAXLEVELS;
                    }
                    break;
                case 'm':
                    if (*(c+1) != 'a' && *(c+1) != 'A')
                    {
                        ud.m_monsters_off = 1;
                        ud.m_player_skill = ud.player_skill = 0;
                        LOG_F(INFO, "Monsters off.");
                    }
                    break;
                case 'n':
                    c++;
                    if (*c == 's' || *c == 'S')
                    {
                        g_noSound = 2;
                        LOG_F(INFO, "Sound off (like you've got a pair).");
                    }
                    else if (*c == 'm' || *c == 'M')
                    {
                        g_noMusic = 1;
                        LOG_F(INFO, "Music off.");
                    }
                    else
                    {
                        G_ShowParameterHelp();
                        Bexit(EXIT_SUCCESS);
                    }
                    break;
                case 'q':
                    if (*(++c) == 0)
                    {
                        ud.multimode = 1;
                        LOG_F(INFO, "Fake multiplayer mode: expected number after -q, falling back to 1 player.");
                    }
                    else
                    {
                        int32_t numpl = Batoi(c);

                        if (numpl < 2 || numpl > MAXPLAYERS)
                        {
                            LOG_F(INFO, "Fake multiplayer mode: expected 2-%d players, falling back to 1.", MAXPLAYERS);
                        }
                        else
                        {
                            ud.multimode = numpl;
                            LOG_F(INFO, "Fake multiplayer mode: %d players.", ud.multimode);

                            g_fakeMultiMode = numpl;
                        }
                    }

                    ud.m_coop = ud.coop = 0;
                    ud.m_marker = ud.marker = 1;
                    ud.m_respawn_monsters = ud.respawn_monsters = 1;
                    ud.m_respawn_items = ud.respawn_items = 1;
                    ud.m_respawn_inventory = ud.respawn_inventory = 1;
                    break;
                case 'r':
                    ud.m_recstat = 1;
                    LOG_F(INFO, "Demo record mode on.");
                    break;
                case 's':
                    c++;
                    ud.m_player_skill = ud.player_skill = clamp(Batoi(c), 0, g_maxDefinedSkill);
                    if (ud.m_player_skill == 4)
                        ud.m_respawn_monsters = ud.respawn_monsters = 1;
                    break;
                case 't':
                    c++;
                    if (*c == '1') ud.m_respawn_monsters = 1;
                    else if (*c == '2') ud.m_respawn_items = 1;
                    else if (*c == '3') ud.m_respawn_inventory = 1;
                    else
                    {
                        ud.m_respawn_monsters = 1;
                        ud.m_respawn_items = 1;
                        ud.m_respawn_inventory = 1;
                    }
                    LOG_F(INFO, "Respawn on.");
                    break;
                case 'u':
                {
                    c++;
                    int j = 0;
                    if (*c)
                    {

                        // Only accept numbers from 0 to 9
                        while ((*c >= '0') && (*c <= '9') && j < min<int>(MAX_WEAPONS, 10))
                        {
                            g_player[0].wchoice[j] = *c - '0';
                            ud.wchoice[j] = *c;
                            c++;
                            j++;
                        }

                        // Apply weapon priority if a valid value is given
                        if (j > 0)
                        {
                            g_forceWeaponChoice = 1;
                            LOG_F(INFO, "Using favorite weapon order(s).");
                            // Pad value when not all weapons are given
                            while (j < 10)
                            {
                                if (j == 9)
                                {
                                    g_player[0].wchoice[9] = 1;
                                    ud.wchoice[9] = '1';
                                }
                                else
                                {
                                    g_player[0].wchoice[j] = 2;
                                    ud.wchoice[j] = '2';
                                }

                                j++;
                            }
                        }
                    }

                    if (!g_forceWeaponChoice)
                    {
                        LOG_F(INFO, "Using default weapon orders.");
                        g_player[0].wchoice[0] = 3;
                        g_player[0].wchoice[1] = 4;
                        g_player[0].wchoice[2] = 5;
                        g_player[0].wchoice[3] = 7;
                        g_player[0].wchoice[4] = 8;
                        g_player[0].wchoice[5] = 6;
                        g_player[0].wchoice[6] = 0;
                        g_player[0].wchoice[7] = 2;
                        g_player[0].wchoice[8] = 9;
                        g_player[0].wchoice[9] = 1;

                        Bsprintf(ud.wchoice, "3457860291");
                    }
                    break;
                }
                case 'v':
                    c++;
                    ud.warp_on = 1;
                    ud.m_volume_number = ud.volume_number = ((unsigned) (Batoi(c)-1))%MAXVOLUMES;
                    break;
                case 'w':
                    ud.coords = 1;
                    break;
                case 'x':
                    c++;
                    if (*c)
                        G_AddCon(c);
                    break;
                case 'z':
                    c++;
                    g_scriptDebug = Batoi(c);
                    if (!g_scriptDebug)
                        g_scriptDebug = 1;
                    break;
                default:
                    ignored_short_opt = 1;
                    break;
                }
            }
            else
            {
                shortopt = 0;

                k = Bstrrchr(c, '.');
                if (k)
                {
                    if (!Bstrcasecmp(k, ".map"))
                    {
                        CommandMap = argv[i++];
                        continue;
                    }
                    if (!Bstrcasecmp(k, ".grp") || !Bstrcasecmp(k, ".zip") || !Bstrcasecmp(k, ".pk3") || !Bstrcasecmp(k, ".pk4"))
                    {
                        G_AddGroup(argv[i++]);
                        continue;
                    }
                    if (!Bstrcasecmp(k, ".con"))
                    {
                        clearScriptNamePtr();
                        g_scriptNamePtr = dup_filename(argv[i++]);
                        LOG_F(INFO, "Using CON file %s", g_scriptNamePtr);
                        continue;
                    }
                    if (!Bstrcasecmp(k, ".def"))
                    {
                        clearDefNamePtr();
                        g_defNamePtr = dup_filename(argv[i++]);
                        LOG_F(INFO, "Using DEF file %s", g_defNamePtr);
                        continue;
                    }
                    if (!Bstrcasecmp(k, ".rts"))
                    {
                        Xfree(g_rtsNamePtr);
                        g_rtsNamePtr = dup_filename(argv[i++]);
                        LOG_F(INFO, "Using RTS file %s", g_rtsNamePtr);
                        continue;
                    }
                }
            }

            if (!shortopt || ignored_short_opt)
                LOG_F(WARNING, "Ignored application parameter '%s'.", oc);

            i++;
        } while (i < argc);
    }
}
