FROM alpine as builder
WORKDIR /src/buildfolder
RUN apk add g++ make openssl-dev zlib-dev brotli-dev
COPY httplib.h /src/buildfolder
COPY example/server.cc /src/buildfolder
COPY example/client.cc /src/buildfolder
COPY example/Makefile /src/buildfolder
RUN make server
RUN make client

FROM alpine
RUN apk --no-cache add brotli libstdc++
COPY --from=builder /src/buildfolder/server /bin/server
COPY --from=builder /src/buildfolder/client /bin/client

CMD ["/bin/server"]

EXPOSE 8080