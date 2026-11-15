using System.Buffers.Binary;
using System.Runtime.CompilerServices;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 가상 분배가 실제 접두·전체 파생 몸체·장기 owner 수명을 보존하는지 확인한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj PostPopTests의 현재 다리/섬/텔레포트와 장기 분배 경계를 옮긴다.</summary>
public sealed class PostPopDispatchCurrentTests
{
    /// <summary>실제 슬롯·현재 TYPE·공통 장부를 같은 주소로 제공하는 입력 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>입력 슬롯만 읽고 네트워크를 열지 않는 실제 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>같은 현재188개 TYPE다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>공통 부모가 읽는 같은 깊이/억제/통계다.</summary>
        public readonly RawPostPopState State = new() { Depth = 1, Suppressed = true };
        /// <summary>실제 현재 가상 분배기다.</summary>
        public readonly RawPostPop Body;
        /// <summary>현재50번 raw 물리다. 할당/자산 사전 검사와 구별한다.</summary>
        public Span<byte> Raw => Pool.RawSlot(50);
        /// <summary>선택한 원본 가상표를 입력으로 기록하고 실행 결과는 만들지 않는다.</summary>
        public Scene(uint table = 0x506d98, Action<int>? connector = null)
        {
            Raw.Clear(); SetTable(table); Raw[10] = 74; Body = new(Pool, Types, State, bridgeConnector: connector); Body.BindCurrentTypes(Types);
        }
        /// <summary>실제 가상표 DWORD를 낮은 바이트 순서로 쓴다.</summary>
        public void SetTable(uint table) => BinaryPrimitives.WriteUInt32LittleEndian(Raw, table);
    }

    /// <summary>미선택 다리 플래그는 raw SID와 연결 장치를 읽지 않는다.</summary>
    [Theory]
    [InlineData(0u)]
    [InlineData(2u)]
    [InlineData(0x20000u)]
    public void BridgeUnselectedSkipsInvalidPhysicalSid(uint flags) => RawPostPop.BridgePrefix(new(24000, false), 65000, flags, null);

    /// <summary>free/contained/둘 다인 물리도 최초 비트2를 저장한 뒤 장치 누락에서 실패한다.</summary>
    [Theory]
    [InlineData(1)]
    [InlineData(8)]
    [InlineData(9)]
    public void BridgeSelectedFailureKeepsFirstStore(byte life)
    {
        var s = new Scene(0x5034c8); s.Raw[10] = 255; s.Raw[11] = life; s.Raw[40] = 0x10; byte[] expected = s.Raw.ToArray(); expected[40] = 0x12;
        Assert.Throws<InvalidOperationException>(() => s.Body.PostPopVirtual(50, 1)); Assert.Equal(expected, s.Raw.ToArray()); Assert.Equal(1u, s.State.Depth);
    }

    /// <summary>flags4의 현재 abstract 관문은 쓰기 없이 장치만 선택한다.</summary>
    [Fact]
    public void BridgeFlag4UsesOnlyCurrentAbstractGate()
    {
        var s = new Scene(0x5034c8); s.Raw[40] = 1; RawPostPop.BridgePrefix(s.Pool, 50, 4, null); Assert.Equal(1, s.Raw[40]);
        s.Raw[40] = 0; Assert.Throws<InvalidOperationException>(() => RawPostPop.BridgePrefix(s.Pool, 50, 4, null)); Assert.Equal(0, s.Raw[40]);
    }

    /// <summary>CD 독립 접두는 +35만 쓰고 선택 장치 누락을 첫 쓰기 전에 거부한다.</summary>
    [Fact]
    public void CdBridgePrefixKeepsLegacyOffsetAndPreflight()
    {
        var pool = new RawSidPool(24000, false, edition: OriginalEdition.Cd1072); int sid = pool.Allocate(SidAllocation.Client);
        pool.RawSlot(sid)[35] = 1; pool.RawSlot(sid)[40] = 0x80; byte[] before = pool.RawSlot(sid).ToArray();
        Assert.Throws<ArgumentException>(() => RawPostPop.BridgePrefix(pool, sid, 1, null)); Assert.Equal(before, pool.RawSlot(sid).ToArray());
        int calls = 0; RawPostPop.BridgePrefix(pool, sid, 1, _ => { calls++; Assert.Equal(3, pool.Slot(sid)[35]); });
        Assert.Equal(1, calls); Assert.Equal(0x80, pool.RawSlot(sid)[40]);
    }

    /// <summary>정상 복귀/예외 뒤 raw 변경을 복원하거나 최초 비트를 다시 쓰지 않는다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void BridgeCallbackKeepsCurrentMutations(bool fail)
    {
        Scene? s = null; s = new(0x5034c8, sid =>
        {
            Assert.Equal(50, sid); Assert.Equal(2, s!.Raw[40]); s.Raw[10] = 255; s.Raw[11] = 9; s.Raw[40] = 0; s.SetTable(0);
            if (fail) throw new InvalidOperationException("명시 다리 중단");
        });
        if (fail) Assert.Throws<InvalidOperationException>(() => s.Body.PostPopVirtual(50, 1)); else s.Body.PostPopVirtual(50, 1);
        Assert.Equal(255, s.Raw[10]); Assert.Equal(9, s.Raw[11]); Assert.Equal(0, s.Raw[40]); Assert.Equal(fail ? 1u : 0u, s.State.Depth);
    }

    /// <summary>두 섬/표면과 텔레포트의 비최초 호출은 TYPE/free와 무관하게 미연결 접두를 생략한다.</summary>
    [Theory]
    [InlineData(0x506d98u)]
    [InlineData(0x506f08u)]
    [InlineData(0x5070c0u)]
    public void NonFirstUsesActualVtableWithoutTypeOrDeviceGate(uint table)
    {
        var s = new Scene(table); s.Raw[10] = 255; s.Raw[11] = 9; s.Raw[40] = 8; byte[] before = s.Raw.ToArray();
        // 최초 비트가 없는 세 입력은 같은 suppressed 공통 부모만 호출한다.
        foreach (uint flags in new[] { 0u, 2u, 4u })
        { s.State.Depth = 1; s.Body.PostPopVirtual(50, flags); Assert.Equal(0u, s.State.Depth); Assert.Equal(before, s.Raw.ToArray()); }
    }

    /// <summary>최초 선택 접두 누락은 공통 앞에서 같은 깊이/물리를 유지하며 실패한다.</summary>
    [Theory]
    [InlineData(0x506d98u)]
    [InlineData(0x506f08u)]
    [InlineData(0x5070c0u)]
    public void FirstRequiresSelectedPrefixBeforeCommon(uint table)
    {
        var s = new Scene(table); byte[] before = s.Raw.ToArray(); Assert.Throws<InvalidOperationException>(() => s.Body.PostPopVirtual(50, 1));
        Assert.Equal(before, s.Raw.ToArray()); Assert.Equal(1u, s.State.Depth);
    }

    /// <summary>접두의 자기 연결 반납/TYPE/수명/VT 변경을 현재 선택 호출과 공통 몸체 끝까지 유지한다.</summary>
    [Theory]
    [InlineData(0x506d98u)]
    [InlineData(0x506f08u)]
    [InlineData(0x5070c0u)]
    public void SelectedPrefixCanUnbindAndReplaceItsPhysicalType(uint table)
    {
        var s = new Scene(table); int calls = 0;
        Action<int, uint> prefix = (sid, flags) =>
        {
            Assert.Equal(50, sid); Assert.Equal(3u, flags); calls++; s.Body.SetIslandPrefix(null); s.Body.SetSurfacePrefix(null); s.Body.SetTeleportPrefix(null);
            s.Raw[10] = 255; s.Raw[11] = 9; s.SetTable(0);
        };
        s.Body.SetIslandPrefix(prefix); s.Body.SetSurfacePrefix(prefix); s.Body.SetTeleportPrefix(prefix); s.Body.PostPopVirtual(50, 3);
        Assert.Equal(1, calls); Assert.Equal(0u, s.State.Depth); Assert.Equal(255, s.Raw[10]); Assert.Equal(9, s.Raw[11]);
    }

    /// <summary>텔레포트 생성 뒤 현재 TYPE 통계를 쓴 다음 선택 하위에서 실패하면 앞부분만 남는다.</summary>
    [Fact]
    public void TeleportLateCommonFailureKeepsCreationAndCurrentLedger()
    {
        var s = new Scene(0x5070c0); s.State.Suppressed = false; s.State.LocalOwner = 1; s.State.PendingPlacement = true; s.Raw[34] = 1;
        int calls = 0; s.Body.SetTeleportPrefix((_, _) => { calls++; s.Raw[10] = 75; });
        Assert.Throws<InvalidOperationException>(() => s.Body.PostPopVirtual(50, 0x2000001)); Assert.Equal(1, calls); Assert.Equal(1u, s.State.GlobalCounts[75]); Assert.Equal(0u, s.State.GlobalCounts[74]); Assert.Equal(1u, s.State.Depth);
    }

    /// <summary>명시 Validate/Activate의 raw 거부와 실제 PATCH 가상 실행을 구분한다.</summary>
    [Fact]
    public void ExplicitPreflightRemainsSeparateFromVirtualDispatch()
    {
        var s = new Scene(0x5070c0); s.Raw[11] = 1; int calls = 0; s.Body.SetTeleportPrefix((_, _) => calls++);
        Assert.Throws<InvalidOperationException>(() => s.Body.Activate(50, 1)); Assert.Equal(0, calls); Assert.Equal(1u, s.State.Depth);
        s.Body.PostPopVirtual(50, 1); Assert.Equal(1, calls); Assert.Equal(0u, s.State.Depth);
    }

    /// <summary>실제 다리/섬 사전 검사는 선택된 장치 누락을 비트 쓰기 전에 거부한다.</summary>
    [Theory]
    [InlineData(0x5034c8u)]
    [InlineData(0x506d98u)]
    [InlineData(0x506f08u)]
    public void ExplicitSurfacePreflightHasNoPrefixEffects(uint table)
    {
        var s = new Scene(table); byte[] before = s.Raw.ToArray(); Assert.Throws<InvalidOperationException>(() => s.Body.Activate(50, 1));
        Assert.Equal(before, s.Raw.ToArray()); Assert.Equal(1u, s.State.Depth);
    }

    /// <summary>장기 실제 자원 분배가 없는 전용 VT를 공통 성공으로 처리하지 않는다.</summary>
    [Fact]
    public void NativeDedicatedBodiesFailAtSelectedDeviceAndUseCurrentBinding()
    {
        var s = new Scene(); s.Body.BindDefaultPatchDevices(new() { AiNotice = (_, _, _) => throw new Exception("미도달 AI"), LocalOwner = () => throw new Exception("미도달 owner"), RecheckSelection = () => throw new Exception("미도달 선택") });
        uint[] tables = [0x508cc8, 0x502c38, 0x502cf0, 0x502db0, 0x507770, 0x509148, 0x509210, 0x5040c0, 0x504198, 0x50a748, 0x50e888, 0x509470, 0x501ea0, 0x503d80, 0x503e50, 0x50e568, 0x50a820, 0x50a8e8];
        // 각 장기 전용 VT는 동일한 설치 전 관문과 설치 후 실제 현재 callback을 선택한다.
        foreach (uint table in tables)
        {
            s.SetTable(table); Assert.Throws<InvalidOperationException>(() => s.Body.PostPopVirtual(50, 0)); Assert.Equal(1u, s.State.Depth);
        }
        int calls = 0; s.Body.BindDefaultResourcePostPop((sid, flags) => { Assert.Equal(50, sid); Assert.Equal(2u, flags); calls++; throw new ApplicationException("실제 선택 하위 중단"); });
        Assert.Throws<ApplicationException>(() => s.Body.PostPopVirtual(50, 2)); Assert.Equal(1, calls); Assert.Equal(1u, s.State.Depth);
    }

    /// <summary>복원한 직접 Carrier/Damageable만 같은 풀로 등록하며 공통 깊이를 중복 감소시키지 않는다.</summary>
    [Theory]
    [InlineData(0x503d80u, true)]
    [InlineData(0x503e50u, true)]
    [InlineData(0x50e568u, true)]
    [InlineData(0x50a820u, false)]
    [InlineData(0x50a8e8u, false)]
    public void RegisteredCarrierAndDamageableUseTheirWholeBody(uint table, bool carrier)
    {
        var s = new Scene(table); s.Raw[40] = 1; int checks = 0, common = 0;
        var body = new RawCarrierPostPop(s.Pool, s.Types, new() { Boss = false }, new()
        {
            CarrierCheck = _ => { checks++; s.Body.SetCarrierPostPop(null); },
            Base = (sid, flags) => { common++; s.Body.PostPopBase(sid, flags); },
        });
        s.Body.SetCarrierPostPop(body); Assert.Equal(carrier, s.Body.HandlesCarrier(50)); Assert.Equal(!carrier, s.Body.HandlesDamageable(50));
        s.Body.PostPopVirtual(50, 0); Assert.Equal(carrier ? 1 : 0, checks); Assert.Equal(1, common); Assert.Equal(0u, s.State.Depth);
        var other = new RawCarrierPostPop(new(24000, false), s.Types, new(), new()); Assert.Throws<ArgumentException>(() => s.Body.SetCarrierPostPop(other));
    }

    /// <summary>실제 호출 소유자가 외부 참조를 반납해도 파생 접두와 공통 선택 끝까지 살아 있게 한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void RuntimeOwnerIsPinnedAcrossPrefixAndFailure(bool fail)
    {
        WeakReference weak = RunPinnedScene(fail); Collect(); Assert.False(weak.IsAlive);
    }

    /// <summary>객체 생성/호출 프레임을 수집 검사와 분리해 JIT 임시 강한 참조가 결과를 가리지 않게 한다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference RunPinnedScene(bool fail)
    {
        var s = new Scene(0x5070c0); object? runtime = new object(); var weak = new WeakReference(runtime); s.Body.BindRuntimePin(() => runtime);
        s.Body.SetTeleportPrefix((_, _) =>
        {
            runtime = null; s.Body.BindRuntimePin(null); Collect(); Assert.True(weak.IsAlive);
            if (fail) throw new ApplicationException("명시 pin 실패");
        });
        if (fail) Assert.Throws<ApplicationException>(() => s.Body.PostPopVirtual(50, 1)); else s.Body.PostPopVirtual(50, 1);
        Assert.Equal(fail ? 1u : 0u, s.State.Depth); return weak;
    }

    /// <summary>호출 프레임의 실제 KeepAlive 경계를 검사할 때만 수집기를 강제한다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void Collect() { GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); }
}
