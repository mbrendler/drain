FROM debian:stable-slim

RUN apt-get update \
  && apt-get upgrade -y \
  && apt-get install -y --no-install-recommends \
       gcc libc6-dev libncurses-dev make cmake cppcheck clang-tidy iwyu \
  && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /code
WORKDIR /code

COPY . /code

ARG VERBOSE

RUN ./bootstrap.sh
