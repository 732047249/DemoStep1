package sdkapp.video.lightsky.com.demostep1;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;

import java.net.InetAddress;
import java.net.UnknownHostException;

import java.lang.Thread;

//import com.gs.gscodec.GSCodec;

public class AudioPlayer extends Thread {

    private int audioSource = MediaRecorder.AudioSource.MIC;
    private static int sampleRateInHz = 44100;
    private static int channelConfig = AudioFormat.CHANNEL_CONFIGURATION_MONO;
    private static int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
    private int bufferSizeInBytes = 0;
    private AudioRecord audioRecord = null;
    public int OpenRecorder(int userid, String ip, int port)
    {
        String strip;
        try {
            InetAddress myServer = InetAddress.getByName(ip);
            strip = myServer.getHostAddress();


            bufferSizeInBytes = AudioRecord.getMinBufferSize(sampleRateInHz,
                    channelConfig, audioFormat);
            audioRecord = new AudioRecord(audioSource, sampleRateInHz,
                    channelConfig, audioFormat, bufferSizeInBytes);

        } catch (UnknownHostException e) {
            return -1;
            //strip = "113.11.199.170";
        }



//        int iret = GSCodec.GetInstance().OpenAacEncoder(userid,strip, port);
//        if(iret < 0)
//        {
//            return -1;
//        }


        return 0;
    }

    public int CloseRecoder()
    {
        this.interrupt();
        while(this.isAlive())
        {
            this.interrupt();
        }
        if(audioRecord != null)
        {
            audioRecord.stop();
            audioRecord.release();
            audioRecord = null;
        }
        //GSCodec.GetInstance().CloseAacEncoder();

        return 0;
    }

    public void run()
    {
        if(audioRecord == null)
        {
            return ;
        }
        audioRecord.startRecording();

        byte[] audiodata = new byte[bufferSizeInBytes];
        int readsize = 0;

        while (!this.isInterrupted() )
        {
            readsize = audioRecord.read(audiodata, 0, bufferSizeInBytes);

            calc1(audiodata, 0, readsize);
            if (AudioRecord.ERROR_INVALID_OPERATION != readsize)
            {
                //GSCodec.GetInstance().AacEncode(audiodata, readsize);
            }
            else
            {
                break;
            }
        }
    }
    //不明原因，能够减少噪音 参考 http://blog.sina.com.cn/s/blog_6d8df7d00101415d.html
    void calc1(byte[] data,int off,int len)
    {
        int i = off;
        while( i < len)
        {
            int j = bytetoshort(data, i );
            short m = (short) (j>>2);
            shorttobyte(m, data, i);
            i += 2;
        }

    }
    short bytetoshort(byte[] data, int off)
    {
        return (short) (((data[off + 1] << 8) | data[off + 0] & 0xff));
    }

    void shorttobyte(short num, byte[] data, int off)
    {
        data[1+off] = (byte)((num >> 8) & 0XFF);
        data[0+off] = (byte)(num & 0XFF);
    }

}
