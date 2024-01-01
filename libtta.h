/*
 * libtta.h
 *
 * Description: TTA1-C++ library interface
 * Copyright (c) 1999-2015 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the COPYING
 * file included in the distribution.
 *
 */

#ifndef _LIBTTA_H
#define _LIBTTA_H

#ifdef __GNUC__
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h> 
#include <stdexcept>

#ifdef CARIBBEAN
#define ALLOW_OS_CODE 1

#include "../../../rmdef/rmdef.h"
#include "../../../rmlibcw/include/rmlibcw.h"
#include "../../../rmcore/include/rmcore.h"
#endif
#else // MSVC
#include <windows.h>
#include <stdexcept>
#endif

#include <functional>
#include <string>

#define MAX_DEPTH 3
#define MAX_BPS (MAX_DEPTH*8)
#define MIN_BPS 16
#define MAX_NCH 6
#define TTA_FIFO_BUFFER_SIZE 5120

#ifdef __GNUC__
#define TTA_EXTERN_API __attribute__((visibility("default")))
#define TTA_ALIGNED(n) __attribute__((aligned(n), packed))
#define __forceinline static __inline
#if defined(__APPLE__)
static __inline void *_aligned_alloc(size_t alignment, size_t size) {
	void *ptr;
	if (posix_memalign(&ptr, alignment, size) == 0) return ptr;
	return nullptr;
}
#endif
#else // MSVC
#define TTA_EXTERN_API __declspec(dllexport)
#define TTA_ALIGNED(n) __declspec(align(n))
#endif

// portability definitions
#ifdef __GNUC__
#ifdef CARIBBEAN
typedef RMint8 (int8_t);
typedef RMint16 (int16_t);
typedef RMint32 (int32_t);
typedef RMint64 (int64_t);
typedef RMuint8 (uint8_t);
typedef RMuint16 (uint16_t);
typedef RMuint32 (uint32_t);
typedef RMuint64 (uint64_t);
#else // GNUC
// typedef int8_t (int8_t);
// typedef int16_t (int16_t);
// typedef int32_t (int32_t);
// typedef int64_t (int64_t);
// typedef uint8_t (uint8_t);
// typedef uint16_t (uint16_t);
// typedef uint32_t (uint32_t);
// typedef uint64_t (uint64_t);
#endif
#else // MSVC
typedef __int8 (int8_t);
typedef __int16 (int16_t);
typedef __int32 (int32_t);
typedef __int64 (int64_t);
typedef unsigned __int8 (uint8_t);
typedef unsigned __int16 (uint16_t);
typedef unsigned __int32 (uint32_t);
typedef unsigned __int64 (uint64_t);
#endif

typedef struct {
	uint32_t format;	// audio format
	uint32_t nch;	// number of channels
	uint32_t bps;	// bits per sample
	uint32_t sps;	// samplerate (sps)
	uint32_t samples;	// data length in samples
} TTA_ALIGNED(16) TTA_info;

typedef struct {
	int32_t index;
	int32_t error;
	int32_t round;
	int32_t shift;
	int32_t qm[8];
	int32_t dx[24];
	int32_t dl[24];
} TTA_ALIGNED(16) TTA_fltst;


namespace tta
{
	// TTA audio format
	#define FORMAT_SIMPLE 1
	#define FORMAT_ENCRYPTED 2

	typedef enum {
		NO_ERROR,	// no known errors found
		OPEN_ERROR,	// can't open file
		FORMAT_ERROR,	// not compatible file format
		FILE_ERROR,	// file is corrupted
		READ_ERROR,	// can't read from input file
		WRITE_ERROR,	// can't write to output file
		SEEK_ERROR,	// file seek error
		MEMORY_ERROR,	// insufficient memory available
		PASSWORD_ERROR,	// password protected file
		NOT_SUPPORTED	// unsupported architecture
	} ERROR;

	typedef enum {
		CPU_ARCH_UNDEFINED,
		CPU_ARCH_IX86_SSE2,
		CPU_ARCH_IX86_SSE3,
		CPU_ARCH_IX86_SSE4_1,
		CPU_ARCH_IX86_SSE4_2,
		CPU_ARCH_IX86_AVX,
		CPU_ARCH_IX86_AVX512,
		CPU_ARCH_ARM,
		CPU_ARCH_AARCH64
	} CPU_ARCH_TYPE;

	// progress callback
	typedef std::function<void(uint32_t, uint32_t, uint32_t)> CALLBACK;

	// architecture type compatibility
	TTA_EXTERN_API CPU_ARCH_TYPE binary_version();

	class TTA_ALIGNED(32) codec_state // avx requires alignment of 32 bytes (for fst)
	{
	public:
		explicit codec_state();
		virtual ~codec_state();
		void init(uint64_t data, int32_t shift, uint32_t k0, uint32_t k1);
		__inline void decode(int32_t* value);
		__inline void encode(int32_t* value);
		__inline uint32_t& k0() { return m_k[0]; }
		__inline uint32_t& k1() { return m_k[1]; }
		__inline uint32_t& sum0() { return m_sum[0]; }
		__inline uint32_t& sum1() { return m_sum[1]; }
	private:
		TTA_fltst m_fltst;
		uint32_t m_k[2];
		uint32_t m_sum[2];
		int32_t m_prev;
	};

	class fileio
	{
	public:
		virtual ~fileio() {}
		virtual int32_t Read(uint8_t *buffer, uint32_t size) = 0;
		virtual int32_t Write(uint8_t *buffer, uint32_t size) = 0;
		virtual int64_t Seek(int64_t offset) = 0;
	};

	class TTA_ALIGNED(16) bufio
	{
	private:
		uint8_t m_buffer[TTA_FIFO_BUFFER_SIZE];
		uint8_t *m_pos;
		uint32_t m_bcount; // count of bits in cache
		uint32_t m_bcache; // bit cache
		uint32_t m_crc;
		uint32_t m_count;
		fileio *m_io;
	public:
		bufio(fileio *io);
		~bufio();

		void io(fileio* io);
		fileio* io() const;

		__inline void reset();
		__inline void reader_start();
		__inline void writer_start();
		__inline uint8_t read_byte();
		__inline uint32_t read_uint16();
		__inline uint32_t read_uint32();
		__inline bool read_crc32();
		__inline int32_t get_value(codec_state& c);
		__inline uint32_t count() const;
		uint32_t read_tta_header(TTA_info *info);
		uint32_t write_tta_header(TTA_info *info);
		void writer_skip_bytes(uint32_t size);
		void writer_done();
		__inline void write_byte(uint32_t value);
		__inline void write_uint16(uint32_t value);
		__inline void write_uint32(uint32_t value);
		__inline void write_crc32();
		__inline void put_value(codec_state& c, int32_t value);
		__inline void flush_bit_cache();

	private:
		void reader_skip_bytes(uint32_t size);
		uint32_t skip_id3v2();
	};


	class codec_base {
	public:
		explicit codec_base(fileio* io);
		virtual ~codec_base();

		virtual void init(TTA_info *info, uint64_t pos, const std::string& password) = 0;
		virtual uint32_t get_rate() = 0;

	protected:
		codec_state m_codec[MAX_NCH]; // codec (1 per channel)
		uint64_t m_data; // codec initialization data
		bufio m_bufio;
		codec_state *m_codec_last;
		uint64_t *seek_table; // the playing position table
		uint32_t format;	// tta data format
		uint32_t rate;	// bitrate (kbps)
		uint64_t offset;	// data start position (header size, bytes)
		uint32_t frames;	// total count of frames
		uint32_t depth;	// bytes per sample
		uint32_t flen_std;	// default frame length in samples
		uint32_t flen_last;	// last frame length in samples
		uint32_t flen;	// current frame length in samples
		uint32_t fnum;	// currently playing frame index
		uint32_t fpos;	// the current position in frame
	};

	/////////////////////// TTA decoder functions /////////////////////////
	class TTA_EXTERN_API tta_decoder : public codec_base {
	public:
		explicit tta_decoder(fileio *io);
		virtual ~tta_decoder();

		void init(TTA_info *info, uint64_t pos, const std::string& password) override;
		void frame_reset(uint32_t frame, fileio *io);
		int process_stream(uint8_t *output, uint32_t out_bytes, CALLBACK callback=nullptr);
		int process_frame(uint32_t in_bytes, uint8_t *output, uint32_t out_bytes);
		void set_position(uint32_t seconds, uint32_t *new_pos);
		uint32_t get_rate() override;

	protected:
		bool seek_allowed;	// seek table flag
		bool read_seek_table();
		void frame_init(uint32_t frame, bool seek_needed);
	}; // class tta_decoder

	/////////////////////// TTA encoder functions /////////////////////////
	class TTA_EXTERN_API tta_encoder : public codec_base {
	public:
		explicit tta_encoder(fileio *io);
		virtual ~tta_encoder();

		void init(TTA_info *info, uint64_t pos, const std::string& password) override;
		void frame_reset(uint32_t frame, fileio *io);
		void process_stream(uint8_t *input, uint32_t in_bytes, CALLBACK callback=nullptr);
		void process_frame(uint8_t *input, uint32_t in_bytes);
		void finalize();
		uint32_t get_rate() override;

	protected:
		uint32_t shift_bits; // packing int to pcm

		void write_seek_table();
		void frame_init(uint32_t frame);
	}; // class tta_encoder

	//////////////////////// TTA exception class //////////////////////////
	class exception : public std::exception {
		ERROR err_code;

	public:
		explicit exception(ERROR code) : err_code(code) {}
		ERROR code() const { return err_code; }
	}; // class exception
} // namespace tta

#endif // _LIBTTA_H
