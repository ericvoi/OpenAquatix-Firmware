/*
 * comm_commands.c
 *
 *  Created on: Mar 25, 2026
 *      Author: jac4e
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "comm_commands.h"
#include "cfg_parameters.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

typedef bool (*CommandHandler_t)(CommandContext_t* context, const char* args);

typedef struct {
  const char* name;
  const char* description;
  const char* usage;
  CommandHandler_t handler;
} CommandEntry_t;

/* Private function prototypes -----------------------------------------------*/

// Utility functions
static char* trimWhitespace(char* text);
static const CommandEntry_t* findCommand(const char* name);
static const char* normalizeCommandName(const char* name);
static bool parseOnOffArgument(const char* args, bool* enabled);

// Help command helpers
static void transmitCommandUsage(CommInterface_t interface, const CommandEntry_t* command);
static void transmitCommandHelp(CommInterface_t interface, const CommandEntry_t* command);

// Command handlers
static bool handleHelpCommand(CommandContext_t* context, const char* args);
static bool handleTagCommand(CommandContext_t* context, const char* args);
static bool handlePrintCommand(CommandContext_t* context, const char* args);
static bool handleTransmitCommand(CommandContext_t* context, const char* args);
static bool handleConfigCommand(CommandContext_t* context, const char* args);

/* Private variables ---------------------------------------------------------*/

static const CommandEntry_t commands[] = {
  {
    "help",
    "Display the available commands or detailed help for one command",
    COMM_COMMAND_DELIMITER_STR "help [command]",
    handleHelpCommand
  },
  {
    "tag",
    "Enable or disable tagged COMM HMI output for the current session",
    COMM_COMMAND_DELIMITER_STR "tag on|off",
    handleTagCommand
  },
  {
    "print",
    "Enable or disable printing of received messages",
    COMM_COMMAND_DELIMITER_STR "print on|off",
    handlePrintCommand
  },
  {
    "tx",
    "Transmit a message, data should be base64 encoded",
    COMM_COMMAND_DELIMITER_STR "tx <base64_data>",
    handleTransmitCommand
  },
  {
    "config",
    "Get or set configuration parameters. Use without arguments to list all parameters and their values. Use get or set followed by the parameter name to get or set a specific parameter. When setting a parameter, include the new value after the parameter name. For example: " COMM_COMMAND_DELIMITER_STR "config set PARAM_NAME value",
    COMM_COMMAND_DELIMITER_STR "config [get|set <parameter_name> [value]]",
    handleConfigCommand
  }
};

/* Exported function definitions ---------------------------------------------*/

bool COMM_Commands_Process(const char* input, CommandContext_t* context)
{
  char command_buffer[MAX_COMM_IN_BUFFER_SIZE];

  if (input == NULL || context == NULL) {
    return false;
  }

  strncpy(command_buffer, input, sizeof(command_buffer) - 1);
  command_buffer[sizeof(command_buffer) - 1] = '\0';

  char* cursor = trimWhitespace(command_buffer);
  if (*cursor != COMM_COMMAND_DELIMITER) {
    return false;
  }

  cursor++;
  cursor = trimWhitespace(cursor);

  context->redraw_menu = true;

  if (*cursor == '\0') {
    COMM_TransmitHmiMachineLine(HMI_TAG_ERROR, "Missing command name", context->interface);
    return true;
  }

  char* args = cursor;
  while (*args != '\0' && isspace((unsigned char) *args) == 0) {
    args++;
  }

  if (*args != '\0') {
    *args = '\0';
    args = trimWhitespace(args + 1);
  }

  const CommandEntry_t* command = findCommand(cursor);
  if (command == NULL) {
    COMM_TransmitHmiMachineLinef(HMI_TAG_ERROR, context->interface,
        "Unknown command: %c%s", COMM_COMMAND_DELIMITER, cursor);
    return true;
  }

  if (command->handler(context, args) == false) {
    transmitCommandUsage(context->interface, command);
  }

  return true;
}

/* Private function definitions ----------------------------------------------*/

static char* trimWhitespace(char* text)
{
  if (text == NULL) {
    return NULL;
  }

  while (*text != '\0' && isspace((unsigned char) *text) != 0) {
    text++;
  }

  size_t length = strlen(text);
  while (length > 0 && isspace((unsigned char) text[length - 1]) != 0) {
    text[length - 1] = '\0';
    length--;
  }

  return text;
}

static const CommandEntry_t* findCommand(const char* name)
{
  const char* normalized_name = normalizeCommandName(name);
  if (normalized_name == NULL || *normalized_name == '\0') {
    return NULL;
  }

  for (uint32_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    if (strcmp(commands[i].name, normalized_name) == 0) {
      return &commands[i];
    }
  }

  return NULL;
}

static const char* normalizeCommandName(const char* name)
{
  if (name == NULL) {
    return NULL;
  }

  while (*name != '\0' && isspace((unsigned char) *name) != 0) {
    name++;
  }

  if (*name == COMM_COMMAND_DELIMITER) {
    name++;
  }

  while (*name != '\0' && isspace((unsigned char) *name) != 0) {
    name++;
  }

  return name;
}

static bool parseOnOffArgument(const char* args, bool* enabled)
{
  if (args == NULL || enabled == NULL) {
    return false;
  }

  if (strcmp(args, "on") == 0) {
    *enabled = true;
    return true;
  }

  if (strcmp(args, "off") == 0) {
    *enabled = false;
    return true;
  }

  return false;
}

static void transmitCommandUsage(CommInterface_t interface, const CommandEntry_t* command)
{
  COMM_TransmitHmiMachineLinef(HMI_TAG_ERROR, interface, "Usage: %s", command->usage);
}

static void transmitCommandHelp(CommInterface_t interface, const CommandEntry_t* command)
{
  COMM_TransmitHmiMachineLinef(HMI_TAG_STATUS, interface,
      "Command: %c%s", COMM_COMMAND_DELIMITER, command->name);
  COMM_TransmitHmiMachineLinef(HMI_TAG_STATUS, interface,
      "Description: %s", command->description);
  COMM_TransmitHmiMachineLinef(HMI_TAG_STATUS, interface,
      "Usage: %s", command->usage);
}

static bool handleHelpCommand(CommandContext_t* context, const char* args)
{
  if (context == NULL) {
    return false;
  }

  if (args == NULL || *trimWhitespace((char*) args) == '\0') {
    COMM_TransmitHmiMachineLine(HMI_TAG_STATUS, "Available commands:", context->interface);

    for (uint32_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
      COMM_TransmitHmiMachineLinef(HMI_TAG_STATUS, context->interface,
          "%c%s - %s",
          COMM_COMMAND_DELIMITER,
          commands[i].name,
          commands[i].description);
    }

    return true;
  }

  const CommandEntry_t* command = findCommand(args);
  if (command == NULL) {
    COMM_TransmitHmiMachineLinef(HMI_TAG_ERROR, context->interface,
        "Unknown command: %s%s",
        COMM_COMMAND_DELIMITER_STR,
        normalizeCommandName(args));
    return true;
  }

  transmitCommandHelp(context->interface, command);
  return true;
}

static bool handleTagCommand(CommandContext_t* context, const char* args)
{
  // Not implemented yet
  COMM_TransmitHmiMachineLine(HMI_TAG_ERROR,
      "Transmit command not implemented yet", context->interface);
  return true;
}

static bool handlePrintCommand(CommandContext_t* context, const char* args)
{
  // Not implemented yet
  COMM_TransmitHmiMachineLine(HMI_TAG_ERROR,
      "Transmit command not implemented yet", context->interface);
  return true;
}

static bool handleTransmitCommand(CommandContext_t* context, const char* args)
{
  // Not implemented yet
  COMM_TransmitHmiMachineLine(HMI_TAG_ERROR,
      "Transmit command not implemented yet", context->interface);
  return true;
}

static bool handleConfigCommand(CommandContext_t* context, const char* args)
{
  // Not implemented yet
  COMM_TransmitHmiMachineLine(HMI_TAG_ERROR,
      "Config command not implemented yet", context->interface);
  return true;
}