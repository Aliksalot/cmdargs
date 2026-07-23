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
#include<optional>

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

  /// Command flag construction is simple. You always provide a long name
  /// used via --long-name and a short name, used via -ln (short for long-name).
  /// If type is set to CommandFlagType::WithValue, then usage is via --my-param <some value>,
  /// otherwise (in the CommandFlagType::NoValue case) usage is via --my-param, which sets a 
  /// boolean flag.
  /// Description can be skipped if don't plan on providing helpful usage to the users
  struct CommandFlag {
    CommandFlagType type;
    std::string longName;
    std::string shortName;
    std::string description;
  };

  /// Args can be optional or not optional, depending on CommandArgType. By arg here is meant
  /// a positional argument. Optional args always have to appear at the end of lists.
  struct CommandArg {
    CommandArgType type;
    std::string text;
    std::string description;
  };

  /// Provides an abstraction over a std::unordered_map for ease of use.
  /// This is the output format for the command parsing that this library uses
  class CommandArgs {
    public:
      CommandArgs& set(const std::string& k, const std::string& v);
      CommandArgs& set(const std::string& k);
      CommandArgs& remove(const std::string& k);

      bool has(const std::string& k) const;

      /// Throws if no such key is present, can be combined with CommandArgs::has for
      /// safe usage. If you want a nothrow "get" function use CommandArgs::valueOpt
      /// or CommandArgs::tryValue with a fallback function
      std::string value(const std::string& k) const;
      
      std::optional<std::string> valueOpt(const std::string& k) const;

      /// Returns fallback's value
      std::string value(const std::string& k, std::function<std::string()> fallback) const;
      
      /// Returns the unordered_map for iteration purposes and whatnot
      const std::unordered_map<std::string, std::string>& data() const;

      std::size_t size() const;
    private:
      std::unordered_map<std::string, std::string> m_data;
  };
//------------------------------------------//
  using CommandHandler = std::function<void(const CommandArgs& args, const CommandArgs& flags)>;
//------------------------------------------//
  

  /// SimpleCommand provides the general interface for building command formats. You can check
  /// examples, but the general idea is that you can chain args and flags, as well as other options.
  /// Quick example:
  /// cmdargs::fromArgv(argc, argv)
  /// .flag(cmdargs::CommandFlagType::NoValue, "my-flag", "mf", "This is my first flag");
  /// .flag(cmdargs::CommandFlagType::WithValue, "other-flag", "of", "This is my other flag. This one has a required value!");
  /// .param(cmdargs::CommandArgType::Required, "req-param1", "This param is required.")
  /// .param(cmdargs::CommandArgType::Required, "req-param2", "This param is also quite required.")
  /// .includeHelp() //Includes --help -h as a flag that provides usage for the command
  /// .into(args, flags);
  /// This class doesn't provide a way to use the parsed arguments. For that you must use some
  /// derivation, check Command or CommandLine
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

    //Mostly implementation details
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
  
  /// This class's main function is to populate a CommandList. Check loop-multiple-commands.cpp
  /// example to understand the main ideology.
  class Command : public SimpleCommand<Command> {
  public:
    Command() = default;
    explicit Command(const std::string& name);

    /// Provides a callback function to be executed on .run
    Command& does(CommandHandler effect);
    
    /// Executes the provided callback function
    void run(const CommandArgs& args, const CommandArgs& flags) const;
  private:
    CommandHandler effect;
  };
//----------------CommandLine----------------//
  
  /// This provides the most general command line parsing solution. Via ::fromArgv(...)
  /// you can build your command by passing in standart argc and argv, and later parse them into
  /// CommandArgs via the .into method. Include help short-circuits the whole application via
  /// std::exit whenever --help or -h is provided, and executes a auto-generated usage screen
  class CommandLine : public SimpleCommand<CommandLine> {
  public:
    CommandLine() = delete;
    CommandLine(int argc, const char** argv);

    /// Parses argc and argv, acorrding to the custom scheme, into args and flags
    bool into(CommandArgs& args, CommandArgs& flags) const;

    /// Includes --help and -h
    CommandLine& includeHelp();
    
    private:
      int argc;
      const char** argv;

      bool helpIncluded = false;
  };

  /// Makes your code more readable
  inline CommandLine fromArgv(int argc, const char** argv) {
    return CommandLine(argc, argv);
  }

//----------------CommandList----------------//
  
  /// Helps you build command line interfaces, via keeping a continous loop, like the pgsql
  /// command line tool. Check loop-multiple-commands.cpp for how to use it.
  /// After providing a command count you MUST exhaust it via .add before you can start doing
  /// .execute's
  template <std::size_t CommandCount>
  class CommandList {
  public:
    CommandList() = default;

    /// Moves a command into the list of commands.
    Command& add(Command&& cmd);
    Command& add(const std::string& name);
    
    /// Parses and lexes the input command. If an error occurs it will print it into std::cerr and
    /// return false. Otherwise returns true.
    bool execute(const std::string& command) const;

    /// Includes custom help command with the signature
    ///   help [command-name] - Provides the usage for specified command, or for all of them
    CommandList& includeHelp();
  private:

    void assertCommandsExhausted() const;

    std::array<Command, CommandCount + 1> commands;
    std::size_t last_command;

    bool helpIncluded = false;
  };
//----------------IMPLEMENTATION-------------

//----------------CommandArgs---------------

  CommandArgs& CommandArgs::set(const std::string& k, const std::string& v) {
    m_data.emplace(k, v);
    return *this;
  }

  CommandArgs& CommandArgs::set(const std::string& k) {
    m_data.emplace(k, "SET");
    return *this;
  }

  CommandArgs& CommandArgs::remove(const std::string& k) {
    auto it = m_data.find(k);
    if(it != m_data.end()) {
      m_data.erase(it);
    }
    return *this;
  }

  bool CommandArgs::has(const std::string& k) const {
    return m_data.count(k);
  }

  std::string CommandArgs::value(const std::string& k) const {
    auto maybeValue = valueOpt(k);
    if(maybeValue.has_value()) {
      return maybeValue.value();
    }

    throw std::logic_error(std::string("No such key present in this CommandArgs"));
  }

  std::optional<std::string> CommandArgs::valueOpt(const std::string& k) const {
    auto it = m_data.find(k);
    if(it != m_data.end()) {
      return it->second;
    }

    return {};
  }

  std::string CommandArgs::value(
    const std::string& k, std::function<std::string()> fallback
  ) const {
    auto maybeValue = valueOpt(k);
    if(maybeValue.has_value()) {
      return maybeValue.value();
    }
    
    return fallback();
  }

  const std::unordered_map<std::string, std::string>& CommandArgs::data() const {
    return m_data;
  }

  std::size_t CommandArgs::size() const {
    return m_data.size();
  }

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
    for(const auto& testFlag: flags) {
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
              if(
                flagPtr->type == CommandFlagType::WithValue 
                && token[0] == '-' 
                && i == tokens.size() - 1
              ) {
                  std::cerr << 
                    "Couldn't parse flag " << token << " - reached end of token list" << std::endl;
                  return FlagResult{false};
              }
              if(flagPtr->type == CommandFlagType::WithValue) {
                flags.set(flagPtr->longName, tokens[++i]);
              }else {
                flags.set(flagPtr->longName);
              }
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
      args.set(getArgNameAt(args.size()), token);
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
    if(!lex_into(arg_vector, args, flags)) {
      std::cout << "Couldn't execute command. Correct usage: \n";
      std::cout << getUsage();
      return false;
    }
    if(helpIncluded && flags.has("help") != 0) {
      std::cout << getUsage();
      std::exit(0);
    }

    return true;
  }

  inline CommandLine& CommandLine::includeHelp() {
    helpIncluded = true;
    flag(CommandFlagType::NoValue, "help", "h", "Provides usage");
    return *this;
  }

//----------------CommandList-------------------

  template <std::size_t CommandCount>
  inline void CommandList<CommandCount>::assertCommandsExhausted() const {
    if(CommandCount != last_command) {
      throw std::logic_error("Non-exhaustive list of commands");
    }
  }

  template <std::size_t CommandCount>
  inline Command& CommandList<CommandCount>::add(Command&& cmd) {
    if(CommandCount == last_command) {
      throw std::logic_error("Max command count exhausted!");
    }
    if(cmd.getName() == "help" && helpIncluded) {
      throw std::logic_error(
        "Can't redefine help command! If you wish to do so, don't call the .includeHelp function on the command list"
      );
    }
    Command& lastCommand = commands[last_command];
    lastCommand = std::move(cmd);
    return lastCommand;
  }

  template <std::size_t CommandCount>
  inline Command& CommandList<CommandCount>::add(const std::string& name) {
    return add(std::move(Command(name)));
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
      std::cerr << "Unknown command. .";
      if(helpIncluded) {
        std::cout << "Check for list of commands via \"help\".";
      }
      std::cout << std::endl;
      return false;
    }
    
    CommandArgs args;
    CommandArgs flags;

    std::vector<std::string> rest(tokens.begin() + 1, tokens.end());

    if(!cmd->lex_into(rest, args, flags)) {
      std::cout << "Couldn't execute command. Correct usage: \n";
      std::cout << cmd->getUsage();
      return false;
    }

    cmd->run(args, flags);
    return true;
  }

  template <std::size_t CommandCount>
  inline CommandList<CommandCount>& CommandList<CommandCount>::includeHelp() {
    for(const auto& cmd: commands) {
      if(cmd.getName() == "help") {
        std::cout << "Couldn't include the default help command, since a custom \"help\" has already been defined. " << std::endl;
        return *this;
      }
    }
    helpIncluded = true;

    commands[CommandCount] = 
      Command("help")
      .describe("Displays the list of commands")
      .arg(CommandArgType::Optional, "command-name", "If provided will print the usage for the specified command. ")
      .does([this](const CommandArgs& args, const CommandArgs&){
        auto opt = args.valueOpt("command-name");
        if(opt.has_value()) {
          auto& cmd_name = opt.value();
          for(const auto& cmd: commands) {
            if(cmd.getName() == cmd_name) {
              std::cout << cmd.getUsage();
              return;
            }
          }
          std::cerr << "Unknown command: " << cmd_name << std::endl;
          return;
        }
        for(const auto& cmd: commands) {
          std::cout << cmd.getUsage();
        }
      });

    return *this;
  }
}
