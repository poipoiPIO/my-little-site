(in-package cl-user)
(ql:quickload '(:cl-css :spinneret :str))

(defparameter *my-contacts* 
  '(("github.com/poipoiPIO" . "Github | " ) ("@lilyape" . "Telegram | ") ("lappee@yahoo.com" . "Email!")))
(defparameter *content* 
  '(("About me:" . "Poi Poi cotton clay pills cat pool iter pill sa ola pie de ramin")
  ("Intrested in:" . #("Poi poi" "Kitty Cat"))))

(defparameter *css* (cl-css:css 
              '((.paragraph :margin "5px 0px 10px 13px") 

                (html :background-image "url('./images/bg.png')"
                      :background-repeat repeat) 

                (body :font-family Helvetica 
                      :font-size 18px 
                      :margin "5% 3%"
                      :padding "9px 5px 15px" 
                      :background-color "#ffffe0"
                      :opacity "0.8"
                      :border "1rem ridge rgba(211, 220, 50, .6)"
                      :cursor "url('./images/cursor.png')")

                (.avatar-image :border "1rem ridge rgba(211, 220, 50, .6)"
                               :height 20rem
                               :width  20rem)

                (.logo :text-align "left")
                (.sharp :color blue))))

(defvar *html* (spinneret:with-html-string 
  (:html 
   (:head 
     (:meta :attrs (list :charset "utf-8")) 
     (:title "lappee-site<3")
     (:link :attrs (list :type "text/css" :rel "stylesheet" :href "./main.css")))
   ;; ---------Head-part-ends-----------
    (:body
   ;; ---------Header-part--------------
    (:header 
      (:div.logo
        (:img.avatar-image :attrs (list :align "right" :src "./images/avatar.jpg" :alt "Profile image"))
        (:h1 "Lappely <3~")
        (:sub 
          (loop for item in *my-contacts* do 
            (:a :href (car item) 
                      (cdr item))))))

  ;; --------Header-part-end-------------
  ;; --------Section-part----------------
    (:section (loop for item in *content* do
      (:div.paragraph
        (:h4 (:span.sharp "#") 
             (car item)) ; car is the head of paragraph 
        (if (and (not (simple-string-p (cdr item))) (vectorp (cdr item)))
           (:ul (loop for paragraph across (cdr item) do (:li paragraph)))
           (:p (cdr item))))))))))

(defun make-markup ()
  (progn 
    (str:to-file "./static/index.html" *html*)
    (str:to-file "./static/main.css" *css*)))

(make-markup)
