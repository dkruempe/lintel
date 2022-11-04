#!/bin/sh

safe_cancel() {
  echo "Process: Cancelling" >> ~/test
  exit
}

trap safe_cancel 2

while [ 1 ]
do
  echo 'Process: Hello World' >> ~/test
  sleep 1
done
