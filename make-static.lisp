(in-package cl-user)
(ql:quickload '(:cl-css :spinneret :str))

(defparameter *my-contacts* '(("https://www.github.com/poipoiPIO" . "Github | " ) 
                              ("https://t.me/lilyape"             . "Telegram | ") 
                              ("lappee@yahoo.com"                 . "Email!")))

(defparameter *content* 
  `(("About me:" .
     "Just another software developer. I like functional languages,
     kitties and have no fear about JS and other frontend stuff.
     This paragraph will complete a bit later. Nyahoi~")
    ("Intrested in:" . 
     #("Functional programming" "Web programming" "Plants growing"  "Compiler-craft"))
    ("Programming-Stack:" .
     #("Python" "JS/HTML/CSS" "SML/OCaml/MLs" "C#" "Lisps"))
    ("Mopclip" .
     ,(spinneret:with-html-string
       (:span "Human-friendly monadic parser-combinator library for StandardML.
               Lightweight and simple DSL library for parsers. "
       (:a :attrs (list :href "https://github.com/poipoiPIO/mopclip") "Meow~"))))))

(defun html-list-tree? (item)
  (and (not (simple-string-p item)) 
    (vectorp item)))

(defparameter *css* (cl-css:css 
              `((@font-face :font-family "gdk9" 
                           :src "url(./fonts/gdk9.ttf)")
                (@font-face :font-family "pf7" 
                           :src "url(./fonts/pf7.ttf)")
                (@font-face :font-family "mplus" 
                           :src "url(./fonts/mplus.ttf)")

                (html :background-image "url('./images/bg.png')"
                      :padding "5% 5%"
                      :background-repeat repeat) 

                (body :font-family mplus 
                      :font-size 18px 
                      :margin "4% auto"
                      :padding "9px 5px 15px" 
                      :background-color "#ffffe0"
                      :opacity "0.9"
                      :border "0.75rem ridge rgba(211, 220, 50, .6)")

                (header :display flex
                        :justify-content space-between
                        :flex-direction row
                        :padding "0.5rem 1rem"
                        :font-family pf7)
                (header>.info
                        :width 100%
                        :margin-right 2rem)

                (header>.avatar>img :width 16rem
                                    :border "1rem ridge rgba(211, 220, 50, .6)")

                (footer :text-align right)
                (a :text-decoration none)
                (h1 :margin-bottom 8px)
                (h4 :margin-bottom 0.3rem)
                (.width-100 :width 100%)
                (.width-75  :width 75%)
                (.logo      :text-align "left")
                (.sharp     :color blue)  

                (.paragraph :margin "5px 0px 10px 13px"
                            :font-family gdk9)
                (.paragraph>p :margin-left 2rem
                              :text-indent 2em
                              :margin-right 2rem)
                (.cattoes :height 4.2rem
                          :margin-top 0.5rem
                          :width 14rem
                          :background "url('./images/catto-bg.webp') 0 0.1rem repeat"
                          :background-size 17rem)

                ("::marker" :color grey)

                ("@media only screen and (min-width: 860px)" 
                  (body :width 720px)
                  (header>.avatar>img :width 16rem
                                      :border "1rem ridge rgba(211, 220, 50, .6)"))

                ("@media only screen and (max-width: 670px)" 
                  (header>.spacer :width 40%)

                  (.cattoes :height 2rem
                            :width 14rem)

                  (header>.avatar>img :width 13rem
                                      :border "1rem ridge rgba(211, 220, 50, .6)"))

                ("@media only screen and (max-width: 590px)" 
                  (.cattoes :height 2rem
                            :width 16.7rem
                            :margin "0.5rem auto")
                  (body :padding-top 0rem)
                  (header :text-align center)
                  (.paragraph>p :margin-left 0rem
                                :text-indent 2em
                                :margin-right 1rem)
                  (header>.avatar :width 100%
                                  :display "flex"
                                  :flex-direction "row"
                                  :justify-content "center")
                  (header>.avatar>img :width 85%
                                      :align-self center
                                      :border "1rem ridge rgba(211, 220, 50, .6)")

                  (header :display flex
                          :flex-direction column
                          :padding 0.5rem
                          :font-family pf7)
                  (.spacer :height 1rem)))))

(defvar *html* (spinneret:with-html-string (:html 
   (:head 
     (:meta :attrs (list :charset "utf-8"))
     (:meta :attrs (list :name "viewport" :content "width=device-width, initial-scale=1"))
     (:link :attrs (list :type "text/css" :rel "stylesheet" :href "./main.css"))
     (:link :attrs (list :rel "icon" :type "image/x-icon" :href "./images/favicon-32x32.png"))
     (:title "lappee-site <3"))
 ;; ---------Head-part-ends-----------

 ;; ---------Header-part--------------
   (:body 
    (:header 
      (:div.info
        (:h1 "Lappely <3~")
          (:b (:em "Nyahoi~ y.o | He/Him | Rus") (:br)
              "☃ Another one cute software developer~") (:br) (:br)
          (loop for item in *my-contacts* do 
            (:a :href (if (str:contains? "email" (cdr item) :ignore-case t) 
                        (format nil "mailto: ~A" (car item)) 
                        (car item)) (cdr item)))
          (:div.cattoes))
      (:div.avatar 
        (:img.avatar-image :attrs (list :src "https://github.com/poipoiPIO.png" :alt "Profile image"))))

  ;; --------Section-part----------------
    (:section (loop for item in *content* do
      (:div.paragraph
        (:h4 (:span.sharp "#") 
             (car item)) ; car is the head of paragraph 
        (if (html-list-tree? (cdr item))
          (:ul (loop 
            for paragraph across (cdr item)
              do (:li paragraph)))
          (:p (:raw (cdr item)))))))

  ;; ---------Footer-part---------------
    (:footer 
      (:sub "Site made with secret alien technology " 
        (:a :attrs (list :href "https://www.github.com/poipoiPIO/my-little-site") "Meow~")))))))

(defun make-markup ()
  (progn 
    (str:to-file "./index.html" *html*)
    (str:to-file "./main.css" *css*)))
