using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// cpppj AiFactoryProductionTests의 실제 내용물·큐·비용 계약을 관리 코드에서 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원소별 기술 선택과 시설 생산 큐의 순서·부작용을 검사한다.</summary>
public sealed class RawAiFactoryProductionTests
{
    /// <summary>같은 본체·목록·실제 풀과 변경 가능한 하위 입력이다.</summary>
    private sealed class Scene
    {
        /// <summary>본체와 큐 backing 메모리다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x47000000, bytes => bytes.Clear());
        /// <summary>기술·생산·시설이 공유하는 실제 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768, true, null, OriginalEdition.Patch1078);
        /// <summary>현재 TYPE 입력이다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>현재 생산 등록 TYPE과 SID다.</summary>
        public readonly Dictionary<uint, int> Products = [];
        /// <summary>실제 생산 조회 순서다.</summary>
        public readonly List<uint> Queries = [];
        /// <summary>현재 내용물 조회 상태다.</summary>
        public readonly ContainedFinderState Finder = new();
        /// <summary>부작용을 바꿀 실제 장치다.</summary>
        public readonly AiFactoryProductionHooks Hooks;
        /// <summary>실제 본체 토큰이다.</summary>
        public readonly uint Token;
        /// <summary>기술 부모 SID다.</summary>
        public readonly int Parent;
        /// <summary>대상 시설 SID다.</summary>
        public readonly int Factory;
        /// <summary>첫 기술 내용물 SID다.</summary>
        public readonly int FirstTechnology;
        /// <summary>생산 등록 호출 수다.</summary>
        public int Added;
        /// <summary>업그레이드 호출 수다.</summary>
        public int Upgrades;
        /// <summary>가상 레벨 조회 수다.</summary>
        public int Levels;
        /// <summary>행동 로그 호출 수다.</summary>
        public int Logs;
        /// <summary>원본 정밀도 확인용 현재 QWORD 시각이다.</summary>
        public double Now = 16777216.25;

        /// <summary>원소3 기술과 실제 두 큐를 준비한다.</summary>
        public Scene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            Parent = New();
            Factory = New();
            Pool.AllocatedBytes(Factory)[10] = 85;
            Types[85].Theme = 3;
            Types[100].Theme = 3;
            Types[101].Theme = 3;
            Types[102].Theme = 1;
            Set(0x260, 2);
            Set(0x294, 100);
            Set(0x298, 7);
            Set(0x2cc, 1);
            Set(0x2d8, 1200);
            Set(0x228, BitConverter.SingleToUInt32Bits(2.5f));
            Set(0x30c, Memory.Allocate(16));
            Set(0x310, 4);
            Set(0x314, 1);
            PutItem(0x30c, 0, (uint)Factory);
            Set(0x318, Memory.Allocate(16));
            Set(0x31c, 4);
            FirstTechnology = Content(Parent, 100, 4);
            Hooks = new()
            {
                Technology = owner => { Assert.Equal(Get(0x260), owner); return Parent; },
                Product = (brain, type) => { Assert.Equal(Token, brain); Queries.Add(type); return Products.GetValueOrDefault(type); },
                AddProduct = (brain, factory, type) =>
                {
                    Assert.Equal(Token, brain);
                    Assert.Equal(Factory, factory);
                    Added++;
                    Products[type] = Content(Parent, type, 8);
                    Content(factory, type, 16);
                },
                Level = factory => { Levels++; return (uint)(Pool.Slot(factory)[11] >> 5); },
                Upgrade = factory => { Assert.Equal(Factory, factory); Upgrades++; },
                Log = (brain, _, _) => { Assert.Equal(Token, brain); Logs++; },
                Demo = () => false,
            };
        }
        /// <summary>실제 클라이언트 슬롯을 명시 0으로 할당한다.</summary>
        private int New()
        {
            int sid = Pool.Allocate(SidAllocation.Client);
            Pool.AllocatedBytes(sid).Clear();
            return sid;
        }
        /// <summary>부모 head에 원문 필드 폭으로 기술 또는 생산 내용물을 연결한다.</summary>
        public int Content(int parent, uint type, uint flags)
        {
            int sid = New();
            Span<byte> raw = Pool.AllocatedBytes(sid);
            raw[10] = 6;
            BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(18, 2), (ushort)type);
            BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(20, 2), (ushort)flags);
            BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(4, 2), BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(parent).Slice(6, 2)));
            BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(parent).Slice(6, 2), (ushort)sid);
            return sid;
        }
        /// <summary>현재 본체 DWORD를 읽는다.</summary>
        public uint Get(int offset) => BinaryPrimitives.ReadUInt32LittleEndian(Memory.Bytes(Token).AsSpan(offset, 4));
        /// <summary>선택한 본체 DWORD를 쓴다.</summary>
        public void Set(int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Memory.Bytes(Token).AsSpan(offset, 4), value);
        /// <summary>현재 큐의 실제 칸을 읽는다.</summary>
        public uint Item(int pointer, int index) => BinaryPrimitives.ReadUInt32LittleEndian(Memory.Bytes(Get(pointer)).AsSpan(index * 4, 4));
        /// <summary>현재 큐에 명시 입력을 쓴다.</summary>
        public void PutItem(int pointer, int index, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Memory.Bytes(Get(pointer)).AsSpan(index * 4, 4), value);
        /// <summary>원본 두 DWORD 기한을 읽는다.</summary>
        public double Time(int offset) => BitConverter.UInt64BitsToDouble(Get(offset) | ((ulong)Get(offset + 4) << 32));
        /// <summary>같은 실제 저장소·현재 장치로 본체를 만든다.</summary>
        public RawAiFactoryProduction Make() => new(Memory, Pool, Types, Finder,
            new AiFactoryProductionGlobals { Now = () => Now, InvalidGraph = () => 254 }, Hooks);
    }

    /// <summary>원소·flags4·현재 생산 여부를 내용물 순서대로 검사한다.</summary>
    [Fact]
    public void FirstMatchingKnownUnregisteredType()
    {
        var scene = new Scene();
        scene.Content(scene.Parent, 102, 4);
        scene.Content(scene.Parent, 101, 0);
        Assert.Equal(100u, scene.Make().SelectTechnology(scene.Token, 3));
        Assert.Equal([100u], scene.Queries);
        scene.Products[100] = scene.FirstTechnology;
        Assert.Equal(0u, scene.Make().SelectTechnology(scene.Token, 3));
        scene.Types[102].Theme = 3;
        Assert.Equal(102u, scene.Make().SelectTechnology(scene.Token, 3));
    }

    /// <summary>배터리의 치환 모드 네 개를 본래 원소·중립·다른 원소와 교차 대조한다.</summary>
    [Theory]
    [InlineData(0u, 100u, 0u)]
    [InlineData(1u, 0u, 100u)]
    [InlineData(2u, 100u, 100u)]
    [InlineData(3u, 0u, 0u)]
    public void BatteryThemeReplacementModes(uint mode, uint own, uint neutral)
    {
        var scene = new Scene();
        scene.Types[100].Theme = 1;
        scene.Types[100].Flags2 = 0x800;
        scene.Set(0x3b8, mode);
        Assert.Equal(own, scene.Make().SelectTechnology(scene.Token, 1));
        Assert.Equal(neutral, scene.Make().SelectTechnology(scene.Token, 3));
        Assert.Equal(0u, scene.Make().SelectTechnology(scene.Token, 2));
    }

    /// <summary>생산 조회가 현재 링크를 끊어도 커서가 먼저 저장한 next를 유지한다.</summary>
    [Fact]
    public void FinderKeepsSavedNextAcrossProductEffects()
    {
        var scene = new Scene();
        int newer = scene.Content(scene.Parent, 101, 4);
        scene.Hooks.Product = (_, type) =>
        {
            scene.Queries.Add(type);
            if (type == 101)
            {
                BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.AllocatedBytes(newer).Slice(4, 2), 0);
                return newer;
            }
            return 0;
        };
        Assert.Equal(100u, scene.Make().SelectTechnology(scene.Token, 3));
        Assert.Equal([101u, 100u], scene.Queries);
    }

    /// <summary>한 종류 추가 뒤 슬롯이 남으면 같은 시설을 유지하고 기한 두 개를 기록한다.</summary>
    [Fact]
    public void AddOneProductKeepsFactoryWithRoom()
    {
        var scene = new Scene();
        scene.Set(0xb00, 1);
        scene.Make().Manage(scene.Token);
        Assert.Equal(1, scene.Added);
        Assert.Equal(1, scene.Levels);
        Assert.Equal(1u, scene.Get(0x314));
        Assert.Equal(0u, scene.Get(0x320));
        Assert.Equal(1, scene.Logs);
        Assert.Equal(1, scene.Make().CountProducts(scene.Factory));
        Assert.Equal(scene.Now + 2.5, scene.Time(0x5a8));
        Assert.Equal(scene.Now + 3.5, scene.Time(0x5b0));
    }

    /// <summary>생산이 가득 차면 중복 대상을 모두 제거하고 다음 기술이 있으면 업그레이드 큐로 옮긴다.</summary>
    [Fact]
    public void FullFactoryMovesToUpgradeQueueAndPreservesOthers()
    {
        var scene = new Scene();
        scene.Content(scene.Factory, 99, 16);
        scene.Content(scene.Parent, 101, 4);
        scene.Set(0x314, 3);
        scene.PutItem(0x30c, 1, 123);
        scene.PutItem(0x30c, 2, (uint)scene.Factory);
        scene.Make().Manage(scene.Token);
        Assert.Equal(1, scene.Added);
        Assert.Equal(2, scene.Make().CountProducts(scene.Factory));
        Assert.Equal(1u, scene.Get(0x314));
        Assert.Equal(123u, scene.Item(0x30c, 0));
        Assert.Equal(1u, scene.Get(0x320));
        Assert.Equal((uint)scene.Factory, scene.Item(0x318, 0));
    }

    /// <summary>지갑1199는 대기하고 1200이면 요청→중복 제거→기한 순서다.</summary>
    [Fact]
    public void UpgradeGatesAndRequestOrder()
    {
        var scene = new Scene();
        scene.Set(0x314, 0);
        scene.Set(0x320, 2);
        scene.PutItem(0x318, 0, (uint)scene.Factory);
        scene.PutItem(0x318, 1, (uint)scene.Factory);
        scene.Set(0x2d8, 1199);
        scene.Make().Manage(scene.Token);
        Assert.Equal(0, scene.Upgrades);
        scene.Set(0x2d8, 1200);
        scene.Hooks.Upgrade = factory =>
        {
            Assert.Equal(scene.Factory, factory);
            Assert.Equal(2u, scene.Get(0x320));
            Assert.Equal(0, scene.Time(0x5a8));
            scene.Upgrades++;
        };
        scene.Make().Manage(scene.Token);
        Assert.Equal(1, scene.Upgrades);
        Assert.Equal(0u, scene.Get(0x320));
        Assert.Equal(scene.Now + 2.5, scene.Time(0x5a8));
    }

    /// <summary>선택된 미연결은 앞선 큐·기한을 소비하지 않는다.</summary>
    [Fact]
    public void SelectedMissingEffectsPreserveQueues()
    {
        var scene = new Scene();
        scene.Hooks.AddProduct = null;
        Assert.Throws<InvalidOperationException>(() => scene.Make().Manage(scene.Token));
        Assert.Equal(1u, scene.Get(0x314));
        Assert.Equal(0, scene.Time(0x5a8));
        scene.Set(0x314, 0);
        scene.Set(0x320, 1);
        scene.PutItem(0x318, 0, (uint)scene.Factory);
        scene.Hooks.Upgrade = null;
        Assert.Throws<InvalidOperationException>(() => scene.Make().Manage(scene.Token));
        Assert.Equal(1u, scene.Get(0x320));
        scene.Set(0x294, 0);
        scene.Make().Manage(scene.Token);
        uint small = scene.Memory.Allocate(4);
        Assert.Throws<ArgumentException>(() => scene.Make().SelectTechnology(small, 3));
    }

    /// <summary>완료 시설 중복을 허용하고 큐가 가득 차도 현재 비용과 데모 지갑을 처리한다.</summary>
    [Fact]
    public void UpgradedRequeuesAndUsesCurrentDemoWallet()
    {
        var scene = new Scene();
        scene.Types[85].Cost = 500.75f;
        scene.Make().Upgraded(scene.Token, scene.Factory);
        Assert.Equal(2u, scene.Get(0x314));
        Assert.Equal((uint)scene.Factory, scene.Item(0x30c, 1));
        Assert.Equal(700u, scene.Get(0x2d8));
        scene.Set(0x314, 4);
        scene.Hooks.Demo = () => { scene.Set(0x2d8, 1400); return true; };
        scene.Make().Upgraded(scene.Token, scene.Factory);
        Assert.Equal(4u, scene.Get(0x314));
        Assert.Equal(1400u, scene.Get(0x2d8));
        scene.Hooks.Demo = () => true;
        scene.Make().Upgraded(scene.Token, scene.Factory);
        Assert.Equal(1190u, scene.Get(0x2d8));
    }
}
