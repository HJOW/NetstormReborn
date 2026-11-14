using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재 해방의 실제 물리 주소·효과 뒤 전역/WORD 조회·함수 사본을 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>원본 재생 외에 Native 진입과 장기 owner 대여의 경계 조건을 검사한다.</summary>
public sealed class DamageableReleaseNativeTests
{
    /// <summary>부모5·후보6/7·생성8/9와 같은 현재 전역/효과를 소유한다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 FIFO로 필요한 슬롯을 미리 확보한 풀이다.</summary>
        public RawSidPool Pool { get; } = new(24000, false);
        /// <summary>Native가 선택한 flatBYTE를 제공한다.</summary>
        public byte[] Spots { get; } = new byte[65536];
        /// <summary>부모 발자국과 후보 genus를 소비 때 읽는 현재 표다.</summary>
        public OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>독립 값과 다른 실제 허용 마스크를 빌릴 상태다.</summary>
        public DamageableReleaseState State { get; } = new() { AllowedGenus = 0 };
        /// <summary>후보마다 현재 값을 바꿀 수 있는 같은 장기 마스크다.</summary>
        public UIntBox Allowed { get; } = new() { Value = 2 };
        /// <summary>현재 Native battle이며 typed 상태의 사본을 쓰지 않는다.</summary>
        public BoolBox Battle { get; } = new();
        /// <summary>생성 효과 뒤 다시 읽을 실제 BridgeTYPE다.</summary>
        public UIntBox Bridge { get; } = new() { Value = 82 };
        /// <summary>하위 효과의 순서·인자를 관측한다.</summary>
        public List<string> Events { get; } = [];
        /// <summary>첫 생성 등의 현재 입력 변경만 외부 효과로 제공한다.</summary>
        public Action<uint>? Created { get; set; }
        /// <summary>원문 함수 사본의 대입 독립성을 검사할 원본 함수 묶음이다.</summary>
        public DamageableReleaseHooks Hooks { get; }
        /// <summary>생성 경계에서 소비한 실제 슬롯 수다.</summary>
        public int Born { get; private set; }

        /// <summary>하나의 일반 해방 후보와 source/genus를 실제 raw에 준비한다.</summary>
        public Scene()
        {
            // 부모/두 후보/두 생성 칸을 실제 할당 후 원문 입력으로 준비한다.
            for (int sid = 5; sid <= 9; sid++) { Assert.Equal(sid, Pool.Allocate(SidAllocation.Client)); Pool.RawSlot(sid).Clear(); }
            Pool.RawSlot(5)[10] = 160; Position(1, 1); Types[160].FootX = Types[160].FootY = 1;
            Put(5, 6, 6); Pool.RawSlot(6)[10] = 6; Put(6, 18, 82); Put(6, 20, 32); Put(6, 12, 0xabcd);
            Types[82].Flags2 = 2; Types[90].Flags2 = 0x40000000; State.SharedAllowedGenus = Allowed; Spots[257] = 6;
            Hooks = new()
            {
                SpawnPriest = (_, _, kind, word) => Events.Add($"P{kind}:{word}"),
                Create = (kind, flags) =>
                {
                    Assert.Equal(0u, flags); int sid = 8 + Born++; Pool.RawSlot(sid)[10] = checked((byte)kind);
                    Events.Add($"N{kind}"); Created?.Invoke(kind); return sid;
                },
                SetOwner = (sid, owner) => { Assert.Equal(0u, owner); Events.Add($"O{sid}"); },
                Pop = (sid, x, y, flags) => { Assert.Equal(0u, flags); Events.Add($"L{sid}:{x}:{y}"); },
                NotifySurface = sid => Events.Add($"S{sid}"),
            };
        }
        /// <summary>같은 현재 raw WORD를 쓴다.</summary>
        public void Put(int sid, int offset, ushort value) => BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(sid)[offset..], value);
        /// <summary>실제 float 비트를 현재 부모에 쓴다.</summary>
        public void Position(float x, float y)
        { BinaryPrimitives.WriteSingleLittleEndian(Pool.RawSlot(5)[14..], x); BinaryPrimitives.WriteSingleLittleEndian(Pool.RawSlot(5)[18..], y); }
        /// <summary>현재 owner/표와 함수 사본을 빌리는 실제 해방 몸체를 만든다.</summary>
        public RawDamageableRelease Body() => new(Pool, Types, Spots, new ContainedFinderState(), State, Hooks);
        /// <summary>Native가 소비하는 시점에 같은 현재 전역을 조회한다.</summary>
        public void Run(RawDamageableRelease body) => body.ReleaseNative(5, () => Battle.Value, () => Bridge.Value);
    }

    /// <summary>Native는 부모 자격/발자국 보드 선검사를 추가하지 않고 도달한 현재 물리만 읽는다.</summary>
    [Fact]
    public void NativeDoesNotRecheckTypedParentOrFootprint()
    {
        var s = new Scene(); s.Put(5, 6, 0); s.Pool.RawSlot(5)[10] = 0; s.Pool.RawSlot(5)[11] = 9;
        s.Types[0].FootX = 0; s.Types[0].FootY = -2;
        var body = s.Body(); Assert.Throws<InvalidOperationException>(() => body.Release(5));
        s.Run(body); Assert.Empty(s.Events);
    }

    /// <summary>축 범위를 미리 닫지 않고 signed64 절삭/하위DWORD와 감긴 flatBYTE를 선택한다.</summary>
    [Theory]
    [InlineData(256f, 0f, 256, "L8:256:0")]
    [InlineData(-256f, 1f, 2, "L8:-255:1")]
    [InlineData(4294967296f, 0f, 0, "L8:0:0")]
    public void NativeUsesTruncatedDwordAndWrappedFlatByte(float x, float y, int spot, string pop)
    {
        var s = new Scene(); s.Position(x, y); s.Spots[spot] = 6; var body = s.Body();
        Assert.Throws<ArgumentOutOfRangeException>(() => body.Release(5)); s.Run(body);
        Assert.Equal(1, s.Born); Assert.Equal(pop, s.Events[^1]);
    }

    /// <summary>물리 범위와 CRT 표현 범위 실패를 구별하며 선택 실패 앞에 생성 효과를 내지 않는다.</summary>
    [Fact]
    public void NativeRejectsOnlyReachedPhysicalAndCrtLimits()
    {
        var s = new Scene(); var body = s.Body(); s.Position(65536, 0);
        Assert.Contains("flatBYTE", Assert.Throws<ArgumentOutOfRangeException>(() => s.Run(body)).Message);
        s.Position(float.PositiveInfinity, 0);
        Assert.Contains("CRT", Assert.Throws<ArgumentOutOfRangeException>(() => s.Run(body)).Message); Assert.Empty(s.Events);
    }

    /// <summary>함수 사본과 실제 공유 마스크·효과 뒤 WORD/BridgeTYPE의 조회 순서를 함께 검사한다.</summary>
    [Fact]
    public void CurrentMaskWordAndBridgeAreReadAfterCreationEffects()
    {
        var s = new Scene(); s.Put(6, 4, 7); s.Pool.RawSlot(7)[10] = 6; s.Put(7, 18, 90); s.Put(7, 20, 32);
        s.Created = kind => { if (kind == 82) { s.Allowed.Value = 0x40000000; s.Put(6, 12, 0xfedc); s.Bridge.Value = 90; } };
        var body = s.Body(); s.Hooks.Create = (_, _) => throw new InvalidOperationException("함수 사본을 바꿔서는 안 됨");
        s.Run(body); Assert.Equal(2, s.Born); Assert.Equal(0u, s.State.AllowedGenus);
        Assert.Equal(0xfedc, BinaryPrimitives.ReadUInt16LittleEndian(s.Pool.Slot(8)[12..]));
        Assert.DoesNotContain("S8", s.Events); Assert.Contains("S9", s.Events);
    }

    /// <summary>Native battle을 최초 값으로 복사하지 않고 다음 후보의 실제 시점에 다시 조회한다.</summary>
    [Fact]
    public void NativeBattleRemainsLiveBetweenCandidates()
    {
        var s = new Scene(); s.Put(6, 4, 7); s.Pool.RawSlot(7)[10] = 6; s.Put(7, 18, 90); s.Put(7, 20, 32);
        s.Created = _ => s.Battle.Value = true; s.Run(s.Body()); Assert.Equal(2, s.Born);
        var blocked = new Scene(); blocked.State.Battle = true; blocked.Allowed.Value = 0;
        blocked.Run(blocked.Body()); Assert.Empty(blocked.Events);
    }

    /// <summary>해방 어댑터도 입력 함수 묶음을 수정하지 않고 실제 몸체를 연결한 값 사본을 돌려준다.</summary>
    [Fact]
    public void ReleaseAdapterPreservesInputAndOtherCallbacks()
    {
        var s = new Scene(); int previous = 0;
        var hooks = new DamageablePreDestroyHooks
        {
            Collapse = _ => { }, Explosion = _ => { }, Sound = _ => { },
            ReleaseContained = _ => previous++, ValidateBase = (_, _) => { }, Base = (_, _) => { },
        };
        DamageablePreDestroyHooks attached = RawDamageableRelease.MakeDamageableReleaseHooks(s.Pool, s.Body(), hooks);
        Assert.NotSame(hooks, attached); Assert.Same(hooks.Collapse, attached.Collapse);
        Assert.Same(hooks.Explosion, attached.Explosion); Assert.Same(hooks.Sound, attached.Sound);
        Assert.Same(hooks.ValidateBase, attached.ValidateBase); Assert.Same(hooks.Base, attached.Base);
        attached.ReleaseContained!(5); Assert.Equal(1, s.Born); Assert.Equal(0, previous);
        hooks.ReleaseContained!(5); Assert.Equal(1, previous);
    }
}
