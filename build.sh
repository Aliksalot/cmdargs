g++ main.cpp -fsanitize=address -o main.out && ./main.out --long-arg "my very \"long\" arg" -short-arg required1 required2 optional notopt
