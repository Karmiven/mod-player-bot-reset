# AzerothCore Module: Player Bot Level Reset

## Overview

This module for **AzerothCore** resets the level of **player bots** when they exceed a configurable maximum level. It supports **random player bots** and integrates with the **AutoMaintenanceOnLevelupAction** system to reinitialize the bot's state upon reset.

## Features

- **Configurable Maximum Level**: Set the maximum level a bot can reach before being reset to level 1.
- **Configurable Reset Chance**: Specify the percentage chance for a bot's level to reset upon reaching the max level.
- **Scaled Reset Chance (New Feature)**: Optionally, make the reset chance **happen on every level-up** and **scale dynamically** as the bot levels up. The reset chance starts low at early levels and increases as the bot approaches max level. At the max level, it reaches the configured Reset Chance.
- **Support for Random Bots**: Only applies the reset to bots managed by `RandomPlayerbotMgr`.
- **Auto Equipment Reset**: Destroys all equipped items when resetting a bot.
- **Auto Maintenance Execution**: Runs `AutoMaintenanceOnLevelupAction` after resetting to ensure bots are properly initialized.
- **Debug Mode**: Optional logging for detailed debugging.

## Installation

Ensure you have **AzerothCore** installed and running.

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

Update the module configuration:
```ini
ResetBotLevel.MaxLevel = 80
ResetBotLevel.ResetChance = 100
ResetBotLevel.ScaledChance = 0
ResetBotLevel.DebugMode = 0
```

Rename the configuration file:
```sh
mv /path/to/azerothcore/modules/mod-player-bot-reset.conf.dist /path/to/azerothcore/modules/mod-player-bot-reset.conf
```

Restart the server:
```sh
./worldserver
```

## Configuration Options

Modify these settings in `mod-player-bot-reset.conf` to customize behavior:

| Setting                     | Description                                             | Default | Valid Values       |
| --------------------------- | ------------------------------------------------------- | ------- | ------------------ |
| `ResetBotLevel.MaxLevel`    | Maximum level before reset                              | `80`    | `2-80`             |
| `ResetBotLevel.ResetChance` | % chance to reset upon reaching max level              | `100`   | `0-100`            |
| `ResetBotLevel.ScaledChance` | Enables reset chance scaling per level-up (NEW)       | `0`     | `0 (off) / 1 (on)` |
| `ResetBotLevel.DebugMode`   | Enable detailed debug logging                          | `0`     | `0 (off) / 1 (on)` |

## Debugging

Enable debug logging by setting:

```ini
ResetBotLevel.DebugMode = 1
```

This will log actions like bot resets, destroyed items, and maintenance execution.

## License

This module is released under the **GNU GPL v2** license, following AzerothCore's licensing model.

## Contribution

Created by Dustin Hendrickson

Pull requests and issues are welcome! Ensure your changes follow AzerothCore's coding standards.
