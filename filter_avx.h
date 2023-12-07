#include <immintrin.h>

typedef union {
	__m256i i256;
    __m128i i128[2];
} __simd_i256 __attribute__((aligned(32)));

static __inline void hybrid_filter_dec(TTA_fltst *fs, TTAint32 *in) {
	TTAint32 *pA = fs->dl;
	TTAint32 *pB = fs->qm;
	TTAint32 *pM = fs->dx;
	TTAint32 sum = fs->round;
	__simd_i256 xa, xb, xm, xd;

	xa.i256 = _mm256_load_si256((__m256i*)pA);
	xb.i256 = _mm256_load_si256((__m256i*)pB);
	xm.i256 = _mm256_load_si256((__m256i*)pM);

	if (fs->error < 0) {
		xb.i256 = _mm256_sub_epi32(xb.i256, xm.i256);
		_mm256_store_si256((__m256i*)pB, xb.i256);
	} else if (fs->error > 0) {
		xb.i256 = _mm256_add_epi32(xb.i256, xm.i256);
		_mm256_store_si256((__m256i*)pB, xb.i256);
	}

	xd.i256 = _mm256_mullo_epi32(xa.i256, xb.i256);
	xd.i128[0] = _mm_add_epi32(xd.i128[0], xd.i128[1]);
	xd.i128[0] = _mm_add_epi32(xd.i128[0], _mm_unpackhi_epi64(xd.i128[0], xd.i128[0]));
	sum += _mm_cvtsi128_si32(xd.i128[0]) + _mm_extract_epi32(xd.i128[0], 1);

	xm.i256 = _mm256_alignr_epi8(_mm256_permute2x128_si256(xm.i256, xm.i256, _MM_SHUFFLE(2, 0, 0, 1)), xm.i256, 4); // ignore hi
	xm.i128[1] = _mm_andnot_si128(_mm_setr_epi32(0, 1, 1, 3),
		_mm_or_si128(_mm_srai_epi32(xa.i128[1], 30), _mm_setr_epi32(1, 2, 2, 4)));
	xa.i256 = _mm256_alignr_epi8(_mm256_permute2x128_si256(xa.i256, xa.i256, _MM_SHUFFLE(2, 0, 0, 1)), xa.i256, 4); // ignore hi

	_mm_store_si128((__m128i*)pA, xa.i128[0]);
	_mm256_store_si256((__m256i*)pM, xm.i256);

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
	__simd_i256 xa, xb, xm, xd;

	xa.i256 = _mm256_load_si256((__m256i*)pA);
	xb.i256 = _mm256_load_si256((__m256i*)pB);
	xm.i256 = _mm256_load_si256((__m256i*)pM);

	if (fs->error < 0) {
		xb.i256 = _mm256_sub_epi32(xb.i256, xm.i256);
		_mm256_store_si256((__m256i*)pB, xb.i256);
	} else if (fs->error > 0) {
		xb.i256 = _mm256_add_epi32(xb.i256, xm.i256);
		_mm256_store_si256((__m256i*)pB, xb.i256);
	}

	xd.i256 = _mm256_mullo_epi32(xa.i256, xb.i256);
	xd.i128[0] = _mm_add_epi32(xd.i128[0], xd.i128[1]);
	xd.i128[0] = _mm_add_epi32(xd.i128[0], _mm_unpackhi_epi64(xd.i128[0], xd.i128[0]));
	sum += _mm_cvtsi128_si32(xd.i128[0]) + _mm_extract_epi32(xd.i128[0], 1);

	xm.i256 = _mm256_alignr_epi8(_mm256_permute2x128_si256(xm.i256, xm.i256, _MM_SHUFFLE(2, 0, 0, 1)), xm.i256, 4); // ignore hi
	xm.i128[1] = _mm_andnot_si128(_mm_setr_epi32(0, 1, 1, 3),
		_mm_or_si128(_mm_srai_epi32(xa.i128[1], 30), _mm_setr_epi32(1, 2, 2, 4)));
	xa.i256 = _mm256_alignr_epi8(_mm256_permute2x128_si256(xa.i256, xa.i256, _MM_SHUFFLE(2, 0, 0, 1)), xa.i256, 4); // ignore hi

	_mm_store_si128((__m128i*)pA, xa.i128[0]);
	_mm256_store_si256((__m256i*)pM, xm.i256);

	pA[4] = -pA[5]; pA[5] = -pA[6];
	pA[6] = *in - pA[7]; pA[7] = *in;
	pA[5] += pA[6]; pA[4] += pA[5];

	*in -= (sum >> fs->shift);
	fs->error = *in;
}
