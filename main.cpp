#include<iostream>
#include<utility>
#include<functional>
#include"./lib/darray.h"
#include"./lib/mystring.h"
#include"./lib/linmap.h"
#include"./lib/utils.h"

using namespace clib;

enum class CommandFlagType {
  Short, Long
};

enum class CommandArgType {
  Optional, Required
};

struct CommandFlag {
  CommandFlagType type;
  Text text;
};

struct CommandArg {
  CommandArgType type;
  Text text;
};

class Command {
public:
  Command() = default;
  Command(const Text& name);
  Command& arg(CommandArgType type, const Text& text);
  Command& flag(CommandFlagType type, const Text& text);
  Command& describe(const Text& text);
  Command& does(std::function
    <void(const linmap<Text, Text>& args, const linmap<Text, Text>& flags)> effect
  );

  bool isValidArgList(const linmap<Text, Text>& list) const;
  bool isValidFlag(const CommandFlag& flag) const;

  Text getArgNameAt(std::size_t index) const;
  std::size_t getRequiredArgCount() const;
  
  void run(const linmap<Text, Text>& args, const linmap<Text, Text>& flags) const;

  Text getName() const;
  Text getDesc() const;
private:
  
  Text name;
  darray<CommandArg> args;
  darray<CommandFlag> flags;
  Text description;
  std::function<void(const linmap<Text, Text>& args, const linmap<Text, Text>& flags)> effect;
};

Command::Command(const Text& name): name(name) { }

Command& Command::arg(CommandArgType type, const Text& text) {
  args.add(CommandArg{type, text});
  return *this;
}

Command& Command::flag(CommandFlagType type, const Text& text) {
  flags.add(CommandFlag{type, text});
  return *this;
}
Command& Command::describe(const Text& text) {
  description = text;
  return *this;
}
Command& Command::does(
  std::function <void(const linmap<Text, Text>& args, const linmap<Text, Text>& flags)> effect
) {
  this->effect = effect;
  return *this;

}

void Command::run(const linmap<Text, Text>& args, const linmap<Text, Text>& flags) const {
  effect(args, flags);
}

std::size_t Command::getRequiredArgCount() const {
  std::size_t i{};
  std::size_t argCount = args.size();
  for(; i < argCount; ++ i) {
    if(args[i].type == CommandArgType::Optional) break;
  }
  return i;
}

Text Command::getArgNameAt(std::size_t index) const {
  return args[index].text;
}

Text Command::getName() const {
  return name;
}
Text Command::getDesc() const {
  return description;

}

bool Command::isValidArgList(const linmap<Text, Text>& list) const {
  return list.count() >= getRequiredArgCount() && list.count() <= args.size();
}

bool Command::isValidFlag(const CommandFlag& flag) const {
  for(auto& testFlag: flags) {
    //std::cout << "Comparing " << testFlag.text << " == " << flag.text << std::endl;
    if(testFlag.text == flag.text && testFlag.type == flag.type) {
      return true;
    }
  }
  return false;
}

class CommandList {
public:
  void add(Command&& cmd);
  void add(const Command& cmd);
  
  bool execute(const Text& command) const;
private:
  static darray<Text> tokenize(const Text& command);

  darray<Command> commands;
};

void CommandList::add(const Command& cmd) {
  commands.add(cmd);
}

void CommandList::add(Command&& cmd) {
  commands.add(clib::move(cmd));
}

darray<Text> CommandList::tokenize(const Text& command_) {
  auto command = command_.trim();
  darray<Text> tokens{};
  tokens.add("");
  char delim = ' ';
  for(std::size_t i = 0; i < command.size(); ++i) {
    if(command[i] == '"') {
      if(delim == '"') {
        delim = ' ';
      }else {
        if(i >= 1 && command[i - 1] != ' ') {
          throw std::invalid_argument(
            "Couldn't parse command. Unexpected \" at the end of string."
          );
        }
        delim = '"';
      }
      continue;
    };

    if(delim == '"' && command[i] == '\\') {
      if(i < command.size() - 1) {
        tokens[tokens.size() - 1] += command[++i];
        continue;
      }else {
        throw std::invalid_argument(
          "Couldn't parse command. Unexpected \\ at the end of string."
        );
      }
    }

    if(command[i] == delim) {
      tokens.add("");
    }else {
      tokens[tokens.size() - 1] += command[i];
    }
  }

  if(delim == '"') {
    throw std::invalid_argument("Couldn't parse command. Unclosed \"");
  }

  return tokens;
}

bool CommandList::execute(const Text& command) const {
  if(commands.size() == 0) {
    std::cerr << "No commands present" << std::endl;
    return false;
  }
  darray<Text> tokens = tokenize(command);
  //Lex into opts, flags, params
  if(tokens.size() == 0) {
    std::cerr << "Couldn't execute empty command" << std::endl;
    return false;
  }
  
  const Command* cmd = nullptr;
  for(const Command& cmd_candidate: commands) {
    if(cmd_candidate.getName() == tokens[0]) {
      cmd = &cmd_candidate;
      break;
    }
  }

  if(cmd == nullptr) {
    std::cerr << "Unknown command." << std::endl;
    return false;
  }
  
  linmap<Text, Text> args;
  linmap<Text, Text> flags;
  for(std::size_t i = 1; i < tokens.size(); i ++) {
    auto& token = tokens[i];
    bool isLongFlag = token.size() >= 2 && token[0] == '-' && token[1] == '-';
    bool isValidLongFlag = isLongFlag && token.size() >= 3 && token[2] != '-';

    if(token[0] == '-' && i == tokens.size() - 1) {
        std::cerr << 
          "Couldn't parse flag " << token << " - reached end of token list" << std::endl;
        return false;
    }

    if(isLongFlag) {
      if(isValidLongFlag) {
        auto trimmed = Text(token.raw() + 2);
        if(cmd->isValidFlag(CommandFlag{CommandFlagType::Long, trimmed})){
          flags.set(trimmed, tokens[++i]);
          continue;
        }else {
          std::cerr << "Invalid long flag: " << token << std::endl;
          return false;
        }
      }else {
        std::cerr << "Invalid token: " << token << std::endl; 
        return false;
      }
    }

    bool isShortFlag = token.size() >= 1 && token[0] == '-';
    bool isValidShortFlag = isShortFlag && token.size() >= 2 && token[1] != '-';

    if(isShortFlag) {
      if(isValidShortFlag) {
        auto trimmed = Text(token.raw() + 1);
        if(cmd->isValidFlag(CommandFlag{CommandFlagType::Short, trimmed})){
          flags.set(trimmed, "");
          continue;
        }else {
          std::cerr << "Invalid short flag: " << token << std::endl;
          return false;
        }
      }else {
        std::cerr << "Invalid token: " << token << std::endl; 
        return false;
      }
    }

    args.set(cmd->getArgNameAt(args.count()), token);
  }

  if(!cmd->isValidArgList(args)) {
    std::cerr << "Invalid argument list" << std::endl;
    return false;
  }

  cmd->run(args, flags);
  return true;
}

int main(void) {

  CommandList cl;
  
  //hello --long-arg myverylongarg --short-arg s required1 required2 [optional]
  Command cmd("hello");
  
  cmd
    .flag(CommandFlagType::Long, "long-arg")
    .flag(CommandFlagType::Short, "short-arg")
    .arg(CommandArgType::Required, "required1")
    .arg(CommandArgType::Required, "required2")
    .arg(CommandArgType::Optional, "opt")
    .describe("Hello, this is command")
    .does([](const linmap<Text, Text>& args, const linmap<Text, Text>& flags) {
      std::cout << "Hello, I do nothing :<" << std::endl;
    });

  cl.add(cmd);

  if(!cl.execute("hello --long-arg \"my \\\"very\\\" long argument\" -short-arg required1 required2 optional")) {
    return 1;
  }
  
  return 0;
}
