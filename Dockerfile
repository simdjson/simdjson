# docker build -t simdjson . && docker run --privileged -t simdjson
FROM gcc:8.3
COPY . /usr/src/
WORKDIR /usr/src/
RUN make clean
RUN make amalgamate
RUN make
RUN make test
RUN make parsingcompetition
CMD ["bash", "scripts/selectparser.sh"]
