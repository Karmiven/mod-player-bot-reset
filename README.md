# AzerothCore Module: Player Bot Level Reset

## Overview

This module for **AzerothCore** resets the level of **player bots** when they exceed a configurable maximum level. It supports **random player bots** and integrates with the **AutoMaintenanceOnLevelupAction** system to reinitialize the bot's state upon reset.

## Features

- **Configurable Maximum Level**: Set the maximum level a bot can reach before being reset.
- **Configurable Reset Chance**: Specify the percentage chance for a bot's level to reset upon reaching the maximum level.
- **Scaled Reset Chance**: Optionally enable per-level checks where the reset chance scales dynamically as the bot levels up. The chance increases as the bot approaches the maximum level, reaching the configured Reset Chance at the maximum.
- **Support for Random Bots**: Applies only to bots managed by `RandomPlayerbotMgr`.
- **Auto Equipment Reset**: Destroys all equipped items when resetting a bot.
- **Auto Maintenance Execution**: Executes `AutoMaintenanceOnLevelupAction` after a reset to ensure proper bot initialization.
- **Death Knight Support**: For Death Knight bots, resets the level to 55 instead of 1.
- **Time-Played Based Reset**: When enabled, bots at or above the maximum level are reset only if they have accumulated a minimum amount of played time at that level. This check is performed periodically via an OnUpdate handler.
- **Debug Mode**: Provides optional detailed logging for debugging purposes.

## Installation

Ensure you have the **AzerothCore Playerbots fork** installed and running.

Clone the module into your AzerothCore modules directory:

```sh
cd /path/to/azerothcore/modules
git clone https://github.com/DustinHendrickson/mod-player-bot-reset.git
```

Recompile AzerothCore:

```sh
cd /path/to/azerothcore
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Update the module configuration by renaming the configuration file:

```sh
mv /path/to/azerothcore/modules/mod-player-bot-reset.conf.dist /path/to/azerothcore/modules/mod-player-bot-reset.conf
```

Restart the server:

```sh
./worldserver
```

## Configuration Options

Modify the following settings in `mod-player-bot-reset.conf` to customize the module's behavior:

| Setting                           | Description                                                                                                                             | Default  | Valid Values       |
| --------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------- | -------- | ------------------ |
| `ResetBotLevel.MaxLevel`          | Maximum level before checking for a reset.                                                                                             | `80`     | `2-80`             |
| `ResetBotLevel.ResetChance`       | Percentage chance to reset upon reaching the maximum level.                                                                            | `100`    | `0-100`            |
| `ResetBotLevel.ScaledChance`      | If enabled (1), the reset chance is evaluated on every level-up and scales based on the bot's current level relative to max level.       | `0`      | `0 (off) / 1 (on)` |
| `ResetBotLevel.DebugMode`         | Enables detailed debug logging for module actions.                                                                                     | `0`      | `0 (off) / 1 (on)` |
| `ResetBotLevel.RestrictTimePlayed`| If enabled (1), bots will only be reset when they have played at least the specified minimum time at the current level when at max level.| `0`      | `0 (off) / 1 (on)` |
| `ResetBotLevel.MinTimePlayed`     | The minimum time in seconds that a bot must have played at its current level before a reset can occur when at max level.                 | `86400`  | Positive Integer   |
| `ResetBotLevel.PlayedTimeCheckFrequency` | The frequency (in seconds) at which the time played check is performed for bots at or above the maximum level.                    | `60`     | Positive Integer   |

## Debugging

To enable detailed debug logging, modify the configuration as follows:

```ini
ResetBotLevel.DebugMode = 1
```

This will output detailed logs for actions such as bot resets, item destruction, and auto maintenance execution.

## License

This module is released under the **GNU GPL v2** license, in accordance with AzerothCore's licensing model.

## Contribution

Created by Dustin Hendrickson.

Pull requests and issues are welcome. Please ensure your contributions follow AzerothCore's coding standards.
