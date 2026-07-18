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

//-----------------utils--------------------//
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

//--------------DECLARATIONS---------------//
  enum class CommandFlagType {
    WithValue, NoValue
  };

  enum class CommandArgType {
    Optional, Required
  };

  struct CommandFlag {
    CommandFlagType type;
    std::string longName;
    std::string shortName;
    std::string description;
  };

  struct CommandArg {
    CommandArgType type;
    std::string text;
    std::string description;
  };
//------------------------------------------//
  using CommandArgs = std::unordered_map<std::string, std::string>;
  using CommandHandler = std::function<void(const CommandArgs& args, const CommandArgs& flags)>;
//------------------------------------------//
  template <typename Derived>
  class SimpleCommand {
  public:
    SimpleCommand() = default;
    SimpleCommand(const std::string& name);

    Derived& arg(CommandArgType type, const std::string& text, const std::string& desc = "");
    Derived& flag(
      CommandFlagType type,
      const std::string& longName,
      const std::string& shortName,
      const std::string& desc = ""
    );
    Derived& giveName(const std::string& name);
    Derived& describe(const std::string& text);

    bool isValidArgList(const CommandArgs& list) const;
    const CommandFlag* getFlag(const std::string& flag, bool isShort = false) const;

    std::string getArgNameAt(std::size_t index) const;
    std::size_t getRequiredArgCount() const;
    std::size_t getArgCount() const;

    std::string getName() const;
    std::string getDesc() const;
    std::string getUsage() const;

    bool lex_into(
      const std::vector<std::string>& tokens,
      CommandArgs& args, CommandArgs& flags
    ) const;

    static std::vector<std::string> tokenize(std::string command);

  private:
    std::string name;
    std::vector<CommandArg> args;
    std::vector<CommandFlag> flags;
    std::string description;
  };
//------------------Command-------------------//
  class Command : public SimpleCommand<Command> {
  public:
    Command() = default;
    Command(const std::string& name);
    Command& does(CommandHandler effect);
    
    void run(const CommandArgs& args, const CommandArgs& flags) const;
  private:
    CommandHandler effect;
  };
//----------------CommandLine----------------//
  class CommandLine : public SimpleCommand<CommandLine> {
  public:
    CommandLine() = delete;
    CommandLine(int argc, const char** argv);

    bool into(CommandArgs& args, CommandArgs& flags) const;

    CommandLine& intoFlags(CommandArgs& args, CommandArgs& flags) const;
    CommandLine& intoArgs(CommandArgs& args, CommandArgs& flags) const;
    
    private:
      int argc;
      const char** argv;
  };

  inline CommandLine parseCommandLine(int argc, const char** argv) {
    return CommandLine(argc, argv);
  }

//----------------CommandList----------------//
  template <std::size_t CommandCount>
  class CommandList {
  public:
    CommandList() = default;
    void add(Command&& cmd);
    
    bool execute(const std::string& command) const;
  private:

    void assertCommandsExhausted() const;

    std::array<Command, CommandCount> commands;
    std::size_t last_command;
  };
//----------------IMPLEMENTATION-------------

//----------------SimpleCommand--------------

  
  template <typename Derived>
  inline SimpleCommand<Derived>::SimpleCommand(const std::string& name): name(name) { };

  template <typename Derived>
  inline Derived& SimpleCommand<Derived>::giveName(const std::string& name) {
    this->name = name;
    return static_cast<Derived&>(*this);
  }

  template <typename Derived>
  inline Derived& SimpleCommand<Derived>::arg(
    CommandArgType type,
    const std::string& text,
    const std::string& description
  ) {
    args.push_back(CommandArg{type, text, description});
    return static_cast<Derived&>(*this);
  }

  template <typename Derived>
  inline Derived& SimpleCommand<Derived>::flag(
    CommandFlagType type,
    const std::string& longName,
    const std::string& shortName,
    const std::string& description
  ) {
    flags.push_back(CommandFlag{type, longName, shortName, description});
    return static_cast<Derived&>(*this);
  }

  template <typename Derived>
  inline Derived& SimpleCommand<Derived>::describe(const std::string& text) {
    description = text;
    return static_cast<Derived&>(*this);
  }

  template <typename Derived>
  inline std::size_t SimpleCommand<Derived>::getRequiredArgCount() const {
    std::size_t i{};
    std::size_t argCount = args.size();
    for(; i < argCount; ++ i) {
      if(args[i].type == CommandArgType::Optional) break;
    }
    return i;
  }

  template <typename Derived>
  inline std::size_t SimpleCommand<Derived>::getArgCount() const {
    return args.size();
  }

  template <typename Derived>
  inline std::string SimpleCommand<Derived>::getArgNameAt(std::size_t index) const {
    if(index >= args.size()) {
      throw std::out_of_range("Trying to get arg name at index that exceeds arg count");
    }
    return args[index].text;
  }

  template <typename Derived>
  inline std::string SimpleCommand<Derived>::getName() const {
    return name;
  }

  template <typename Derived>
  inline std::string SimpleCommand<Derived>::getDesc() const {
    return description;
  }

  template <typename Derived>
  inline std::string SimpleCommand<Derived>::getUsage() const {
    std::string usage = (name == "" ? "(Unnamed)" : name) + " - " + description + '\n';
    if(flags.size() > 0) {
      usage += "  OPTIONAL FLAGS\n";
      for(const auto& flag: flags) {
        usage += "    --" + flag.longName + " -" + flag.shortName
          + (flag.type == CommandFlagType::WithValue ? " <value>" : "")
          + " - " 
          + (flag.description == "" ? "No description provided" : flag.description)
          + '\n';
      }
    }
    if(args.size() > 0) {
      usage += "  POSITIONAL ARGS\n";
      for(const auto& arg: args) {
        usage += "    ";
        if(arg.type == CommandArgType::Optional) {
          usage += "[" + arg.text + "]";
        }else {
          usage += "<" + arg.text + ">";
        }
        usage += " - " + (arg.description == "" ? "No description provided" : arg.description) + '\n';
      }
    }
    return usage;
  }

  template <typename Derived>
  inline bool SimpleCommand<Derived>::isValidArgList(const CommandArgs& list) const {
    return list.size() >= getRequiredArgCount() && list.size() <= args.size();
  }

  template <typename Derived>
  inline const CommandFlag* SimpleCommand<Derived>::getFlag(const std::string& name, bool isShort) const {
    for(auto& testFlag: flags) {
      if(
        (isShort && testFlag.shortName == name) ||
        (!isShort && testFlag.longName == name)
      ) {
        return &testFlag;
      }
    }
    return nullptr;
  }

  template <typename Derived>
  inline std::vector<std::string> SimpleCommand<Derived>::tokenize(std::string command) {
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

  template <typename Derived>
  inline bool SimpleCommand<Derived>::lex_into(
      const std::vector<std::string>& tokens,
      CommandArgs& args, CommandArgs& flags
  ) const {

    for(std::size_t i = 0; i < tokens.size(); i ++) {
      auto& token = tokens[i];
      if(token.length() == 0) {
        continue;
      }

      if(token[0] == '-' && i == tokens.size() - 1) {
          std::cerr << 
            "Couldn't parse flag " << token << " - reached end of token list" << std::endl;
          return false;
      }
      
      struct FlagResult {
        bool status;
        bool shouldContinue;
      };
      auto handleFlag = [this, &tokens, &flags, &token, &i]
        (const std::string& trimmedName, bool isFlag, bool isValid, bool isShort) {
        if(isFlag) {
          if(isValid) {
            auto flagPtr = getFlag(trimmedName, isShort);
            if(flagPtr != nullptr){
              flags.emplace(flagPtr->longName, 
                flagPtr->type == CommandFlagType::WithValue 
                  ? tokens[++i]
                  : "SET"
              );
              return FlagResult{true, true};
            }else {
              std::cerr << "Unknown flag: " << token << std::endl;
              return FlagResult{false};
            }
          }else {
            std::cerr << "Invalid token: " << token << std::endl; 
            return FlagResult{false};
          }
        }
        return FlagResult{true,false};
      };

      bool isLongFlag = token.length() >= 2 && token[0] == '-' && token[1] == '-';
      bool isValidLongFlag = isLongFlag && token.length() >= 3 && token[2] != '-';

      FlagResult longResult = handleFlag(token.substr(2), isLongFlag, isValidLongFlag, false);

      if(!longResult.status) return false;
      if(longResult.shouldContinue) continue;

      bool isShortFlag = token.length() >= 1 && token[0] == '-';
      bool isValidShortFlag = isShortFlag && token.length() >= 2 && token[1] != '-';

      FlagResult shortResult = handleFlag(token.substr(1), isShortFlag, isValidShortFlag, true);

      if(!shortResult.status) return false;
      if(shortResult.shouldContinue) continue;

      if(args.size() >= getArgCount()) {
        std::cerr << "Number of placement arguments exceeds expected maximum" << std::endl;
        return false;
      }
      args.emplace(getArgNameAt(args.size()), token);
    }


    if(!isValidArgList(args)) {
      std::cerr << "Invalid argument list" << std::endl;
      return false;
    }

    return true;
  }

//-----------------Command---------------------
  inline Command::Command(const std::string& name): SimpleCommand(name) { }

  inline Command& Command::does(CommandHandler effect) {
    this->effect = effect;
    return *this;

  }

  inline void Command::run(const CommandArgs& args, const CommandArgs& flags) const {
    effect(args, flags);
  }

//----------------CommandLine-------------------

  inline CommandLine::CommandLine(int argc, const char** argv)
    : argc(argc), argv(argv) { }

  inline bool CommandLine::into(CommandArgs& args, CommandArgs& flags) const {
    auto arg_vector = std::vector<std::string>(argv + 1, argv + argc);
    return lex_into(
      arg_vector,
      args, flags
    );
  }

//----------------CommandList-------------------

  template <std::size_t CommandCount>
  inline void CommandList<CommandCount>::assertCommandsExhausted() const {
    assert(("Non-exhaustive list of commands", CommandCount == last_command));
  }

  template <std::size_t CommandCount>
  inline void CommandList<CommandCount>::add(Command&& cmd) {
    assert(("Max command count exhausted!", CommandCount != last_command));
    commands[last_command++] = std::move(cmd);
  }


  template <std::size_t CommandCount>
  inline bool CommandList<CommandCount>::execute(const std::string& command) const {
    assertCommandsExhausted();
    std::vector<std::string> tokens = SimpleCommand<Command>::tokenize(command);
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

    std::vector<std::string> rest(tokens.begin() + 1, tokens.end());
    cmd->lex_into(rest, args, flags);


    cmd->run(args, flags);
    return true;
  }
}
