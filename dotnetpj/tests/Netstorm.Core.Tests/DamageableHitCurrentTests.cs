using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재 일반 피격·burning의 실제 HP 부모와 진단/가상 분배·판본 저장 폭을 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>원본 재생 외에 현재 owner·함수 사본·선택 효과의 시점을 합성한다.</summary>
public sealed class DamageableHitCurrentTests
{
    /// <summary>피해자5·공격자6·지면7과 실제 타입/HP·전역·관측 효과를 소유한다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 판본의 실제 FIFO 풀이다.</summary>
        public RawSidPool Pool { get; }
        /// <summary>기존 HP 검사 부모를 위한 실제 물리188칸이다.</summary>
        public OriginalTypeRecord[] Physical { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>선택 판본의188/171 TYPE다. CD도 같은 앞 물리 레코드를 빌린다.</summary>
        public OriginalTypeRecord[] Types { get; }
        /// <summary>일반 피해의 현재 원본 전역이다.</summary>
        public DamageableHitState State { get; } = new() { Server = false };
        /// <summary>burning의 현재 역할/진단이다.</summary>
        public DamageableBurningState Burning { get; } = new();
        /// <summary>선택된 효과의 순서와 인자를 관측한다.</summary>
        public List<string> Events { get; } = [];
        /// <summary>함수 사본의 원본 입력이다.</summary>
        public DamageableHitHooks Hooks { get; }
        /// <summary>현재 판본 소유자 BYTE 위치다.</summary>
        public int OwnerOffset => Pool.Edition == OriginalEdition.Patch1078 ? 34 : 32;

        /// <summary>최소 실제 슬롯과 일반/벌프 HP 메타데이터를 준비한다.</summary>
        public Scene(OriginalEdition edition = OriginalEdition.Patch1078)
        {
            Pool = new RawSidPool(24000, false, edition: edition); Types = Physical.Take(edition == OriginalEdition.Patch1078 ? 188 : 171).ToArray();
            // 필요한 세 슬롯을 실제 할당한 뒤 원본 raw 입력으로 준비한다.
            for (int sid = 5; sid <= 7; sid++) { Assert.Equal(sid, Pool.Allocate(SidAllocation.Client)); Pool.RawSlot(sid).Clear(); }
            // 피격/벌프의 두 실제 분류에서만 HP/효과를 켠다.
            foreach (int type in new[] { 71, 135 }) { Types[type].Flags1 = 0x10; Types[type].MaxHitPoints = 300; Types[type].DamageEffect = 1; }
            Pool.RawSlot(5)[10] = 71; Pool.RawSlot(5)[OwnerOffset] = 2; Pool.RawSlot(6)[10] = 128; Pool.RawSlot(7)[10] = 97;
            State.Levels.LevelCorrectionDisabled = true; Health(100); Position(20.75f, 21.9f);
            Hooks = new()
            {
                RejectDamage = (_, _) => false, Destroy = (sid, flags) => Events.Add($"D:{sid}:{flags}"),
                OwnerDamageNotice = (owner, _, _) => Events.Add($"N:{owner}"), Refresh = _ => Events.Add("R"),
                Unrealize = _ => Events.Add("U"), IslandAt = (_, _) => 0,
                RealTime = () => 10, WarningEvent = () => Events.Add("E"), WarningSound = () => Events.Add("S"), WarningText = () => Events.Add("T"),
            };
        }
        /// <summary>선택 판본의 실제 HP 폭만 원본 입력으로 쓴다.</summary>
        public void Health(int health)
        {
            if (Pool.Edition == OriginalEdition.Patch1078) BinaryPrimitives.WriteInt32LittleEndian(Pool.RawSlot(5)[26..], health);
            else BinaryPrimitives.WriteInt16LittleEndian(Pool.RawSlot(5)[26..], unchecked((short)health));
        }
        /// <summary>실제 현재 위치의 float 비트를 쓴다.</summary>
        public void Position(float x, float y)
        { BinaryPrimitives.WriteSingleLittleEndian(Pool.RawSlot(5)[14..], x); BinaryPrimitives.WriteSingleLittleEndian(Pool.RawSlot(5)[18..], y); }
        /// <summary>같은 현재 피해 전역과 함수 사본으로 일반 피격을 구성한다.</summary>
        public RawDamageableHit Hit() => new(Pool, Types, State, Hooks);
        /// <summary>PATCH는 실제 현재 보상HP, CD는 TypeHitPoints만 쓰는 기존 물리 부모를 연결한다.</summary>
        public RawDamageableBurning Burn(DamageableBurningHooks hooks)
        {
            if (Pool.Edition != OriginalEdition.Patch1078)
                return new(Pool, Types, new RawHitPoints(Pool, Physical, new RawHitPointState()), Burning, hooks);
            var store = new RawSpStore(new GameRandom(1), () => 1); store.Initialize();
            var hp = new RawSquidReward(Pool, Types, new RawPostPopState(), new SquidRewardState(), store); hp.BindCurrentTypes(Types);
            return new(Pool, Types, hp, Burning, hooks);
        }
    }

    /// <summary>복귀 진단 뒤 현재 타입/위치는 읽지만 앞의 current/half/첫결과는 재판정하지 않는다.</summary>
    [Fact]
    public void ReturningBurningDiagnosticKeepsSavedNumbersAndCurrentReads()
    {
        var s = new Scene(); uint checking = 0; var view = new RawCheckingFlag(() => checking);
        s.Burning.Checking = view; view.Set(false); checking = 0x100;
        var hooks = new DamageableBurningHooks
        {
            Find = (_, type, index) =>
            {
                s.Events.Add($"F:{type}:{index}");
                if (index == 0) { s.Burning.BurningType = 15; s.Pool.RawSlot(5)[10] = 135; s.Health(1000); s.Position(30, 31); return 0; }
                return 7;
            },
            Assertion = (expression, file, line) => { Assert.Contains("findProcess", expression); Assert.Equal(@"\Ns\O\Damageable.cpp", file); Assert.Equal(0x210u, line); s.Events.Add("A"); },
            BulfSound = (_, x, y) => s.Events.Add($"S:{x}:{y}"), Create = _ => { s.Events.Add("C"); return 7; },
        };
        RawDamageableBurning body = s.Burn(hooks); hooks.Find = (_, _, _) => throw new InvalidOperationException("함수 사본 변경");
        Assert.Equal(7, body.Refresh(5)); Assert.Equal(["F:14:0", "F:15:1", "A", "S:30:31", "C"], s.Events);
    }

    /// <summary>선택된 벌프 소리를 요청한 뒤에만 생성자 미연결을 보고한다.</summary>
    [Fact]
    public void BurningSoundPrecedesMissingCreationBoundary()
    {
        var s = new Scene(); s.Pool.RawSlot(5)[10] = 135;
        var body = s.Burn(new() { Find = (_, _, _) => 0, BulfSound = (_, _, _) => s.Events.Add("S") });
        Assert.Throws<InvalidOperationException>(() => body.Refresh(5)); Assert.Equal(["S"], s.Events);
    }

    /// <summary>CD는 signed HP WORD만 읽고 PATCH 진단/두번째 검색은 선택하지 않는다.</summary>
    [Theory]
    [InlineData(150, 0)]
    [InlineData(149, 7)]
    [InlineData(-1, 7)]
    [InlineData(32767, 0)]
    public void CdBurningUsesSignedWordAndSkipsPatchDiagnostic(int hp, int expected)
    {
        var s = new Scene(OriginalEdition.Cd1072); s.Health(hp); s.Pool.RawSlot(5)[28] = 0xa7; s.Pool.RawSlot(5)[29] = 0xb8;
        s.Burning.Checking = new((Func<uint>)(() => throw new InvalidOperationException("CD checking 미조회")));
        var body = s.Burn(new() { Find = (_, _, index) => { Assert.Equal(0u, index); return 0; }, Create = _ => 7 });
        Assert.Equal(expected, body.Refresh(5)); Assert.Equal(0xa7, s.Pool.Slot(5)[28]); Assert.Equal(0xb8, s.Pool.Slot(5)[29]);
    }

    /// <summary>비치명상에서는 서버 조회를 건너뛰고 방어 뒤 현재 owner로 통지한다.</summary>
    [Fact]
    public void NonLethalHitDoesNotReadServerAndUsesCurrentOwner()
    {
        var s = new Scene(); s.State.ServerRead = () => throw new InvalidOperationException("비치명 서버 미조회");
        s.Hooks.RejectDamage = (_, _) => { s.Pool.RawSlot(5)[34] = 3; return false; };
        Assert.True(s.Hit().BaseHit(5, 10, 2, 6)); Assert.Equal(90, BinaryPrimitives.ReadInt32LittleEndian(s.Pool.Slot(5)[26..]));
        Assert.Equal(["N:3"], s.Events);
    }

    /// <summary>CD 일반 HP 쓰기는 옆 WORD와 소유자/진단을 보존한다.</summary>
    [Fact]
    public void CdHitWritesOnlyWordAndIgnoresPatchHeldAssertion()
    {
        var s = new Scene(OriginalEdition.Cd1072); s.Pool.RawSlot(5)[28..].Fill(0xa7); s.Pool.RawSlot(5)[32] = 2; s.Pool.RawSlot(5)[35] = 0;
        byte[] tail = s.Pool.Slot(5)[28..].ToArray(); s.State.ServerRead = () => throw new InvalidOperationException("비치명 서버 미조회");
        var body = s.Hit(); Assert.True(body.BaseHit(5, 10, 2, 6));
        Assert.Equal(90, BinaryPrimitives.ReadInt16LittleEndian(s.Pool.Slot(5)[26..])); Assert.Equal(tail, s.Pool.Slot(5)[28..].ToArray());
        s.State.Checking = new((Func<uint>)(() => throw new InvalidOperationException("CD checking 미조회"))); s.Pool.RawSlot(5)[35] = 1;
        s.Events.Clear(); Assert.True(body.BaseHit(5, 10, 2, 6)); Assert.Empty(s.Events);
    }

    /// <summary>CD 섬 경고는 소리만 요청하고 PATCH 사건/문장 장치를 요구하지 않는다.</summary>
    [Fact]
    public void CdWarningUsesOnlySoundAndCurrentPrimaryIsland()
    {
        var s = new Scene(OriginalEdition.Cd1072); s.Pool.RawSlot(5)[32] = 1; s.Types[97].Flags2 = 2;
        BinaryPrimitives.WriteUInt16LittleEndian(s.Pool.RawSlot(7)[8..], 4); s.State.PrimaryIslandRead = owner => { Assert.Equal(1, owner); return 4; };
        s.Hooks.IslandAt = (_, _) => 7; s.Hooks.WarningEvent = null; s.Hooks.WarningText = null;
        Assert.True(s.Hit().Hit(5, 10, 2, 6)); Assert.Equal(["S", "N:1", "R"], s.Events); Assert.Equal(40d, s.State.WarningTime);
    }

    /// <summary>현재 VT/빌린 진입 주소로 Base/Hit/fallback을 고르고 TYPE 번호로 추측하지 않는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void GunCallbackReadsCurrentVtableAndBorrowedAddressTable(OriginalEdition edition)
    {
        uint baseAddress = edition == OriginalEdition.Patch1078 ? 0x4add10u : 0x4aeda0u;
        uint hitAddress = edition == OriginalEdition.Patch1078 ? 0x44c8e0u : 0x462300u;
        var s = new Scene(edition); var hit = s.Hit(); BinaryPrimitives.WriteUInt32LittleEndian(s.Pool.RawSlot(5), 0x11223344);
        DamageableHitEntry[] entries = [new(0x11223344, baseAddress), new(0x11223344, hitAddress)]; int fallback = 0;
        var damage = hit.MakeDamageableGunHitCallback(entries, (victim, amount, owner, attacker) => { Assert.Equal((5, 3, (byte)2, 6), (victim, amount, owner, attacker)); fallback++; });
        damage(5, 10, 2, 6); Assert.Equal(["N:2"], s.Events);
        s.Events.Clear(); entries[0] = new(0x11223344, hitAddress); damage(5, 10, 2, 6); Assert.Equal(["N:2", "R"], s.Events);
        BinaryPrimitives.WriteUInt32LittleEndian(s.Pool.RawSlot(5), 0x55667788); damage(5, 3, 2, 6); Assert.Equal(1, fallback);
        Assert.Throws<InvalidOperationException>(() => hit.MakeDamageableGunHitCallback(entries)(5, 3, 2, 6));
        Assert.Throws<ArgumentException>(() => hit.MakeDamageableGunHitCallback(ReadOnlyMemory<DamageableHitEntry>.Empty));
    }

    /// <summary>burning 어댑터는 입력/다른 효과를 유지한 값 사본으로 실제 판정을 연결한다.</summary>
    [Fact]
    public void ActualBurningAdapterPreservesInputCallbacks()
    {
        var s = new Scene(); s.Hooks.Refresh = null; int created = 0;
        var burning = s.Burn(new() { Find = (_, _, _) => 0, Create = _ => { created++; return 7; } });
        DamageableHitHooks bound = RawDamageableHit.MakeDamageableHitBurningHooks(s.Pool, burning, s.Hooks);
        Assert.Null(s.Hooks.Refresh); Assert.Same(s.Hooks.Destroy, bound.Destroy);
        var hit = new RawDamageableHit(s.Pool, s.Types, s.State, bound); Assert.True(hit.Hit(5, 10, 2, 6)); Assert.Equal(1, created);
        Assert.Throws<ArgumentException>(() => RawDamageableHit.MakeDamageableHitBurningHooks(new RawSidPool(24000, false), burning, s.Hooks));
    }
}
