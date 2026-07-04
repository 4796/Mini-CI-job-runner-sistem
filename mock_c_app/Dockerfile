# Stage 1: Build stage using gcc in Alpine
FROM alpine:3.18 AS builder
RUN apk add --no-cache gcc musl-dev
WORKDIR /build
COPY main.c .
RUN gcc -O2 -static -o server main.c

# Stage 2: Runtime stage using minimal scratch image
FROM scratch
COPY --from=builder /build/server /server
EXPOSE 8080
ENTRYPOINT ["/server"]
