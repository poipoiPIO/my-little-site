(in-package cl-user)
(ql:quickload '(:clack :cl-css :spinneret :ningle))

(defvar *app*  (make-instance 'ningle:<app>))
(defparameter *port* 8000)
(defparameter *my-contacts* '(("github.com/poipoiPIO" . "Github | " ) ("@lilyape" . "Telegram")))
(defparameter *content* '(
  ("About me:" . "Hi-hi there!~ My name is Kirill and I'm another one novice programmer.")
  ("Intrested in:" . #("Poi poi" "Kitty Cat"))))

(defparameter *html-color* "#2F2D2E")
(defparameter *main-section-color* "#2F2D2E")
(defparameter *text-color* "#2F2D2E")

(defparameter *paragraph-style* 
  (cl-css:inline-css '(:margin 5px 0px 10px 13px)))

(defparameter *html-style* 
  (cl-css:inline-css '(:background-color *html-color*)))

(defparameter *logo-style* 
  (cl-css:inline-css '(:text-align center)))

(defparameter *sharp-style* 
  (cl-css:inline-css '(:color blue)))

(defparameter *body-style* 
  (cl-css:inline-css '(:color *text-color* :font-family Helvetica 
                       :font-size 18px :margin-top 3% :margin-bottom 3% 
                       :margin-left 5% :margin-right 5% :padding-top 10px 
                       :padding-bottom 15px :padding-left 5px :padding-right 5px
                       :background-color *main-secrion-color* :opacity 0.8)))

(defvar html (spinneret:with-html-string 
  (:html :style *html-style* 
    (:body :style *body-style* (:div :class :header
      (:span :class :center :style *logo-style*
        (:h1 "Lappely <3~")
        (:p (:sub 
          (loop for item in *my-contacts* do 
            (:a :href (car item) (cdr item)))))))
    (:div :class :main
        (loop for item in *content* do
              (:div :class :text :style *paragraph-style*
                (:h4 (:span :style *sharp-style* "#")
                     (car item))
                (if (and (not (simple-string-p (cdr item))) (vectorp (cdr item)))
                    (:ul 
                    (loop for paragraph across (cdr item) do
                          (:li paragraph)))
                    (:p (cdr item))))))))))

(clack:clackup *app* :port *port*)

(setf (ningle:route *app* "/") html)

