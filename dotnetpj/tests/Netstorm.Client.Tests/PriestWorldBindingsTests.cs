using System.Buffers.Binary;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 풀·후처리·프로세스 몸체를 함께 조립하여 소유·지연 연결·수명 계약을 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>사제 월드 조립의 원문 경계와 현재 공유 저장소 검사다.</summary>
public sealed class PriestWorldBindingsTests
{
    /// <summary>합성 입력과 실제 공장·삭제·Host를 같은 풀에 둔다. 효과는 관찰 기록만 공급한다.</summary>
    private sealed class Scene
    {
        /// <summary>동일 월드의 실제 슬롯이다.</summary>
        public readonly RawSidPool Pool = new(24000, true);
        /// <summary>현재 타입 배열이며 필요한 생성자만 원본 주소로 연결한다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord { MaxHitPoints = 120 }).ToArray();
        /// <summary>현재 실제 공간 해시다.</summary>
        public readonly RawSquidHash Hash = new();
        /// <summary>공통 장부를 억제하여 조립·목록·프로세스 효과만 관찰한다.</summary>
        public readonly RawPostPopState Book = new() { Suppressed = true };
        /// <summary>같은 실제 사제 상태다.</summary>
        public readonly RawPriestWorldState State = new();
        /// <summary>현재 외부 효과의 호출 기록이다.</summary>
        public readonly List<string> Events = [];
        /// <summary>실제 프로세스 시각이다.</summary>
        public readonly SquidProcessState ProcessState = new() { Now = 10 };
        /// <summary>실제 프로세스 Kernel이다.</summary>
        public readonly Kernel Kernel = new(16);
        /// <summary>실제 자원 묶음이다.</summary>
        public readonly RawPriestWorldCore Core;
        /// <summary>생성 시 복사할 효과 객체다.</summary>
        public readonly RawPriestWorldEffects Effects;
        /// <summary>현재 보호막 finder가 볼 사제 타입이다.</summary>
        public uint FoundType;
        /// <summary>현재 보호막 finder의 결과다.</summary>
        public int Found = 1;
        /// <summary>한 번의 실제 조립 입력을 만든다.</summary>
        public Scene()
        {
            Types[2].ConstructorAddress = OriginalConstructors.Address(2);
            Types[158].ConstructorAddress = OriginalConstructors.Address(158);
            Types[167].ConstructorAddress = OriginalConstructors.Address(167);
            var mode = new SquidOwnerMode { Battle = true };
            var owner = new SquidOwner(Pool, Types, Book, mode);
            var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
            var random = new GameRandom(1);
            Core = new(Pool, Hash, Types, frames, new byte[65536], Hash.BorrowEntries(0),
                new RawSquidReward(Pool, Types, Book, new SquidRewardState(), new RawSpStore(random, () => 1)), random,
                new RawSquidFactory(Pool, Types), owner, mode, new RawPostPop(Pool, Types, Book),
                new SquidFrame(Pool, Types, new() { FrameSize = (_, _) => [1, 1], Update = (_, _) => { }, Unpop = (_, _) => { }, Pop = (_, _, _, _) => { } }));
            State.Priests.Priests.Entries = new uint[16];
            Effects = new()
            {
                Pop = (sid, _, _, _) => Events.Add($"pop:{sid}"), Unpop = (_, _) => { }, Repop = (_, _) => { },
                Destroy = (sid, flags) => Events.Add($"destroy:{sid}:{flags}"), ClaimGround = (_, _, _) => { },
                OwnerNotice = sid => Events.Add($"owner:{sid}"), NotifySurface = _ => { }, DependentFound = () => { },
                ReserveRegular = () => true, FallSound = (_, _, _) => { },
                NonPriestOwner = (sid, player) => Events.Add($"other:{sid}:{player}"),
                FindTypeAt = (_, _, type) => { FoundType = type; return Found; },
                Shield = new() { ReserveSound = () => false, LoadSound = _ => 0, AttachSound = (_, _, _, _) => { },
                    NoticeSound = _ => { }, Translate = text => text, Tell = _ => { } },
                Regen = new() { Refresh = _ => { }, Damageable = sid => Events.Add($"burn:{sid}"),
                    NeutralAction = _ => { }, PatchAudit = () => { }, Measurement = () => 0 },
            };
        }
        /// <summary>실제 할당 뒤 필요한 raw 입력을 구성한다.</summary>
        public int Slot(byte type, uint table, byte extra = 9)
        {
            int sid = Pool.Allocate(); Span<byte> raw = Pool.AllocatedBytes(sid); raw.Clear();
            raw[10] = type; raw[40] = extra; BinaryPrimitives.WriteUInt32LittleEndian(raw, table);
            return sid;
        }
        /// <summary>기존 실제 공장·가상 삭제·Kernel을 연결한 Host다.</summary>
        public SquidProcessHost Host(RawPriestWorldBindings world)
        {
            var destroy = new RawSquidDestroy(Pool, new RawUnpop(Pool, Hash, Core.Spots), Types);
            var host = new SquidProcessHost(Pool, Types, Kernel, destroy, ProcessState, world.RegularHandler(), world.DestroyHooks(new() { Emit = _ => { } }));
            host.BindFormFactory(Core.Factory); return host;
        }
        /// <summary>실제 피해 전처리의 효과만 관찰 기록으로 공급한다.</summary>
        public RawDamageablePreDestroy Damageable() => new(Pool, Core.Spots, State.Contained, new(), new()
        { Collapse = _ => Events.Add("collapse"), Explosion = _ => Events.Add("explosion"), Sound = _ => { },
            ReleaseContained = _ => Events.Add("release"), ValidateBase = (_, _) => Events.Add("validate"), Base = (_, _) => Events.Add("base") });
        /// <summary>같은 풀의 실제 배치 파이프라인에 명시적인 기하 입력을 공급한다.</summary>
        public RawPriestPlacement Placement() => new(Pool, Types, new(), new()
        { Shape = (_, _, _) => new(0, 0, 1, 1), InspectGeometry = (_, _) => true });
    }

    /// <summary>지도 사본·빠진 효과는 선거부하고 늦은 실패도 가상 후처리를 등록하지 않는다.</summary>
    [Fact]
    public void ConstructorRegistersOnlyAfterAllModulesSucceed()
    {
        var scene = new Scene(); int priest = scene.Slot(158, 0x50f210), field = scene.Slot(167, ForcefieldLifecycle.PatchVtable);
        Assert.Same(scene.Core.Surfaces, scene.Hash.BorrowEntries(0));
        Assert.Throws<ArgumentException>(() => new RawPriestWorldBindings(scene.Core with { Surfaces = scene.Core.Surfaces.ToArray() }, scene.State, scene.Effects));
        Assert.Null(scene.State.ForcefieldRegular.SharedPriestType);
        scene.Effects.DependentFound = null; Assert.Throws<ArgumentException>(() => new RawPriestWorldBindings(scene.Core, scene.State, scene.Effects));
        scene.Effects.DependentFound = () => { }; scene.Effects.Regen.Refresh = null;
        Assert.Throws<ArgumentException>(() => new RawPriestWorldBindings(scene.Core, scene.State, scene.Effects));
        Assert.Same(scene.State.Movement.PriestBox, scene.State.ForcefieldRegular.SharedPriestType);
        Assert.False(scene.Core.PostPop.HandlesPriest(priest)); Assert.False(scene.Core.PostPop.HandlesForcefield(field));
        scene.Effects.Regen.Refresh = _ => { };
        using var world = new RawPriestWorldBindings(scene.Core, scene.State, scene.Effects);
        Assert.True(scene.Core.PostPop.HandlesPriest(priest)); Assert.True(scene.Core.PostPop.HandlesForcefield(field));
        scene.Effects.Regen.Refresh = null;
        Assert.Throws<ArgumentException>(() => new RawPriestWorldBindings(scene.Core, scene.State, scene.Effects));
        Assert.True(scene.Core.PostPop.HandlesPriest(priest)); Assert.True(scene.Core.PostPop.HandlesForcefield(field));
        Assert.Same(scene.State.Priests, world.Priests); Assert.Same(scene.State.Contained, world.Carrier.FinderState);
        world.Dispose(); world.Dispose(); Assert.False(scene.Core.PostPop.HandlesPriest(priest)); Assert.False(scene.Core.PostPop.HandlesForcefield(field));
    }

    /// <summary>실제 네 소비자의 권한을 값 복사하지 않고 같은 owner로 대여하며 실패는 원자적으로 거부한다.</summary>
    [Fact]
    public void AuthorityUsesSameCurrentOwnerAndPreservesIndependentValues()
    {
        var scene = new Scene(); using var world = new RawPriestWorldBindings(scene.Core, scene.State, scene.Effects);
        world.SetAuthority(false); var boss = new RawAuthorityState(); world.BindAuthority(boss); world.BindAuthority(boss);
        Assert.True(scene.State.HitPoints.GetAuthority()); Assert.True(scene.State.Fall.GetAuthority());
        boss.Boss = false; Assert.False(scene.State.Carrier.ReadBoss()); Assert.False(scene.State.Contained.ReadBoss());
        boss.Boss = true; Assert.True(scene.State.HitPoints.GetAuthority()); Assert.False(scene.State.HitPoints.Authority);
        Assert.Throws<InvalidOperationException>(() => world.SetAuthority(false));
        Assert.Throws<ArgumentException>(() => world.BindAuthority(new RawAuthorityState()));
        Assert.Equal((Func<bool>)boss.ReadBoss, scene.State.Contained.SharedBoss);
        var conflict = new Scene(); using var other = new RawPriestWorldBindings(conflict.Core, conflict.State, conflict.Effects);
        conflict.State.Contained.SharedBoss = new RawAuthorityState().ReadBoss;
        Assert.Throws<ArgumentException>(() => other.BindAuthority(boss)); Assert.Null(conflict.State.HitPoints.SharedAuthority);
    }

    /// <summary>같은 Player9 배열만 재사용하고 크기·주소 실패는 기존 대여를 보존하며 정리 시 반납한다.</summary>
    [Fact]
    public void PlayerBorrowAndLoadingStateHaveActualLifetime()
    {
        var scene = new Scene(); using var world = new RawPriestWorldBindings(scene.Core, scene.State, scene.Effects);
        var players = new byte[9 * 0xb4]; world.BindPlayerRecords(players); world.BindPlayerRecords(players);
        Assert.Throws<ArgumentException>(() => world.BindPlayerRecords(new byte[players.Length]));
        Assert.Throws<ArgumentException>(() => world.BindPlayerRecords(new byte[1])); Assert.Same(players, scene.State.HitPoints.PlayerRecords);
        world.SetLoadingDepth(0x80000000); Assert.Equal(0x80000000u, scene.State.Owner.LoadingDepth);
        Assert.Equal(scene.State.Owner.LoadingDepth, scene.State.Carrier.LoadingDepth); Assert.Equal(scene.State.Owner.LoadingDepth, scene.State.Shield.LoadingDepth); Assert.True(scene.State.Regen.NeutralBlocked);
        world.SetLoadingDepth(0); Assert.False(scene.State.Regen.NeutralBlocked);
        world.SetLocalPlayer(8); Assert.Equal((8u, 8u, 8u), (scene.State.Owner.BattlePlayer, scene.State.Owner.FortPlayer, scene.State.Shield.LocalPlayer));
        Assert.Throws<ArgumentOutOfRangeException>(() => world.SetLocalPlayer(9)); world.Dispose(); Assert.Null(scene.State.HitPoints.PlayerRecords);
    }

    /// <summary>모든 지연 경계가 준비되어야 하며 다른 풀·중복 연결은 원래 실제 연결을 바꾸지 않는다.</summary>
    [Fact]
    public void DeferredBindingsRequireSamePoolAndOneConnection()
    {
        var scene = new Scene(); using var world = new RawPriestWorldBindings(scene.Core, scene.State, scene.Effects);
        var foreign = new Scene(); using var foreignWorld = new RawPriestWorldBindings(foreign.Core, foreign.State, foreign.Effects);
        Assert.Contains("ProcessHost", Assert.Throws<InvalidOperationException>(world.ValidateReady).Message);
        Assert.Throws<ArgumentException>(() => world.BindProcesses(foreign.Host(foreignWorld)));
        var host = scene.Host(world); world.BindProcesses(host); Assert.Throws<ArgumentException>(() => world.BindProcesses(host));
        Assert.Contains("Damageable", Assert.Throws<InvalidOperationException>(world.ValidateReady).Message);
        Assert.Throws<ArgumentException>(() => world.BindDamageable(foreign.Damageable()));
        var damageable = scene.Damageable(); world.BindDamageable(damageable); Assert.Throws<ArgumentException>(() => world.BindDamageable(damageable));
        Assert.Contains("배치", Assert.Throws<InvalidOperationException>(world.ValidateReady).Message);
        Assert.Throws<ArgumentException>(() => world.BindPlacement(foreign.Placement()));
        var placement = scene.Placement(); world.BindPlacement(placement); Assert.Throws<ArgumentException>(() => world.BindPlacement(placement)); world.ValidateReady();
    }

    /// <summary>분배 TYPE158은 이동 선택 전역과 구별하며 보호막은 PATCH에서 실제 VT만으로 현재 TYPE를 검색한다.</summary>
    [Fact]
    public void RegularDispatchKeepsVtableRulesAndMutableSelection()
    {
        var scene = new Scene(); using var world = new RawPriestWorldBindings(scene.Core, scene.State, scene.Effects);
        int priest = scene.Slot(158, 0x50f210), field = scene.Slot(0, ForcefieldLifecycle.PatchVtable);
        scene.State.Movement.Priest = 159; var handler = world.RegularHandler((_, _, _, _) => 7);
        Assert.Equal(3, handler(priest, 601, 0, float.NaN));
        Assert.Equal(-1, handler(priest, 601, uint.MaxValue, 0)); Assert.Equal($"destroy:{priest}:1048576", Assert.Single(scene.Events)); scene.Events.Clear();
        Assert.Equal(7, handler(priest, 604, 2, 0)); Assert.Throws<InvalidOperationException>(() => world.RegularHandler()(priest, 604, 0, 0));
        scene.Pool.AllocatedBytes(priest)[10] = 159; Assert.Equal(7, handler(priest, 601, 0, 0));
        Assert.Equal(0.01f, handler(field, 2, 0, 0)); Assert.Equal(159u, scene.FoundType);
        scene.State.Movement.Priest = 0x1000009f; scene.Found = 0; Assert.Equal(-1, handler(field, 2, 0, 0));
        Assert.Equal(0x1000009fu, scene.FoundType); Assert.Equal($"destroy:{field}:0", Assert.Single(scene.Events));
        scene.Pool.AllocatedBytes(field).Clear(); Assert.Equal(9, world.RegularHandler()(field, 0, 0, 9));
    }

    /// <summary>free와 damageEffect0은 자연 반환하고 원래 burning 경계와 외부 함수 사본을 보존한다.</summary>
    [Fact]
    public void RefreshAndOwnerUseCurrentDataWithFunctionSnapshots()
    {
        var scene = new Scene(); using var world = new RawPriestWorldBindings(scene.Core, scene.State, scene.Effects);
        int sid = scene.Slot(90, 0); scene.Effects.Regen.Damageable = _ => throw new InvalidOperationException("사본 아님");
        scene.Effects.NonPriestOwner = null; world.SetOwner(sid, 3); Assert.Equal($"other:{sid}:3", Assert.Single(scene.Events)); scene.Events.Clear();
        world.DamageableRefresh(sid); Assert.Empty(scene.Events); scene.Types[90].DamageEffect = -1; world.DamageableRefresh(sid);
        Assert.Equal($"burn:{sid}", Assert.Single(scene.Events)); scene.Events.Clear(); Span<byte> raw = scene.Pool.AllocatedBytes(sid); raw[11] = 1; raw[10] = 255;
        world.DamageableRefresh(sid); raw[11] = 0; Assert.Throws<ArgumentOutOfRangeException>(() => world.DamageableRefresh(sid));
        var missing = new Scene(); missing.Effects.Regen.Damageable = null; using var second = new RawPriestWorldBindings(missing.Core, missing.State, missing.Effects);
        int other = missing.Slot(90, 0); second.DamageableRefresh(other); missing.Types[90].DamageEffect = 1;
        Assert.Throws<InvalidOperationException>(() => second.DamageableRefresh(other));
    }

    /// <summary>등록한 실제 가상 후처리가 지연 Host를 요구하고 연결 뒤 같은 Kernel에 회복 Regular를 만든다.</summary>
    [Fact]
    public void ActualPostPopUsesHostAndSharedPriestList()
    {
        var scene = new Scene(); using var world = new RawPriestWorldBindings(scene.Core, scene.State, scene.Effects);
        int priest = scene.Slot(158, 0x50f210, 0);
        Assert.Throws<InvalidOperationException>(() => scene.Core.PostPop.PostPopVirtual(priest, 1));
        Assert.Equal((uint)priest, scene.State.Priests.GetPriests().Items()[0]);
        var host = scene.Host(world); world.BindProcesses(host); scene.Pool.AllocatedBytes(priest)[40] = 1;
        scene.Core.PostPop.PostPopVirtual(priest, 1); Assert.Equal(0, scene.Kernel.Size());
        scene.Pool.AllocatedBytes(priest)[40] = 0; scene.State.Carrier.LoadingDepth = 1;
        // Carrier 뒤 낙하·보호막 분기를 생략하도록 충분한 현재 HP와 지면 비트를 입력한다.
        BinaryPrimitives.WriteInt32LittleEndian(scene.Pool.AllocatedBytes(priest)[26..], 120); scene.Core.Spots[0] = 6;
        scene.Core.PostPop.PostPopVirtual(priest, 1);
        Assert.Equal(1u, scene.State.Priests.GetPriests().Count); Assert.NotNull(host.FindEvent(priest, 602)); Assert.Equal(1, scene.Kernel.Size());
        Assert.Single(scene.State.Priests.GetPriests().Items().ToArray());
        RegularProcess process = host.FindEvent(priest, 602)!;
        scene.Kernel.RunFrame(); Assert.Equal(1u, process.Count()); Assert.Equal(10d + process.Payload(), process.Time());
        Assert.Equal(120, world.PriestState.CurrentHitPoints(priest));
        host.Kill(process, 0); Assert.Equal(0, scene.Kernel.Size());
    }
    /// <summary>같은 장기 Native 모듈을 사본 없이 빌리며 실제 삭제·정기 분배가 그 모듈에 도달한다.</summary>
    [Fact]
    public void NativeSharedBodiesAreBorrowedByActualDispatch()
    {
        var scene = new Scene(); scene.State.Contained.Boss = false;
        var carrier = new RawCarrierPreDestroy(scene.Pool, scene.State.Contained, new()
        { ValidateDamageable = (_, _) => throw new InvalidOperationException("Native 선검사 미선택"),
            DamageablePre = (_, _) => scene.Events.Add("shared-parent"), DependentFound = () => { } }); carrier.EnableNativeBody();
        var pre = new RawPriestPreDestroy(scene.Pool, scene.State.Priests,
            RawCarrierPreDestroy.MakeCarrierPreDestroyHooks(scene.Pool, carrier, new() { FindForcefield = _ => 0, DestroyForcefield = (_, _) => { } })); pre.EnableNativeBody();
        var lookup = new RawPriestForcefield(scene.Pool, scene.Hash, scene.Types, scene.State.Forcefield);
        var regular = new RawForcefieldRegular(scene.Pool, scene.Hash, scene.Types, scene.State.ForcefieldRegular,
            new() { FindTypeAt = (_, _, type) => { scene.Events.Add($"shared-field:{type}"); return 1; } });
        scene.Effects.SharedCarrier = carrier; scene.Effects.SharedPriestPre = pre;
        scene.Effects.SharedForcefield = lookup; scene.Effects.SharedForcefieldRegular = regular;
        using var world = new RawPriestWorldBindings(scene.Core, scene.State, scene.Effects);
        Assert.Same(carrier, world.Carrier); Assert.Same(lookup, world.Forcefield);
        int sid = scene.Slot(0, 0x50f210); var hooks = world.DestroyHooks(new() { Emit = _ => scene.Events.Add("fallback") });
        hooks.Emit!(new(SquidDestroyEffect.PreDestroy, sid, 0, 0)); Assert.Equal(["shared-parent"], scene.Events);
        scene.Events.Clear(); scene.Pool.AllocatedBytes(sid)[0] = 0; hooks.Emit(new(SquidDestroyEffect.PreDestroy, sid, 0, 0)); Assert.Equal(["fallback"], scene.Events);
        scene.Events.Clear(); scene.State.Movement.Priest = 0x12345678; int field = scene.Slot(0, ForcefieldLifecycle.PatchVtable);
        Assert.Equal(0.01f, world.RegularHandler()(field, 2, 0, 0)); Assert.Equal(["shared-field:305419896"], scene.Events);
    }

    /// <summary>선택한 네 장기 모듈 모두 같은 실제 풀만 허용하고 실패 시 후처리 등록이 남지 않는다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)] [InlineData(3)]
    public void ForeignSharedBodiesAreRejected(int selected)
    {
        var scene = new Scene(); var foreign = new Scene();
        if (selected == 0) scene.Effects.SharedCarrier = new(foreign.Pool, foreign.State.Contained,
            new() { ValidateDamageable = (_, _) => { }, DamageablePre = (_, _) => { }, DependentFound = () => { } });
        if (selected == 1) scene.Effects.SharedForcefield = new(foreign.Pool, foreign.Hash, foreign.Types, foreign.State.Forcefield);
        if (selected == 2) scene.Effects.SharedForcefieldRegular = new(foreign.Pool, foreign.Hash, foreign.Types, foreign.State.ForcefieldRegular, new());
        if (selected == 3) scene.Effects.SharedPriestPre = new(foreign.Pool, foreign.State.Priests,
            new() { FindForcefield = _ => 0, DestroyForcefield = (_, _) => { }, ValidateCarrier = (_, _) => { }, CarrierPre = (_, _) => { } });
        int priest = scene.Slot(158, 0x50f210);
        Assert.Throws<ArgumentException>(() => new RawPriestWorldBindings(scene.Core, scene.State, scene.Effects));
        Assert.False(scene.Core.PostPop.HandlesPriest(priest));
    }

}
