(defun Ch (x y z)
  (logxor (logand x y) (logand (lognot x) z)))

(defun Maj (x y z)
  (logxor (logand x y) (logand x z) (logand y z)))

(defun ROTR (n x)
  (logior (ash x (- n)) (ash x (- 64 n))))

(defun S0_512 (x)
  (logxor (ROTR 28 x) (ROTR 34 x) (ROTR 39 x)))

(defun S1_512 (x)
  (logxor (ROTR 14 x) (ROTR 18 x) (ROTR 41 x)))

(defun SHR (n x)
  (ash x (- n)))

(defun s0_512 (x)
  (logxor (ROTR 1 x) (ROTR 8 x) (SHR 7 x)))

(defun s1_512 (x)
  (logxor (ROTR 19 x) (ROTR 61 x) (SHR 6 x)))
