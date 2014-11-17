#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <jpeglib.h>
#include <asm/types.h>
#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

struct buffer {
 void * start;
 size_t length;
};

static char * dev_name = "/dev/video0";
static int fd = -1;
struct buffer * buffers = NULL;

FILE *file_fd;
static unsigned long file_length;
static unsigned char *file_name;



#define OUTPUT_BUF_SIZE  4096

typedef struct {
    struct jpeg_destination_mgr pub; /* public fields */

    JOCTET * buffer;    /* start of buffer */

    unsigned char *outbuffer;
    int outbuffer_size;
    unsigned char *outbuffer_cursor;
    int *written;

} mjpg_destination_mgr;

typedef mjpg_destination_mgr * mjpg_dest_ptr;

/******************************************************************************
Description.:
Input Value.:
Return Value:
******************************************************************************/
METHODDEF(void) init_destination(j_compress_ptr cinfo)
{
    mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;

    /* Allocate the output buffer --- it will be released when done with image */
    dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_IMAGE, OUTPUT_BUF_SIZE * sizeof(JOCTET));

    *(dest->written) = 0;

    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

/******************************************************************************
Description.: called whenever local jpeg buffer fills up
Input Value.:
Return Value:
******************************************************************************/
METHODDEF(boolean) empty_output_buffer(j_compress_ptr cinfo)
{
    mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;

    memcpy(dest->outbuffer_cursor, dest->buffer, OUTPUT_BUF_SIZE);
    dest->outbuffer_cursor += OUTPUT_BUF_SIZE;
    *(dest->written) += OUTPUT_BUF_SIZE;

    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

    return TRUE;
}

/******************************************************************************
Description.: called by jpeg_finish_compress after all data has been written.
              Usually needs to flush buffer.
Input Value.:
Return Value:
******************************************************************************/
METHODDEF(void) term_destination(j_compress_ptr cinfo)
{
    mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;
    size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

    /* Write any data remaining in the buffer */
    memcpy(dest->outbuffer_cursor, dest->buffer, datacount);
    dest->outbuffer_cursor += datacount;
    *(dest->written) += datacount;
}

/******************************************************************************
Description.: Prepare for output to a stdio stream.
Input Value.: buffer is the already allocated buffer memory that will hold
              the compressed picture. "size" is the size in bytes.
Return Value: -
******************************************************************************/
GLOBAL(void) dest_buffer(j_compress_ptr cinfo, unsigned char *buffer, int size, int *written)
{
    mjpg_dest_ptr dest;

    if(cinfo->dest == NULL) {
        cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(mjpg_destination_mgr));
    }

    dest = (mjpg_dest_ptr) cinfo->dest;
    dest->pub.init_destination = init_destination;
    dest->pub.empty_output_buffer = empty_output_buffer;
    dest->pub.term_destination = term_destination;
    dest->outbuffer = buffer;
    dest->outbuffer_size = size;
    dest->outbuffer_cursor = buffer;
    dest->written = written;
}

/******************************************************************************
Description.: yuv2jpeg function is based on compress_yuyv_to_jpeg written by
              Gabriel A. Devenyi.
              It uses the destination manager implemented above to compress
              YUYV data to JPEG. Most other implementations use the
              "jpeg_stdio_dest" from libjpeg, which can not store compressed
              pictures to memory instead of a file.
Input Value.: video structure from v4l2uvc.c/h, destination buffer and buffersize
              the buffer must be large enough, no error/size checking is done!
Return Value: the buffer will contain the compressed data
******************************************************************************/
int compress_yuyv_to_jpeg(unsigned char *frame, unsigned char *buffer, int width, int height, int size, int quality)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    unsigned char *line_buffer, *yuyv;
    int z;
    static int written;

    line_buffer = calloc(width * 3, 1);
    yuyv = frame;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    /* jpeg_stdio_dest (&cinfo, file); */
    dest_buffer(&cinfo, buffer, size, &written);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    z = 0;
    while(cinfo.next_scanline < height) {
        int x;
        unsigned char *ptr = line_buffer;

        for(x = 0; x < width; x++) {
            int r, g, b;
            int y, u, v;

            if(!z)
                y = yuyv[0] << 8;
            else
                y = yuyv[2] << 8;
            u = yuyv[1] - 128;
            v = yuyv[3] - 128;

            r = (y + (359 * v)) >> 8;
            g = (y - (88 * u) - (183 * v)) >> 8;
            b = (y + (454 * u)) >> 8;

            *(ptr++) = (r > 255) ? 255 : ((r < 0) ? 0 : r);
            *(ptr++) = (g > 255) ? 255 : ((g < 0) ? 0 : g);
            *(ptr++) = (b > 255) ? 255 : ((b < 0) ? 0 : b);

            if(z++) {
                z = 0;
                yuyv += 4;
            }
        }

        row_pointer[0] = line_buffer;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    free(line_buffer);

    return (written);
}


int main (int argc,char ** argv)
{
 struct v4l2_capability cap;
 struct v4l2_format fmt;
 struct v4l2_cropcap cropcap;
 struct v4l2_crop crop;
 struct v4l2_requestbuffers req;
 enum v4l2_buf_type type;
 struct v4l2_buffer *buf, queue_buf;
 struct buffer* buffers;
 int n_buffers, i;
 int index;
 unsigned char *frame_buf, *jpeg_buf;
 int len;
 int size;
 char filename[128];
 

//open device
 fd = open (dev_name, O_RDWR, 0);

//init device
 ioctl (fd, VIDIOC_QUERYCAP, &cap);

 CLEAR(cropcap);
 cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 if ( ioctl(fd, VIDIOC_CROPCAP, &cropcap ) < 0 )
 {
  perror("ioctl");
 }
 
 CLEAR(crop);
 crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 crop.c = cropcap.defrect;
 ioctl(fd, VIDIOC_S_CROP, &crop);

 CLEAR (fmt);
 fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 fmt.fmt.pix.width = 640;
 fmt.fmt.pix.height = 480;
 fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
 fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
 ioctl (fd, VIDIOC_S_FMT, &fmt);

//init mmap 
 CLEAR(req);
 req.count = 4;
 req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 req.memory = V4L2_MEMORY_MMAP;
 ioctl(fd, VIDIOC_REQBUFS, &req);
 
 buffers = (struct buffer *)calloc(req.count, sizeof(*buffers));

 for ( n_buffers = 0; n_buffers < req.count; n_buffers++)
 {
  buf = (struct v4l2_buffer *)malloc(sizeof(struct v4l2_buffer));
  memset(buf, 0, sizeof(struct v4l2_buffer));
  buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf->memory = V4L2_MEMORY_MMAP;
  buf->index = n_buffers;
  ioctl(fd, VIDIOC_QUERYBUF, buf);
  buffers[n_buffers].length = buf->length;
  buffers[n_buffers].start = mmap(NULL, buf->length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf->m.offset);
 }

//start capturing
 for ( i = 0; i < n_buffers; i++)
 {
  buf = (struct v4l2_buffer *)malloc(sizeof(struct v4l2_buffer));
  memset(buf, 0, sizeof(struct v4l2_buffer));
  buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf->memory = V4L2_MEMORY_MMAP;
  buf->index = i;
  ioctl(fd, VIDIOC_QBUF, buf);
 }
 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 ioctl(fd, VIDIOC_STREAMON, &type);

//get frame
 for ( i = 0; i < 30; i++)
 {
  CLEAR(queue_buf);
  queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  queue_buf.memory = V4L2_MEMORY_MMAP;
  ioctl(fd, VIDIOC_DQBUF, &queue_buf);
  frame_buf = buffers[queue_buf.index].start;
  len = buffers[queue_buf.index].length;
  index = queue_buf.index;
  jpeg_buf = (unsigned char *)malloc(len);
  memset(jpeg_buf, 0, len);
  size = compress_yuyv_to_jpeg(frame_buf, jpeg_buf, 640, 480, len, 80);
  printf("%d\n", size);
  sprintf( filename, "test%d.jpg", i);
  file_fd = fopen(filename, "wb");
  fwrite(jpeg_buf, size, 1, file_fd);
  fclose(file_fd);
  //unget frame
  CLEAR(queue_buf);
  queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  queue_buf.memory = V4L2_MEMORY_MMAP;
  queue_buf.index = index;
  ioctl(fd, VIDIOC_QBUF, &queue_buf);
 }
 
//stop capturing
 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 ioctl(fd, VIDIOC_STREAMOFF, &type);

//close device
 close(fd);
 return 0;
}