using System.Threading.Channels;

namespace Netstorm.AnalyzeManager;

/// <summary>입력 훅을 디스크 저장과 분리하고 제한된 대기열을 순서대로 비운다.</summary>
public sealed class GuidedInputBuffer : IDisposable
{
    /// <summary>마우스 이동 50회/초로 약 80초를 수용하되 메모리를 무한히 늘리지 않는다.</summary>
    private const int DefaultCapacity = 4096;
    private readonly Channel<Entry> _channel;
    private readonly Action<GuidedInput, DateTimeOffset, long> _write;
    private readonly Thread _thread;
    private Exception? _error;
    private int _disposed;

    /// <summary>훅 진입 시각과 입력 스냅샷을 함께 보존한다.</summary>
    private sealed record Entry(GuidedInput Input, DateTimeOffset Utc, long Counter);

    /// <summary>저장 실패나 대기열 포화를 녹화 창이 오류로 표시하게 한다.</summary>
    public Exception? Error => Volatile.Read(ref _error);

    /// <summary>저장은 전용 스레드에서만 실행하고 호출자 스레드에서는 기다리지 않는다.</summary>
    public GuidedInputBuffer(Action<GuidedInput, DateTimeOffset, long> write, int capacity = DefaultCapacity)
    {
        ArgumentNullException.ThrowIfNull(write);
        if (capacity <= 0) throw new ArgumentOutOfRangeException(nameof(capacity));
        _write = write;
        _channel = Channel.CreateBounded<Entry>(new BoundedChannelOptions(capacity)
        {
            SingleReader = true, SingleWriter = false, AllowSynchronousContinuations = false,
            FullMode = BoundedChannelFullMode.Wait,
        });
        _thread = new Thread(Run) { IsBackground = true, Name = "NetStorm 분석 입력 저장" };
        _thread.Start();
    }

    /// <summary>훅은 대기 없이 입력을 넘긴다. 포화 때 조용히 누락시키지 않고 오류로 중단한다.</summary>
    public bool TryWrite(GuidedInput input, DateTimeOffset utc, long counter)
    {
        if (Volatile.Read(ref _disposed) != 0 || Error != null) return false;
        if (_channel.Writer.TryWrite(new(input, utc, counter))) return true;
        Interlocked.CompareExchange(ref _error, new InvalidOperationException("입력 저장 대기열이 가득 찼습니다. 녹화를 다시 시작하세요."), null);
        return false;
    }

    /// <summary>받아 둔 입력만 순서대로 저장하며 파일 오류는 호출자에게 전달한다.</summary>
    private void Run()
    {
        try
        {
            // 입력이 오거나 종료될 때까지 저장 스레드에서만 기다린다.
            while (_channel.Reader.WaitToReadAsync().AsTask().GetAwaiter().GetResult())
            {
                // 종료 전까지 수락한 입력의 시각·순서를 그대로 유지한다.
                while (_channel.Reader.TryRead(out Entry? entry)) _write(entry.Input, entry.Utc, entry.Counter);
            }
        }
        catch (Exception error) { Interlocked.CompareExchange(ref _error, error, null); }
    }

    /// <summary>새 입력을 막고 수락한 입력이 저장된 뒤 반환한다.</summary>
    public void Dispose()
    {
        if (Interlocked.Exchange(ref _disposed, 1) != 0) return;
        _channel.Writer.TryComplete();
        _thread.Join();
    }
}
