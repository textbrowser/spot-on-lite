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

(setf s_sha_512_h (make-array 8))

(setf (aref s_sha_512_h 0) (parse-integer "6a09e667f3bcc908" :radix 16))
(setf (aref s_sha_512_h 1) (parse-integer "bb67ae8584caa73b" :radix 16))
(setf (aref s_sha_512_h 2) (parse-integer "3c6ef372fe94f82b" :radix 16))
(setf (aref s_sha_512_h 3) (parse-integer "a54ff53a5f1d36f1" :radix 16))
(setf (aref s_sha_512_h 4) (parse-integer "510e527fade682d1" :radix 16))
(setf (aref s_sha_512_h 5) (parse-integer "9b05688c2b3e6c1f" :radix 16))
(setf (aref s_sha_512_h 6) (parse-integer "1f83d9abfb41bd6b" :radix 16))
(setf (aref s_sha_512_h 7) (parse-integer "5be0cd19137e2179" :radix 16))
