#!/bin/sh
make
make -C cart
# I use WSL; for someone on pure Linux, just remove the .exe
tic80.exe --fs cart --cmd "load ticboy.tic & run & exit"
