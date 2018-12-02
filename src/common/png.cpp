#include <common/png.h>
#include <common/debugLog.h>
#include <system/device.h>
#include <psp2/io/fcntl.h>

#define printf debugLog
#include <png.h>

#define PNG_SIGSIZE (8)
namespace v {
    void read_png_file_fn(png_structp png_ptr, png_bytep data, png_size_t length) {
        File* fp = (File*)png_get_io_ptr(png_ptr);
        fp->read(data, length);
    }
    GxmTexture* load_png_internal (const void *io_ptr, png_rw_ptr read_data_fn, Device* device) {
        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (png_ptr == NULL) return NULL;

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (info_ptr == NULL) {
            png_destroy_read_struct(&png_ptr, (png_infopp)0, (png_infopp)0);
            return NULL;
        }

        png_bytep *row_ptrs = NULL;

        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)0);
            if (row_ptrs != NULL) free(row_ptrs);
            return NULL;
        }

        png_set_read_fn(png_ptr, (png_voidp)io_ptr, read_data_fn);
        png_set_sig_bytes(png_ptr, PNG_SIGSIZE);
        png_read_info(png_ptr, info_ptr);

        unsigned int width, height;
        int bit_depth, color_type;

        png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

        if ((color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8)
            || (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
            || png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)
            || (bit_depth == 16)) {
                png_set_expand(png_ptr);
        }

        if (bit_depth == 16) png_set_scale_16(png_ptr);

        if (bit_depth == 8 && color_type == PNG_COLOR_TYPE_RGB) png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png_ptr);

        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            png_set_palette_to_rgb(png_ptr);
            png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
        }

        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);

        if (bit_depth < 8) png_set_packing(png_ptr);

        png_read_update_info(png_ptr, info_ptr);

        row_ptrs = (png_bytep *)malloc(sizeof(png_bytep) * height);
        if (!row_ptrs) {
            png_destroy_info_struct(png_ptr, &info_ptr);
            return NULL;
        }

        GxmTexture* tex = new GxmTexture(width, height, SCE_GXM_TEXTURE_FORMAT_A8B8G8R8, false, device->gpu().context());
        if(tex->bad()) {
            delete tex;
            png_destroy_info_struct(png_ptr, &info_ptr);
            free(row_ptrs);
            return NULL;
        }
        void *texture_data = tex->data();
        unsigned int stride = tex->stride();

        int i;
        for (i = 0; i < height; i++) {
          row_ptrs[i] = (png_bytep)(((u8*)texture_data) + (i * stride * 4));
        }
      
        png_read_image(png_ptr, row_ptrs);

        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)0);
        free(row_ptrs);

        return tex;
    }

    GxmTexture* load_png (const char* filename, Device* device) {
        png_byte pngsig[PNG_SIGSIZE];
        
        File* fp = device->open_file(filename, "rb");
        if (!fp) {
            return NULL;
        }

        if (!fp->read(pngsig, PNG_SIGSIZE)) {
            delete fp;
            return NULL;
        }

        if (png_sig_cmp(pngsig, 0, PNG_SIGSIZE) != 0) {
            delete fp;
            return NULL;
        }

        return load_png_internal((void *)fp, read_png_file_fn, device);
    }
};
