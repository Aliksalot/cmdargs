#include"cmdargs.h"

using namespace cmdargs;

bool single(int argc, const char** argv){

  CommandArgs args, flags;
  
  auto cmd = cmdargs::parseCommandLine(argc, argv)
    .flag(CommandFlagType::WithValue, "long-arg", "la", "This is description")
    .flag(CommandFlagType::NoValue, "short-arg", "sa")
    .arg(CommandArgType::Required, "required1")
    .arg(CommandArgType::Required, "required2")
    .arg(CommandArgType::Optional, "opt")
    .giveName("hello")
    .describe("Hello, this is command");
  std::cout << cmd.getUsage() << std::endl;
  bool result = cmd.into(args, flags);

  for(const auto& [k, v]: args) {
    std::cout << k << ": " << v << "\n";
  }
  for(const auto& [k, v]: flags) {
    std::cout << k << ": " << v << "\n";
  }

  if(!result) {
    return 1;
  }



  return 0;
}

bool cl_test() {
  cmdargs::CommandList<1> cl;
  
  //hello --long-arg myverylongarg --short-arg s required1 required2 [optional]
  cmdargs::Command cmd("hello");
  
  cmd
    .flag(CommandFlagType::WithValue, "long-arg", "la", "This is description")
    .flag(CommandFlagType::NoValue, "short-arg", "sa")
    .arg(CommandArgType::Required, "required1")
    .arg(CommandArgType::Required, "required2")
    .arg(CommandArgType::Optional, "opt")
    .describe("Hello, this is command")
    .does([](
      const CommandArgs& args,
      const CommandArgs& flags
    ) {
      for(const auto& [k, v]: args) {
        std::cout << k << ": " << v << "\n";
      }
      for(const auto& [k, v]: flags) {
        std::cout << k << ": " << v << "\n";
      }
      std::cout << "Hello, I do nothing :<" << std::endl;
    });

  cl.add(std::move(cmd));

  cl.includeHelp();

   if(!cl.execute("hello --long-arg \"my \\\"very\\\" long argument\" -sa required1 required2 optional")) {
     return 1;
   }

  if(!cl.execute("help command ddd")) {
    return 1;
  }

  return 0;
}

int main(int argc, const char** argv) {
  return cl_test();
  //return single(argc, argv);
}
