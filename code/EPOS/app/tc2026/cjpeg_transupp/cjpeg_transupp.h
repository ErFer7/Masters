#pragma once

/*
  this program is part of the taclebench benchmark suite.
  version v 2.0

  name: jpeglib

  author: thomas g. lane

  function: this file defines the application interface for the jpeg library.
    most applications using the library need only include this file, and perhaps
    jerror.h if they want to know the exact error codes.

  source: mediabench ii
          http://euler.slu.edu/~fritts/mediabench (mirror)

  original name: cjpeg

  changes: no major functional changes.

  license: see the accompanying readme file.

*/

namespace CJpegTransupp {

/* The basic DCT block is 8x8 samples */
#define DCTSIZE 8

/* DCTSIZE squared; # of elements in a block */
#define DCTSIZE2 64

class CJpegTransupp {
   private:
    /*
      Data structures for images (arrays of samples and of DCT coefficients).
      On 80x86 machines, the image arrays are too big for near pointers, but the
      pointer arrays can fit in near memory.
    */

    /* ptr to one image row of pixel samples. */
    typedef unsigned char *JSAMPROW;

    /* ptr to some rows (a 2-D sample array) */
    typedef JSAMPROW *JSAMPARRAY;

    /* one block of coefficients */
    typedef signed char JBLOCK[DCTSIZE2];

    /* pointer to one row of coefficient blocks */
    typedef JBLOCK *JBLOCKROW;

    /* a 2-D array of coefficient blocks */
    typedef JBLOCKROW *JBLOCKARRAY;

    /* useful in a couple of places */
    typedef signed char *JCOEFPTR;

    /*
      DCT coefficient quantization tables.
    */

    struct JQUANT_TBL {
        /* quantization step for each coefficient */
        /*
          This array gives the coefficient quantizers in natural array order (not the
          zigzag order in which they are stored in a JPEG DQT marker).
          CAUTION: IJG versions prior to v6a kept this array in zigzag order.
        */
        unsigned short quantval[DCTSIZE2];

        /* 1 when table has been output */
        /*
          This field is used only during compression. It's initialized 0 when the
          table is created, and set 1 when it's been output to the file. You could
          suppress output of a table by setting this to 1. (See jpeg_suppress_tables
          for an example.)
        */
        int sent_table;
    };

    /*
      Huffman coding tables.
    */

    struct JHUFF_TBL {
        /* These two fields directly represent the contents of a JPEG DHT marker */

        /* bits[k] = # of symbols with codes of */
        /* length k bits; bits[0] is unused */
        unsigned char bits[17];

        /* The symbols, in order of incr code length */
        /*
          This field is used only during compression. It's initialized 0 when the
          table is created, and set 1 when it's been output to the file. You could
          suppress output of a table by setting this to 1. (See jpeg_suppress_tables
          for an example.)
        */
        unsigned char huffval[256];

        /* 1 when table has been output */
        int sent_table;
    };

    /*
      Basic info about one component (color channel).
    */

    struct jpeg_component_info {
        /*
          These values are fixed over the whole image. For compression, they must be
          supplied by parameter setup; for decompression, they are read from the SOF
          marker.
        */

        /* identifier for this component (0..255) */
        int component_id;

        /* its index in SOF or cinfo->comp_info[] */
        int component_index;

        /* horizontal sampling factor (1..4) */
        int h_samp_factor;

        /* vertical sampling factor (1..4) */
        int v_samp_factor;

        /* quantization table selector (0..3) */
        int quant_tbl_no;

        /*
          These values may vary between scans. For compression, they must be supplied
          by parameter setup; for decompression, they are read from the SOS marker.
          The decompressor output side may not use these variables.
        */

        /* DC entropy table selector (0..3) */
        int dc_tbl_no;

        /* AC entropy table selector (0..3) */
        int ac_tbl_no;

        /* Remaining fields should be treated as private by applications. */

        /*
          These values are computed during compression or decompression startup:
          Component's size in DCT blocks. Any dummy blocks added to complete an MCU
          are not counted; therefore these values do not depend on whether a scan is
          interleaved or not.
        */
        unsigned int width_in_blocks;
        unsigned int height_in_blocks;

        /*
          Size of a DCT block in samples. Always DCTSIZE for compression. For
          decompression this is the size of the output from one DCT block, reflecting
          any scaling we choose to apply during the IDCT step. Values of 1,2,4,8 are
          likely to be supported. Note that different components may receive different
          IDCT scalings.
        */
        int DCT_scaled_size;

        /*
          The downsampled dimensions are the component's actual, unpadded number of
          samples at the main buffer (preprocessing/compression interface), thus
          downsampled_width = ceil(image_width * Hi/Hmax) and similarly for height.
          For decompression, IDCT scaling is included, so
          downsampled_width = ceil(image_width * Hi/Hmax * DCT_scaled_size/DCTSIZE)
        */

        /* actual width in samples */
        unsigned int downsampled_width;

        /* actual height in samples */
        unsigned int downsampled_height;

        /*
          This flag is used only for decompression. In cases where some of the
          components will be ignored (eg grayscale output from YCbCr image), we can
          skip most computations for the unused components.
        */

        /* do we need the value of this component? */
        int component_needed;

        /*
          These values are computed before starting a scan of the component. The
          decompressor output side may not use these variables.
        */

        /* number of blocks per MCU, horizontally */
        int MCU_width;

        /* number of blocks per MCU, vertically */
        int MCU_height;

        /* MCU_width * MCU_height */
        int MCU_blocks;

        /* MCU width in samples, MCU_width*DCT_scaled_size */
        int MCU_sample_width;

        /* # of non-dummy blocks across in last MCU */
        int last_col_width;

        /* # of non-dummy blocks down in last MCU */
        int last_row_height;

        /*
          Saved quantization table for component; (void*)0 if none yet saved. See
          jdinput.c comments about the need for this information. This field is
          currently used only for decompression.
        */
        JQUANT_TBL *quant_table;

        /* Private per-component storage for DCT or IDCT subsystem. */
        void *dct_table;
    };

    /*
      The script for encoding a multiple-scan file is an array of these:
    */

    struct jpeg_scan_info {
        /* number of components encoded in this scan */
        int comps_in_scan;

        /* their SOF/comp_info[] indexes */
        int component_index[4];

        /* progressive JPEG spectral selection parms */
        int Ss, Se;

        /* progressive JPEG successive approx. parms */
        int Ah, Al;
    };

    /*
      Known color spaces.
    */

    enum J_COLOR_SPACE {
        /* error/unspecified */
        JCS_UNKNOWN,

        /* monochrome */
        JCS_GRAYSCALE,

        /* red/green/blue */
        JCS_RGB,

        /* Y/Cb/Cr (also known as YUV) */
        JCS_YCbCr,

        /* C/M/Y/K */
        JCS_CMYK,

        /* Y/Cb/Cr/K */
        JCS_YCCK
    };

    /*
      DCT/IDCT algorithm options.
    */

    enum J_DCT_METHOD {
        /* slow but accurate integer algorithm */
        JDCT_ISLOW,

        /* faster, less accurate integer method */
        JDCT_IFAST,

        /* floating-point: accurate, fast on fast HW */
        JDCT_FLOAT
    };

    /*
      Common fields between JPEG compression and decompression master structs.
    */

#define jpeg_common_fields                       \
    /* Error handler module */                   \
    struct jpeg_error_mgr *err;                  \
    /* Memory manager module */                  \
    struct jpeg_memory_mgr *mem;                 \
    /* Progress monitor, or (void*)0 if none */  \
    struct jpeg_progress_mgr *progress;          \
    /* Available for use by application */       \
    void *client_data;                           \
    /* So common code can tell which is which */ \
    int is_decompressor;                         \
    /* For checking call sequence validity */    \
    int global_state

    /*
      Routines that are to be used by both halves of the library are declared to
      receive a pointer to this structure. There are no actual instances of
      jpeg_common_struct, only of jpeg_compress_struct and jpeg_decompress_struct.
    */

    struct jpeg_common_struct {
        /* Fields common to both master struct types */
        jpeg_common_fields;

        /*
          Additional fields follow in an actual jpeg_compress_struct or
          jpeg_decompress_struct. All three structs must agree on these initial
          fields!  (This would be a lot cleaner in C++.)
        */
    };

    struct jpeg_common_struct;
    struct jpeg_compress_struct;
    struct jpeg_decompress_struct;

    typedef struct jpeg_common_struct *j_common_ptr;
    typedef struct jpeg_compress_struct *j_compress_ptr;
    typedef struct jpeg_decompress_struct *j_decompress_ptr;

    /*
      Master record for a compression instance
    */

    struct jpeg_compress_struct {
        /* Fields shared with jpeg_decompress_struct */
        jpeg_common_fields;

        /* Destination for compressed data */
        struct jpeg_destination_mgr *dest;

        /*
          Description of source image --- these fields must be filled in by outer
          application before starting compression. in_color_space must be correct
          before you can even call jpeg_set_defaults().
        */

        /* input image width */
        unsigned int image_width;

        /* input image height */
        unsigned int image_height;

        /* # of color components in input image */
        int input_components;

        /* colorspace of input image */
        J_COLOR_SPACE in_color_space;

        /* image gamma of input image */
        double input_gamma;

        /*
          Compression parameters --- these fields must be set before calling
          jpeg_start_compress(). We recommend calling jpeg_set_defaults() to
          initialize everything to reasonable defaults, then changing anything the
          application specifically wants to change. That way you won't get burnt when
          new parameters are added. Also note that there are several helper routines
          to simplify changing parameters.
        */

        /* bits of precision in image data */
        int data_precision;

        /* # of color components in JPEG image */
        int num_components;

        /* colorspace of JPEG image */
        J_COLOR_SPACE jpeg_color_space;

        /* comp_info[i] describes component that appears i'th in SOF */
        jpeg_component_info *comp_info;

        /* ptrs to coefficient quantization tables, or (void*)0 if not defined */
        JQUANT_TBL *quant_tbl_ptrs[4];

        /* ptrs to Huffman coding tables, or (void*)0 if not defined */
        JHUFF_TBL *dc_huff_tbl_ptrs[4];
        JHUFF_TBL *ac_huff_tbl_ptrs[4];

        /* L values for DC arith-coding tables */
        unsigned char arith_dc_L[16];

        /* U values for DC arith-coding tables */
        unsigned char arith_dc_U[16];

        /* Kx values for AC arith-coding tables */
        unsigned char arith_ac_K[16];

        /* # of entries in scan_info array */
        int num_scans;

        /*
          script for multi-scan file, or (void*)0
          The default value of scan_info is (void*)0, which causes a single-scan
          sequential JPEG file to be emitted. To create a multi-scan file, set
          num_scans and scan_info to point to an array of scan definitions.
        */
        const jpeg_scan_info *scan_info;

        /* 1=caller supplies downsampled data */
        int raw_data_in;

        /* 1=arithmetic coding, 0=Huffman */
        int arith_code;

        /* 1=optimize entropy encoding parms */
        int optimize_coding;

        /* 1=first samples are cosited */
        int CCIR601_sampling;

        /* 1..100, or 0 for no input smoothing */
        int smoothing_factor;

        /* DCT algorithm selector */
        J_DCT_METHOD dct_method;

        /*
          The restart interval can be specified in absolute MCUs by setting
          restart_interval, or in MCU rows by setting restart_in_rows (in which case
          the correct restart_interval will be figured for each scan).
        */

        /* MCUs per restart, or 0 for no restart */
        unsigned int restart_interval;

        /* if > 0, MCU rows per restart interval */
        int restart_in_rows;

        /*
          Parameters controlling emission of special markers.
        */

        /* should a JFIF marker be written? */
        int write_JFIF_header;

        /* What to write for the JFIF version number */
        unsigned char JFIF_major_version;
        unsigned char JFIF_minor_version;

        /*
          These three values are not used by the JPEG code, merely copied into the
          JFIF APP0 marker. density_unit can be 0 for unknown,  1 for dots/inch, or 2
          for dots/cm. Note that the pixel aspect ratio is defined by
          X_density/Y_density even when density_unit=0.
        */

        /* JFIF code for pixel size units */
        unsigned char density_unit;

        /* Horizontal pixel density */
        unsigned short X_density;

        /* Vertical pixel density */
        unsigned short Y_density;

        /* should an Adobe marker be written? */
        int write_Adobe_marker;

        /*
          State variable: index of next scanline to be written to
          jpeg_write_scanlines(). Application may use this to control its processing
          loop, e.g., "while (next_scanline < image_height)".
        */

        /* 0 .. image_height-1  */
        unsigned int next_scanline;

        /*
          Remaining fields are known throughout compressor, but generally should not
          be touched by a surrounding application.
        */

        /*
          These fields are computed during compression startup
        */

        /* 1 if scan script uses progressive mode */
        int progressive_mode;

        /* largest h_samp_factor */
        int max_h_samp_factor;

        /* largest v_samp_factor */
        int max_v_samp_factor;

        /*
          # of iMCU rows to be input to coef ctlr
          The coefficient controller receives data in units of MCU rows as defined for
          fully interleaved scans (whether the JPEG file is interleaved or not). There
          are v_samp_factor * DCTSIZE sample rows of each component in an "iMCU"
          (interleaved MCU) row.
        */
        unsigned int total_iMCU_rows;

        /*
           These fields are valid during any one scan. They describe the components
           and MCUs actually appearing in the scan.
        */

        /* # of JPEG components in this scan */
        int comps_in_scan;

        /* *cur_comp_info[i] describes component that appears i'th in SOS */
        jpeg_component_info *cur_comp_info[4];

        /* # of MCUs across the image */
        unsigned int MCUs_per_row;

        /* # of MCU rows in the image */
        unsigned int MCU_rows_in_scan;

        /* # of DCT blocks per MCU */
        int blocks_in_MCU;

        /*
          MCU_membership[i] is index in cur_comp_info of component owning i'th block
          in an MCU
        */
        int MCU_membership[10];

        /* progressive JPEG parameters for scan */
        int Ss, Se, Ah, Al;

        /*
           Links to compression subobjects (methods and private variables of modules)
        */

        struct jpeg_comp_master *master;
        struct jpeg_c_main_controller *main;
        struct jpeg_c_prep_controller *prep;
        struct jpeg_c_coef_controller *coef;
        struct jpeg_marker_writer *marker;
        struct jpeg_color_converter *cconvert;
        struct jpeg_downsampler *downsample;
        struct jpeg_forward_dct *fdct;
        struct jpeg_entropy_encoder *entropy;

        /* workspace for jpeg_simple_progression */
        jpeg_scan_info *script_space;

        int script_space_size;
    };

    /*
      "Object" declarations for JPEG modules that may be supplied or called directly
      by the surrounding application. As with all objects in the JPEG library, these
      structs only define the publicly visible methods and state variables of a
      module. Additional private fields may exist after the public ones.
    */

    /*
      Error handler object
    */

    struct jpeg_error_mgr {
        /* Error exit handler: does not return to caller */
        void (*error_exit)(j_common_ptr cinfo);

        /* Conditionally emit a trace or warning message */
        void (*emit_message)(j_common_ptr cinfo, int msg_level);

        /* Routine that actually outputs a trace or error message */
        void (*output_message)(j_common_ptr cinfo);

        /* Format a message string for the most recent JPEG error or message */
        void (*format_message)(j_common_ptr cinfo, char *buffer);

        /* Reset error state variables at start of a new image */
        void (*reset_error_mgr)(j_common_ptr cinfo);

        /*
          The message ID code and any parameters are saved here. A message can have
          one string parameter or up to 8 int parameters.
        */
        int msg_code;

        union {
            int i[8];
            char s[80];
        } msg_parm;

        /*
          Standard state variables for error facility
        */

        /* max msg_level that will be displayed */
        int trace_level;

        /*
          For recoverable corrupt-data errors, we emit a warning message, but keep
          going unless emit_message chooses to abort. emit_message should count
          warnings in num_warnings. The surrounding application can check for bad data
          by seeing if num_warnings is nonzero at the end of processing.
        */

        /* number of corrupt-data warnings */
        long num_warnings;

        /*
          These fields point to the table(s) of error message strings. An application
          can change the table pointer to switch to a different message list
          (typically, to change the language in which errors are reported). Some
          applications may wish to add additional error codes that will be handled by
          the JPEG library error mechanism; the second table pointer is used for this
          purpose.

           First table includes all errors generated by JPEG library itself. Error
           code 0 is reserved for a "no such error string" message.
        */

        /* Library errors */
        const char *const *jpeg_message_table;

        /* Table contains strings 0..last_jpeg_message */
        int last_jpeg_message;

        /*
          Second table can be added by application (see cjpeg/djpeg for example). It
          contains strings numbered first_addon_message..last_addon_message.
        */

        /* Non-library errors */
        const char *const *addon_message_table;

        /* code for first string in addon table */
        int first_addon_message;

        /* code for last string in addon table */
        int last_addon_message;
    };

    /*
      Progress monitor object
    */

    struct jpeg_progress_mgr {
        void (*progress_monitor)(j_common_ptr cinfo);

        /* work units completed in this pass */
        long pass_counter;

        /* total number of work units in this pass */
        long pass_limit;

        /* passes completed so far */
        int completed_passes;

        /* total number of passes expected */
        int total_passes;
    };

    /*
      Data destination object for compression
    */

    struct jpeg_destination_mgr {
        /* => next byte to write in buffer */
        unsigned char *next_output_byte;

        /* # of byte spaces remaining in buffer */
        long unsigned int free_in_buffer;

        void (*init_destination)(j_compress_ptr cinfo);
        int (*empty_output_buffer)(j_compress_ptr cinfo);
        void (*term_destination)(j_compress_ptr cinfo);
    };

    /*
      Memory manager object.
      Allocates "small" objects (a few K total), "large" objects (tens of K), and
      "really big" objects (virtual arrays with backing store if needed). The memory
      manager does not allow individual objects to be freed; rather, each created
      object is assigned to a pool, and whole pools can be freed at once. This is
      faster and more convenient than remembering exactly what to free, especially
      where malloc()/free() are not too speedy.
      NB: alloc routines never return (void*)0. They exit to error_exit if not
      successful.
    */

    typedef struct jvirt_sarray_control *jvirt_sarray_ptr;
    typedef struct jvirt_barray_control *jvirt_barray_ptr;

    struct jpeg_memory_mgr {
        /*
          Method pointers
        */
        void *(*alloc_small)(j_common_ptr cinfo, int pool_id, long unsigned int sizeofobject);

        void *(*alloc_large)(j_common_ptr cinfo, int pool_id, long unsigned int sizeofobject);

        JSAMPARRAY (*alloc_sarray)(j_common_ptr cinfo, int pool_id, unsigned int samplesperrow, unsigned int numrows);

        JBLOCKARRAY (*alloc_barray)(j_common_ptr cinfo, int pool_id, unsigned int blocksperrow, unsigned int numrows);

        jvirt_sarray_ptr (*request_virt_sarray)(j_common_ptr cinfo,
                                                int pool_id,
                                                int pre_zero,
                                                unsigned int samplesperrow,
                                                unsigned int numrows,
                                                unsigned int maxaccess);

        jvirt_barray_ptr (*request_virt_barray)(j_common_ptr cinfo,
                                                int pool_id,
                                                int pre_zero,
                                                unsigned int blocksperrow,
                                                unsigned int numrows,
                                                unsigned int maxaccess);

        void (*realize_virt_arrays)(j_common_ptr cinfo);

        JSAMPARRAY (*access_virt_sarray)(j_common_ptr cinfo,
                                         jvirt_sarray_ptr ptr,
                                         unsigned int start_row,
                                         unsigned int num_rows,
                                         int writable);

        JBLOCKARRAY (*access_virt_barray)(j_common_ptr cinfo,
                                          jvirt_barray_ptr ptr,
                                          unsigned int start_row,
                                          unsigned int num_rows,
                                          int writable);

        void (*free_pool)(j_common_ptr cinfo, int pool_id);

        void (*self_destruct)(j_common_ptr cinfo);

        /*
          Limit on memory allocation for this JPEG object. (Note that this is merely
          advisory, not a guaranteed maximum; it only affects the space used for
          virtual-array buffers.) May be changed by outer application after creating
          the JPEG object.
        */
        long max_memory_to_use;

        /* Maximum allocation request accepted by alloc_large. */
        long max_alloc_chunk;
    };

   public:
    CJpegTransupp() {
        int i;

        cjpeg_transupp_dstinfo.max_h_samp_factor = 2;
        cjpeg_transupp_dstinfo.max_v_samp_factor = 2;
        cjpeg_transupp_dstinfo.num_components = 3;

        cjpeg_transupp_initSeed();

        for (i = 0; i < 256; i++) cjpeg_transupp_input[i] = cjpeg_transupp_randomInteger();

        for (i = 0; i < 80; i++) cjpeg_transupp_input2[i] = cjpeg_transupp_randomInteger();

        for (i = 0; i < 65; i++) cjpeg_transupp_input3[i] = cjpeg_transupp_randomInteger();

        for (i = 0; i < 65; i++) cjpeg_transupp_input3_2[i] = cjpeg_transupp_randomInteger();

        for (i = 0; i < 64; i++) cjpeg_transupp_input4[i] = cjpeg_transupp_randomInteger();

        for (i = 0; i < 65; i++) cjpeg_transupp_input5[i] = cjpeg_transupp_randomInteger();

        for (i = 0; i < 65; i++) cjpeg_transupp_input5_2[i] = cjpeg_transupp_randomInteger();
    }

    ~CJpegTransupp() {}

    inline int run() {
        cjpeg_transupp_dstinfo.image_width = 227;
        cjpeg_transupp_dstinfo.image_height = 149;

        cjpeg_transupp_do_flip_v(&cjpeg_transupp_dstinfo);

        cjpeg_transupp_dstinfo.image_width = 149;
        cjpeg_transupp_dstinfo.image_height = 227;

        cjpeg_transupp_do_rot_90(&cjpeg_transupp_dstinfo);
        cjpeg_transupp_do_rot_270(&cjpeg_transupp_dstinfo);

        cjpeg_transupp_dstinfo.image_width = 227;
        cjpeg_transupp_dstinfo.image_height = 149;

        cjpeg_transupp_do_rot_180(&cjpeg_transupp_dstinfo);

        cjpeg_transupp_dstinfo.image_width = 149;
        cjpeg_transupp_dstinfo.image_height = 227;

        cjpeg_transupp_do_transverse(&cjpeg_transupp_dstinfo);

        return cjpeg_transupp_return() - 1624 != 0;
    }

   private:
    int cjpeg_transupp_return(void) {
        int checksum = 0;
        unsigned int i;

        for (i = 0; i < 512; i++) checksum += cjpeg_transupp_output_data[i];

        for (i = 0; i < 512; i++) checksum += cjpeg_transupp_output_data2[i];

        for (i = 0; i < 512; i++) checksum += cjpeg_transupp_output_data3[i];

        for (i = 0; i < 512; i++) checksum += cjpeg_transupp_output_data4[i];

        for (i = 0; i < 512; i++) checksum += cjpeg_transupp_output_data5[i];

        return (checksum);
    }

    /*
  Initialization- and return-value-related functions
*/

    void cjpeg_transupp_initSeed(void) { cjpeg_transupp_seed = 0; }

    /*
      cjpeg_transupp_RandomInteger generates random integers between -128 and 127.
    */
    signed char cjpeg_transupp_randomInteger(void) {
        cjpeg_transupp_seed = (((cjpeg_transupp_seed * 133) + 81) % 256) - 128;
        return (cjpeg_transupp_seed);
    }

    /*
      Algorithm core functions
    */

    /*
      Vertical flip
    */
    void cjpeg_transupp_do_flip_v(j_compress_ptr dstinfo) {
        unsigned int MCU_rows, comp_height, dst_blk_x, dst_blk_y;
        int ci, i, j, offset_y;
        JCOEFPTR src_ptr, dst_ptr;

        /*
          We output into a separate array because we can't touch different rows of the
          source virtual array simultaneously. Otherwise, this is a pretty
          straightforward analog of horizontal flip.
          Within a DCT block, vertical mirroring is done by changing the signs of
          odd-numbered rows.
          Partial iMCUs at the bottom edge are copied verbatim.
        */
        MCU_rows = dstinfo->image_height / (dstinfo->max_v_samp_factor * DCTSIZE);

        int compptr_v_samp_factor = 8;
        unsigned int compptr_height_in_blocks = 19;
        unsigned int compptr_width_in_blocks = 29;

        for (ci = 0; ci < dstinfo->num_components; ci++, compptr_v_samp_factor = 1, compptr_width_in_blocks = 15) {
            comp_height = MCU_rows * compptr_v_samp_factor;

            compptr_height_in_blocks = 10;
            for (dst_blk_y = 0; dst_blk_y < compptr_height_in_blocks; dst_blk_y += compptr_v_samp_factor) {
                for (offset_y = 0; offset_y < compptr_v_samp_factor; offset_y++) {
                    if (dst_blk_y < comp_height) {
                        /* Row is within the mirrorable area. */
                        for (dst_blk_x = 0; dst_blk_x < compptr_width_in_blocks; dst_blk_x++) {
                            src_ptr = cjpeg_transupp_input;
                            dst_ptr = cjpeg_transupp_output_data;

                            for (i = 0; i < DCTSIZE; i += 2) {
                                /* copy even row */
                                j = 0;
                                do {
                                    if (dst_blk_x < comp_height) *dst_ptr++ = *src_ptr++;
                                    j++;
                                } while (j < DCTSIZE);

                                /* copy odd row with sign change */
                                j = 0;
                                do {
                                    if (dst_blk_x < comp_height) *dst_ptr++ = -*src_ptr++;
                                    j++;
                                } while (j < DCTSIZE);
                            }
                        }
                    }
                }
            }
        }
    }

    /*
      90 degree rotation is equivalent to
       1. Transposing the image;
       2. Horizontal mirroring.
      These two steps are merged into a single processing routine.
    */
    void cjpeg_transupp_do_rot_90(j_compress_ptr dstinfo) {
        unsigned int MCU_cols, comp_width, dst_blk_x, dst_blk_y;
        int ci, i, j, offset_x, offset_y;
        JCOEFPTR src_ptr, dst_ptr;

        /*
          Because of the horizontal mirror step, we can't process partial iMCUs at the
          (output) right edge properly. They just get transposed and not mirrored.
        */
        MCU_cols = dstinfo->image_width / (dstinfo->max_h_samp_factor * DCTSIZE);

        int compptr_h_samp_factor = 2;
        int compptr_v_samp_factor = 8;
        unsigned int compptr_height_in_blocks = 29;
        unsigned int compptr_width_in_blocks = 19;

        for (ci = 0; ci < dstinfo->num_components; ci++,
            compptr_h_samp_factor = compptr_v_samp_factor = 1,
            compptr_height_in_blocks = 15,
            compptr_width_in_blocks = 10) {
            comp_width = MCU_cols * compptr_h_samp_factor;

            for (dst_blk_y = 0; dst_blk_y < compptr_height_in_blocks; dst_blk_y += compptr_v_samp_factor) {
                offset_y = 0;
                for (; offset_y < compptr_v_samp_factor; offset_y++) {
                    dst_blk_x = 0;
                    for (; dst_blk_x < compptr_width_in_blocks; dst_blk_x += compptr_h_samp_factor) {
                        offset_x = 0;
                        for (; offset_x < compptr_h_samp_factor; offset_x++) {
                            src_ptr = cjpeg_transupp_input2;

                            if (dst_blk_x < comp_width) {
                                /* Block is within the mirrorable area. */
                                dst_ptr = cjpeg_transupp_output_data2;

                                for (i = 0; i < DCTSIZE; i++) {
                                    j = 0;
                                    for (; j < DCTSIZE; j++) dst_ptr[j * DCTSIZE + i] = src_ptr[i * DCTSIZE + j];

                                    i++;

                                    for (j = 0; j < DCTSIZE; j++) dst_ptr[j * DCTSIZE + i] = -src_ptr[i * DCTSIZE + j];
                                }
                            } else {
                                /* Edge blocks are transposed but not mirrored. */
                                dst_ptr = cjpeg_transupp_output_data2;

                                for (i = 0; i < DCTSIZE; i++) j = 0;
                                for (; j < DCTSIZE; j++) {
                                    if (dst_blk_y < comp_width) dst_ptr[j * DCTSIZE + i] = src_ptr[i * DCTSIZE + j];
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /*
      270 degree rotation is equivalent to
        1. Horizontal mirroring;
        2. Transposing the image.
      These two steps are merged into a single processing routine.
    */
    void cjpeg_transupp_do_rot_270(j_compress_ptr dstinfo) {
        unsigned int MCU_rows, comp_height, dst_blk_x, dst_blk_y;
        int ci, i, j, offset_x, offset_y;
        JCOEFPTR src_ptr, dst_ptr;

        /*
          Because of the horizontal mirror step, we can't process partial iMCUs at the
          (output) bottom edge properly. They just get transposed and not mirrored.
        */
        MCU_rows = dstinfo->image_height / (dstinfo->max_v_samp_factor * DCTSIZE);

        int compptr_h_samp_factor = 2;
        int compptr_v_samp_factor = 8;
        unsigned int compptr_height_in_blocks = 29;
        unsigned int compptr_width_in_blocks = 19;

        for (ci = 0; ci < dstinfo->num_components; ci++,
            compptr_h_samp_factor = compptr_v_samp_factor = 1,
            compptr_height_in_blocks = 15,
            compptr_width_in_blocks = 10) {
            comp_height = MCU_rows * compptr_v_samp_factor;

            for (dst_blk_y = 0; dst_blk_y < compptr_height_in_blocks; dst_blk_y += compptr_v_samp_factor) {
                offset_y = 0;
                for (; offset_y < compptr_v_samp_factor; offset_y++) {
                    dst_blk_x = 0;
                    for (; dst_blk_x < compptr_width_in_blocks; dst_blk_x += compptr_h_samp_factor) {
                        offset_x = 0;
                        for (; offset_x < compptr_h_samp_factor; offset_x++) {
                            dst_ptr = cjpeg_transupp_output_data3;

                            if (dst_blk_y < comp_height) {
                                /* Block is within the mirrorable area. */
                                src_ptr = cjpeg_transupp_input3;

                                for (i = 0; i < DCTSIZE; i++) {
                                    j = 0;
                                    for (; j < DCTSIZE; j++) {
                                        dst_ptr[j * DCTSIZE + i] = src_ptr[i * DCTSIZE + j];
                                        j++;
                                        dst_ptr[j * DCTSIZE + i] = -src_ptr[i * DCTSIZE + j];
                                    }
                                }
                            } else {
                                /* Edge blocks are transposed but not mirrored. */
                                src_ptr = cjpeg_transupp_input3_2;

                                for (i = 0; i < DCTSIZE; i++) j = 0;
                                for (; j < DCTSIZE; j++) {
                                    if (dst_blk_y < comp_height) dst_ptr[j * DCTSIZE + i] = src_ptr[i * DCTSIZE + j];
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /*
      180 degree rotation is equivalent to
        1. Vertical mirroring;
        2. Horizontal mirroring.
      These two steps are merged into a single processing routine.
    */
    void cjpeg_transupp_do_rot_180(j_compress_ptr dstinfo) {
        unsigned int MCU_cols, MCU_rows, comp_width, comp_height, dst_blk_x, dst_blk_y;
        int ci, i, j, offset_y;
        JCOEFPTR src_ptr, dst_ptr;

        int compptr_h_samp_factor = 2;
        int compptr_v_samp_factor = 8;
        unsigned int compptr_width_in_blocks = 29;
        unsigned int compptr_height_in_blocks = 19;

        MCU_cols = dstinfo->image_width / (dstinfo->max_h_samp_factor * DCTSIZE);
        MCU_rows = dstinfo->image_height / (dstinfo->max_v_samp_factor * DCTSIZE);

        for (ci = 0; ci < dstinfo->num_components; ci++,
            compptr_h_samp_factor = compptr_v_samp_factor = 1,
            compptr_width_in_blocks = 15,
            compptr_height_in_blocks = 10) {
            comp_width = MCU_cols * compptr_h_samp_factor;
            comp_height = MCU_rows * compptr_v_samp_factor;

            for (dst_blk_y = 0; dst_blk_y < compptr_height_in_blocks; dst_blk_y += compptr_v_samp_factor) {
                offset_y = 0;
                for (; offset_y < compptr_v_samp_factor; offset_y++) {
                    if (dst_blk_y < comp_height) {
                        /* Row is within the mirrorable area. */

                        /* Process the blocks that can be mirrored both ways. */
                        for (dst_blk_x = 0; dst_blk_x < comp_width; dst_blk_x++) {
                            dst_ptr = cjpeg_transupp_output_data4;
                            src_ptr = cjpeg_transupp_input4;

                            for (i = 0; i < DCTSIZE; i += 2) {
                                j = 0;
                                /* For even row, negate every odd column. */
                                for (; j < DCTSIZE; j += 2) {
                                    if (dst_blk_x < comp_height) {
                                        *dst_ptr++ = *src_ptr++;
                                        *dst_ptr++ = -*src_ptr++;
                                    }
                                }

                                j = 0;
                                /* For odd row, negate every even column. */
                                for (; j < DCTSIZE; j += 2) {
                                    if (dst_blk_x < comp_height) {
                                        *dst_ptr++ = -*src_ptr++;
                                        *dst_ptr++ = *src_ptr++;
                                    }
                                }
                            }
                        }

                        /* Any remaining right-edge blocks are only mirrored vertically. */
                        for (; dst_blk_x < compptr_width_in_blocks; dst_blk_x++) {
                            dst_ptr = cjpeg_transupp_output_data4;
                            src_ptr = cjpeg_transupp_input4;
                            for (i = 0; i < DCTSIZE; i += 2) {
                                j = 0;
                                for (; j < DCTSIZE; j++) *dst_ptr++ = *src_ptr++;
                                j = 0;
                                for (; j < DCTSIZE; j++) *dst_ptr++ = -*src_ptr++;
                            }
                        }
                    } else {
                        /* Remaining rows are just mirrored horizontally. */
                        dst_blk_x = 0;
                        /* Process the blocks that can be mirrored. */
                        do {
                            dst_ptr = cjpeg_transupp_output_data4;
                            src_ptr = cjpeg_transupp_input4;

                            i = 0;
                            while (i < DCTSIZE2) {
                                *dst_ptr++ = *src_ptr++;
                                *dst_ptr++ = -*src_ptr++;
                                i += 2;
                                dst_ptr += 0;
                            }
                            dst_blk_x++;
                            dst_ptr += 0;
                        } while (dst_blk_x < comp_width);

                        /* Any remaining right-edge blocks are only copied. */
                        for (; dst_blk_x < compptr_width_in_blocks; dst_blk_x++) {
                            dst_ptr = cjpeg_transupp_output_data4;
                            src_ptr = cjpeg_transupp_input4;
                            do {
                                *dst_ptr++ = *src_ptr++;
                                i++;
                            } while (i < DCTSIZE2);
                        }
                    }
                }
            }
        }
    }

    /*
      Transverse transpose is equivalent to
        1. 180 degree rotation;
        2. Transposition;
      or
        1. Horizontal mirroring;
        2. Transposition;
        3. Horizontal mirroring.
      These steps are merged into a single processing routine.
    */
    void cjpeg_transupp_do_transverse(j_compress_ptr dstinfo) {
        unsigned int MCU_cols, MCU_rows, comp_width, comp_height, dst_blk_x, dst_blk_y;
        int ci, i, j, offset_x, offset_y;
        JCOEFPTR src_ptr, dst_ptr;

        int compptr_h_samp_factor = 2;
        int compptr_v_samp_factor = 8;
        unsigned int compptr_height_in_blocks = 29;
        unsigned int compptr_width_in_blocks = 19;

        MCU_cols = dstinfo->image_width / (dstinfo->max_h_samp_factor * DCTSIZE);
        MCU_rows = dstinfo->image_height / (dstinfo->max_v_samp_factor * DCTSIZE);

        for (ci = 0; ci < dstinfo->num_components; ci++,
            compptr_h_samp_factor = compptr_v_samp_factor = 1,
            compptr_height_in_blocks = 15,
            compptr_width_in_blocks = 10) {
            comp_width = MCU_cols * compptr_h_samp_factor;
            comp_height = MCU_rows * compptr_v_samp_factor;

            for (dst_blk_y = 0; dst_blk_y < compptr_height_in_blocks; dst_blk_y += compptr_v_samp_factor) {
                offset_y = 0;
                do {
                    dst_blk_x = 0;
                    do {
                        offset_x = 0;
                        for (; offset_x < compptr_h_samp_factor; offset_x++) {
                            if (dst_blk_y < comp_height) {
                                src_ptr = cjpeg_transupp_input5;

                                if (dst_blk_x < comp_width) {
                                    /* Block is within the mirrorable area. */
                                    dst_ptr = cjpeg_transupp_output_data5;

                                    for (i = 0; i < DCTSIZE; i++) {
                                        j = 0;
                                        for (; j < DCTSIZE; j++) {
                                            if (dst_blk_y < comp_width)
                                                dst_ptr[j * DCTSIZE + i] = src_ptr[i * DCTSIZE + j];
                                            j++;
                                            dst_ptr[j * DCTSIZE + i] = -src_ptr[i * DCTSIZE + j];
                                        }
                                        i++;
                                        for (j = 0; j < DCTSIZE; j++) {
                                            if (dst_blk_y < comp_width)
                                                dst_ptr[j * DCTSIZE + i] = -src_ptr[i * DCTSIZE + j];
                                            j++;
                                            dst_ptr[j * DCTSIZE + i] = src_ptr[i * DCTSIZE + j];
                                        }
                                    }
                                } else {
                                    /* Right-edge blocks are mirrored in y only */
                                    dst_ptr = cjpeg_transupp_output_data5;
                                    for (i = 0; i < DCTSIZE; i++) {
                                        j = 0;
                                        for (; j < DCTSIZE; j++) {
                                            if (dst_blk_y < comp_width)
                                                dst_ptr[j * DCTSIZE + i] = src_ptr[i * DCTSIZE + j];
                                            j++;
                                            dst_ptr[j * DCTSIZE + i] = -src_ptr[i * DCTSIZE + j];
                                        }
                                    }
                                }
                            } else {
                                src_ptr = cjpeg_transupp_input5_2;

                                if (dst_blk_x < comp_width) {
                                    /* Bottom-edge blocks are mirrored in x only */
                                    dst_ptr = cjpeg_transupp_output_data5;

                                    for (i = 0; i < DCTSIZE; i++) {
                                        j = 0;
                                        for (; j < DCTSIZE; j++) dst_ptr[j * DCTSIZE + i] = src_ptr[i * DCTSIZE + j];
                                        i++;
                                        for (j = 0; j < DCTSIZE; j++)
                                            dst_ptr[j * DCTSIZE + i] = -src_ptr[i * DCTSIZE + j];
                                    }
                                } else {
                                    /* At lower right corner, just transpose, no mirroring */
                                    dst_ptr = cjpeg_transupp_output_data5;
                                    for (i = 0; i < DCTSIZE; i++) {
                                        j = 0;
                                        for (; j < DCTSIZE; j++) dst_ptr[j * DCTSIZE + i] = src_ptr[i * DCTSIZE + j];
                                    }
                                }
                            }
                            dst_blk_x += compptr_h_samp_factor;
                        }
                    } while (dst_blk_x < compptr_width_in_blocks);
                    offset_y++;
                } while (offset_y < compptr_v_samp_factor);
            }
        }
    }

   private:
    volatile int cjpeg_transupp_seed;

    signed char cjpeg_transupp_input[256];
    signed char cjpeg_transupp_input2[80];
    signed char cjpeg_transupp_input3[65];
    signed char cjpeg_transupp_input3_2[65];
    signed char cjpeg_transupp_input4[64];
    signed char cjpeg_transupp_input5[65];
    signed char cjpeg_transupp_input5_2[65];

    /* Output arrays replace writing of results into a file. */
    signed char cjpeg_transupp_output_data[512];
    signed char cjpeg_transupp_output_data2[512];
    signed char cjpeg_transupp_output_data3[512];
    signed char cjpeg_transupp_output_data4[512];
    signed char cjpeg_transupp_output_data5[512];

    jpeg_compress_struct cjpeg_transupp_dstinfo;
};
}  // namespace CJpegTransupp
