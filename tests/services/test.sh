#!/bin/zsh

echo "My pid is: $$"
finish=0
trap 'finish=1' SIGINT
while ((finish != 1)); do
  sleep 1
done
exit 0