FROM alpine:3.21 AS builder

WORKDIR /src
RUN apk add --no-cache build-base
COPY meow.c quotes.inc ./
RUN cc -O2 -pipe -std=c11 -Wall -Wextra meow.c -o meow

FROM alpine:3.21

WORKDIR /app
COPY --from=builder /src/meow ./meow
COPY static ./static

ENV MEOW_COUNTER_FILE=/app/data/visitor-count.txt
RUN mkdir -p /app/data
VOLUME ["/app/data"]

EXPOSE 8000
ENTRYPOINT ["./meow", "8000"]
