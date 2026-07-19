g++ main.cpp -fsanitize=address -o main.out && ./main.out --long-arg "my very \"long\" arg" -sa --short-arg required1 required2 optional --help
