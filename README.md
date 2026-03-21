# my-little-site

My homepage.

## Run locally

```bash
cc -Wall -Wextra -std=c11 meow.c -o meow

./meow <PORT: defaults to 8000>
```

## Run with Docker

```bash
docker build -t my-little-site .

docker volume create my-little-site-data
docker run --rm -p 8000:8000 -v my-little-site-data:/app/data my-little-site
```
