using System.Collections.Concurrent;

namespace Netstorm.AnalyzeManager.Tests;

/// <summary>게임·전역 훅 없이 느린 저장·순서 유지·포화·오류 전달을 검사한다.</summary>
public sealed class GuidedInputBufferTests
{
    /// <summary>저장이 막혀도 입력을 넘기는 호출은 기다리지 않고 돌아온다.</summary>
    [Fact]
    public async Task SlowWriterDoesNotBlockProducerAndDisposeDrains()
    {
        using var entered = new ManualResetEventSlim();
        using var release = new ManualResetEventSlim();
        var saved = new ConcurrentQueue<long>();
        var buffer = new GuidedInputBuffer((_, _, counter) => { entered.Set(); release.Wait(); saved.Enqueue(counter); });
        try
        {
            Assert.True(buffer.TryWrite(new("keyboard", "down"), DateTimeOffset.UtcNow, 1));
            Assert.True(entered.Wait(TimeSpan.FromSeconds(5), TestContext.Current.CancellationToken));
            Task<bool> producer = Task.Run(() => buffer.TryWrite(new("keyboard", "up"), DateTimeOffset.UtcNow, 2));
            Assert.True(await producer.WaitAsync(TimeSpan.FromSeconds(2), TestContext.Current.CancellationToken));
            Assert.Empty(saved);
        }
        finally { release.Set(); buffer.Dispose(); }
        Assert.Equal(new long[] { 1, 2 }, saved);
        Assert.Null(buffer.Error);
        Assert.False(buffer.TryWrite(new("keyboard", "down"), DateTimeOffset.UtcNow, 3));
    }

    /// <summary>대기열 포화는 입력을 기다리게 하지 않고 명시적인 오류로 남긴다.</summary>
    [Fact]
    public void FullQueueReportsErrorWithoutBlocking()
    {
        using var entered = new ManualResetEventSlim();
        using var release = new ManualResetEventSlim();
        var saved = new ConcurrentQueue<long>();
        var buffer = new GuidedInputBuffer((_, _, counter) => { entered.Set(); release.Wait(); saved.Enqueue(counter); }, 1);
        try
        {
            Assert.True(buffer.TryWrite(new("mouse", "down"), DateTimeOffset.UtcNow, 1));
            Assert.True(entered.Wait(TimeSpan.FromSeconds(5), TestContext.Current.CancellationToken));
            Assert.True(buffer.TryWrite(new("mouse", "up"), DateTimeOffset.UtcNow, 2));
            Assert.False(buffer.TryWrite(new("mouse", "down"), DateTimeOffset.UtcNow, 3));
            Assert.IsType<InvalidOperationException>(buffer.Error);
        }
        finally { release.Set(); buffer.Dispose(); }
        Assert.Equal(new long[] { 1, 2 }, saved);
    }

    /// <summary>저장 작업 실패를 녹화기에서 확인할 수 있고 종료가 멈추지 않는다.</summary>
    [Fact]
    public void WriterFailureIsReported()
    {
        var failure = new IOException("테스트 저장 실패");
        using var buffer = new GuidedInputBuffer((_, _, _) => throw failure);
        Assert.True(buffer.TryWrite(new("keyboard", "down"), DateTimeOffset.UtcNow, 1));
        Assert.True(SpinWait.SpinUntil(() => buffer.Error != null, TimeSpan.FromSeconds(5)));
        Assert.Same(failure, buffer.Error);
        Assert.False(buffer.TryWrite(new("keyboard", "up"), DateTimeOffset.UtcNow, 2));
    }

    /// <summary>키·마우스가 섞여도 훅 진입 시각과 원래 입력 값·순서가 유지된다.</summary>
    [Fact]
    public void PreservesSnapshotsAndTimestampsInOrder()
    {
        var saved = new List<(GuidedInput, DateTimeOffset, long)>();
        var utc = new DateTimeOffset(2026, 10, 1, 14, 0, 0, TimeSpan.Zero);
        var first = new GuidedInput("mouse", "down") { ScreenX = 321, ScreenY = 123, HookTimeMs = 456 };
        var second = new GuidedInput("keyboard", "up") { Key = "F6" };
        using (var buffer = new GuidedInputBuffer((input, stamp, counter) => saved.Add((input, stamp, counter))))
        {
            Assert.True(buffer.TryWrite(first, utc, 100));
            Assert.True(buffer.TryWrite(second, utc.AddMilliseconds(20), 200));
        }
        Assert.Equal(new[] { (first, utc, 100L), (second, utc.AddMilliseconds(20), 200L) }, saved);
    }
}
