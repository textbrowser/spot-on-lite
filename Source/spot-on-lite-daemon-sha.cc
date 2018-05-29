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
  {"428a2f98d728ae22", "7137449123ef65cd", "b5c0fbcfec4d3b2f",
   "e9b5dba58189dbbc",
   "3956c25bf348b538", "59f111f1b605d019", "923f82a4af194f9b",
   "ab1c5ed5da6d8118",
   "d807aa98a3030242", "12835b0145706fbe", "243185be4ee4b28c",
   "550c7dc3d5ffb4e2",
   "72be5d74f27b896f", "80deb1fe3b1696b1", "9bdc06a725c71235",
   "c19bf174cf692694",
   "e49b69c19ef14ad2", "efbe4786384f25e3", "0fc19dc68b8cd5b5",
   "240ca1cc77ac9c65",
   "2de92c6f592b0275", "4a7484aa6ea6e483", "5cb0a9dcbd41fbd4",
   "76f988da831153b5",
   "983e5152ee66dfab", "a831c66d2db43210", "b00327c898fb213f",
   "bf597fc7beef0ee4",
   "c6e00bf33da88fc2", "d5a79147930aa725", "06ca6351e003826f",
   "142929670a0e6e70",
   "27b70a8546d22ffc", "2e1b21385c26c926", "4d2c6dfc5ac42aed",
   "53380d139d95b3df",
   "650a73548baf63de", "766a0abb3c77b2a8", "81c2c92e47edaee6",
   "92722c851482353b",
   "a2bfe8a14cf10364", "a81a664bbc423001", "c24b8b70d0f89791",
   "c76c51a30654be30",
   "d192e819d6ef5218", "d69906245565a910", "f40e35855771202a",
   "106aa07032bbd1b8",
   "19a4c116b8d2d0c8", "1e376c085141ab53", "2748774cdf8eeb99",
   "34b0bcb5e19b48a8",
   "391c0cb3c5c95a63", "4ed8aa4ae3418acb", "5b9cca4f7763e373",
   "682e6ff3d6b2b8a3",
   "748f82ee5defb2fc", "78a5636f43172f60", "84c87814a1f0ab72",
   "8cc702081a6439ec",
   "90befffa23631e28", "a4506cebde82bde9", "bef9a3f7b2c67915",
   "c67178f2e372532b",
   "ca273eceea26619c", "d186b8c721c0c207", "eada7dd6cde0eb1e",
   "f57d4f7fee6ed178",
   "06f067aa72176fba", "0a637dc5a2c898a6", "113f9804bef90dae",
   "1b710b35131c471b",
   "28db77f523047d84", "32caab7b40c72493", "3c9ebe0a15c9bebc",
   "431d67c49c100d4c",
   "4cc5d4becb3e42b6", "597f299cfc657e2a", "5fcb6fab3ad6faec",
   "6c44198c4a475817"};

spot_on_lite_daemon_sha::spot_on_lite_daemon_sha(void)
{
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

  cl_object c = c_string_to_object(bytes.constData());

  if(c)
    c = cl_safe_eval(c, Cnil, Cnil);
  else
    return hash;

  if(!c)
    return hash;

  for(int i = 0; i < 8; i++)
    {
      QByteArray h(8, 0);
      cl_object e = ecl_aref(c, i);

      if(e)
	qToBigEndian(ecl_to_uint64_t(e), reinterpret_cast<uchar *> (h.data()));

      hash.append(h);
    }

  return hash;
#else
  /*
  ** Please read the NIST.FIPS.180-4.pdf publication.
  */

  QByteArray number(8, 0);
  QVector<quint64> H;
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

  for(int i = 0; i < 8; i++)
    H << qFromBigEndian<quint64>
      (reinterpret_cast<const uchar *> (s_sha_512_h[i].constData()));

  /*
  ** Computing the hash (6.4.2).
  */

  for(int i = 0; i < N; i++)
    {
      QVector<quint64> M;

      for(int j = 0; j < 128; j += 8)
	M << qFromBigEndian<quint64>
	  (reinterpret_cast<const uchar *> (hash.mid(128 * i + j, 8).
					    constData()));

      QVector<quint64> W;

      for(size_t t = 0; t <= 79; t++)
	if(t <= 15)
	  W << M[t];
	else
	  W << s1_512(W[t - 2]) + W[t - 7] + s0_512(W[t - 15]) + W[t - 16];

      quint64 a = H[0];
      quint64 b = H[1];
      quint64 c = H[2];
      quint64 d = H[3];
      quint64 e = H[4];
      quint64 f = H[5];
      quint64 g = H[6];
      quint64 h = H[7];

      for(size_t t = 0; t <= 79; t++)
	{
	  quint64 K = qFromBigEndian<quint64>
	    (reinterpret_cast<const uchar *> (QByteArray::
					      fromHex(s_sha_512_k[t]).
					      constData()));
	  quint64 T1 = h + S1_512(e) + Ch(e, f, g) + K + W[t];
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

      H[0] += a;
      H[1] += b;
      H[2] += c;
      H[3] += d;
      H[4] += e;
      H[5] += f;
      H[6] += g;
      H[7] += h;
    }

  hash.clear();

  for(size_t i = 0; i < 8; i++)
    {
      QByteArray h(8, 0);

      qToBigEndian(H[i], reinterpret_cast<uchar *> (h.data()));
      hash.append(h);
    }

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

  QByteArray hmac;
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
    left[i] = k.at(i) ^ opad.at(i);

  QByteArray right(block_length, 0);

  for(int i = 0; i < block_length; i++)
    right[i] = k.at(i) ^ ipad.at(i);

  return sha_512(left.append(sha_512(right.append(data))));
}
