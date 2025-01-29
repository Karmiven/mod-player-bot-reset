#include "mod-player-bot-reset.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Common.h"
#include "Chat.h"
#include "Log.h"
#include "PlayerbotAIBase.h"
#include "Configuration/Config.h"
#include "PlayerbotMgr.h"
#include "PlayerbotAI.h"
#include "AutoMaintenanceOnLevelupAction.h"
#include "ObjectMgr.h" 
#include "WorldSession.h"
#include "Item.h"
#include "RandomPlayerbotMgr.h"


// -----------------------------------------------------------------------------
// GLOBALS: Configuration Values
// -----------------------------------------------------------------------------
static uint8 g_ResetBotMaxLevel      = 80;
static uint8 g_ResetBotChancePercent = 100;
static bool g_DebugMode = false;

// -----------------------------------------------------------------------------
// LOAD CONFIGURATION USING sConfigMgr
// -----------------------------------------------------------------------------
static void LoadPlayerBotResetConfig()
{
    // Load ResetBotLevel.MaxLevel from worldserver.conf
    g_ResetBotMaxLevel = static_cast<uint8>(sConfigMgr->GetOption<uint32>("ResetBotLevel.MaxLevel", 80));

    // Validate the loaded MaxLevel
    if (g_ResetBotMaxLevel < 2 || g_ResetBotMaxLevel > 80)
    {
        LOG_ERROR("server.loading", "[mod-player-bot-reset] Invalid ResetBotLevel.MaxLevel value: {}. It must be between 2 and 80. Using default value 80.", g_ResetBotMaxLevel);
        g_ResetBotMaxLevel = 80;
    }

    // Load ResetBotLevel.ResetChance from worldserver.conf
    g_ResetBotChancePercent = static_cast<uint8>(sConfigMgr->GetOption<uint32>("ResetBotLevel.ResetChance", 100));

    // Validate the loaded ResetChancePercent
    if (g_ResetBotChancePercent > 100)
    {
        LOG_ERROR("server.loading", "[mod-player-bot-reset] Invalid ResetBotLevel.ResetChance value: {}. It must be between 0 and 100. Using default value 100.", g_ResetBotChancePercent);
        g_ResetBotChancePercent = 100;
    }

    // Load Debug Mode from worldserver.conf
    g_DebugMode = sConfigMgr->GetOption<bool>("ResetBotLevel.DebugMode", false);
}

// -----------------------------------------------------------------------------
// DETECT IF PLAYER IS A BOT
// -----------------------------------------------------------------------------
static bool IsPlayerBot(Player* player)
{
    if (!player)
    {
        LOG_ERROR("server.loading", "[mod-player-bot-reset] IsPlayerBot called with nullptr.");
        return false;
    }

    // Retrieve the PlayerbotAI instance using PlayerbotsMgr
    PlayerbotAI* botAI = sPlayerbotsMgr->GetPlayerbotAI(player);
    
    if (botAI && botAI->IsBotAI())
    {
        return true;
    }

    return false;
}

static bool IsPlayerRandomBot(Player* player)
{
    if (!player)
    {
        LOG_ERROR("server.loading", "[mod-player-bot-reset] IsPlayerBot called with nullptr.");
        return false;
    }

    if(sRandomPlayerbotMgr->IsRandomBot(player))
    {
        return true;
    }

    return false;
}

// -----------------------------------------------------------------------------
// WORLD SCRIPT: Load Configuration on Startup
// -----------------------------------------------------------------------------
class ResetBotLevelWorldScript : public WorldScript
{
public:
    ResetBotLevelWorldScript() : WorldScript("ResetBotLevelWorldScript") { }

    void OnStartup() override
    {
        LoadPlayerBotResetConfig();

        // Log the active status and configuration values of the module
        LOG_INFO("server.loading", "[mod-player-bot-reset] Loaded and active with MaxLevel = {} and ResetChance = {}%.", 
                 static_cast<int>(g_ResetBotMaxLevel), 
                 static_cast<int>(g_ResetBotChancePercent));
    }
};

// -----------------------------------------------------------------------------
// PLAYER SCRIPT: Handle OnLogin and Reset Bot Level if Necessary
// -----------------------------------------------------------------------------
class ResetBotLevelPlayerScript : public PlayerScript
{
public:
    ResetBotLevelPlayerScript() : PlayerScript("ResetBotLevelPlayerScript") { }

    // OnLogin event to notify players that the module is active
    void OnLogin(Player* player) override
    {
        if (!player)
            return;

        const std::string loadMessage = "The [mod-player-bot-reset] module is active on this server.";
        ChatHandler(player->GetSession()).SendSysMessage(loadMessage);
    }

    // OnLevelChanged event to reset bot level if it exceeds the configured maximum
    void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        uint8 newLevel = player->GetLevel();
        if(newLevel == 1)
        {
            return;
        }

        if (!player)
        {
            LOG_ERROR("server.loading", "[mod-player-bot-reset] OnLevelChanged called with nullptr player.");
            return;
        }

        if (!IsPlayerBot(player))
        {    
            if (g_DebugMode)
            {
                LOG_INFO("server.loading", "[mod-player-bot-reset] Player '{}' is not a bot. Skipping level reset check.", player->GetName());
            }
            return;
        }

        if (!IsPlayerRandomBot(player))
        {
            if (g_DebugMode)
            {
                LOG_INFO("server.loading", "[mod-player-bot-reset] Player '{}' is not a random bot. Skipping level reset check.", player->GetName());
            }
            return;
        }

        if (newLevel >= g_ResetBotMaxLevel)
        {
            // Roll between 0 and 99; if less than ResetChancePercent, reset the level
            if (urand(0, 99) < g_ResetBotChancePercent)
            {
                player->SetLevel(1);
                player->SetUInt32Value(PLAYER_XP, 0);

                // Notify the player bot that their level has been reset
                const std::string resetMessage = "[mod-player-bot-reset] Your level has been reset to 1.";
                ChatHandler(player->GetSession()).SendSysMessage(resetMessage);

                // Destroy all equipped items
                for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                {
                    if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    {
                        std::string itemName = item->GetTemplate()->Name1;
                        player->DestroyItem(INVENTORY_SLOT_BAG_0, slot, true);
                        if (g_DebugMode)
                        {
                            LOG_INFO("server.loading", "[mod-player-bot-reset] Destroyed item '{}' in slot {} for bot '{}'.", itemName, slot, player->GetName());
                        }
                    }
                }

                if (g_DebugMode)
                {
                    LOG_INFO("server.loading", "[mod-player-bot-reset] Bot '{}' hit level {} and was randomly reset to level 1.", player->GetName(), g_ResetBotMaxLevel);
                }

                // Retrieve the PlayerbotAI instance
                PlayerbotAI* botAI = sPlayerbotsMgr->GetPlayerbotAI(player);
                if (botAI)
                {
                    // Instantiate the AutoMaintenanceOnLevelupAction
                    AutoMaintenanceOnLevelupAction maintenanceAction(botAI);

                    // Execute the maintenance actions
                    maintenanceAction.Execute(Event());

                    if (g_DebugMode)
                    {
                        LOG_INFO("server.loading", "[mod-player-bot-reset] AutoMaintenanceOnLevelupAction executed for bot '{}'.", player->GetName());
                    }
                }
                else
                {
                    LOG_ERROR("server.loading", "[mod-player-bot-reset] Failed to retrieve PlayerbotAI for bot '{}'.", player->GetName());
                }
            }
        }
    }
};

// -----------------------------------------------------------------------------
// ENTRY POINT: Register Scripts
// -----------------------------------------------------------------------------
void Addmod_player_bot_resetScripts()
{
    new ResetBotLevelWorldScript();
    new ResetBotLevelPlayerScript();
}
