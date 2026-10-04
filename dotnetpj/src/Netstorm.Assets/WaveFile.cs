using System.Buffers.Binary;

namespace Netstorm.Assets;

/// <summary>
/// RIFF WAV 파일(PCM). 원본 효과음 `sound/*.wav`(대부분 22,050Hz 8bit 모노)와 배경음악 `music/*.mus`
/// (확장자만 다른 WAV, 22,050Hz 16bit 스테레오)를 그대로 읽는다 (LEFT_JOBS 1.2절).
/// 재생 장치는 16bit 표본을 받으므로 <see cref="ToPcm16"/> 로 바꿔 쓴다.
/// </summary>
public sealed class WaveFile
{
    /// <summary>RIFF 머리 길이 ("RIFF" + 크기 + "WAVE")</summary>
    private const int RiffHeaderSize = 12;

    /// <summary>청크 머리 길이 (이름 4바이트 + 크기 4바이트)</summary>
    private const int ChunkHeaderSize = 8;

    /// <summary>fmt 청크의 PCM 형식 번호 (WAVE_FORMAT_PCM)</summary>
    private const ushort PcmFormat = 1;

    /// <summary>fmt 청크의 최소 길이 (형식·채널·표본율·초당 바이트·블록 정렬·표본 비트)</summary>
    private const int MinFormatSize = 16;

    /// <summary>8bit PCM 의 무음 값 (부호 없는 표본의 가운데)</summary>
    private const int Unsigned8Center = 128;

    /// <summary>표본율 (Hz)</summary>
    public int SampleRate { get; }

    /// <summary>채널 수 (1 = 모노, 2 = 스테레오)</summary>
    public int Channels { get; }

    /// <summary>표본 하나의 비트 수 (8 또는 16)</summary>
    public int BitsPerSample { get; }

    /// <summary>파일 안에서 data 청크 내용이 시작하는 위치 (스트리밍 재생용)</summary>
    public long DataOffset { get; }

    /// <summary>data 청크 내용의 바이트 수</summary>
    public int DataLength { get; }

    /// <summary>한 표본 묶음(모든 채널)의 바이트 수</summary>
    public int BlockAlign => Channels * BitsPerSample / 8;

    /// <summary>재생 길이(초)</summary>
    public double DurationSeconds => DataLength / (double)(BlockAlign * SampleRate);

    /// <summary>해석한 머리 값으로 만든다</summary>
    private WaveFile(int sampleRate, int channels, int bits, long dataOffset, int dataLength)
    {
        SampleRate = sampleRate;
        Channels = channels;
        BitsPerSample = bits;
        DataOffset = dataOffset;
        DataLength = dataLength;
    }

    /// <summary>
    /// WAV 머리를 해석한다. 데이터는 읽지 않으므로 큰 음악 파일도 머리만 본다.
    /// PCM 8/16bit, 모노/스테레오만 받는다 (원본 파일은 모두 이 범위다).
    /// </summary>
    /// <param name="stream">처음 위치가 RIFF 머리인 읽기 가능한 스트림</param>
    public static WaveFile ReadHeader(Stream stream)
    {
        Span<byte> head = stackalloc byte[RiffHeaderSize];
        stream.ReadExactly(head);
        if (!head[..4].SequenceEqual("RIFF"u8) || !head[8..12].SequenceEqual("WAVE"u8))
        {
            throw new InvalidDataException("RIFF WAVE 파일이 아닙니다.");
        }
        long position = RiffHeaderSize;
        (int Rate, int Channels, int Bits)? format = null;
        Span<byte> chunk = stackalloc byte[ChunkHeaderSize];
        Span<byte> fmt = stackalloc byte[MinFormatSize];
        // 청크를 차례로 넘기며 fmt 와 data 를 찾는다 (다른 청크는 건너뛴다)
        while (true)
        {
            stream.Position = position;
            if (stream.Read(chunk) < ChunkHeaderSize)
            {
                throw new InvalidDataException("data 청크가 없습니다.");
            }
            int size = BinaryPrimitives.ReadInt32LittleEndian(chunk[4..]);
            long body = position + ChunkHeaderSize;
            if (chunk[..4].SequenceEqual("fmt "u8))
            {
                if (size < MinFormatSize)
                {
                    throw new InvalidDataException("fmt 청크가 너무 짧습니다.");
                }
                stream.ReadExactly(fmt);
                ushort tag = BinaryPrimitives.ReadUInt16LittleEndian(fmt);
                int channels = BinaryPrimitives.ReadUInt16LittleEndian(fmt[2..]);
                int rate = BinaryPrimitives.ReadInt32LittleEndian(fmt[4..]);
                int bits = BinaryPrimitives.ReadUInt16LittleEndian(fmt[14..]);
                if (tag != PcmFormat || channels is < 1 or > 2 || bits is not (8 or 16) || rate <= 0)
                {
                    throw new InvalidDataException($"지원하지 않는 WAV 형식입니다 (형식 {tag}, {channels}채널, {bits}bit).");
                }
                format = (rate, channels, bits);
            }
            else if (chunk[..4].SequenceEqual("data"u8))
            {
                if (format is not { } f)
                {
                    throw new InvalidDataException("fmt 청크보다 data 청크가 먼저 나왔습니다.");
                }
                // 일부 파일은 크기 값이 실제 파일보다 크다 — 파일 끝까지로 줄인다
                long available = stream.Length - body;
                int length = (int)Math.Min(size < 0 ? available : size, available);
                length -= length % (f.Channels * f.Bits / 8);
                return new WaveFile(f.Rate, f.Channels, f.Bits, body, length);
            }
            // RIFF 청크는 짝수 바이트로 정렬된다
            position = body + size + (size & 1);
        }
    }

    /// <summary>파일에서 머리를 읽는다</summary>
    /// <param name="path">WAV 파일 경로</param>
    public static WaveFile ReadHeader(string path)
    {
        using FileStream stream = File.OpenRead(path);
        return ReadHeader(stream);
    }

    /// <summary>
    /// 원본 표본을 부호 있는 16bit 리틀 엔디언 PCM 으로 바꾼다 (채널 배치는 그대로).
    /// 8bit 표본(부호 없음, 가운데 128)은 256 배로 늘린다.
    /// </summary>
    /// <param name="data">data 청크 내용 (<see cref="BlockAlign"/> 의 배수)</param>
    public byte[] ToPcm16(ReadOnlySpan<byte> data)
    {
        if (BitsPerSample == 16)
        {
            return data.ToArray();
        }
        var result = new byte[data.Length * 2];
        // 8bit 표본마다 16bit 값으로 늘려 쓴다
        for (int i = 0; i < data.Length; i++)
        {
            BinaryPrimitives.WriteInt16LittleEndian(result.AsSpan(i * 2), (short)((data[i] - Unsigned8Center) << 8));
        }
        return result;
    }

    /// <summary>
    /// 16bit PCM 을 다른 표본율로 바꾼다 (선형 보간). 원본에는 재생 장치가 받지 않는 6,000Hz 파일
    /// (thunderCrack.wav, distantWindQuiet-3000.wav, ThunderQuietDistant.wav)이 있어 재생 전에 올려 쓴다.
    /// </summary>
    /// <param name="pcm">부호 있는 16bit 리틀 엔디언 표본 (채널 교차 배치)</param>
    /// <param name="channels">채널 수</param>
    /// <param name="fromRate">원래 표본율</param>
    /// <param name="toRate">바꿀 표본율</param>
    public static byte[] ResamplePcm16(ReadOnlySpan<byte> pcm, int channels, int fromRate, int toRate)
    {
        int frames = pcm.Length / (2 * channels);
        if (fromRate == toRate || frames == 0)
        {
            return pcm.ToArray();
        }
        int outFrames = (int)((long)frames * toRate / fromRate);
        var result = new byte[outFrames * 2 * channels];
        // 새 표본마다 원래 시간 위치의 앞뒤 표본을 보간한다
        for (int i = 0; i < outFrames; i++)
        {
            double source = (double)i * fromRate / toRate;
            int left = Math.Min((int)source, frames - 1);
            int right = Math.Min(left + 1, frames - 1);
            double t = source - left;
            // 채널마다 따로 보간한다
            for (int c = 0; c < channels; c++)
            {
                short a = BinaryPrimitives.ReadInt16LittleEndian(pcm[(((left * channels) + c) * 2)..]);
                short b = BinaryPrimitives.ReadInt16LittleEndian(pcm[(((right * channels) + c) * 2)..]);
                BinaryPrimitives.WriteInt16LittleEndian(result.AsSpan(((i * channels) + c) * 2), (short)Math.Round(a + (b - a) * t));
            }
        }
        return result;
    }

    /// <summary>파일 전체 표본을 16bit PCM 으로 읽는다 (효과음처럼 짧은 파일용)</summary>
    /// <param name="stream">WAV 스트림</param>
    /// <param name="wave">해석한 머리</param>
    public static byte[] ReadPcm16(Stream stream, out WaveFile wave)
    {
        wave = ReadHeader(stream);
        var data = new byte[wave.DataLength];
        stream.Position = wave.DataOffset;
        stream.ReadExactly(data);
        return wave.ToPcm16(data);
    }
}
