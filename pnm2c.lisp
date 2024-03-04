(defconstant +max-colors+ 4)
(defconstant +strip-height+ 8)

(defparameter *colors* (make-array +max-colors+ :fill-pointer 0))
(defparameter *pixels* nil)

(defvar *input* nil)
(defvar *output* nil)

(defun get-index (color)
  (dotimes (i (length *colors*))
    (when (equalp color (elt *colors* i))
      (return-from get-index i)))
  (vector-push-extend color *colors*)
  (1- (length *colors*)))

(defun read-triple ()
  (vector (read *input*) (read *input*) (read *input*)))

(defun read-index ()
  (let ((index (get-index (read-triple))))
    (when (>= index +max-colors+)
      (format *output* "#error Too much colors in picture~%")
      (exit))
    index))

(defun construct-byte (p1 p2 p3 p4)
  (logior (ash p1 6) (ash p2 4) (ash p3 2) (ash p4 0)))

(defun read-as-byte ()
  (construct-byte (read-index) (read-index) (read-index) (read-index)))

(defun swizzle-bytes (x y)
  (dotimes (strip-index (/ y +strip-height+))
    (let* ((width (/ x +max-colors+))
	   (size (* +strip-height+ width))
	   (strip (make-array size)))
      (dotimes (j +strip-height+)
	(dotimes (i width)
	  (setf (elt strip (+ j (* i +strip-height+))) (read-as-byte))))
      (dotimes (i size)
	(vector-push-extend (elt strip i) *pixels*)))))

(defun straight-bytes (x y)
  (dotimes (j y)
    (dotimes (i (/ x +max-colors+))
      (vector-push-extend (read-as-byte) *pixels*)))
  (vector-push-extend 0 *pixels*)) ; add extra byte for alignment to 64

(defun n= (a b)
  (not (= a b)))

(defun pixel (index)
  (elt *pixels* index))

(defun bytes-left (index)
  (- (length *pixels*) index))

(defun count-bytes (index predicate)
  (cond ((< (bytes-left index) 1) 0)
	((= (bytes-left index) 1) 1)
	((funcall predicate (pixel index) (pixel (1+ index)))
	 (1+ (count-bytes (1+ index) predicate)))
	(t 1)))

(defun count-bytes-safe (index predicate)
  (min 127 (count-bytes index predicate)))

(defun consume-diff (dst index count)
  (vector-push-extend (+ 128 count) dst)
  (dotimes (i count)
    (vector-push-extend (pixel (+ index i)) dst))
  count)

(defun consume-same (dst index count)
  (vector-push-extend count dst)
  (vector-push-extend (pixel index) dst)
  count)

(defun consume-chunk (dst index)
  (let ((c-same (count-bytes-safe index #'=))
	(c-diff (count-bytes-safe index #'n=)))
    (cond ((> c-same 1) (consume-same dst index c-same))
	  ((> c-diff 0) (consume-diff dst index c-diff))
	  (t 0))))

(defun consume-all-chunks (dst index)
  (let ((inc (consume-chunk dst index)))
    (cond ((> inc 0) (consume-all-chunks dst (+ index inc)))
	  (t dst))))

(defun rle-compress ()
  (consume-all-chunks (make-array 0 :fill-pointer t) 0))

(defun rle-pixels ()
  (let ((buf (rle-compress)))
    (dotimes (i (length buf))
      (format *output* "0x~2,'0x, " (elt buf i))
      (when (= 7 (mod i 8))
	(format *output* "~%")))
    (format *output* "~%")))

(defun convert-pnm (src fn)
  (setf *pixels* (make-array 0 :fill-pointer 0))
  (with-open-file (*input* src :direction :input)
     (read *input*) ; read ID
     (when (eq #\# (peek-char nil *input*))
       (read-line *input*))
     (let ((x (read *input*))
	   (y (read *input*)))
       (read *input*) ; skip 255
       (funcall fn x y)
       (rle-pixels))))

(defun format-variable (str)
  (subseq str 0 (- (length str) 2)))

(defun convert-fn (src dst fn)
  (with-open-file (*output* dst :direction :output :if-exists :supersede)
    (format *output* "static unsigned char ~A[] = {~%" (format-variable dst))
    (mapc (lambda (x) (convert-pnm x fn)) src)
    (format *output* "0x00~%")
    (format *output* "};"))
  (exit))

(defun convert (src dst)
  (convert-fn (list src) dst #'swizzle-bytes))

(defparameter *pnm-files* nil)

(defun add-file (file-name)
  (push file-name *pnm-files*))

(defun add-color (color)
  (vector-push-extend color *colors*))

(defun straight (dst)
  (convert-fn (reverse *pnm-files*) dst #'straight-bytes))
