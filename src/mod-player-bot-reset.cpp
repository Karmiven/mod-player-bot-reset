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
static bool g_DebugMode              = false;
static bool g_ScaledChance           = false;

// -----------------------------------------------------------------------------
// LOAD CONFIGURATION USING sConfigMgr
// -----------------------------------------------------------------------------
static void LoadPlayerBotResetConfig()
{
    g_ResetBotMaxLevel = static_cast<uint8>(sConfigMgr->GetOption<uint32>("ResetBotLevel.MaxLevel", 80));

    if (g_ResetBotMaxLevel < 2 || g_ResetBotMaxLevel > 80)
    {
        LOG_ERROR("server.loading", "[mod-player-bot-reset] Invalid ResetBotLevel.MaxLevel value: {}. Using default value 80.", g_ResetBotMaxLevel);
        g_ResetBotMaxLevel = 80;
    }

    g_ResetBotChancePercent = static_cast<uint8>(sConfigMgr->GetOption<uint32>("ResetBotLevel.ResetChance", 100));

    if (g_ResetBotChancePercent > 100)
    {
        LOG_ERROR("server.loading", "[mod-player-bot-reset] Invalid ResetBotLevel.ResetChance value: {}. Using default value 100.", g_ResetBotChancePercent);
        g_ResetBotChancePercent = 100;
    }

    g_DebugMode = sConfigMgr->GetOption<bool>("ResetBotLevel.DebugMode", false);

    g_ScaledChance = sConfigMgr->GetOption<bool>("ResetBotLevel.ScaledChance", false);
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

    PlayerbotAI* botAI = sPlayerbotsMgr->GetPlayerbotAI(player);
    return botAI && botAI->IsBotAI();
}

static bool IsPlayerRandomBot(Player* player)
{
    if (!player)
    {
        LOG_ERROR("server.loading", "[mod-player-bot-reset] IsPlayerRandomBot called with nullptr.");
        return false;
    }

    return sRandomPlayerbotMgr->IsRandomBot(player);
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
        LOG_INFO("server.loading", "[mod-player-bot-reset] Loaded and active with MaxLevel = {}, ResetChance = {}%, ScaledChance = {}.",
                 static_cast<int>(g_ResetBotMaxLevel),
                 static_cast<int>(g_ResetBotChancePercent),
                 g_ScaledChance ? "Enabled" : "Disabled");
    }
};

// -----------------------------------------------------------------------------
// PLAYER SCRIPT: Handle OnLogin and Reset Bot Level if Necessary
// -----------------------------------------------------------------------------
class ResetBotLevelPlayerScript : public PlayerScript
{
public:
    ResetBotLevelPlayerScript() : PlayerScript("ResetBotLevelPlayerScript") { }

    void OnLogin(Player* player) override
    {
        if (!player)
            return;

        ChatHandler(player->GetSession()).SendSysMessage("The [mod-player-bot-reset] module is active on this server.");
    }

    void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if (!player)
        {
            LOG_ERROR("server.loading", "[mod-player-bot-reset] OnLevelChanged called with nullptr player.");
            return;
        }

        uint8 newLevel = player->GetLevel();
        if (newLevel == 1)
        {
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

        // Compute the scaled reset chance if enabled
        uint8 resetChance = g_ResetBotChancePercent;
        if (g_ScaledChance)
        {
            resetChance = static_cast<uint8>((static_cast<float>(newLevel) / g_ResetBotMaxLevel) * g_ResetBotChancePercent);
            if (g_DebugMode)
            {
                LOG_INFO("server.loading", "[mod-player-bot-reset] Scaled reset chance for bot '{}' at level {}: {}%", player->GetName(), newLevel, resetChance);
            }
        }

        // If scaling is enabled, reset check happens at **every level-up**
        // Otherwise, only check at max level
        if (g_ScaledChance || newLevel >= g_ResetBotMaxLevel)
        {
            if (urand(0, 99) < resetChance)
            {
                player->SetLevel(1);
                player->SetUInt32Value(PLAYER_XP, 0);

                ChatHandler(player->GetSession()).SendSysMessage("[mod-player-bot-reset] Your level has been reset to 1.");

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
                    LOG_INFO("server.loading", "[mod-player-bot-reset] Bot '{}' hit level {} and was reset to level 1.", player->GetName(), newLevel);
                }

                PlayerbotAI* botAI = sPlayerbotsMgr->GetPlayerbotAI(player);
                if (botAI)
                {
                    AutoMaintenanceOnLevelupAction maintenanceAction(botAI);
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
