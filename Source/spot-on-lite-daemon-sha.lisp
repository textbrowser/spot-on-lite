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

(setf s_sha_512_h (make-array 8))

(setf (aref s_sha_512_h 0) (parse-integer "6a09e667f3bcc908" :radix 16))
(setf (aref s_sha_512_h 1) (parse-integer "bb67ae8584caa73b" :radix 16))
(setf (aref s_sha_512_h 2) (parse-integer "3c6ef372fe94f82b" :radix 16))
(setf (aref s_sha_512_h 3) (parse-integer "a54ff53a5f1d36f1" :radix 16))
(setf (aref s_sha_512_h 4) (parse-integer "510e527fade682d1" :radix 16))
(setf (aref s_sha_512_h 5) (parse-integer "9b05688c2b3e6c1f" :radix 16))
(setf (aref s_sha_512_h 6) (parse-integer "1f83d9abfb41bd6b" :radix 16))
(setf (aref s_sha_512_h 7) (parse-integer "5be0cd19137e2179" :radix 16))

(setf s_sha_512_k (make-array 80))
