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

using ParsedArgs = linmap<Text, Text>;

class Command {
public:
  Command() = default;
  Command(const Text& name);
  Command& arg(CommandArgType type, const Text& text);
  Command& flag(CommandFlagType type, const Text& text);
  Command& describe(const Text& text);
  Command& does(std::function<void(const ParsedArgs& args)> effect);
  
  void run(const darray<Text>& tokens) const;
private:
  Text name;
  darray<CommandArg> args;
  darray<CommandFlag> flags;
  Text description;
  std::function<void(const ParsedArgs& args)> effect;
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
Command& Command::does(std::function<void(const ParsedArgs& args)> effect) {
  this->effect = effect;
  return *this;
}

void Command::run(const darray<Text>& tokens) const {

  for(auto& t: tokens) {
    if(t[0] == '-' && t[1] == '-') {
      
    }else if(t[0] == '-') {
      
    }
  }

  //-mybool TRUE
  //--promenliva STOINOST
  //mycommand -toggle_bool --
  TODO("run");
}

class CommandList {
  darray<Command> commands;
public:
  void add(Command&& cmd);
  void add(const Command& cmd);
  
  bool execute(const Text& command) const;
};

void CommandList::add(const Command& cmd) {
  commands.add(cmd);
}

void CommandList::add(Command&& cmd) {
  commands.add(clib::move(cmd));
}

bool CommandList::execute(const Text& command_) const {
  auto command = command_.trim();
  darray<Text> words{};
  words.add("");
  char delim = ' ';
  for(std::size_t i = 0; i < command.size(); ++i) {
    if(command[i] == '"') {
      if(delim == '"') {
        delim = ' ';
      }else {
        if(i >= 1 && command[i - 1] != ' ') {
          std::cout << "Couldn't parse command. Unexpected \"" << std::endl;
          return false;
        }
        delim = '"';
      }
      continue;
    };

    if(delim == '"' && command[i] == '\\') {
      if(i < command.size() - 1) {
        words[words.size() - 1] += command[++i];
        continue;
      }else {
        std::cout << "Couldn't parse command. Unexpected \\ at the end of string." << std::endl;
        return false;
      }
    }

    if(command[i] == delim) {
      words.add("");
    }else {
      words[words.size() - 1] += command[i];
    }
  }
  if(delim == '"') {
    std::cout << "Couldn't parse command. Unclosed \"" << std::endl;
    return false;
  }

  for(std::size_t i = 0; i < words.size(); ++i) {
    std::cout << words[i] << std::endl;
  }
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
    .does([](const ParsedArgs& args) {
      std::cout << "Hello, I do nothing :<" << std::endl;
    });

  cl.add(cmd);

  if(!cl.execute("hello --long-arg \"my \\\"very\\\" long argument\" -short-arg \"Hello\" required1 required2 [optional]")) {
    return 1;
  }
  
  return 0;
}
