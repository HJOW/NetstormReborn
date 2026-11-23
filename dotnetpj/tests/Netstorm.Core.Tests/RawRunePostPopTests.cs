using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// Rune 후처리의 조회·시간 helper와 PostPop 분기를 기대값 수정 없이 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 Rune 예약/신전조회/건설시간과 공통 호출 순서를 확인한다.</summary>
public sealed class RawRunePostPopTests
{
    /// <summary>같은 풀/해시/TYPE 표를 공유하는 장면이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>같은 실제 풀이다.</summary>
        internal RawSidPool Pool { get; } = new(24000, false);
        /// <summary>같은 현재 해시다.</summary>
        internal RawSquidHash Hash { get; } = new();
        /// <summary>현재 TYPE 표다.</summary>
        internal OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>공통 후처리의 실제 몸체다. 표시는 억제해 공간 쓰기만 둔다.</summary>
        internal RawPostPop Base { get; }
        /// <summary>현재 원본 전역이다.</summary>
        internal RunePostPopState State { get; } = new();
        /// <summary>실제 몸체와 효과를 빌리는 공장이다.</summary>
        internal RawRunePostPop Body { get; }
        /// <summary>같은 풀에 Rune166을 배치한다.</summary>
        internal Scene()
        {
            for (int sid = 5; sid <= 53; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            Base = new RawPostPop(Pool, Types, new RawPostPopState { Suppressed = true });
            Body = new RawRunePostPop(Pool, Hash, Types, State, Base, new());
        }
        /// <summary>같은 실제 풀을 반납한다.</summary>
        public void Dispose() { }
    }
    /// <summary>지정 슬롯의 원문 필드를 직접 쓰고 같은 해시 셀에 등록한다.</summary>
    private static void Place(Scene scene, int sid, uint type, float x, float y, int owner)
    {
        Span<byte> raw = scene.Pool.RawSlot(sid); raw.Clear();
        ref ushort head = ref scene.Hash.Bucket(1, x, y); BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], head); head = (ushort)sid;
        raw[10] = (byte)type; BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y);
        BinaryPrimitives.WriteUInt16LittleEndian(raw[34..], (ushort)owner);
    }
    /// <summary>지정 슬롯을 Rune166 free 형태로 둔다.</summary>
    private static void PlaceRune(Scene scene, int sid, float x, float y, int owner)
    {
        Span<byte> raw = scene.Pool.RawSlot(sid); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x005075d8);
        raw[10] = 166; raw[11] = 1; BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y);
        BinaryPrimitives.WriteUInt16LittleEndian(raw[34..], (ushort)owner);
    }
    /// <summary>PATCH 가상 진입은 현재 TYPE와 무관하다. CD 결합은 RawPostPop이 188 고정을 풀 때까지 후속이다.</summary>
    [Fact]
    public void HandlesSelectsPatchVtableRegardlessOfType()
    {
        using var scene = new Scene(); PlaceRune(scene, 50, 20.75F, 21.25F, 1);
        Assert.True(scene.Body.Handles(50));
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Pool.RawSlot(50), 0); Assert.False(scene.Body.Handles(50));
    }
    /// <summary>타입/가상표와 몸체/판본 연결을 거부한다. CD 결합은 RawPostPop이 188 고정을 풀 때까지 후속이다.</summary>
    [Fact]
    public void ValidateRejectsTypeMismatchAndUnconnectedBody()
    {
        using var scene = new Scene(); PlaceRune(scene, 50, 20.75F, 21.25F, 1);
        scene.Pool.RawSlot(50)[10] = 165; Assert.Throws<InvalidOperationException>(() => scene.Body.Validate(50, 0));
        var other = new RawSidPool(24000, false);
        Assert.Throws<ArgumentException>(() => new RawRunePostPop(other, scene.Hash, scene.Types, new(), scene.Base, new()));
        Assert.Throws<ArgumentNullException>(() => new RawRunePostPop(null!, scene.Hash, scene.Types, new(), scene.Base, new()));
    }
    /// <summary>현재 좌표와 patch+34/CD+32 owner를 읽는다.</summary>
    [Fact]
    public void PositionAndOwnerReadCurrentFields()
    {
        using var scene = new Scene(); PlaceRune(scene, 50, 20.75F, 21.25F, 3);
        Assert.Equal(new RawPathPoint(20.75F, 21.25F), scene.Body.Position(50)); Assert.Equal(3u, scene.Body.Owner(50));
    }
    /// <summary>반경1 finder에서 같은 owner의 첫 Dais만 읽는다.</summary>
    [Fact]
    public void FindOwnedDaisSelectsSameOwnerInRadius()
    {
        using var scene = new Scene();
        PlaceRune(scene, 50, 40.75F, 41.25F, 2);
        Place(scene, 51, 165, 40.75F, 41.25F, 2);
        Assert.Equal((ushort)51, scene.Body.FindOwnedDais(50));
        scene.Pool.RawSlot(51)[34] = 3; Assert.Equal(0, scene.Body.FindOwnedDais(50));
        scene.Pool.RawSlot(51)[34] = 2; scene.Pool.RawSlot(51)[10] = 164; Assert.Equal(0, scene.Body.FindOwnedDais(50));
    }
    /// <summary>같은 셀의 현재 타입 altar를 읽고 없으면 0을 둔다.</summary>
    [Fact]
    public void AltarAtFindsCurrentTypeAtSameCell()
    {
        using var scene = new Scene();
        PlaceRune(scene, 50, 40.75F, 41.25F, 2);
        Place(scene, 53, 164, 40.75F, 41.25F, 2);
        Assert.Equal((ushort)53, scene.Body.AltarAt(50));
        scene.State.AltarType = 999; Assert.Throws<ArgumentOutOfRangeException>(() => scene.Body.AltarAt(50));
        scene.State.AltarType = 164; scene.Pool.RawSlot(53)[10] = 163; Assert.Equal(0, scene.Body.AltarAt(50));
    }
    /// <summary>altar 부재·owner 0·억제·깃발·기록 순서로 7/10/25를 고른다.</summary>
    [Fact]
    public void ConstructionSecondsUsesAltarOwnerAndFlags()
    {
        using var scene = new Scene(); PlaceRune(scene, 50, 40.75F, 41.25F, 2);
        Assert.Equal(10, scene.Body.ConstructionSeconds(50));
        Place(scene, 53, 164, 40.75F, 41.25F, 0);
        Assert.Equal(10, scene.Body.ConstructionSeconds(50));
        BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(53)[12..], 2);
        uint flags = 0; var hooked = new RawRunePostPop(scene.Pool, scene.Hash, scene.Types, scene.State, scene.Base, new() { PlayerFlags = _ => flags });
        Assert.Equal(7, hooked.ConstructionSeconds(50)); flags = 1; Assert.Equal(25, hooked.ConstructionSeconds(50));
        var suppressed = new RawRunePostPop(scene.Pool, scene.Hash, scene.Types, scene.State, scene.Base, new() { SuppressDuration = () => true });
        Assert.Equal(10, suppressed.ConstructionSeconds(50));
        scene.State.SuppressDuration = true; Assert.Equal(10, scene.Body.ConstructionSeconds(50)); scene.State.SuppressDuration = false;
        scene.State.PlayerFlags[2] = 1; Assert.Equal(25, scene.Body.ConstructionSeconds(50));
        scene.State.PlayerFlags[2] = 0; Assert.Equal(7, scene.Body.ConstructionSeconds(50));
        BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(53)[12..], 9);
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Body.ConstructionSeconds(50));
    }
    /// <summary>예약·건설 분기와 공통 후처리를 원문 순서로 실행한다.</summary>
    [Fact]
    public void PostPopBranchesReserveConstructAndBase()
    {
        using var scene = new Scene(); PlaceRune(scene, 50, 40.75F, 41.25F, 2);
        Place(scene, 51, 165, 40.75F, 41.25F, 2); Place(scene, 53, 164, 40.75F, 41.25F, 2);
        BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(53)[12..], 2);
        var calls = new List<string>(); bool reserve = true;
        var body = new RawRunePostPop(scene.Pool, scene.Hash, scene.Types, scene.State, scene.Base, new()
        {
            ReserveRegular = () => { calls.Add("reserve"); return reserve; },
            AddRegular = (sid, ev, payload) => calls.Add($"add:{sid}:{ev}:{payload}"),
            StartConstruction = (sid, seconds) => calls.Add($"start:{sid}:{seconds}"),
            PlayerFlags = _ => 0,
        });
        body.PostPop(50, 0); Assert.Empty(calls);
        body.PostPop(50, 1); Assert.Equal(["reserve", "add:50:772:0.01"], calls);
        calls.Clear(); reserve = false; body.PostPop(50, 1); Assert.Equal(["reserve"], calls);
        calls.Clear(); reserve = true; body.PostPop(50, 3); Assert.Equal(["reserve", "add:50:772:0.01", "start:50:7"], calls);
    }
    /// <summary>같은 풀의 Host/건설 몸체를 묶고 빈 공급을 거부한다.</summary>
    [Fact]
    public void MakeRunePostPopHooksBindsHostAndConstruction()
    {
        using var scene = new Scene();
        var kernel = new Kernel();
        var destroy = new RawSquidDestroy(scene.Pool, new RawUnpop(scene.Pool, new RawSquidHash(), new byte[65536]), scene.Types);
        var host = new SquidProcessHost(scene.Pool, scene.Types, kernel, destroy, new SquidProcessState());
        var construction = new RawConstructionProcess(scene.Pool, scene.Types, new(), new ConstructionProcessHooks
        {
            Reserve = () => true,
            Effect = (_, _) => { },
            LoadSound = _ => 0,
            ReserveSound = () => false,
            AttachSound = (_, _, _, _) => { },
            Refresh = () => { },
            MaxHitPoints = _ => 0,
            UpdateDisplay = _ => { },
            RemoveContained = (_, _) => { },
            PlayAt = (_, _, _, _) => { },
            ReadySound = _ => 0,
            Play = _ => { },
            Unpop = (_, _) => { },
            Repop = (_, _) => { },
            Push = (_, _, _, _) => { },
        }, host);
        var hooks = RawRunePostPop.MakeRunePostPopHooks(scene.Pool, host, construction, new());
        Assert.NotNull(hooks.AddRegular); Assert.NotNull(hooks.StartConstruction);
        Assert.Throws<ArgumentNullException>(() => RawRunePostPop.MakeRunePostPopHooks(null!, host, construction, new()));
        var other = new RawSidPool(24000, false);
        Assert.Throws<ArgumentException>(() => RawRunePostPop.MakeRunePostPopHooks(other, host, construction, new()));
        kernel.DrainOwnedForShutdown();
    }
}
