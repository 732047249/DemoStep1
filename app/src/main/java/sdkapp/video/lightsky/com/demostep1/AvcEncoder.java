package sdkapp.video.lightsky.com.demostep1;

import android.annotation.SuppressLint;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;

import java.io.IOException;
import java.nio.ByteBuffer;

public class AvcEncoder {

    private MediaCodec mediaCodec;
    int m_width;
    int m_height;
    byte[] m_info = null;

    private byte[] yuv420 = null;

    @SuppressLint("NewApi")
    public  AvcEncoder(int width, int height, int framerate, int bitrate) {

        m_width = width;
        m_height = height;
        yuv420 = new byte[width * height * 3 / 2];

        try {
            mediaCodec = MediaCodec.createEncoderByType("video/avc");
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        MediaFormat mediaFormat = MediaFormat.createVideoFormat("video/avc",
                width, height);
        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, bitrate);
        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, framerate);
        mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT,
                //MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar);
                MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar);

        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);

        mediaCodec.configure(mediaFormat, null, null,
                MediaCodec.CONFIGURE_FLAG_ENCODE);
        mediaCodec.start();
    }

    @SuppressLint("NewApi")
    public void close() {
        try {
            mediaCodec.stop();
            mediaCodec.release();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @SuppressLint("NewApi")
    public int offerEncoder(byte[] input, byte[] output) {
        int pos = 0;
        swapYV12toI420P(input, yuv420, m_width, m_height);
        //swapNV21toI420SP(input, yuv420, m_width, m_height);
        try {
            ByteBuffer[] inputBuffers = mediaCodec.getInputBuffers();
            ByteBuffer[] outputBuffers = mediaCodec.getOutputBuffers();
            int inputBufferIndex = mediaCodec.dequeueInputBuffer(0);
            if (inputBufferIndex >= 0) {
                ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
                inputBuffer.clear();
                inputBuffer.put(yuv420);
                mediaCodec.queueInputBuffer(inputBufferIndex, 0, yuv420.length,
                        0, 0);
            }

            MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
            int outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo,
                    0);

            while (outputBufferIndex >= 0) {
                ByteBuffer outputBuffer = outputBuffers[outputBufferIndex];
                byte[] outData = new byte[bufferInfo.size];
                outputBuffer.get(outData);

                if (m_info != null) {
                    System.arraycopy(outData, 0, output, pos, outData.length);
                    pos += outData.length;

                }

                else {
                    ByteBuffer spsPpsBuffer = ByteBuffer.wrap(outData);
                    if (spsPpsBuffer.getInt() == 0x00000001) {
                        m_info = new byte[outData.length];
                        System.arraycopy(outData, 0, m_info, 0, outData.length);
                    } else {
                        return -1;
                    }
                }

                mediaCodec.releaseOutputBuffer(outputBufferIndex, false);
                outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo,
                        0);
            }

            if ((output[4] & (byte)0x1f) == (byte) 0x05) // key frame
            {
                System.arraycopy(output, 0, yuv420, 0, pos);
                System.arraycopy(m_info, 0, output, 0, m_info.length);
                System.arraycopy(yuv420, 0, output, m_info.length, pos);
                pos += m_info.length;
            }

        } catch (Throwable t) {
            t.printStackTrace();
        }

        return pos;
    }

    private void swapYV12toI420P(byte[] yv12bytes, byte[] i420bytes, int width,
                                 int height) {
        System.arraycopy(yv12bytes, 0, i420bytes, 0, width * height);
        System.arraycopy(yv12bytes, width * height + width * height / 4,
                i420bytes, width * height, width * height / 4);
        System.arraycopy(yv12bytes, width * height, i420bytes, width * height
                + width * height / 4, width * height / 4);
    }

    private void swapNV21toI420SP(byte[] yv12bytes, byte[] i420spbytes, int width,
                                  int height)
    {
        int wh = width*height;
        int hwh = 3*wh/2;
        System.arraycopy(yv12bytes, 0, i420spbytes, 0,  wh);
        for (int i = wh; i < hwh; )
        {
            i420spbytes[i]   = yv12bytes[i+1];
            i420spbytes[i+1]   = yv12bytes[i];
            i += 2;
        }


    }


}
