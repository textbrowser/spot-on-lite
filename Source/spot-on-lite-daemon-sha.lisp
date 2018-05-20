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

(defparameter s_sha_512_h (setf s_sha_512_h (make-array 8)))

(setf (aref s_sha_512_h 0) (parse-integer "6a09e667f3bcc908" :radix 16))
(setf (aref s_sha_512_h 1) (parse-integer "bb67ae8584caa73b" :radix 16))
(setf (aref s_sha_512_h 2) (parse-integer "3c6ef372fe94f82b" :radix 16))
(setf (aref s_sha_512_h 3) (parse-integer "a54ff53a5f1d36f1" :radix 16))
(setf (aref s_sha_512_h 4) (parse-integer "510e527fade682d1" :radix 16))
(setf (aref s_sha_512_h 5) (parse-integer "9b05688c2b3e6c1f" :radix 16))
(setf (aref s_sha_512_h 6) (parse-integer "1f83d9abfb41bd6b" :radix 16))
(setf (aref s_sha_512_h 7) (parse-integer "5be0cd19137e2179" :radix 16))

(defparameter s_sha_512_k (setf s_sha_512_k (make-array 80)))

(setf (aref s_sha_512_k 0) (parse-integer "428a2f98d728ae22" :radix 16))
(setf (aref s_sha_512_k 1) (parse-integer "7137449123ef65cd" :radix 16))
(setf (aref s_sha_512_k 2) (parse-integer "b5c0fbcfec4d3b2f" :radix 16))
(setf (aref s_sha_512_k 3) (parse-integer "e9b5dba58189dbbc" :radix 16))
(setf (aref s_sha_512_k 4) (parse-integer "3956c25bf348b538" :radix 16))
(setf (aref s_sha_512_k 5) (parse-integer "59f111f1b605d019" :radix 16))
(setf (aref s_sha_512_k 6) (parse-integer "923f82a4af194f9b" :radix 16))
(setf (aref s_sha_512_k 7) (parse-integer "ab1c5ed5da6d8118" :radix 16))
(setf (aref s_sha_512_k 8) (parse-integer "d807aa98a3030242" :radix 16))
(setf (aref s_sha_512_k 9) (parse-integer "12835b0145706fbe" :radix 16))
(setf (aref s_sha_512_k 10) (parse-integer "243185be4ee4b28c" :radix 16))
(setf (aref s_sha_512_k 11) (parse-integer "550c7dc3d5ffb4e2" :radix 16))
(setf (aref s_sha_512_k 12) (parse-integer "72be5d74f27b896f" :radix 16))
(setf (aref s_sha_512_k 13) (parse-integer "80deb1fe3b1696b1" :radix 16))
(setf (aref s_sha_512_k 14) (parse-integer "9bdc06a725c71235" :radix 16))
(setf (aref s_sha_512_k 15) (parse-integer "c19bf174cf692694" :radix 16))
(setf (aref s_sha_512_k 16) (parse-integer "e49b69c19ef14ad2" :radix 16))
(setf (aref s_sha_512_k 17) (parse-integer "efbe4786384f25e3" :radix 16))
(setf (aref s_sha_512_k 18) (parse-integer "0fc19dc68b8cd5b5" :radix 16))
(setf (aref s_sha_512_k 19) (parse-integer "240ca1cc77ac9c65" :radix 16))
(setf (aref s_sha_512_k 20) (parse-integer "2de92c6f592b0275" :radix 16))
(setf (aref s_sha_512_k 21) (parse-integer "4a7484aa6ea6e483" :radix 16))
(setf (aref s_sha_512_k 22) (parse-integer "5cb0a9dcbd41fbd4" :radix 16))
(setf (aref s_sha_512_k 23) (parse-integer "76f988da831153b5" :radix 16))
(setf (aref s_sha_512_k 24) (parse-integer "983e5152ee66dfab" :radix 16))
(setf (aref s_sha_512_k 25) (parse-integer "a831c66d2db43210" :radix 16))
(setf (aref s_sha_512_k 26) (parse-integer "b00327c898fb213f" :radix 16))
(setf (aref s_sha_512_k 27) (parse-integer "bf597fc7beef0ee4" :radix 16))
(setf (aref s_sha_512_k 28) (parse-integer "c6e00bf33da88fc2" :radix 16))
(setf (aref s_sha_512_k 29) (parse-integer "d5a79147930aa725" :radix 16))
(setf (aref s_sha_512_k 30) (parse-integer "06ca6351e003826f" :radix 16))
(setf (aref s_sha_512_k 31) (parse-integer "142929670a0e6e70" :radix 16))
(setf (aref s_sha_512_k 32) (parse-integer "27b70a8546d22ffc" :radix 16))
(setf (aref s_sha_512_k 33) (parse-integer "2e1b21385c26c926" :radix 16))
(setf (aref s_sha_512_k 34) (parse-integer "4d2c6dfc5ac42aed" :radix 16))
(setf (aref s_sha_512_k 35) (parse-integer "53380d139d95b3df" :radix 16))
(setf (aref s_sha_512_k 36) (parse-integer "650a73548baf63de" :radix 16))
(setf (aref s_sha_512_k 37) (parse-integer "766a0abb3c77b2a8" :radix 16))
(setf (aref s_sha_512_k 38) (parse-integer "81c2c92e47edaee6" :radix 16))
(setf (aref s_sha_512_k 39) (parse-integer "92722c851482353b" :radix 16))
(setf (aref s_sha_512_k 40) (parse-integer "a2bfe8a14cf10364" :radix 16))
(setf (aref s_sha_512_k 41) (parse-integer "a81a664bbc423001" :radix 16))
(setf (aref s_sha_512_k 42) (parse-integer "c24b8b70d0f89791" :radix 16))
(setf (aref s_sha_512_k 43) (parse-integer "c76c51a30654be30" :radix 16))
(setf (aref s_sha_512_k 44) (parse-integer "d192e819d6ef5218" :radix 16))
(setf (aref s_sha_512_k 45) (parse-integer "d69906245565a910" :radix 16))
(setf (aref s_sha_512_k 46) (parse-integer "f40e35855771202a" :radix 16))
(setf (aref s_sha_512_k 47) (parse-integer "106aa07032bbd1b8" :radix 16))
(setf (aref s_sha_512_k 48) (parse-integer "19a4c116b8d2d0c8" :radix 16))
(setf (aref s_sha_512_k 49) (parse-integer "1e376c085141ab53" :radix 16))
(setf (aref s_sha_512_k 50) (parse-integer "2748774cdf8eeb99" :radix 16))
(setf (aref s_sha_512_k 51) (parse-integer "34b0bcb5e19b48a8" :radix 16))
(setf (aref s_sha_512_k 52) (parse-integer "391c0cb3c5c95a63" :radix 16))
(setf (aref s_sha_512_k 53) (parse-integer "4ed8aa4ae3418acb" :radix 16))
(setf (aref s_sha_512_k 54) (parse-integer "5b9cca4f7763e373" :radix 16))
(setf (aref s_sha_512_k 55) (parse-integer "682e6ff3d6b2b8a3" :radix 16))
(setf (aref s_sha_512_k 56) (parse-integer "748f82ee5defb2fc" :radix 16))
(setf (aref s_sha_512_k 57) (parse-integer "78a5636f43172f60" :radix 16))
(setf (aref s_sha_512_k 58) (parse-integer "84c87814a1f0ab72" :radix 16))
(setf (aref s_sha_512_k 59) (parse-integer "8cc702081a6439ec" :radix 16))
(setf (aref s_sha_512_k 60) (parse-integer "90befffa23631e28" :radix 16))
(setf (aref s_sha_512_k 61) (parse-integer "a4506cebde82bde9" :radix 16))
(setf (aref s_sha_512_k 62) (parse-integer "bef9a3f7b2c67915" :radix 16))
(setf (aref s_sha_512_k 63) (parse-integer "c67178f2e372532b" :radix 16))
(setf (aref s_sha_512_k 64) (parse-integer "ca273eceea26619c" :radix 16))
(setf (aref s_sha_512_k 65) (parse-integer "d186b8c721c0c207" :radix 16))
(setf (aref s_sha_512_k 66) (parse-integer "eada7dd6cde0eb1e" :radix 16))
(setf (aref s_sha_512_k 67) (parse-integer "f57d4f7fee6ed178" :radix 16))
(setf (aref s_sha_512_k 68) (parse-integer "06f067aa72176fba" :radix 16))
(setf (aref s_sha_512_k 69) (parse-integer "0a637dc5a2c898a6" :radix 16))
(setf (aref s_sha_512_k 70) (parse-integer "113f9804bef90dae" :radix 16))
(setf (aref s_sha_512_k 71) (parse-integer "1b710b35131c471b" :radix 16))
(setf (aref s_sha_512_k 72) (parse-integer "28db77f523047d84" :radix 16))
(setf (aref s_sha_512_k 73) (parse-integer "32caab7b40c72493" :radix 16))
(setf (aref s_sha_512_k 74) (parse-integer "3c9ebe0a15c9bebc" :radix 16))
(setf (aref s_sha_512_k 75) (parse-integer "431d67c49c100d4c" :radix 16))
(setf (aref s_sha_512_k 76) (parse-integer "4cc5d4becb3e42b6" :radix 16))
(setf (aref s_sha_512_k 77) (parse-integer "597f299cfc657e2a" :radix 16))
(setf (aref s_sha_512_k 78) (parse-integer "5fcb6fab3ad6faec" :radix 16))
(setf (aref s_sha_512_k 79) (parse-integer "6c44198c4a475817" :radix 16))

(defun sha_512 (data)
  (setf H (make-array 8
		      :element-type '(unsigned-byte 8)
		      :initial-element 0))
  (setf N (ceiling (/ (+ (length data) 17.0) 128.0)))
  (setf number (make-array 8
			   :element-type '(unsigned-byte 8)
			   :initial-element N))
  ;;
  (setf hash (make-array (* 128 N)
			 :element-type '(unsigned-byte 8)
			 :initial-element 0)))
