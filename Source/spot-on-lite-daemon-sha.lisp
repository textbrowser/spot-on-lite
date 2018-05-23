;; Copyright (c) 2011 - 10^10^10, Alexis Megas.
;; All rights reserved.
;;
;; Redistribution and use in source and binary forms, with or without
;; modification, are permitted provided that the following conditions
;; are met:
;; 1. Redistributions of source code must retain the above copyright
;;    notice, this list of conditions and the following disclaimer.
;; 2. Redistributions in binary form must reproduce the above copyright
;;    notice, this list of conditions and the following disclaimer in the
;;    documentation and/or other materials provided with the distribution.
;; 3. The name of the author may not be used to endorse or promote products
;;    derived from Spot-On without specific prior written permission.
;;
;; SPOT-ON-LITE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
;; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
;; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
;; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
;; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
;; NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
;; SPOT-ON-LITE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

(defun Ch (x y z)
  (logxor (logand x y) (logand (lognot x) z))
)

(defun Maj (x y z)
  (logxor (logand x y) (logand x z) (logand y z))
)

(defun ROTR (n x)
  (logior (logand (ash x (- (mod n 64))) (1- (ash 1 64)))
	  (logand (ash x (- 64 (mod n 64))) (1- (ash 1 64))))
)

(defun SA0_512 (x)
  (logxor (ROTR 28 x) (ROTR 34 x) (ROTR 39 x))
)

(defun SA1_512 (x)
  (logxor (ROTR 14 x) (ROTR 18 x) (ROTR 41 x))
)

(defun SHR (n x)
  (logand (ash x (- n)) (1- (ash 1 64)))
)

(defun sb0_512 (x)
  (logxor (ROTR 1 x) (ROTR 8 x) (SHR 7 x))
)

(defun sb1_512 (x)
  (logxor (ROTR 19 x) (ROTR 61 x) (SHR 6 x))
)

(defvar s_sha_512_h
  (setf s_sha_512_h (make-array 8
				:element-type '(unsigned-byte 64)
				:initial-contents '(#x6a09e667f3bcc908
						    #xbb67ae8584caa73b
						    #x3c6ef372fe94f82b
						    #xa54ff53a5f1d36f1
						    #x510e527fade682d1
						    #x9b05688c2b3e6c1f
						    #x1f83d9abfb41bd6b
						    #x5be0cd19137e2179))))

(defvar s_sha_512_k
  (setf s_sha_512_k (make-array 80
				:element-type '(unsigned-byte 64)
				:initial-contents '(#x428a2f98d728ae22
						    #x7137449123ef65cd
						    #xb5c0fbcfec4d3b2f
						    #xe9b5dba58189dbbc
						    #x3956c25bf348b538
						    #x59f111f1b605d019
						    #x923f82a4af194f9b
						    #xab1c5ed5da6d8118
						    #xd807aa98a3030242
						    #x12835b0145706fbe
						    #x243185be4ee4b28c
						    #x550c7dc3d5ffb4e2
						    #x72be5d74f27b896f
						    #x80deb1fe3b1696b1
						    #x9bdc06a725c71235
						    #xc19bf174cf692694
						    #xe49b69c19ef14ad2
						    #xefbe4786384f25e3
						    #x0fc19dc68b8cd5b5
						    #x240ca1cc77ac9c65
						    #x2de92c6f592b0275
						    #x4a7484aa6ea6e483
						    #x5cb0a9dcbd41fbd4
						    #x76f988da831153b5
						    #x983e5152ee66dfab
						    #xa831c66d2db43210
						    #xb00327c898fb213f
						    #xbf597fc7beef0ee4
						    #xc6e00bf33da88fc2
						    #xd5a79147930aa725
						    #x06ca6351e003826f
						    #x142929670a0e6e70
						    #x27b70a8546d22ffc
						    #x2e1b21385c26c926
						    #x4d2c6dfc5ac42aed
						    #x53380d139d95b3df
						    #x650a73548baf63de
						    #x766a0abb3c77b2a8
						    #x81c2c92e47edaee6
						    #x92722c851482353b
						    #xa2bfe8a14cf10364
						    #xa81a664bbc423001
						    #xc24b8b70d0f89791
						    #xc76c51a30654be30
						    #xd192e819d6ef5218
						    #xd69906245565a910
						    #xf40e35855771202a
						    #x106aa07032bbd1b8
						    #x19a4c116b8d2d0c8
						    #x1e376c085141ab53
						    #x2748774cdf8eeb99
						    #x34b0bcb5e19b48a8
						    #x391c0cb3c5c95a63
						    #x4ed8aa4ae3418acb
						    #x5b9cca4f7763e373
						    #x682e6ff3d6b2b8a3
						    #x748f82ee5defb2fc
						    #x78a5636f43172f60
						    #x84c87814a1f0ab72
						    #x8cc702081a6439ec
						    #x90befffa23631e28
						    #xa4506cebde82bde9
						    #xbef9a3f7b2c67915
						    #xc67178f2e372532b
						    #xca273eceea26619c
						    #xd186b8c721c0c207
						    #xeada7dd6cde0eb1e
						    #xf57d4f7fee6ed178
						    #x06f067aa72176fba
						    #x0a637dc5a2c898a6
						    #x113f9804bef90dae
						    #x1b710b35131c471b
						    #x28db77f523047d84
						    #x32caab7b40c72493
						    #x3c9ebe0a15c9bebc
						    #x431d67c49c100d4c
						    #x4cc5d4becb3e42b6
						    #x597f299cfc657e2a
						    #x5fcb6fab3ad6faec
						    #x6c44198c4a475817))))

(defun bytes_to_number (data start)
  (setf number (logior (logand (aref data (+ start 7)) #xff)
		       (ash (logand (aref data (+ start 6)) #xff) 8)
		       (ash (logand (aref data (+ start 5)) #xff) 16)
		       (ash (logand (aref data (+ start 4)) #xff) 24)
		       (ash (logand (aref data (+ start 3)) #xff) 32)
		       (ash (logand (aref data (+ start 2)) #xff) 40)
		       (ash (logand (aref data (+ start 1)) #xff) 48)
		       (ash (logand (aref data start) #xff) 56)))
  number
)

(defun number_to_bytes (number)
  (setf bytes (make-array 8
			  :element-type '(unsigned-byte 8)
			  :initial-element 0))
  (setf (aref bytes 0) (logand (ash number (- 56)) #xff))
  (setf (aref bytes 1) (logand (ash number (- 48)) #xff))
  (setf (aref bytes 2) (logand (ash number (- 40)) #xff))
  (setf (aref bytes 3) (logand (ash number (- 32)) #xff))
  (setf (aref bytes 4) (logand (ash number (- 24)) #xff))
  (setf (aref bytes 5) (logand (ash number (- 16)) #xff))
  (setf (aref bytes 6) (logand (ash number (- 8)) #xff))
  (setf (aref bytes 7) (logand number #xff))
  bytes
)

(defun sha_512 (data)
  ;; Initializations.

  (setf HH (make-array 8
		       :element-type '(unsigned-byte 64)
		       :initial-element 0))
  (setf N (ceiling (/ (+ (array-total-size data) 17.0) 128.0)))
  (setf d8 (* (array-total-size data) 8))
  (setf number (make-array 8
			   :element-type '(unsigned-byte 8)
			   :initial-element 0))

  ;; Padding the hash object (5.1.2).

  (setf hash (make-array (* 128 N)
			 :element-type '(unsigned-byte 8)
			 :initial-element 0))
  (setf number (number_to_bytes d8))

  ;; Place the contents of the data container into the hash container.

  (dotimes (i (array-total-size data))
    (setf (aref hash i) (aref data i)))

  ;; Place 0x80 at hash[data.length()].

  (setf (aref hash (array-total-size data)) #x80)

  ;; Place the number at the end of the hash container.

  (setf (aref hash (- (array-total-size hash) 8)) (aref number 0))
  (setf (aref hash (- (array-total-size hash) 7)) (aref number 1))
  (setf (aref hash (- (array-total-size hash) 6)) (aref number 2))
  (setf (aref hash (- (array-total-size hash) 5)) (aref number 3))
  (setf (aref hash (- (array-total-size hash) 4)) (aref number 4))
  (setf (aref hash (- (array-total-size hash) 3)) (aref number 5))
  (setf (aref hash (- (array-total-size hash) 2)) (aref number 6))
  (setf (aref hash (- (array-total-size hash) 1)) (aref number 7))

  ;; Initialize HH (5.3.5).

  (dotimes (i 8)
    (setf (aref HH i) (aref s_sha_512_h i)))

  ;; Let's compute the hash (6.4.2).

  (dotimes (i N)
    (setf M (make-array 16
			:element-type '(unsigned-byte 64)
			:initial-element 0))

    (loop for j from 0 to 120 by 8 do
	  (setf n (bytes_to_number hash (+ (* 128 i) j)))
	  (setf (aref M (/ j 8)) n))

    (setf W (make-array 80
			:element-type '(unsigned-byte 64)
			:initial-element 0))

    (loop for tt from 0 to 15 do
	  (setf (aref W tt) (aref M tt)))

    (loop for tt from 16 to 79 do
	  (setf (aref W tt)
		(logand (+ (sb1_512 (aref W (- tt 2)))
			   (aref W (- tt 7))
			   (sb0_512 (aref W (- tt 15)))
			   (aref W (- tt 16))) #xffffffffffffffff)))

    (setf a (aref HH 0))
    (setf b (aref HH 1))
    (setf c (aref HH 2))
    (setf d (aref HH 3))
    (setf e (aref HH 4))
    (setf f (aref HH 5))
    (setf g (aref HH 6))
    (setf h (aref HH 7))

    (loop for tt from 0 to 79 do
	  (setf K (aref s_sha_512_k tt))
	  (setf T1 (logand (+ h (SA1_512 e) (Ch e f g) K (aref W tt))
			   #xffffffffffffffff))
	  (setf T2 (logand (+ (SA0_512 a) (Maj a b c))
			   #xffffffffffffffff))
	  (setf h g)
	  (setf g f)
	  (setf f e)
	  (setf e (logand (+ d T1) #xffffffffffffffff))
	  (setf d c)
	  (setf c b)
	  (setf b a)
	  (setf a (logand (+ T1 T2) #xffffffffffffffff)))

    (setf (aref HH 0) (logand (+ (aref HH 0) a) #xffffffffffffffff))
    (setf (aref HH 1) (logand (+ (aref HH 1) b) #xffffffffffffffff))
    (setf (aref HH 2) (logand (+ (aref HH 2) c) #xffffffffffffffff))
    (setf (aref HH 3) (logand (+ (aref HH 3) d) #xffffffffffffffff))
    (setf (aref HH 4) (logand (+ (aref HH 4) e) #xffffffffffffffff))
    (setf (aref HH 5) (logand (+ (aref HH 5) f) #xffffffffffffffff))
    (setf (aref HH 6) (logand (+ (aref HH 6) g) #xffffffffffffffff))
    (setf (aref HH 7) (logand (+ (aref HH 7) h) #xffffffffffffffff)))
  HH
)

(defun test1 ()
  (bytes_to_number (number_to_bytes 1234567890123456789) 0)
)

(defun test2 ()
  ;; "abc"
  (write-to-string (sha_512 (make-array 3
					:element-type '(unsigned-byte 8)
					:initial-contents '(97 98 99)))
		   :base 16)
)

(defun test3()
  ;; "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn
  ;;  hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
  (setf a "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu")
  (setf d (make-array (length a)
		      :element-type '(unsigned-byte 8)))
  (loop for i from 0 to (1- (length a)) do
	(setf (aref d i) (char-code (aref a i))))
  (write-to-string (sha_512 d) :base 16)
)
