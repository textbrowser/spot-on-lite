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

(defparameter s_sha_512_h
  (setf s_sha_512_h (make-array 8 :element-type '(unsigned-byte 64))))

(setf (aref s_sha_512_h 0) #x6a09e667f3bcc908)
(setf (aref s_sha_512_h 1) #xbb67ae8584caa73b)
(setf (aref s_sha_512_h 2) #x3c6ef372fe94f82b)
(setf (aref s_sha_512_h 3) #xa54ff53a5f1d36f1)
(setf (aref s_sha_512_h 4) #x510e527fade682d1)
(setf (aref s_sha_512_h 5) #x9b05688c2b3e6c1f)
(setf (aref s_sha_512_h 6) #x1f83d9abfb41bd6b)
(setf (aref s_sha_512_h 7) #x5be0cd19137e2179)

(defparameter s_sha_512_k
  (setf s_sha_512_k (make-array 80 :element-type '(unsigned-byte 64))))

(setf (aref s_sha_512_k 0) #x428a2f98d728ae22)
(setf (aref s_sha_512_k 1) #x7137449123ef65cd)
(setf (aref s_sha_512_k 2) #xb5c0fbcfec4d3b2f)
(setf (aref s_sha_512_k 3) #xe9b5dba58189dbbc)
(setf (aref s_sha_512_k 4) #x3956c25bf348b538)
(setf (aref s_sha_512_k 5) #x59f111f1b605d019)
(setf (aref s_sha_512_k 6) #x923f82a4af194f9b)
(setf (aref s_sha_512_k 7) #xab1c5ed5da6d8118)
(setf (aref s_sha_512_k 8) #xd807aa98a3030242)
(setf (aref s_sha_512_k 9) #x12835b0145706fbe)
(setf (aref s_sha_512_k 10) #x243185be4ee4b28c)
(setf (aref s_sha_512_k 11) #x550c7dc3d5ffb4e2)
(setf (aref s_sha_512_k 12) #x72be5d74f27b896f)
(setf (aref s_sha_512_k 13) #x80deb1fe3b1696b1)
(setf (aref s_sha_512_k 14) #x9bdc06a725c71235)
(setf (aref s_sha_512_k 15) #xc19bf174cf692694)
(setf (aref s_sha_512_k 16) #xe49b69c19ef14ad2)
(setf (aref s_sha_512_k 17) #xefbe4786384f25e3)
(setf (aref s_sha_512_k 18) #x0fc19dc68b8cd5b5)
(setf (aref s_sha_512_k 19) #x240ca1cc77ac9c65)
(setf (aref s_sha_512_k 20) #x2de92c6f592b0275)
(setf (aref s_sha_512_k 21) #x4a7484aa6ea6e483)
(setf (aref s_sha_512_k 22) #x5cb0a9dcbd41fbd4)
(setf (aref s_sha_512_k 23) #x76f988da831153b5)
(setf (aref s_sha_512_k 24) #x983e5152ee66dfab)
(setf (aref s_sha_512_k 25) #xa831c66d2db43210)
(setf (aref s_sha_512_k 26) #xb00327c898fb213f)
(setf (aref s_sha_512_k 27) #xbf597fc7beef0ee4)
(setf (aref s_sha_512_k 28) #xc6e00bf33da88fc2)
(setf (aref s_sha_512_k 29) #xd5a79147930aa725)
(setf (aref s_sha_512_k 30) #x06ca6351e003826f)
(setf (aref s_sha_512_k 31) #x142929670a0e6e70)
(setf (aref s_sha_512_k 32) #x27b70a8546d22ffc)
(setf (aref s_sha_512_k 33) #x2e1b21385c26c926)
(setf (aref s_sha_512_k 34) #x4d2c6dfc5ac42aed)
(setf (aref s_sha_512_k 35) #x53380d139d95b3df)
(setf (aref s_sha_512_k 36) #x650a73548baf63de)
(setf (aref s_sha_512_k 37) #x766a0abb3c77b2a8)
(setf (aref s_sha_512_k 38) #x81c2c92e47edaee6)
(setf (aref s_sha_512_k 39) #x92722c851482353b)
(setf (aref s_sha_512_k 40) #xa2bfe8a14cf10364)
(setf (aref s_sha_512_k 41) #xa81a664bbc423001)
(setf (aref s_sha_512_k 42) #xc24b8b70d0f89791)
(setf (aref s_sha_512_k 43) #xc76c51a30654be30)
(setf (aref s_sha_512_k 44) #xd192e819d6ef5218)
(setf (aref s_sha_512_k 45) #xd69906245565a910)
(setf (aref s_sha_512_k 46) #xf40e35855771202a)
(setf (aref s_sha_512_k 47) #x106aa07032bbd1b8)
(setf (aref s_sha_512_k 48) #x19a4c116b8d2d0c8)
(setf (aref s_sha_512_k 49) #x1e376c085141ab53)
(setf (aref s_sha_512_k 50) #x2748774cdf8eeb99)
(setf (aref s_sha_512_k 51) #x34b0bcb5e19b48a8)
(setf (aref s_sha_512_k 52) #x391c0cb3c5c95a63)
(setf (aref s_sha_512_k 53) #x4ed8aa4ae3418acb)
(setf (aref s_sha_512_k 54) #x5b9cca4f7763e373)
(setf (aref s_sha_512_k 55) #x682e6ff3d6b2b8a3)
(setf (aref s_sha_512_k 56) #x748f82ee5defb2fc)
(setf (aref s_sha_512_k 57) #x78a5636f43172f60)
(setf (aref s_sha_512_k 58) #x84c87814a1f0ab72)
(setf (aref s_sha_512_k 59) #x8cc702081a6439ec)
(setf (aref s_sha_512_k 60) #x90befffa23631e28)
(setf (aref s_sha_512_k 61) #xa4506cebde82bde9)
(setf (aref s_sha_512_k 62) #xbef9a3f7b2c67915)
(setf (aref s_sha_512_k 63) #xc67178f2e372532b)
(setf (aref s_sha_512_k 64) #xca273eceea26619c)
(setf (aref s_sha_512_k 65) #xd186b8c721c0c207)
(setf (aref s_sha_512_k 66) #xeada7dd6cde0eb1e)
(setf (aref s_sha_512_k 67) #xf57d4f7fee6ed178)
(setf (aref s_sha_512_k 68) #x06f067aa72176fba)
(setf (aref s_sha_512_k 69) #x0a637dc5a2c898a6)
(setf (aref s_sha_512_k 70) #x113f9804bef90dae)
(setf (aref s_sha_512_k 71) #x1b710b35131c471b)
(setf (aref s_sha_512_k 72) #x28db77f523047d84)
(setf (aref s_sha_512_k 73) #x32caab7b40c72493)
(setf (aref s_sha_512_k 74) #x3c9ebe0a15c9bebc)
(setf (aref s_sha_512_k 75) #x431d67c49c100d4c)
(setf (aref s_sha_512_k 76) #x4cc5d4becb3e42b6)
(setf (aref s_sha_512_k 77) #x597f299cfc657e2a)
(setf (aref s_sha_512_k 78) #x5fcb6fab3ad6faec)
(setf (aref s_sha_512_k 79) #x6c44198c4a475817)

(defun bytes_to_number (data start)
  (setf number (logior (logand (aref data (+ start 7)) #xff)
		       (ash (logand (aref data (+ start 6)) #xff) 8)
		       (ash (logand (aref data (+ start 5)) #xff) 16)
		       (ash (logand (aref data (+ start 4)) #xff) 24)
		       (ash (logand (aref data (+ start 3)) #xff) 32)
		       (ash (logand (aref data (+ start 2)) #xff) 40)
		       (ash (logand (aref data (+ start 1)) #xff) 48)
		       (ash (logand (aref data start) #xff) 56)))
  number)

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
  bytes)

(defun sha_512 (data)
  (setf H (make-array 8
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
  (setf number (word_to_bytes d8))

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

  ;; Initialize H (5.3.5).

  (dotimes (i 8)
    (setf (aref H i) (aref s_sha_512_h i)))

  ;; Let's compute the hash (6.4.2).

  (dotimes (i N)
    (setf M (make-array 16
			:element-type '(unsigned-byte 64)
			:initial-element 0))
    (setf n 0)
    (dotimes (j 16)
      (setf n (bytes_to_number hash (+ (* 128 i) j)))
      (setf (aref M j) n)))
)

(defun test1 () (bytes_to_number (number_to_bytes 1234567890123456789) 0))
