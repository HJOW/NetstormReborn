using System.Runtime.InteropServices;
using System.Text;

namespace Netstorm.AnalyzeManager;

/// <summary>Windows 기본 재생 장치의 소리를 루프백으로 받아 48 MB 이하 WAV 조각으로 저장한다.</summary>
public sealed class GuidedAudio : IDisposable
{
    /// <summary>헤더와 마지막 오디오 패킷을 포함해도 50 MB에 닿지 않는 조각 크기.</summary>
    public const long SegmentLimitBytes = 48_000_000;
    /// <summary>Windows 오디오 루프백에 필요한 공유 모드 플래그.</summary>
    private const uint LoopbackFlag = 0x00020000;
    /// <summary>버퍼가 묵음일 때 해당 위치를 0으로 채우는 플래그.</summary>
    private const uint SilentFlag = 0x00000002;
    /// <summary>기본 Windows 출력 장치를 찾는 COM 클래스 식별자.</summary>
    private static readonly Guid DeviceEnumeratorId = new("BCDE0395-E52F-467C-8E3D-C4579291692E");
    /// <summary>공유 모드 오디오 클라이언트의 COM 인터페이스 식별자.</summary>
    private static readonly Guid AudioClientId = new("1CB9AD4C-DBFA-4c32-B178-C2F568A703B2");
    /// <summary>캡처 버퍼를 읽는 COM 인터페이스 식별자.</summary>
    private static readonly Guid CaptureClientId = new("C8ADBD64-E71E-48a0-A4DE-185C395CD317");
    private readonly string _directory;
    private readonly CancellationTokenSource _stop = new();
    private readonly ManualResetEventSlim _ready = new(false);
    private readonly Thread _thread;
    private volatile Exception? _error;
    private int _segment;
    private bool _disposed;

    /// <summary>지난 녹화의 WAV를 덮지 않고 별도의 캡처 스레드를 시작한다.</summary>
    public GuidedAudio(string directory)
    {
        SessionStore.RejectReparse(directory);
        Directory.CreateDirectory(directory);
        _directory = directory;
        _segment = Directory.EnumerateFiles(directory, "audio-*.wav")
            .Select(path => int.TryParse(Path.GetFileNameWithoutExtension(path).AsSpan(6), out int number) ? number : 0)
            .DefaultIfEmpty(0).Max();
        _thread = new Thread(Run) { IsBackground = true, Name = "NetStorm 분석 오디오 캡처" };
        _thread.Start();
        if (!_ready.Wait(TimeSpan.FromSeconds(5)))
        {
            Dispose();
            throw new TimeoutException("기본 재생 장치의 오디오 캡처가 시작되지 않았습니다.");
        }
        if (_error != null)
        {
            Dispose();
            throw new InvalidOperationException("시스템 출력 오디오 캡처 시작 실패: " + _error.Message, _error);
        }
    }

    /// <summary>캡처 중 발생한 오류를 안내 창이 검사할 수 있게 제공한다.</summary>
    public Exception? Error => _error;

    /// <summary>COM 오디오 객체를 한 스레드에서 만들고 같은 스레드에서 해제한다.</summary>
    private void Run()
    {
        IMMDeviceEnumerator? enumerator = null;
        IMMDevice? device = null;
        IAudioClient? client = null;
        IAudioCaptureClient? capture = null;
        IntPtr mixFormat = IntPtr.Zero;
        WaveSegment? output = null;
        try
        {
            enumerator = (IMMDeviceEnumerator)Activator.CreateInstance(Type.GetTypeFromCLSID(DeviceEnumeratorId)!)!;
            enumerator.GetDefaultAudioEndpoint(0, 0, out device);
            Guid audioClientId = AudioClientId;
            device.Activate(ref audioClientId, 23, IntPtr.Zero, out IntPtr clientPointer);
            try { client = (IAudioClient)Marshal.GetObjectForIUnknown(clientPointer); }
            finally { Marshal.Release(clientPointer); }
            client.GetMixFormat(out mixFormat);
            int formatSize = 18 + Marshal.ReadInt16(mixFormat, 16);
            if (formatSize is < 18 or > 256) throw new InvalidDataException("Windows 오디오 형식 길이가 유효하지 않습니다.");
            byte[] format = new byte[formatSize];
            Marshal.Copy(mixFormat, format, 0, formatSize);
            int blockAlign = Marshal.ReadInt16(mixFormat, 12);
            int sampleRate = Marshal.ReadInt32(mixFormat, 4);
            if (blockAlign <= 0) throw new InvalidDataException("Windows 오디오 블록 크기가 유효하지 않습니다.");
            if (sampleRate <= 0) throw new InvalidDataException("Windows 오디오 샘플 속도가 유효하지 않습니다.");
            Guid empty = Guid.Empty;
            client.Initialize(0, LoopbackFlag, 0, 0, mixFormat, ref empty);
            Guid captureClientId = CaptureClientId;
            client.GetService(ref captureClientId, out IntPtr capturePointer);
            try { capture = (IAudioCaptureClient)Marshal.GetObjectForIUnknown(capturePointer); }
            finally { Marshal.Release(capturePointer); }
            DateTimeOffset streamStartedUtc = DateTimeOffset.UtcNow;
            long streamStartedCounter = System.Diagnostics.Stopwatch.GetTimestamp();
            long writtenFrames = 0;
            ulong? firstDevicePosition = null;
            long firstPacketOffsetFrames = 0;
            client.Start();
            _ready.Set();
            // 게임이 조용한 동안에도 취소를 확인하며 캡처 버퍼를 짧은 간격으로 비운다.
            while (!_stop.IsCancellationRequested)
            {
                capture.GetNextPacketSize(out uint available);
                if (available == 0)
                {
                    Thread.Sleep(10);
                    continue;
                }
                capture.GetBuffer(out IntPtr buffer, out uint frames, out uint flags, out ulong devicePosition, out ulong qpcPosition);
                try
                {
                    // 장치 위치는 큰 절대값으로 시작할 수 있으므로 첫 패킷을 기준으로 정규화한다.
                    if (firstDevicePosition == null)
                    {
                        firstDevicePosition = devicePosition;
                        long startedQpc100ns = (long)(streamStartedCounter * (10_000_000.0 / System.Diagnostics.Stopwatch.Frequency));
                        if (qpcPosition > (ulong)startedQpc100ns)
                        {
                            long maximumOffset = checked((long)(System.Diagnostics.Stopwatch.GetElapsedTime(streamStartedCounter).TotalSeconds * sampleRate) + sampleRate);
                            firstPacketOffsetFrames = Math.Clamp((long)((qpcPosition - (ulong)startedQpc100ns) * (double)sampleRate / 10_000_000), 0, maximumOffset);
                        }
                    }
                    ulong relativePosition = devicePosition >= firstDevicePosition.Value ? devicePosition - firstDevicePosition.Value : 0;
                    long maximumTarget = checked((long)(System.Diagnostics.Stopwatch.GetElapsedTime(streamStartedCounter).TotalSeconds * sampleRate) + sampleRate);
                    long targetFrame = Math.Min(firstPacketOffsetFrames + checked((long)relativePosition), maximumTarget);
                    // 장치가 무음 기간의 패킷을 주지 않아도 실제 시간만큼 0 샘플을 채워 동기화를 유지한다.
                    if (targetFrame > writtenFrames)
                        WriteSilence((ulong)(targetFrame - writtenFrames), blockAlign, sampleRate, format, streamStartedUtc, ref writtenFrames, ref output);
                    int bytes = checked((int)frames * blockAlign);
                    byte[] samples = new byte[bytes];
                    if ((flags & SilentFlag) == 0 && bytes != 0) Marshal.Copy(buffer, samples, 0, bytes);
                    if (output == null || output.Length + bytes >= SegmentLimitBytes)
                    {
                        output?.Dispose();
                        output = OpenSegment(format, streamStartedUtc.AddSeconds((double)writtenFrames / sampleRate));
                    }
                    output.Write(samples);
                    writtenFrames += frames;
                }
                finally { capture.ReleaseBuffer(frames); }
            }
            client.Stop();
            long finalFrames = checked((long)(System.Diagnostics.Stopwatch.GetElapsedTime(streamStartedCounter).TotalSeconds * sampleRate));
            if (finalFrames > writtenFrames)
                WriteSilence((ulong)(finalFrames - writtenFrames), blockAlign, sampleRate, format, streamStartedUtc, ref writtenFrames, ref output);
        }
        catch (Exception error)
        {
            _error = error;
            _ready.Set();
        }
        finally
        {
            output?.Dispose();
            if (mixFormat != IntPtr.Zero) Marshal.FreeCoTaskMem(mixFormat);
            if (capture != null) Marshal.ReleaseComObject(capture);
            if (client != null) Marshal.ReleaseComObject(client);
            if (device != null) Marshal.ReleaseComObject(device);
            if (enumerator != null) Marshal.ReleaseComObject(enumerator);
        }
    }

    /// <summary>오디오 조각의 시작 시각을 인덱스에 남긴다.</summary>
    private WaveSegment OpenSegment(byte[] format, DateTimeOffset start)
    {
        _segment++;
        string name = $"audio-{_segment:0000}.wav";
        string path = Path.Combine(_directory, name);
        string index = Path.Combine(_directory, $"audio-{_segment:0000}.start.txt");
        SessionStore.RejectReparse(path);
        SessionStore.RejectReparse(index);
        SessionStore.WriteSmallFile(index, Encoding.UTF8.GetBytes($"{name},{start:O}\n"));
        return new WaveSegment(path, format);
    }

    /// <summary>소리 없는 구간을 실제 샘플 수만큼 분할해 WAV에 기록한다.</summary>
    private void WriteSilence(ulong frames, int blockAlign, int sampleRate, byte[] format, DateTimeOffset startedUtc,
        ref long writtenFrames, ref WaveSegment? output)
    {
        // 큰 무음 구간도 작은 패킷으로 나누어 단일 파일 제한과 메모리 사용량을 지킨다.
        while (frames > 0)
        {
            int count = (int)Math.Min(frames, (ulong)Math.Min(8192, (SegmentLimitBytes - 256) / blockAlign));
            byte[] zeros = new byte[checked(count * blockAlign)];
            if (output == null || output.Length + zeros.Length >= SegmentLimitBytes)
            {
                output?.Dispose();
                output = OpenSegment(format, startedUtc.AddSeconds((double)writtenFrames / sampleRate));
            }
            output.Write(zeros);
            writtenFrames += count;
            frames -= (ulong)count;
        }
    }

    /// <summary>녹화를 중지하고 WAV 헤더가 닫힐 때까지 기다린다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        _stop.Cancel();
        if (Thread.CurrentThread != _thread) _thread.Join();
        _stop.Dispose();
        _ready.Dispose();
    }

    /// <summary>분할 중에도 항상 읽을 수 있도록 데이터 길이를 갱신하는 WAV 작성기.</summary>
    private sealed class WaveSegment : IDisposable
    {
        private readonly FileStream _stream;
        private readonly BinaryWriter _writer;
        private readonly long _dataSizePosition;
        private readonly long _factFramesPosition;
        private readonly int _blockAlign;

        /// <summary>Windows 혼합 형식을 그대로 WAV fmt 청크에 기록한다.</summary>
        public WaveSegment(string path, byte[] format)
        {
            _stream = new FileStream(path, FileMode.CreateNew, FileAccess.ReadWrite, FileShare.Read);
            _writer = new BinaryWriter(_stream, Encoding.ASCII, true);
            _writer.Write(Encoding.ASCII.GetBytes("RIFF"));
            _writer.Write(0);
            _writer.Write(Encoding.ASCII.GetBytes("WAVEfmt "));
            _writer.Write(format.Length);
            _writer.Write(format);
            if ((format.Length & 1) != 0) _writer.Write((byte)0);
            _blockAlign = BitConverter.ToUInt16(format, 12);
            if (BitConverter.ToUInt16(format, 0) != 1)
            {
                _writer.Write(Encoding.ASCII.GetBytes("fact"));
                _writer.Write(4);
                _factFramesPosition = _stream.Position;
                _writer.Write(0);
            }
            _writer.Write(Encoding.ASCII.GetBytes("data"));
            _dataSizePosition = _stream.Position;
            _writer.Write(0);
            Patch();
        }

        /// <summary>완성된 패킷을 더하고 실제 파일 길이로 WAV 헤더를 갱신한다.</summary>
        public void Write(byte[] bytes)
        {
            if (_stream.Length + bytes.Length >= SegmentLimitBytes) throw new InvalidOperationException("오디오 조각이 분할 한도를 넘었습니다.");
            _writer.Write(bytes);
            Patch();
        }

        /// <summary>RIFF와 data 크기를 매 패킷 갱신하여 중간 중단에도 앞선 내용이 남게 한다.</summary>
        private void Patch()
        {
            _writer.Flush();
            long end = _stream.Position;
            _stream.Position = 4;
            _writer.Write(checked((int)(end - 8)));
            _stream.Position = _dataSizePosition;
            _writer.Write(checked((int)(end - _dataSizePosition - 4)));
            if (_factFramesPosition != 0)
            {
                _stream.Position = _factFramesPosition;
                _writer.Write(checked((int)((end - _dataSizePosition - 4) / _blockAlign)));
            }
            _stream.Position = end;
            _stream.Flush();
        }

        /// <summary>현재 WAV 조각의 실제 바이트 길이를 반환한다.</summary>
        public long Length => _stream.Length;

        /// <summary>현재 조각을 닫는다.</summary>
        public void Dispose()
        {
            _writer.Dispose();
            _stream.Dispose();
        }
    }

    /// <summary>기본 출력 장치를 찾는 Windows COM 인터페이스.</summary>
    [ComImport, Guid("A95664D2-9614-4F35-A746-DE8DB63617E6"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IMMDeviceEnumerator
    {
        void EnumAudioEndpoints();
        void GetDefaultAudioEndpoint(int dataFlow, int role, out IMMDevice device);
        void GetDevice();
        void RegisterEndpointNotificationCallback();
        void UnregisterEndpointNotificationCallback();
    }

    /// <summary>출력 장치에서 오디오 클라이언트를 여는 Windows COM 인터페이스.</summary>
    [ComImport, Guid("D666063F-1587-4E43-81F1-B948E807363F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IMMDevice
    {
        void Activate(ref Guid iid, uint context, IntPtr activationParameters, out IntPtr instance);
        void OpenPropertyStore();
        void GetId();
        void GetState();
    }

    /// <summary>공유 오디오 스트림을 시작하고 캡처 서비스를 찾는 Windows COM 인터페이스.</summary>
    [ComImport, Guid("1CB9AD4C-DBFA-4c32-B178-C2F568A703B2"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IAudioClient
    {
        void Initialize(int shareMode, uint streamFlags, long bufferDuration, long periodicity, IntPtr format, ref Guid audioSession);
        void GetBufferSize();
        void GetStreamLatency();
        void GetCurrentPadding();
        void IsFormatSupported();
        void GetMixFormat(out IntPtr format);
        void GetDevicePeriod();
        void Start();
        void Stop();
        void Reset();
        void SetEventHandle();
        void GetService(ref Guid iid, out IntPtr service);
    }

    /// <summary>기본 출력 장치의 재생된 버퍼를 읽는 Windows COM 인터페이스.</summary>
    [ComImport, Guid("C8ADBD64-E71E-48a0-A4DE-185C395CD317"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IAudioCaptureClient
    {
        void GetBuffer(out IntPtr data, out uint frames, out uint flags, out ulong devicePosition, out ulong qpcPosition);
        void ReleaseBuffer(uint frames);
        void GetNextPacketSize(out uint frames);
    }
}
