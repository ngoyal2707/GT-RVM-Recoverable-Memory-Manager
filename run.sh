make clean
make
rm ./a.out
g++ -std=c++11 basic.c -L. -lrvm -o basic
g++ -std=c++11 multi.c -L. -lrvm -o multi
g++ -std=c++11 abort.c -L. -lrvm -o abort
g++ -std=c++11 multi-abort.c -L. -lrvm -o multi-abort
g++ -std=c++11 truncate.c -L. -lrvm -o truncate
g++ -std=c++11 overlap.c -L. -lrvm -o overlap
g++ -std=c++11 failure.c -L. -lrvm -o failure