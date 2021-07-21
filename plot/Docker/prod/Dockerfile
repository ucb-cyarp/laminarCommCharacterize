FROM ubuntu:20.04

ARG DEBIAN_FRONTEND="noninteractive"
ENV TZ="America/Los_Angeles"

#Install Graphviz & Python
RUN apt-get update && apt-get install -y sudo python3 python3-pip

#Copy Requirements
RUN mkdir /srv/plot
COPY requirements.txt /srv/plot/.

#Install Dependencies
WORKDIR /srv/plot
RUN pip3 install -r requirements.txt

#Copy Program
RUN mkdir /srv/plot/src
COPY src/* /srv/plot/src/
