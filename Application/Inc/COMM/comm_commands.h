/*
 * comm_commands.h
 *
 *  Created on: Mar 25, 2026
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef COMM_COMM_COMMANDS_H_
#define COMM_COMM_COMMANDS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "comm_main.h"
#include "comm_menu_system.h"
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

#define COMM_COMMAND_DELIMITER     ':'
#define COMM_COMMAND_DELIMITER_STR ":"

typedef struct {
  CommInterface_t interface;
  MenuNode_t** current_menu;
  bool redraw_menu;
} CommandContext_t;

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Attempts to parse and execute a command
 *
 * The command must begin with COMM_COMMAND_DELIMITER after optional leading
 * whitespace. When a command is recognized, the appropriate handler is invoked
 * and the function returns true. Unknown commands and invalid arguments are
 * treated as handled errors and also return true.
 *
 * @param input HMI input buffer
 * @param context Command execution context
 *
 * @return true if the input was treated as a command, false otherwise
 */
bool COMM_Commands_Process(const char* input, CommandContext_t* context);

#ifdef __cplusplus
}
#endif

#endif /* COMM_COMM_COMMANDS_H_ */
