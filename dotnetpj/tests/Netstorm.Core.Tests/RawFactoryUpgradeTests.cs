using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 원본 업그레이드의 그림·완료 순서와 실제 Kernel/Host 정리를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj FactoryUpgradeTests의 합성 계약을 실제 풀·프로세스 호스트로 검사한다.</summary>
public sealed class RawFactoryUpgradeTests
{
    /// <summary>현재 원문·호스트와 변경 가능한 가상·장치 입력이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>시설과 그림·감시 Form의 실제 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768, true, null, OriginalEdition.Patch1078);
        /// <summary>프로세스가 사용하는 실제 Kernel이다.</summary>
        public readonly Kernel Kernel = new();
        /// <summary>같은 실제 프로세스 수명 호스트다.</summary>
        public readonly SquidProcessHost Host;
        /// <summary>실제 부모 시설 SID다.</summary>
        public readonly int Parent;
        /// <summary>가상·장치 호출 순서다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>현재 감시 Form별 상대 객체 입력이다.</summary>
        public readonly Dictionary<int, int> Watchers = [];
        /// <summary>선택된 그림 삭제 대상이다.</summary>
        public readonly List<int> Removed = [];
        /// <summary>원본 진단 행 기록이다.</summary>
        public readonly List<uint> Assertions = [];
        /// <summary>현재 전역을 읽는 장치다.</summary>
        public readonly FactoryUpgradeState State;
        /// <summary>효과 뒤 현재값을 바꿀 수 있는 하위 장치다.</summary>
        public readonly FactoryUpgradeHooks Hooks;
        /// <summary>현재 아웃포스트 TYPE다.</summary>
        public uint Outpost = 126;
        /// <summary>현재 로컬 소유자다.</summary>
        public uint Local = 2;
        /// <summary>현재 권한이다.</summary>
        public bool Boss = true;
        /// <summary>현재 도전 모드다.</summary>
        public bool Challenge;
        /// <summary>현재 AI 존재 입력이다.</summary>
        public bool Ai;
        /// <summary>새로 만든 실제 그림 SID다.</summary>
        public int Image;

        /// <summary>같은 원문 생성자 표·Host·삭제 수명과 합성 장치 효과를 준비한다.</summary>
        public Scene()
        {
            OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord
            {
                ConstructorAddress = OriginalConstructors.Address(OriginalEdition.Patch1078, index),
            }).ToArray();
            var destroy = new RawSquidDestroy(Pool, new RawUnpop(Pool, new RawSquidHash(), new byte[65536]), types);
            Host = new(Pool, types, Kernel, destroy, new SquidProcessState(), asset: new SquidDestroyHooks
            {
                Emit = effect =>
                {
                    if (effect.Effect == SquidDestroyEffect.PreDestroy) destroy.CompletePreDestroy();
                    else if (effect.Effect == SquidDestroyEffect.PostDestroy) destroy.CompletePostDestroy();
                },
                Selected = () => 0,
            });
            Parent = New();
            Pool.AllocatedBytes(Parent)[10] = 85;
            Pool.AllocatedBytes(Parent)[34] = 2;
            Put(Pool.AllocatedBytes(Parent), 0, 0x509148);
            Put(Pool.AllocatedBytes(Parent), 14, BitConverter.SingleToUInt32Bits(7.25f));
            Put(Pool.AllocatedBytes(Parent), 18, BitConverter.SingleToUInt32Bits(8.5f));
            State = new() { Outpost = () => Outpost, DependType = () => 1, LocalPlayer = () => Local, Boss = () => Boss, Challenge = () => Challenge };
            Hooks = new()
            {
                Level = parent => { Assert.Equal(Parent, parent); Trace.Add("level"); return (uint)(Pool.Slot(parent)[11] >> 5); },
                SetLevel = (sid, level) => { Trace.Add("set"); Pool.AllocatedBytes(sid)[11] = (byte)((Pool.Slot(sid)[11] & 31u) | (level << 5)); },
                MaxHp = parent => { Assert.Equal(1, Pool.Slot(parent)[11] >> 5); Trace.Add("hp"); return 321; },
                AdjustFrame = parent => { Assert.Equal(321, BinaryPrimitives.ReadInt32LittleEndian(Pool.Slot(parent).Slice(26, 4))); Trace.Add("frame"); },
                Display = _ => Trace.Add("display"),
                SoundAt = (x, y, name) => { Assert.Equal(7.25f, x); Assert.Equal(8.5f, y); Assert.Equal("upgradeDone.wav", name); Trace.Add("position"); },
                Sound = name => { Assert.Equal("upgradeComplete.wav", name); Trace.Add("sound"); },
                Flare = (parent, count, ev) => { Assert.Equal(Parent, parent); Assert.Equal(6, count); Assert.Equal(2u, ev); Trace.Add("flare"); },
                Create = (type, flags) =>
                {
                    Assert.Equal(85u, type);
                    Assert.Equal(2u, flags);
                    Trace.Add("create");
                    Image = New();
                    Pool.AllocatedBytes(Image)[10] = 85;
                    Pool.AllocatedBytes(Image)[11] = 4;
                    BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(Image).Slice(12, 2), 0xa5a5);
                    Put(Pool.AllocatedBytes(Image), 36, 77);
                    return Image;
                },
                Owner = (sid, owner) =>
                {
                    Assert.Equal(9, Pool.Slot(sid)[40]);
                    Assert.Equal((ushort)0, BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(sid).Slice(12, 2)));
                    Assert.Equal(77u, BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid).Slice(36, 4)));
                    Pool.AllocatedBytes(sid)[34] = (byte)owner;
                    Trace.Add("owner");
                },
                Pop = (sid, x, y, flags) =>
                {
                    Assert.Equal(Image, sid);
                    Assert.Equal(7.25f, x);
                    Assert.Equal((float)(8.5 + 0.0010000000474974513), y);
                    Assert.Equal(0u, flags);
                    Assert.Equal(2, Pool.Slot(sid)[34]);
                    Trace.Add("pop");
                },
                Construction = (sid, duration) => { Assert.Equal(Image, sid); Assert.Equal(15f, duration); Trace.Add("construction"); },
                Depend = (parent, image) => { Assert.Equal(Parent, parent); Trace.Add("depend"); Link(image); },
                Watcher = form => { Trace.Add("watcher"); return Watchers[form]; },
                Destroy = (sid, flags) => { Assert.Equal(0u, flags); Removed.Add(sid); Trace.Add("destroy"); },
                AiPresent = _ => { Trace.Add("ai"); return Ai; },
                NotifyAi = (owner, parent) => { Assert.Equal(Parent, parent); Assert.Equal((uint)Pool.Slot(parent)[34], owner); Trace.Add("notify"); },
                Log = (_, _) => Trace.Add("log"),
                Assertion = line => Assertions.Add(line),
            };
        }
        /// <summary>검사 입력 슬롯을 명시0으로 할당한다.</summary>
        public int New(SidAllocation allocation = SidAllocation.Client)
        {
            int sid = Pool.Allocate(allocation);
            Pool.AllocatedBytes(sid).Clear();
            return sid;
        }
        /// <summary>부모 head에 원본 WORD 링크를 넣고 상대 객체 입력을 연결한다.</summary>
        public int Link(int watcher, byte type = 1)
        {
            int form = New();
            Pool.AllocatedBytes(form)[10] = type;
            BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(form).Slice(4, 2), BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(Parent).Slice(6, 2)));
            BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(Parent).Slice(6, 2), (ushort)form);
            Watchers[form] = watcher;
            return form;
        }
        /// <summary>같은 현재 원문·실제 호스트로 본체를 만든다.</summary>
        public RawFactoryUpgrade Make() => new(Pool, Host, State, Hooks);
        /// <summary>Kernel의 남은 실제 프로세스를 연결 장치보다 먼저 정리한다.</summary>
        public void Dispose() => Kernel.Clear();
    }

    /// <summary>실제 원문 DWORD 필드를 쓴다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(at, 4), value);

    /// <summary>생성→두 레벨 조회→상태·owner→Pop→건설→감시 순서와 원문 필드를 대조한다.</summary>
    [Fact]
    public void BeginUsesActualRawOrder()
    {
        using var scene = new Scene();
        scene.Make().Begin(scene.Parent, 15);
        Assert.Equal(["create", "level", "level", "set", "owner", "pop", "log", "construction", "depend"], scene.Trace);
        Assert.Equal(1, scene.Pool.Slot(scene.Image)[11] >> 5);
        Assert.True(scene.Make().HasPicture(scene.Parent));
    }

    /// <summary>상한이면 한 번만 읽고 그 밖에는 두 번째 현재 조회 결과를 사용한다.</summary>
    [Fact]
    public void NextLevelRereadsAndCaps()
    {
        using var scene = new Scene();
        scene.Pool.AllocatedBytes(scene.Parent)[11] = 96;
        Assert.Equal(3u, scene.Make().NextLevel(scene.Parent));
        Assert.Single(scene.Trace);
        int calls = 0;
        scene.Hooks.Level = _ => ++calls == 1 ? 0u : 2u;
        Assert.Equal(3u, scene.Make().NextLevel(scene.Parent));
        Assert.Equal(2, calls);
    }

    /// <summary>완료 효과 뒤 현재 owner와 AI를 다시 조회하고 HP·표시·소리·통지 순서를 유지한다.</summary>
    [Fact]
    public void FinishOrderAndCurrentOwner()
    {
        using var scene = new Scene { Ai = true };
        scene.Hooks.SoundAt = (_, _, _) => { scene.Trace.Add("position"); scene.Pool.AllocatedBytes(scene.Parent)[34] = 3; scene.Local = 3; };
        Assert.Equal(-1f, scene.Make().Finish(scene.Parent));
        Assert.Equal(["log", "level", "level", "set", "hp", "frame", "display", "position", "sound", "flare", "ai", "notify"], scene.Trace);
    }

    /// <summary>아웃포스트 완료는 생략하지만 Finish의 후속 현재 AI 통지는 유지한다.</summary>
    [Fact]
    public void OutpostSkipsCompleteButKeepsAiNotice()
    {
        using var scene = new Scene { Outpost = 85, Ai = true };
        scene.Make().Complete(scene.Parent);
        Assert.Empty(scene.Trace);
        Assert.Equal(-1f, scene.Make().Finish(scene.Parent));
        Assert.Equal(["log", "ai", "notify"], scene.Trace);
    }

    /// <summary>실제 Host의 일반/공유 사건4·3을 모두 지우고 사건8은 보존한다.</summary>
    [Fact]
    public void CompleteRemovesActualTimers()
    {
        using var scene = new Scene();
        RegularProcess? other = scene.Host.AddRegular(scene.Parent, 8, 3);
        Assert.NotNull(other);
        Assert.NotNull(scene.Host.AddRegular(scene.Parent, 4, 20));
        Assert.NotNull(scene.Host.AddSharedRegular(scene.Parent, 4, 20));
        Assert.NotNull(scene.Host.AddSharedRegular(scene.Parent, 3, 15));
        Assert.NotNull(scene.Host.AddRegular(scene.Parent, 3, 15));
        Assert.True(scene.Make().IsUpgrading(scene.Parent));
        scene.Make().Complete(scene.Parent);
        Assert.Null(scene.Host.FindEvent(scene.Parent, 4));
        Assert.Null(scene.Host.FindEvent(scene.Parent, 3));
        Assert.Same(other, scene.Host.FindEvent(scene.Parent, 8));
        Assert.Equal(1, scene.Kernel.Size());
        Assert.False(scene.Make().IsUpgrading(scene.Parent));
    }

    /// <summary>저장 next를 유지하며 같은 TYPE·extra8·클라이언트 범위 그림만 삭제한다.</summary>
    [Fact]
    public void PictureFiltersAndSavedNext()
    {
        using var scene = new Scene();
        int first = scene.New(), second = scene.New(), server = scene.New(SidAllocation.Server), foreign = scene.New();
        // 네 감시 대상의 명시 TYPE·extra8 입력을 준비한다.
        foreach (int sid in new[] { first, second, server, foreign })
        {
            scene.Pool.AllocatedBytes(sid)[10] = 85;
            scene.Pool.AllocatedBytes(sid)[40] = 8;
        }
        scene.Pool.AllocatedBytes(foreign)[10] = 87;
        scene.Link(server);
        scene.Link(foreign);
        scene.Link(second);
        int head = scene.Link(first);
        scene.Link(first, 70);
        scene.Hooks.Destroy = (sid, _) =>
        {
            scene.Removed.Add(sid);
            if (sid == first) BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.AllocatedBytes(head).Slice(4, 2), 0);
        };
        Assert.True(scene.Make().HasPicture(scene.Parent));
        scene.Make().Complete(scene.Parent);
        Assert.Equal([first, second], scene.Removed);
    }

    /// <summary>dead 부모·자기 감시자 진단과 선택된 미연결 전 원문 쓰기를 보존한다.</summary>
    [Fact]
    public void AssertionsAndSelectedMissingEffects()
    {
        using var scene = new Scene { Boss = false };
        scene.State.Checking.Set(1u);
        scene.Pool.AllocatedBytes(scene.Parent)[11] = 2;
        scene.Link(scene.Parent);
        Assert.False(scene.Make().HasPicture(scene.Parent));
        Assert.Equal([0x158u, 0xadu], scene.Assertions);
        scene.Challenge = true;
        scene.Assertions.Clear();
        Assert.False(scene.Make().HasPicture(scene.Parent));
        Assert.Equal([0xadu], scene.Assertions);
        scene.Hooks.Construction = null;
        Assert.Throws<InvalidOperationException>(() => scene.Make().Begin(scene.Parent, 15));
        Assert.Equal("log", scene.Trace[^1]);
        Assert.Equal(9, scene.Pool.Slot(scene.Image)[40]);
    }
}
