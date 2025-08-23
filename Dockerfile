FROM fukamachi/roswell:latest

RUN ros run --eval "(progn (ql:quickload '(:cl-css :spinneret :str :ningle :clack :lack)) (exit))"
COPY . .
EXPOSE 8000

ENTRYPOINT ["./make.sh"]
