/*
** Copyright (c) 2011 - 10^10^10, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Spot-On-Lite without specific prior written permission.
**
** SPOT-ON-LITE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** SPOT-ON-LITE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QtCore>
#include <QtEndian>

#include "spot-on-lite-daemon-sha.h"

#ifdef SPOTON_LITE_DAEMON_CHILD_ECL_SUPPORTED
#ifdef FALSE
#undef FALSE
#endif
#ifdef SLOT
#undef SLOT
#endif
#ifdef TRUE
#undef TRUE
#endif
#ifdef slots
#undef slots
#endif
#include <ecl/ecl.h>
#endif

#define Ch(x, y, z) ((x & y) ^ (~x & z))
#define Maj(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define ROTR(n, x) ((x >> n) | (x << (64 - n)))
#define S0_512(x) (ROTR(28, x) ^ ROTR(34, x) ^ ROTR(39, x))
#define S1_512(x) (ROTR(14, x) ^ ROTR(18, x) ^ ROTR(41, x))
#define SHR(n, x) (x >> n)
#define s0_512(x) (ROTR(1, x) ^ ROTR(8, x) ^ SHR(7, x))
#define s1_512(x) (ROTR(19, x) ^ ROTR(61, x) ^ SHR(6, x))

static QByteArray s_sha_512_h[] = {QByteArray::fromHex("6a09e667f3bcc908"),
				   QByteArray::fromHex("bb67ae8584caa73b"),
				   QByteArray::fromHex("3c6ef372fe94f82b"),
				   QByteArray::fromHex("a54ff53a5f1d36f1"),
				   QByteArray::fromHex("510e527fade682d1"),
				   QByteArray::fromHex("9b05688c2b3e6c1f"),
				   QByteArray::fromHex("1f83d9abfb41bd6b"),
				   QByteArray::fromHex("5be0cd19137e2179")};
static QByteArray s_sha_512_k[] =
  {QByteArray::fromHex("428a2f98d728ae22"),
   QByteArray::fromHex("7137449123ef65cd"),
   QByteArray::fromHex("b5c0fbcfec4d3b2f"),
   QByteArray::fromHex("e9b5dba58189dbbc"),
   QByteArray::fromHex("3956c25bf348b538"),
   QByteArray::fromHex("59f111f1b605d019"),
   QByteArray::fromHex("923f82a4af194f9b"),
   QByteArray::fromHex("ab1c5ed5da6d8118"),
   QByteArray::fromHex("d807aa98a3030242"),
   QByteArray::fromHex("12835b0145706fbe"),
   QByteArray::fromHex("243185be4ee4b28c"),
   QByteArray::fromHex("550c7dc3d5ffb4e2"),
   QByteArray::fromHex("72be5d74f27b896f"),
   QByteArray::fromHex("80deb1fe3b1696b1"),
   QByteArray::fromHex("9bdc06a725c71235"),
   QByteArray::fromHex("c19bf174cf692694"),
   QByteArray::fromHex("e49b69c19ef14ad2"),
   QByteArray::fromHex("efbe4786384f25e3"),
   QByteArray::fromHex("0fc19dc68b8cd5b5"),
   QByteArray::fromHex("240ca1cc77ac9c65"),
   QByteArray::fromHex("2de92c6f592b0275"),
   QByteArray::fromHex("4a7484aa6ea6e483"),
   QByteArray::fromHex("5cb0a9dcbd41fbd4"),
   QByteArray::fromHex("76f988da831153b5"),
   QByteArray::fromHex("983e5152ee66dfab"),
   QByteArray::fromHex("a831c66d2db43210"),
   QByteArray::fromHex("b00327c898fb213f"),
   QByteArray::fromHex("bf597fc7beef0ee4"),
   QByteArray::fromHex("c6e00bf33da88fc2"),
   QByteArray::fromHex("d5a79147930aa725"),
   QByteArray::fromHex("06ca6351e003826f"),
   QByteArray::fromHex("142929670a0e6e70"),
   QByteArray::fromHex("27b70a8546d22ffc"),
   QByteArray::fromHex("2e1b21385c26c926"),
   QByteArray::fromHex("4d2c6dfc5ac42aed"),
   QByteArray::fromHex("53380d139d95b3df"),
   QByteArray::fromHex("650a73548baf63de"),
   QByteArray::fromHex("766a0abb3c77b2a8"),
   QByteArray::fromHex("81c2c92e47edaee6"),
   QByteArray::fromHex("92722c851482353b"),
   QByteArray::fromHex("a2bfe8a14cf10364"),
   QByteArray::fromHex("a81a664bbc423001"),
   QByteArray::fromHex("c24b8b70d0f89791"),
   QByteArray::fromHex("c76c51a30654be30"),
   QByteArray::fromHex("d192e819d6ef5218"),
   QByteArray::fromHex("d69906245565a910"),
   QByteArray::fromHex("f40e35855771202a"),
   QByteArray::fromHex("106aa07032bbd1b8"),
   QByteArray::fromHex("19a4c116b8d2d0c8"),
   QByteArray::fromHex("1e376c085141ab53"),
   QByteArray::fromHex("2748774cdf8eeb99"),
   QByteArray::fromHex("34b0bcb5e19b48a8"),
   QByteArray::fromHex("391c0cb3c5c95a63"),
   QByteArray::fromHex("4ed8aa4ae3418acb"),
   QByteArray::fromHex("5b9cca4f7763e373"),
   QByteArray::fromHex("682e6ff3d6b2b8a3"),
   QByteArray::fromHex("748f82ee5defb2fc"),
   QByteArray::fromHex("78a5636f43172f60"),
   QByteArray::fromHex("84c87814a1f0ab72"),
   QByteArray::fromHex("8cc702081a6439ec"),
   QByteArray::fromHex("90befffa23631e28"),
   QByteArray::fromHex("a4506cebde82bde9"),
   QByteArray::fromHex("bef9a3f7b2c67915"),
   QByteArray::fromHex("c67178f2e372532b"),
   QByteArray::fromHex("ca273eceea26619c"),
   QByteArray::fromHex("d186b8c721c0c207"),
   QByteArray::fromHex("eada7dd6cde0eb1e"),
   QByteArray::fromHex("f57d4f7fee6ed178"),
   QByteArray::fromHex("06f067aa72176fba"),
   QByteArray::fromHex("0a637dc5a2c898a6"),
   QByteArray::fromHex("113f9804bef90dae"),
   QByteArray::fromHex("1b710b35131c471b"),
   QByteArray::fromHex("28db77f523047d84"),
   QByteArray::fromHex("32caab7b40c72493"),
   QByteArray::fromHex("3c9ebe0a15c9bebc"),
   QByteArray::fromHex("431d67c49c100d4c"),
   QByteArray::fromHex("4cc5d4becb3e42b6"),
   QByteArray::fromHex("597f299cfc657e2a"),
   QByteArray::fromHex("5fcb6fab3ad6faec"),
   QByteArray::fromHex("6c44198c4a475817")};

spot_on_lite_daemon_sha::spot_on_lite_daemon_sha(void)
{
  m_K.resize(80);

  for(size_t i = 0; i <= 79; i++)
    m_K[i] = qFromBigEndian<quint64>
      (reinterpret_cast<const uchar *> (s_sha_512_k[i].constData()));
}

QByteArray spot_on_lite_daemon_sha::sha_512(const QByteArray &data) const
{
  QByteArray hash;

#ifdef SPOTON_LITE_DAEMON_CHILD_ECL_SUPPORTED
  QByteArray bytes(QString("(sha_512 '#%1(").arg(data.length()).toLatin1());

  for(int i = 0; i < data.length(); i++)
    {
      bytes.append(QByteArray::number(static_cast<unsigned char> (data.at(i))));
      bytes.append(' ');
    }

  bytes = bytes.mid(0, bytes.length() - 1);
  bytes.append("))");
  ecl_import_current_thread(ECL_NIL, ECL_NIL);

  cl_object c = c_string_to_object(bytes.data()); // constData()?

  if(c)
    c = cl_safe_eval(c, Cnil, Cnil);
  else
    goto done_label;

  if(!c)
    goto done_label;

  for(cl_index i = 0; i < 8; i++)
    {
      QByteArray h(8, 0);
      cl_object e = ecl_aref(c, i);

      if(e)
	qToBigEndian(static_cast<quint64> (ecl_to_uint64_t(e)),
		     reinterpret_cast<uchar *> (h.data()));

      hash.append(h);
    }

 done_label:
  ecl_release_current_thread();
  return hash;
#else
  /*
  ** Please read the NIST.FIPS.180-4.pdf publication.
  */

  QByteArray number(8, 0);
  QVector<quint64> H(8);
  int N = qCeil(static_cast<double> (data.size() + 17) / 128.0);

  /*
  ** Padding the hash (5.1.2).
  */

  qToBigEndian(static_cast<quint64> (8 * data.length()),
	       reinterpret_cast<uchar *> (number.data()));
  hash = QByteArray(128 * N, 0);
  hash.replace(0, data.length(), data);
  hash.replace(data.length(), 1, QByteArray::fromHex("80"));
  hash.replace(hash.length() - number.length(), number.length(), number);

  /*
  ** Initializing H (5.3.5).
  */

  H[0] = qFromBigEndian<quint64>
    (reinterpret_cast<const uchar *> (s_sha_512_h[0].constData()));
  H[1] = qFromBigEndian<quint64>
    (reinterpret_cast<const uchar *> (s_sha_512_h[1].constData()));
  H[2] = qFromBigEndian<quint64>
    (reinterpret_cast<const uchar *> (s_sha_512_h[2].constData()));
  H[3] = qFromBigEndian<quint64>
    (reinterpret_cast<const uchar *> (s_sha_512_h[3].constData()));
  H[4] = qFromBigEndian<quint64>
    (reinterpret_cast<const uchar *> (s_sha_512_h[4].constData()));
  H[5] = qFromBigEndian<quint64>
    (reinterpret_cast<const uchar *> (s_sha_512_h[5].constData()));
  H[6] = qFromBigEndian<quint64>
    (reinterpret_cast<const uchar *> (s_sha_512_h[6].constData()));
  H[7] = qFromBigEndian<quint64>
    (reinterpret_cast<const uchar *> (s_sha_512_h[7].constData()));

  /*
  ** Computing the hash (6.4.2).
  */

  for(int i = 0; i < N; i++)
    {
      QVector<quint64> M(16);

      M[0] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 0, 8).
					  constData()));
      M[1] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 8, 8).
					  constData()));
      M[2] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 16, 8).
					  constData()));
      M[3] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 24, 8).
					  constData()));
      M[4] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 32, 8).
					  constData()));
      M[5] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 40, 8).
					  constData()));
      M[6] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 48, 8).
					  constData()));
      M[7] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 56, 8).
					  constData()));
      M[8] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 64, 8).
					  constData()));
      M[9] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 72, 8).
					  constData()));
      M[10] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 80, 8).
					  constData()));
      M[11] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 88, 8).
					  constData()));
      M[12] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 96, 8).
					  constData()));
      M[13] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 104, 8).
					  constData()));
      M[14] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 112, 8).
					  constData()));
      M[15] = qFromBigEndian<quint64>
	(reinterpret_cast<const uchar *> (hash.mid(128 * i + 120, 8).
					  constData()));

      QVector<quint64> W(M);

      for(int t = 16; t <= 79; t++)
	W << (s1_512(W.at(t - 2)) +
	      W.at(t - 7) +
	      s0_512(W.at(t - 15)) +
	      W.at(t - 16));

      quint64 a = H.at(0);
      quint64 b = H.at(1);
      quint64 c = H.at(2);
      quint64 d = H.at(3);
      quint64 e = H.at(4);
      quint64 f = H.at(5);
      quint64 g = H.at(6);
      quint64 h = H.at(7);

      for(int t = 0; t <= 79; t++)
	{
	  quint64 T1 = h + S1_512(e) + Ch(e, f, g) + m_K.at(t) + W.at(t);
	  quint64 T2 = S0_512(a) + Maj(a, b, c);

	  h = g;
	  g = f;
	  f = e;
	  e = d + T1;
	  d = c;
	  c = b;
	  b = a;
	  a = T1 + T2;
	}

      H.replace(0, H.at(0) + a);
      H.replace(1, H.at(1) + b);
      H.replace(2, H.at(2) + c);
      H.replace(3, H.at(3) + d);
      H.replace(4, H.at(4) + e);
      H.replace(5, H.at(5) + f);
      H.replace(6, H.at(6) + g);
      H.replace(7, H.at(7) + h);
    }

  hash.clear();

  QByteArray h(8, 0);

  qToBigEndian(H.at(0), reinterpret_cast<uchar *> (h.data()));
  hash.append(h);
  qToBigEndian(H.at(1), reinterpret_cast<uchar *> (h.data()));
  hash.append(h);
  qToBigEndian(H.at(2), reinterpret_cast<uchar *> (h.data()));
  hash.append(h);
  qToBigEndian(H.at(3), reinterpret_cast<uchar *> (h.data()));
  hash.append(h);
  qToBigEndian(H.at(4), reinterpret_cast<uchar *> (h.data()));
  hash.append(h);
  qToBigEndian(H.at(5), reinterpret_cast<uchar *> (h.data()));
  hash.append(h);
  qToBigEndian(H.at(6), reinterpret_cast<uchar *> (h.data()));
  hash.append(h);
  qToBigEndian(H.at(7), reinterpret_cast<uchar *> (h.data()));
  hash.append(h);
#endif
  return hash;
}

QByteArray spot_on_lite_daemon_sha::sha_512_hmac(const QByteArray &data,
						 const QByteArray &key) const
{
  /*
  ** Block length is 1024 bits.
  ** Please read https://en.wikipedia.org/wiki/SHA-2.
  */

  QByteArray k(key);
  static int block_length = 1024 / CHAR_BIT;

  if(block_length < k.length())
    k = sha_512(k);

  if(block_length > k.length())
    k.append(QByteArray(block_length - k.length(), 0));

  static QByteArray ipad(block_length, 0x36);
  static QByteArray opad(block_length, 0x5c);

  QByteArray left(block_length, 0);

  for(int i = 0; i < block_length; i++)
    left[i] = static_cast<char> (k.at(i) ^ opad.at(i));

  QByteArray right(block_length, 0);

  for(int i = 0; i < block_length; i++)
    right[i] = static_cast<char> (k.at(i) ^ ipad.at(i));

  return sha_512(left.append(sha_512(right.append(data))));
}
