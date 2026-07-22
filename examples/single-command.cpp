#include<iostream>
#include<string>
#include"../cmdargs.h"

int main(int argc, const char** argv) {
  cmdargs::CommandArgs flags, args;

  bool result = cmdargs::fromArgv(argc, argv)
    .giveName("my-command")
    .flag(cmdargs::CommandFlagType::NoValue, "my-flag", "mf", "This is my first flag")
    .flag(cmdargs::CommandFlagType::WithValue, "other-flag", "of", "This is my other flag. This one has a required value!")
    .arg(cmdargs::CommandArgType::Required, "req-param1", "This param is required.")
    .arg(cmdargs::CommandArgType::Required, "req-param2", "This param is also quite required.")
    .arg(cmdargs::CommandArgType::Optional, "opt-param", "This param is very much NOT required.")
    .includeHelp() //Includes --help -h as a flag that provides usage for the command
    .into(args, flags);
  
  if(!result) return 1;

  if(flags.has("my-flag")) {
    std::cout << "my-flag: " << std::boolalpha << flags.has("my-flag") << std::endl;
  }
  std::cout << "other-flag: " << flags.valueOpt("other-flag").value_or("no value ;(") << std::endl;
  
  std::string optParam = args.value("opt-param", []() {
    return "You didn't set it";
  });

  std::cout << "opt-param: " << optParam << std::endl;

  std::cout << "req-param1: " << args.value("req-param1") << std::endl;
  std::cout << "req-param2: " << args.value("req-param2") << std::endl;

  return 0;
}
