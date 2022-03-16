(in-package cl-user)
(ql:quickload :spinneret)

(defparameter *my-contacts* '(("github.meow" . "cute") ("wow" . "put")))
(defparameter *content* '(
  ("# About me:" . "Hi-hi there!~ My name is Kirill and I'm another one novice programmer.")
  ("# Intrested in:" . #("Poi poi" "Kitty Cat"))))

(spinneret:with-html-string
  (:body
    (:div :class :header
      (:span :class :center
        (:h1 "Lappely <3~")
        (:p (:sub 
          (loop for item in *my-contacts* do 
            (:a :href (car item) (cdr item)))))) 
      :br)
    (:div :class :main
        (loop for item in *content* do
              (:h4 (car item))
              (if (and (not (simple-string-p (cdr item))) (vectorp (cdr item)))
                  (:ul 
                  (loop for paragraph across (cdr item) do
                        (:li paragraph)))
                  (:p (cdr item)))))))
