#include <immintrin.h>

static __inline void hybrid_filter_dec(TTA_fltst *fs, TTAint32 *in) {
	TTAint32 *pA = fs->dl;
	TTAint32 *pB = fs->qm;
	TTAint32 *pM = fs->dx;
	TTAint32 sum = fs->round;
	__m256i xa, xb, xm, xd;
	__m128i xdlo;

	xa = _mm256_load_si256((__m256i*)pA);
	xb = _mm256_load_si256((__m256i*)pB);
	xm = _mm256_load_si256((__m256i*)pM);

	if (fs->error < 0) {
		xb = _mm256_sub_epi32(xb, xm);
		_mm256_store_si256((__m256i*)pB, xb);
	} else if (fs->error > 0) {
		xb = _mm256_add_epi32(xb, xm);
		_mm256_store_si256((__m256i*)pB, xb);
	}

	xd = _mm256_mullo_epi32(xa, xb);
	xdlo = _mm_add_epi32(_mm256_castsi256_si128(xd), _mm256_extracti128_si256(xd, 1));
	xdlo = _mm_add_epi32(xdlo, _mm_unpackhi_epi64(xdlo, xdlo));
	sum += _mm_cvtsi128_si32(xdlo) + _mm_extract_epi32(xdlo, 1);

	xm = _mm256_alignr_epi8(_mm256_permute2x128_si256(xm, xm, _MM_SHUFFLE(2, 0, 0, 1)), xm, 4); // ignore hi
	xm = _mm256_inserti128_si256(xm, _mm_andnot_si128(_mm_setr_epi32(0, 1, 1, 3),
		_mm_or_si128(_mm_srai_epi32(_mm256_extracti128_si256(xa, 1), 30), _mm_setr_epi32(1, 2, 2, 4))), 1);
	xa = _mm256_alignr_epi8(_mm256_permute2x128_si256(xa, xa, _MM_SHUFFLE(2, 0, 0, 1)), xa, 4); // ignore hi

	_mm_store_si128((__m128i*)pA, _mm256_castsi256_si128(xa));
	_mm256_store_si256((__m256i*)pM, xm);

	fs->error = *in;
	*in += (sum >> fs->shift);

	pA[4] = -pA[5]; pA[5] = -pA[6];
	pA[6] = *in - pA[7]; pA[7] = *in;
	pA[5] += pA[6]; pA[4] += pA[5];
}

static __inline void hybrid_filter_enc(TTA_fltst *fs, TTAint32 *in) {
	TTAint32 *pA = fs->dl;
	TTAint32 *pB = fs->qm;
	TTAint32 *pM = fs->dx;
	TTAint32 sum = fs->round;
	__m256i xa, xb, xm, xd;
	__m128i xdlo;

	xa = _mm256_load_si256((__m256i*)pA);
	xb = _mm256_load_si256((__m256i*)pB);
	xm = _mm256_load_si256((__m256i*)pM);

	if (fs->error < 0) {
		xb = _mm256_sub_epi32(xb, xm);
		_mm256_store_si256((__m256i*)pB, xb);
	} else if (fs->error > 0) {
		xb = _mm256_add_epi32(xb, xm);
		_mm256_store_si256((__m256i*)pB, xb);
	}

	xd = _mm256_mullo_epi32(xa, xb);
	xdlo = _mm_add_epi32(_mm256_castsi256_si128(xd), _mm256_extracti128_si256(xd, 1));
	xdlo = _mm_add_epi32(xdlo, _mm_unpackhi_epi64(xdlo, xdlo));
	sum += _mm_cvtsi128_si32(xdlo) + _mm_extract_epi32(xdlo, 1);

	xm = _mm256_alignr_epi8(_mm256_permute2x128_si256(xm, xm, _MM_SHUFFLE(2, 0, 0, 1)), xm, 4); // ignore hi
	xm = _mm256_inserti128_si256(xm, _mm_andnot_si128(_mm_setr_epi32(0, 1, 1, 3),
		_mm_or_si128(_mm_srai_epi32(_mm256_extracti128_si256(xa, 1), 30), _mm_setr_epi32(1, 2, 2, 4))), 1);
	xa = _mm256_alignr_epi8(_mm256_permute2x128_si256(xa, xa, _MM_SHUFFLE(2, 0, 0, 1)), xa, 4); // ignore hi

	_mm_store_si128((__m128i*)pA, _mm256_castsi256_si128(xa));
	_mm256_store_si256((__m256i*)pM, xm);

	pA[4] = -pA[5]; pA[5] = -pA[6];
	pA[6] = *in - pA[7]; pA[7] = *in;
	pA[5] += pA[6]; pA[4] += pA[5];

	*in -= (sum >> fs->shift);
	fs->error = *in;
}
