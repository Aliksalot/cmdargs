#pragma once

#include<vector>
#include<array>
#include<string>
#include<functional>
#include<iostream>
#include<utility>
#include<map>
#include<algorithm>
#include<cassert>

namespace cmdargs {

inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}
inline void trim(std::string& s) {
  rtrim(s);
  ltrim(s);
}

//------------------------------------------//
  enum class CommandFlagType {
    Short, Long
  };

  enum class CommandArgType {
    Optional, Required
  };

  struct CommandFlag {
    CommandFlagType type;
    std::string text;
  };

  struct CommandArg {
    CommandArgType type;
    std::string text;
  };
//------------------------------------------//
  using CommandArgs = std::unordered_map<std::string, std::string>;
  using CommandHandler = std::function<void(const CommandArgs& args, const CommandArgs& flags)>;
//------------------------------------------//
  class Command {
  public:
    Command() = default;
    Command(const std::string& name);
    Command& arg(CommandArgType type, const std::string& text);
    Command& flag(CommandFlagType type, const std::string& text);
    Command& describe(const std::string& text);
    Command& does(CommandHandler effect
    );

    bool isValidArgList(const CommandArgs& list) const;
    bool isValidFlag(const CommandFlag& flag) const;

    std::string getArgNameAt(std::size_t index) const;
    std::size_t getRequiredArgCount() const;
    
    void run(const CommandArgs& args, const CommandArgs& flags) const;

    std::string getName() const;
    std::string getDesc() const;
  private:
    
    std::string name;
    std::vector<CommandArg> args;
    std::vector<CommandFlag> flags;
    std::string description;
    CommandHandler effect;
  };

  Command::Command(const std::string& name): name(name) { }

  Command& Command::arg(CommandArgType type, const std::string& text) {
    args.push_back(CommandArg{type, text});
    return *this;
  }

  Command& Command::flag(CommandFlagType type, const std::string& text) {
    flags.push_back(CommandFlag{type, text});
    return *this;
  }
  Command& Command::describe(const std::string& text) {
    description = text;
    return *this;
  }
  Command& Command::does(
    std::function <void(const CommandArgs& args, const CommandArgs& flags)> effect
  ) {
    this->effect = effect;
    return *this;

  }

  void Command::run(const CommandArgs& args, const CommandArgs& flags) const {
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

  std::string Command::getArgNameAt(std::size_t index) const {
    return args[index].text;
  }

  std::string Command::getName() const {
    return name;
  }
  std::string Command::getDesc() const {
    return description;

  }

  bool Command::isValidArgList(const CommandArgs& list) const {
    return list.size() >= getRequiredArgCount() && list.size() <= args.size();
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

//------------------------------------------//
  template <std::size_t CommandCount>
  class CommandList {
  public:
    CommandList();
    void add(Command&& cmd);
    
    bool execute(const std::string& command) const;
  private:
    static std::vector<std::string> tokenize(std::string command);

    void assertCommandsExhausted() const;

    std::array<Command, CommandCount> commands;
    std::size_t last_command;
  };

  template <std::size_t CommandCount>
  CommandList<CommandCount>::CommandList(): last_command() { };

  template <std::size_t CommandCount>
  void CommandList<CommandCount>::assertCommandsExhausted() const {
    assert(("Non-exhaustive list of commands", CommandCount == last_command));
  }

  template <std::size_t CommandCount>
  void CommandList<CommandCount>::add(Command&& cmd) {
    assert(("Max command count exhausted!", CommandCount != last_command));
    commands[last_command++] = std::move(cmd);
  }

  template <std::size_t CommandCount>
  std::vector<std::string> CommandList<CommandCount>::tokenize(std::string command) {
    trim(command);
    std::vector<std::string> tokens{};
    tokens.push_back("");
    char delim = ' ';
    for(std::size_t i = 0; i < command.length(); ++i) {
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
        if(i < command.length() - 1) {
          tokens[tokens.size() - 1] += command[++i];
          continue;
        }else {
          throw std::invalid_argument(
            "Couldn't parse command. Unexpected \\ at the end of string."
          );
        }
      }

      if(command[i] == delim) {
        tokens.push_back("");
      }else {
        tokens.back() += command[i];
      }
    }

    if(delim == '"') {
      throw std::invalid_argument("Couldn't parse command. Unclosed \"");
    }

    return tokens;
  }

  template <std::size_t CommandCount>
  bool CommandList<CommandCount>::execute(const std::string& command) const {
    assertCommandsExhausted();
    std::vector<std::string> tokens = tokenize(command);
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
    
    CommandArgs args;
    CommandArgs flags;
    for(std::size_t i = 1; i < tokens.size(); i ++) {
      auto& token = tokens[i];
      bool isLongFlag = token.length() >= 2 && token[0] == '-' && token[1] == '-';
      bool isValidLongFlag = isLongFlag && token.length() >= 3 && token[2] != '-';

      if(token[0] == '-' && i == tokens.size() - 1) {
          std::cerr << 
            "Couldn't parse flag " << token << " - reached end of token list" << std::endl;
          return false;
      }

      if(isLongFlag) {
        if(isValidLongFlag) {
          auto trimmed = token.substr(2);
          if(cmd->isValidFlag(CommandFlag{CommandFlagType::Long, trimmed})){
            flags.emplace(trimmed, tokens[++i]);
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

      bool isShortFlag = token.length() >= 1 && token[0] == '-';
      bool isValidShortFlag = isShortFlag && token.length() >= 2 && token[1] != '-';

      if(isShortFlag) {
        if(isValidShortFlag) {
          auto trimmed = token.substr(1);
          if(cmd->isValidFlag(CommandFlag{CommandFlagType::Short, trimmed})){
            flags.emplace(trimmed, std::string(""));
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

      args.emplace(cmd->getArgNameAt(args.size()), token);
    }

    if(!cmd->isValidArgList(args)) {
      std::cerr << "Invalid argument list" << std::endl;
      return false;
    }

    cmd->run(args, flags);
    return true;
  }
}
