// 프로세스 기본·정기의 식별자·예약 자료를 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 프로세스 기본을 검증하는 테스트 클래스다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.
public sealed class RawSquidProcessTests
{
    /// <summary>등록 번호를 읽는 검사 전용 파생이다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    private sealed class PidProcess : SquidProcess
    {
        /// <summary>부착 전 등록 번호를 공개한다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
        /// <returns>등록 번호다.</returns>
        public uint ExposedPid() => Pid();

        /// <summary>부착 전 호스트를 공개한다. 2026-11-07 F2d 프로세스 호출 수명 이식에서 추가됐다.</summary>
        /// <returns>부착한 호스트다.</returns>
        public SquidProcessHost? ExposedHost() => Host();
    }

    /// <summary>부착 전 식별자는 모두 0이다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    [Fact]
    public void DetachedIdentityIsZero()
    {
        var process = new PidProcess();
        Assert.Equal(0, process.Form());
        Assert.Equal(0, process.Parent());
        Assert.Equal(0u, process.ExposedPid());
    }

    /// <summary>원본 base의 세 가상 통지는 빈 함수다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    [Fact]
    public void BaseVirtualsAreEmpty()
    {
        var process = new SquidProcess();
        process.OnFormDestroy(0u);
        process.OnParentDying(0);
        process.OnAttached();
    }

    /// <summary>복원하지 않은 파생 통지는 선택 시 예외로 거부한다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    [Fact]
    public void NotifyThrowsUntilRestored()
    {
        Assert.Throws<InvalidOperationException>(() => new SquidProcess().Notify(1u, 2u, 3u));
        Assert.Throws<InvalidOperationException>(() => new RegularProcess(4u, 1.5f).Notify(1u, 2u, 3u));
    }

    /// <summary>생성자는 event·payload만 둔다. 횟수·시각·공유는 초기값이다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    [Fact]
    public void RegularCtorKeepsEventAndPayload()
    {
        var process = new RegularProcess(0x5102u, 0.05f);
        Assert.Equal(0x5102u, process.Event());
        Assert.Equal(0.05f, process.Payload());
        Assert.Equal(0u, process.Count());
        Assert.Equal(0.0, process.Time());
        Assert.False(process.Shared());
        Assert.Equal(0, process.Form());
        Assert.Equal(0, process.Parent());
    }

    /// <summary>작업 재설정·기한 덮기·초기 횟수는 다른 필드를 보존한다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    [Fact]
    public void RegularTaskWritersPreserveOtherFields()
    {
        var process = new RegularProcess(0x5101u, 2.5f);
        process.SetInitialCount(4u);
        process.ResetTask(0x5102u, 17.5);
        Assert.Equal(0x5102u, process.Event());
        Assert.Equal(0u, process.Count());
        Assert.Equal(17.5, process.Time());
        Assert.Equal(2.5f, process.Payload());
        process.SetNextTime(33.25);
        Assert.Equal(33.25, process.Time());
        Assert.Equal(0x5102u, process.Event());
        process.SetInitialCount(4u);
        Assert.Equal(4u, process.Count());
        Assert.Equal(33.25, process.Time());
    }

    /// <summary>실행 본체는 Host 본체 단위에서 잇는다. 미연결 호출은 명시 실패한다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    [Fact]
    public void RegularRunFrameThrowsUntilHost()
    {
        Assert.Throws<InvalidOperationException>(() => new RegularProcess(1u, 0f).RunFrame());
    }

    /// <summary>빈 guard는 아무것도 세지 않는다. 2026-11-07 F2d 프로세스 호출 수명 이식에서 추가됐다.</summary>
    [Fact]
    public void RuntimeCallGuardNullIsNoop()
    {
        using (var guard = new RuntimeCallGuard(null))
        {
        }
    }

    /// <summary>호출 수명은 묶음의 대여 수를 올리고 정상·예외 복귀 때 내린다. 2026-11-07 F2d 프로세스 호출 수명 이식에서 추가됐다.</summary>
    [Fact]
    public void RuntimeCallGuardCountsActiveCalls()
    {
        var bindings = new ProcessHostRuntimeBindings();
        Assert.Equal(0ul, bindings.ActiveCalls);
        var guard = new RuntimeCallGuard(bindings);
        Assert.Equal(1ul, bindings.ActiveCalls);
        guard.Dispose();
        Assert.Equal(0ul, bindings.ActiveCalls);
        guard.Dispose();
        Assert.Equal(0ul, bindings.ActiveCalls);
        try
        {
            using (var scoped = new RuntimeCallGuard(bindings))
            {
                Assert.Equal(1ul, bindings.ActiveCalls);
                throw new InvalidOperationException("호출 실패");
            }
        }
        catch (InvalidOperationException)
        {
        }

        Assert.Equal(0ul, bindings.ActiveCalls);
        using (var outer = new RuntimeCallGuard(bindings))
        {
            using (var inner = new RuntimeCallGuard(bindings))
            {
                Assert.Equal(2ul, bindings.ActiveCalls);
            }

            Assert.Equal(1ul, bindings.ActiveCalls);
        }

        Assert.Equal(0ul, bindings.ActiveCalls);
    }

    /// <summary>부착 식별자·호스트·런타임 배선은 Host 본체 단위에서 쓴다. 2026-11-07 F2d 프로세스 호출 수명 이식에서 추가됐다.</summary>
    [Fact]
    public void AttachmentWiringPinsHostAndIdentity()
    {
        var process = new PidProcess();
        Assert.Null(process.ExposedHost());
        Assert.Null(process.RuntimeLink);
        process.SetIdentity(5, 7, 3u);
        Assert.Equal(5, process.Form());
        Assert.Equal(7, process.Parent());
        Assert.Equal(3u, process.ExposedPid());
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var host = new SquidProcessHost(pool, types, new Kernel(8u), new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types), new SquidProcessState());
        var runtime = new ProcessHostRuntimeBindings();
        process.HostLink = host;
        process.RuntimeLink = runtime;
        Assert.Same(host, process.ExposedHost());
        Assert.Same(runtime, process.RuntimeLink);
    }
}
