(in-package cl-user)
(ql:quickload '(:cl-css :spinneret :str :ningle :clack :lack))

(defvar *app* (make-instance 'ningle:app))
(defvar *counter* 0)

(defvar *quirky-quotes* #(
  "Swim with the current, but dream upstream like a salmon on a mission."
  "Let your scales shine with the light of Genesis 1:3."
  "Be as curiously observant as a character in a Panpanya manga."
  "Find your rhythm in the school, syncopated like a cool jazz bassline."
  "Avoid the fisherman's hook, for his gifts are not truly free."
  "Your life is a journey worthy of a multi-volume surreal adventure."
  "Eat your plankton like it's manna from heaven."
  "Remember, even a whale was once small enough to be swallowed; grow steady."
  "Navigate by the stars, just like the wise men of old."
  "Dart through life with the improvisational spirit of a jazz solo."
  "Let the river's flow be your guide, a constant bassline to your melody."
  "The ocean is your promise land, keep swimming towards it."
  "Be obscure, be strange, be beautiful, like a panel from 'The Strange Tale of Panorama Island.'"
  "Sometimes you must leap over obstacles with faith and a flick of your tail."
  "School together, for a chord is richer than a single note."
  "Remember the lesson of the loaves and fishes: there is often enough to share."
  "Your strange perspective is your greatest strength, little fish."
  "Find the harmony in the current's flow and your own beating heart."
  "Let your journey be a slow-burn epic, not a hurried short story."
  "Avoid the bear's paw, for it is not a high-five of friendship."
  "Be a citizen of the river, but remember your passport to the sea is your instinct."
  "Even in the deepest water, a sliver of light points the way up."
  "Approach new eddies with the curiosity of a jazz musician finding a new scale."
  "Let your first leap be an act of creation, separating the water from the air."
  "Your life may seem a bizarre, wordless comic to some, but you understand the plot."
  "Rest in the quiet pools, where the music of the stream is soft and cool."
  "Grow strong on a diet of hope and tiny, floating things."
  "Trust in the path your ancestors have swum, it's a classic standard for a reason."
  "Be the weird, wonderful fish you were always meant to be."
  "Improvise your way around rocks and logs like a master at the keys."
  "The entire river is your cathedral; swim in awe."
  "See the world reflected on the surface, but don't forget the depths below."
  "Your existence is a non-sequitur; make it a joyful one."
  "Avoid the sharp-beaked bird, for his tune is a funeral dirge."
  "Let the moon pull you towards your destiny, as it pulls the tides."
  "School not for conformity, but for the beautiful, shifting shape you create together."
  "Even a journey of a thousand miles begins with a single, fin-powered kick."
  "See the beauty in the detour, the meander, and the stagnant backwater."
  "Be a gentle current in the lives of other fish."
  "Remember the parables; the smallest fry can have the biggest impact."
  "Approach life with a silent, observational wonder."
  "Find the groove in the river's flow and settle into it."
  "Your strange, dreamlike journey is valid."
  "Leap for the joy of leaping, not just to avoid predators."
  "Let your heart be as steadfast as a timeless hymn."
  "The water is your home, your bandstand, and your salvation."
  "See the net before it sees you; perception is key."
  "Be nourished by the word, and by the abundant zooplankton."
  "Not all that glitters is a mayfly; sometimes it's a hook."
  "Swim through the pages of your own story, one panel at a time."
  "Let your life be a peaceful, meandering solo in a world of noise."
  "Remember that every great river was once a humble spring."
  "Embrace your otherness; it is your ticket to adventure."
  "Change and grow, like a melody that develops and transforms."
  "Trust the current to take you where you need to go, even when the path is unclear."
  "Avoid the stagnant pond of complacency."
  "Let your spirit be as free as an experimental jazz composition."
  "See the divine in a single sunbeam cutting through the water."
  "Your journey may be absurd, but it is uniquely yours."
  "Find solace in the quiet, deep places where the light is blue."
  "Be a positive ripple in the watershed."
  "Approach each new day as its own unique, stand-alone chapter."
  "Sing a bubblesong of praise for the water that holds you."
  "Even in the crowd, remember your own unique swing."
  "Let the rocks smooth your edges, not break your spirit."
  "The heron's shadow is brief; your journey is long."
  "Be as patient and profound as the book of Job, but with more swimming."
  "Navigate by a logic that is entirely your own."
  "Listen to the rhythm of the rain on the surface above."
  "Your purpose is written in your DNA, a holy text of instinct."
  "See the world from a tilted, fascinating angle."
  "Find the magic in the mundane, the surreal in the stream."
  "Dart with purpose, like a staccato note in a complex arrangement."
  "Remember, you are part of a creation vast and wonderfully weird."
  "School for protection, but never lose your individual spark."
  "Let the memory of the ocean be a psalm in your heart."
  "Avoid the fisherman's wader-clad legs; they are the pillars of a false temple."
  "Be a little piece of living art, moving through a liquid canvas."
  "Improvise, adapt, overcome, and keep swimming."
  "Let your life be a gentle, enigmatic comic that leaves them thinking."
  "Grow strong on challenges, like a prophet in the wilderness."
  "See the net, not as a wall, but as a thing to be avoided with style."
  "Your strange path is the correct one."
  "Find the cool, shaded spot under the bank where the music is calm."
  "Be a beacon of quirky hope to the other fry."
  "Remember the story of the mustard seed; small beginnings have great ends."
  "Approach life with a silent, keen-eyed wonder."
  "Sync your fins with the heartbeat of the river."
  "Your existence is a beautiful, unexplained panel."
  "Leap for the sheer joy of breaking the surface and seeing the sky."
  "Let your faith be your compass, your instinct your map."
  "Water is your medium, your music, and your life."
  "Perceive the difference between a real fly and a clever fake."
  "Be filled with the spirit, and with nutritious insect larvae."
  "Not every shimmer is a promise; some are just fool's gold."
  "Swim through the narrative of your life at your own pace."
  "Let your journey be a smooth, cool melody."
  "Remember that even the widest river is crossed one stroke at a time."
  "Your unique perspective is your gift."
  "Change is the only constant, like a chord progression moving to resolution."
  "Trust the flow, even when it takes you through dark canyons."
  "Avoid the lazy water where ambition goes to die."
  "Let your soul be as deep and boundless as free jazz."
  "See the miracle in every gill-flap, every fin-twitch."
  "Your bizarre adventure is what you make of it."
  "Find peace in the deep, dark silence of the pool."
  "Be a force for good in your aquatic community."
  "Treat each rapid as its own self-contained story."
  "Sing a silent song of gratitude for the journey."
  "Even in the biggest school, don't be afraid to play your own riff."
  "Let the current polish you into a thing of beauty."
  "The osprey dives quickly, but your vigilance is constant."
  "Be as resilient as Noah's dove, searching for a place to rest."
  "Navigate the world with a quiet, internal wisdom."
  "Listen to the ancient song of the river."
  "Your destiny is a great fish, waiting to be caught."
  "See the world as a series of interconnected, surreal vignettes."
  "Find the extraordinary in a single grain of sand."
  "Dart with confidence, like a trumpet player hitting a high note."
  "Remember, you are a beloved part of a crazy, chaotic, beautiful creation."
  "School for camaraderie, not just for survival."
  "Let the salt of the future sea be a promise on your lips."
  "Avoid the two-legged ones on the shore; they are not your curators."
  "Be a living testament to the weird and wonderful."
  "Improvise your path around every new obstacle."
  "Let your life be an open-ended, philosophical comic."
  "Grow not just in size, but in grace and weirdness."
  "See the weir not as a barrier, but as a test of your leaping ability."
  "Your peculiar way is the right way for you."
  "Find the still point in the turning world of the whirlpool."
  "Be a little glimmer of joy in the water."
  "Remember the lilies of the field; they toil not, but you definitely have to swim."
  "Approach each moment with a mindful, artistic eye."
  "Feel the swing of the seasons, the tempo of the tides."
  "Your life is a beautiful, abstract sequence."
  "Leap and trust that the water will be there to meet you."
  "Let your instincts be your book of revelation."
  "The river is your band, your audience, and your stage."
  "Discern the true current from the deceptive eddy."
  "Be filled with grace, and with delicious water fleas."
  "Not every offering of food is a benevolent act."
  "Swim through the panels of your days with intention."
  "Let your path be a sweet, meandering blues tune."
  "Remember that the mightiest waterfall was once a peaceful stream."
  "Your strangeness is your salvation."
  "Adapt like a melody changing key, beautifully and without warning."
  "Trust the deep, ancient knowledge in your genes."
  "Avoid the places where the music of the river grows dull."
  "Let your spirit be as innovative as bebop."
  "See the face of the divine in the dance of the minnows."
  "Your surreal journey is the whole point."
  "Find strength in the quiet, abyssal depths of your own being."
  "Be a positive, quirky influence on the ecosystem."
  "View each challenge as a single, impactful frame in your story."
  "Sing a water-hymn of joy for the simple fact of being."
  "Even in the synchronized school, add your own subtle syncopation."
  "Let time and the current smooth away your fears."
  "The pike strikes fast, but your reflexes are faster."
  "Be as determined as Moses leading his people, but with fewer plagues."
  "Move through the world with a quiet, unshakeable confidence."
  "Listen for the rhythm of your own heart, your most important beat."
  "Your purpose is a deep and mysterious scroll, unfurling as you swim."
  "See the connection between the starry sky and the sparkling riverbed."
  "Find the magic in the driftwood, the mystery in the moss."
  "Dart with the precision of a well-practiced musical phrase."
  "Remember, you are a unique note in the great symphony of creation."
  "Swim together, creating a harmony more beautiful than any solo."
  "Let the taste of the distant ocean fuel your dreams."
  "Avoid the traps set by those who do not understand your journey."
  "Be a little piece of living, swimming surrealism."
  "Improvise, innovate, and inspire."
  "Let your story be one of peaceful exploration."
  "Grow in wisdom, and in girth."
  "See the waterfall not as an end, but as a glorious transition."
  "Your weird and wonderful path is an inspiration."
  "Find calm in the center of the stormy rapid."
  "Be a flicker of silver hope in the dark water."
  "Remember the ant who prepares for winter; be prepared for your ocean journey."
  "Observe the world with a detached, artistic calm."
  "Move to the timeless pulse of the earth's waters."
  "Your life is a series of beautiful, enigmatic moments."
  "Leap towards the sun, even if you fall back down."
  "Let faith be the fin that pushes you forward when you're tired."
  "The water is your salvation, your solace, and your swing."
  "Learn the difference between a friend and a well-disguised foe."
  "Be full of life, and of countless tiny copepods."
  "Not every shadow is a predator; some are just clouds passing overhead."
  "Navigate the stream of consciousness with ease."
  "Let your journey be a classic, timeless standard."
  "Remember that every salmon was once a tiny egg, full of potential."
  "Your unique view is a gift to the world."
  "Change and metamorphose like a melody that tells a story."
  "Trust that the river knows the way to the sea."
  "Avoid the dull, lifeless places where creativity goes to die."
  "Let your essence be as complex and satisfying as a John Coltrane solo."
  "See the holy in the whirl of a maple seed sinking through the water."
  "Your bizarre existence is a perfect joke only you understand."
  "Find power in the silent, dark undercurrent."
  "Be a builder of a better, quirkier river."
  "Treat each day as a fresh page, a new panel."
  "Sing a grateful song for the water that sustains you."
  "Even in the most structured school, find space for your own expression."
  "Let the journey itself shape you into who you are meant to be."
  "The otter plays, but his play is deadly; be watchful."
  "Be as steadfast in your purpose as a psalmist in his praise."
  "Travel through life with a serene and knowing silence."
  "Hear the music of the spheres in the rush of the stream."
  "Your destiny is a great story, and you are its author."
  "See the world as a vast, interconnected, and slightly off-kilter comic strip."
  "Find wonder in a single, sun-dappled pebble."
  "Move with the effortless cool of a jazz virtuoso."
  "Remember, you are a cherished part of this strange and glorious creation."
  "Swim in harmony, creating a living, moving masterpiece."
  "Let the call of the vast, open ocean be your guiding star."))

(defun get-quote ()
  (aref *quirky-quotes* (random (length *quirky-quotes*))))

(defparameter *my-contacts* '(("https://www.github.com/poipoiPIO" . "Github | " ) 
                              ("https://t.me/lilyape"             . "Telegram | ") 
                              ("https://www.linkedin.com/in/kirill-gnapovskii-b96b1a292" . "LinkedIn | ") 
                              ("lappee@yahoo.com"                 . "Email!")))

(defparameter *content* 
  `(("About me:" .
     "A bookworm software engineer curious about in-depth details of the surrounding world.")
    ("Things I like:" . 
     #("System Programming" "Software engineering" "Fishing"))))

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

                (body :background-image "url('./images/bg.png')"
                      :padding "min(5vw, 5vh)"
                      :background-repeat repeat) 

                (.cheer-me-up :position fixed
                              :font-family gdk9
                              :bottom 2rem
                              :display none
                              :height 10vw)

                (.cheer-me-up>div :width 220px :text-align left)
                (.cheer-me-up>a>img :height 100%)

                (.wrapper :font-family mplus 
                      :font-size 14px 
                      :padding "min(1vw, 1vh)" 
                      :background-color "#ffffe0"
                      :opacity "0.9"
                      :width 70vw
                      :border "0.75rem ridge rgba(211, 220, 50, .6)")

                (header :display flex
                        :flex-direction row
                        :justify-content space-between
                        :gap 1rem
                        :padding "0.5rem 1rem"
                        :font-family pf7)

                (header>.avatar>img :width 90%
                                    :border "1rem ridge rgba(211, 220, 50, .6)")

                (footer :display flex :justify-content "space-between")
                (.right-footer :text-align end)
                (a :text-decoration none)
                (h1 :margin-bottom 8px)
                (h4 :margin-bottom 0.3rem)

                (.body :display flex
                       :align-items center
                       :justify-content center)

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

                ("@keyframes float" 
                  ("0%" :transform "translatey(0px)")
                  ("50%" :transform "translatey(-20px)")
                  ("100%" :transform "translatey(0px)"))

                ("@media only screen and (min-width: 1320px)"
                  (.cheer-me-up :display flex)
                  (.cheer-me-up :animation "float 5s ease-in-out infinite")) 

                ("@media only screen and (min-width: 810px)" 
                  (.wrapper :width 62vw :max-width 540px)
                  (header>.avatar>img :width 12rem
                                      :border "1rem ridge rgba(211, 220, 50, .6)"))

                ("@media only screen and (max-width: 590px)" 
                  (.cattoes :height 2rem
                            :width 16.7rem
                            :margin "0.5rem auto")
                  (.wrapper :padding-top 0rem :width 85vw)
                  (header :text-align center)
                  (.paragraph>p :margin-left 0rem
                                :text-indent 2em
                                :margin-right 1rem)
                  (header>.avatar :width 100%
                                  :display "flex"
                                  :flex-direction "row"
                                  :justify-content "center")
                  (header>.avatar>img :width 70vw
                                      :align-self center
                                      :border "1rem ridge rgba(211, 220, 50, .6)")

                  (header :display flex
                          :flex-direction column
                          :padding 0.5rem
                          :font-family pf7)
                  (.spacer :height 1rem)))))

(defun get-html-string (counter) (spinneret:with-html-string (:html 
   (:head 
     (:meta :attrs (list :charset "utf-8"))
     (:meta :attrs (list :name "viewport" :content "width=device-width, initial-scale=1"))
     (:link :attrs (list :type "text/css" :rel "stylesheet" :href "./static/main.css"))
     (:link :attrs (list :rel "prefetch" :href "https://avatars.githubusercontent.com/u/82707867"))
     (:link :attrs (list :rel "icon" :type "image/x-icon" :href "./static/images/favicon-32x32.png'"))
     (:title "lappee-site <3"))
 ;; ---------Head-part-ends-----------

 ;; ---------Header-part--------------
   (:body 
    (:div.cheer-me-up 
      (:a :attrs (list :href "")
        (:img :attrs (list :src "./static/images/trout.png" :alt "CheerMeUp")))
      (:div (get-quote)))
    (:div.body
      (:div.wrapper
        (:header 
          (:div.info
            (:h1 "Lappely")
              (:b (:em "19 y.o | He/Him | Rus") (:br)
                  "☃ Somehow a software engineer!") (:br) (:br)
              (loop for item in *my-contacts* do 
                (:a :href (if (str:contains? "email" (cdr item) :ignore-case t) 
                            (format nil "mailto: ~A" (car item)) 
                            (car item)) (cdr item)))
              (:div.cattoes))
          (:div.avatar 
            (:img.avatar-image :attrs (list :src "https://avatars.githubusercontent.com/u/82707867" :alt "Profile image"))))

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
          (:sub (format nil "You are our very ~Dth visitor!" counter))
          (:sub.right-footer "Site made with secret alien technology " 
            (:a :attrs (list :href "https://www.github.com/poipoiPIO/my-little-site") "Link!")))))))))

(defun make-markup () (str:to-file "./static/main.css" *css*))

(setf (ningle:route *app* "/") #'(lambda (_) 
  (progn (setf *counter* (+ *counter* 1))
         (print "hanlding connection")
         (get-html-string *counter*))))

(defun main (port)
  (progn (clack:clackup 
    (lack:builder
      :session
      (:static :path "/static/"
               :root #P"./static/")
                *app*) :address "0.0.0.0" :port port :debug nil)
  (format t "Server is started on port: ~D" port)

    (handler-case (bt:join-thread (find-if (lambda (th) (search "hunchentoot" (bt:thread-name th))) (bt:all-threads)))
      (#+sbcl sb-sys:interactive-interrupt
        #+ccl  ccl:interrupt-signal-condition
        #+clisp system::simple-interrupt-condition
        #+ecl ext:interactive-interrupt
        #+allegro excl:interrupt-signal
        () (progn
             (format *error-output* "Aborting.~&")
             (clack:stop *app*)
             (uiop:quit)))
      (error (c) (format t "Woops, an unknown error occured:~&~a~&" c)))))

