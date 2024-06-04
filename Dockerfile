FROM ubuntu:latest

RUN apt-get update
RUN apt-get -y upgrade
RUN apt-get install -y gcc libncurses5-dev

WORKDIR /app
COPY game.c /app/
RUN gcc -Wall -o scheisse game.c -lncurses -lpthread

ENTRYPOINT ["./scheisse"]

#              how to build 
#              build(image) -> run
#                   ^  
#        /                  \
#       
# createcontainer           rmcontainer
#       
#       \                   /
#               run app