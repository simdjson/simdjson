FROM gcc:10
RUN echo "deb http://deb.debian.org/debian buster-backports main" >> /etc/apt/sources.list
RUN apt-get update -qq
RUN apt-get -t buster-backports install -y cmake

