FROM ubuntu:20.04 as builder

WORKDIR /src

RUN apt update && apt install -y build-essential && apt install -y git libuv1-dev

COPY . /src
RUN git clone https://github.com/BLAKE3-team/BLAKE3.git && \
  make all

FROM ubuntu:20.04

RUN apt update && apt install -y libuv1 && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/bin/cpu-miner /cpu-miner

USER nobody

ENTRYPOINT ["/cpu-miner"]
