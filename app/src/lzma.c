///////////////////////////////////////////////////////////////////////////////
//
/// \file       01_compress_easy.c
/// \brief      Compress from stdin to stdout in multi-call mode
///
/// Usage:      ./01_compress_easy PRESET < INFILE > OUTFILE
///
/// Example:    ./01_compress_easy 6 < foo > foo.xz
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <lzma.h>

static void show_usage(const char *argv0, FILE *fout_p)
{
    fprintf(fout_p,
            "Usage: %s PRESET DATA\n"
            "PRESET is a number 0-9 and can optionally be "
            "followed by `e' to indicate extreme preset\n",
            argv0);
}

static bool get_args(int argc,
                     char **argv,
                     uint32_t *preset_p,
                     const char **data_pp,
                     FILE *fout_p)
{
    // One argument whose first char must be 0-9.
    if (argc != 3 || argv[1][0] < '0' || argv[1][0] > '9') {
        show_usage(argv[0], fout_p);
        return (false);
    }

    // Calculate the preste level 0-9.
    *preset_p = argv[1][0] - '0';
    *data_pp = argv[2];

    // If there is a second char, it must be 'e'. It will set
    // the LZMA_PRESET_EXTREME flag.
    if (argv[1][1] != '\0') {
        if (argv[1][1] != 'e' || argv[1][2] != '\0') {
            show_usage(argv[0], fout_p);
            return (false);
        }

        *preset_p |= LZMA_PRESET_EXTREME;
    }

    return (true);
}

static bool
init_encoder(lzma_stream *strm, uint32_t preset, FILE *fout_p)
{
    // Initialize the encoder using a preset. Set the integrity to check
    // to CRC64, which is the default in the xz command line tool. If
    // the .xz file needs to be decompressed with XZ Embedded, use
    // LZMA_CHECK_CRC32 instead.
    lzma_ret ret = lzma_easy_encoder(strm, preset, LZMA_CHECK_CRC64);

    // Return successfully if the initialization went fine.
    if (ret == LZMA_OK)
        return true;

    // Something went wrong. The possible errors are documented in
    // lzma/container.h (src/liblzma/api/lzma/container.h in the source
    // package or e.g. /usr/include/lzma/container.h depending on the
    // install prefix).
    const char *msg;
    switch (ret) {
    case LZMA_MEM_ERROR:
        msg = "Memory allocation failed";
        break;

    case LZMA_OPTIONS_ERROR:
        msg = "Specified preset is not supported";
        break;

    case LZMA_UNSUPPORTED_CHECK:
        msg = "Specified integrity check is not supported";
        break;

    default:
        // This is most likely LZMA_PROG_ERROR indicating a bug in
        // this program or in liblzma. It is inconvenient to have a
        // separate error message for errors that should be impossible
        // to occur, but knowing the error code is important for
        // debugging. That's why it is good to print the error code
        // at least when there is no good error message to show.
        msg = "Unknown error, possibly a bug";
        break;
    }

    fprintf(fout_p,
            "Error initializing the encoder: %s (error code %u)\n",
            msg,
            ret);

    return false;
}


static bool
compress(lzma_stream *strm, const char *data_p, FILE *fout_p)
{
    // This will be LZMA_RUN until the end of the input file is reached.
    // This tells lzma_code() when there will be no more input.
    lzma_action action = LZMA_RUN;

    // Buffers to temporarily hold uncompressed input
    // and compressed output.
    uint8_t inbuf[BUFSIZ];
    uint8_t outbuf[BUFSIZ];
    size_t length;
    size_t offset;

    offset = 0;
    length = strlen(data_p);

    // Initialize the input and output pointers. Initializing next_in
    // and avail_in isn't really necessary when we are going to encode
    // just one file since LZMA_STREAM_INIT takes care of initializing
    // those already. But it doesn't hurt much and it will be needed
    // if encoding more than one file like we will in 02_decompress.c.
    //
    // While we don't care about strm->total_in or strm->total_out in this
    // example, it is worth noting that initializing the encoder will
    // always reset total_in and total_out to zero. But the encoder
    // initialization doesn't touch next_in, avail_in, next_out, or
    // avail_out.
    strm->next_in = NULL;
    strm->avail_in = 0;
    strm->next_out = outbuf;
    strm->avail_out = sizeof(outbuf);

    // Loop until the file has been successfully compressed or until
    // an error occurs.
    while (true) {
        // Fill the input buffer if it is empty.
        if (strm->avail_in == 0 && (offset < length)) {
            strm->next_in = inbuf;
            inbuf[0] = data_p[offset++];
            strm->avail_in = 1;

            // Once the end of the input file has been reached,
            // we need to tell lzma_code() that no more input
            // will be coming and that it should finish the
            // encoding.
            if (offset == length)
                action = LZMA_FINISH;
        }

        // Tell liblzma do the actual encoding.
        //
        // This reads up to strm->avail_in bytes of input starting
        // from strm->next_in. avail_in will be decremented and
        // next_in incremented by an equal amount to match the
        // number of input bytes consumed.
        //
        // Up to strm->avail_out bytes of compressed output will be
        // written starting from strm->next_out. avail_out and next_out
        // will be incremented by an equal amount to match the number
        // of output bytes written.
        //
        // The encoder has to do internal buffering, which means that
        // it may take quite a bit of input before the same data is
        // available in compressed form in the output buffer.
        lzma_ret ret = lzma_code(strm, action);

        // If the output buffer is full or if the compression finished
        // successfully, write the data from the output bufffer to
        // the output file.
        if (strm->avail_out == 0 || ret == LZMA_STREAM_END) {
            // When lzma_code() has returned LZMA_STREAM_END,
            // the output buffer is likely to be only partially
            // full. Calculate how much new data there is to
            // be written to the output file.
            size_t write_size = sizeof(outbuf) - strm->avail_out;
            int i;

            for (i = 0; i < write_size; i++) {
                fprintf(fout_p, "%02X", outbuf[i]);
            }

            fprintf(fout_p, "\n");

            // Reset next_out and avail_out.
            strm->next_out = outbuf;
            strm->avail_out = sizeof(outbuf);
        }

        // Normally the return value of lzma_code() will be LZMA_OK
        // until everything has been encoded.
        if (ret != LZMA_OK) {
            // Once everything has been encoded successfully, the
            // return value of lzma_code() will be LZMA_STREAM_END.
            //
            // It is important to check for LZMA_STREAM_END. Do not
            // assume that getting ret != LZMA_OK would mean that
            // everything has gone well.
            if (ret == LZMA_STREAM_END)
                return true;

            // It's not LZMA_OK nor LZMA_STREAM_END,
            // so it must be an error code. See lzma/base.h
            // (src/liblzma/api/lzma/base.h in the source package
            // or e.g. /usr/include/lzma/base.h depending on the
            // install prefix) for the list and documentation of
            // possible values. Most values listen in lzma_ret
            // enumeration aren't possible in this example.
            const char *msg;
            switch (ret) {
            case LZMA_MEM_ERROR:
                msg = "Memory allocation failed";
                break;

            case LZMA_DATA_ERROR:
                // This error is returned if the compressed
                // or uncompressed size get near 8 EiB
                // (2^63 bytes) because that's where the .xz
                // file format size limits currently are.
                // That is, the possibility of this error
                // is mostly theoretical unless you are doing
                // something very unusual.
                //
                // Note that strm->total_in and strm->total_out
                // have nothing to do with this error. Changing
                // those variables won't increase or decrease
                // the chance of getting this error.
                msg = "File size limits exceeded";
                break;

            default:
                // This is most likely LZMA_PROG_ERROR, but
                // if this program is buggy (or liblzma has
                // a bug), it may be e.g. LZMA_BUF_ERROR or
                // LZMA_OPTIONS_ERROR too.
                //
                // It is inconvenient to have a separate
                // error message for errors that should be
                // impossible to occur, but knowing the error
                // code is important for debugging. That's why
                // it is good to print the error code at least
                // when there is no good error message to show.
                msg = "Unknown error, possibly a bug";
                break;
            }

            fprintf(fout_p,
                    "Encoder error: %s (error code %u)\n",
                    msg,
                    ret);

            return false;
        }
    }
}


int command_lzma_compress(int argc, char **argv, FILE *fout_p)
{
    // Get the preset number from the command line.
    uint32_t preset;
    const char *data_p;

    if (!get_args(argc, argv, &preset, &data_p, fout_p)) {
        return (-1);
    }

    // Initialize a lzma_stream structure. When it is allocated on stack,
    // it is simplest to use LZMA_STREAM_INIT macro like below. When it
    // is allocated on heap, using memset(strmptr, 0, sizeof(*strmptr))
    // works (as long as NULL pointers are represented with zero bits
    // as they are on practically all computers today).
    lzma_stream strm = LZMA_STREAM_INIT;

    // Initialize the encoder. If it succeeds, compress from
    // stdin to stdout.
    bool success = init_encoder(&strm, preset, fout_p);
    if (success)
        success = compress(&strm, data_p, fout_p);

    // Free the memory allocated for the encoder. If we were encoding
    // multiple files, this would only need to be done after the last
    // file. See 02_decompress.c for handling of multiple files.
    //
    // It is OK to call lzma_end() multiple times or when it hasn't been
    // actually used except initialized with LZMA_STREAM_INIT.
    lzma_end(&strm);

    return (success ? 0 : -1);
}
