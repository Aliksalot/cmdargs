#include<iostream>
#include"../cmdargs.h"

int main(int argc, const char** argv) {
  ParsedArgs flags, args;
  cmdargs::fromArgv(argc, argv)
    .flag(cmdargs::CommandFlagType::NoValue, "my-flag", "mf", "This is my first flag");
    .flag(cmdargs::CommandFlagType::WithValue, "other-flag", "of", "This is my other flag. This one has a required value!");
    .param(cmdargs::CommandArgType::Required, "req-param1", "This param is required.")
    .param(cmdargs::CommandArgType::Required, "req-param2", "This param is also quite required.")
    .includeHelp() //Includes --help -h as a flag that provides usage for the command
    .into(args, flags);
  //TODO show usage
  return 0;
}
