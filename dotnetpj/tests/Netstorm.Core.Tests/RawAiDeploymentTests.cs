using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 풀·내용물 생성으로 cpppj AiDeploymentTests의 호출·바이트·현재값 계약을 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>생산 교체와 권한별 배치 및 비용 절삭을 검사한다. 게임 플레이 재현은 하지 않는다.</summary>
public sealed class RawAiDeploymentTests
{
    /// <summary>cpppj 검사와 같은 TYPE 133을 사용한다.</summary>
    private const uint Type = 133;

    /// <summary>현재 전역과 하위 부작용을 바꿔 원본 재조회 순서를 관찰한다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 PATCH 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768, true, null, OriginalEdition.Patch1078);
        /// <summary>생산·지갑이 공유하는 본체 메모리다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x45000000, bytes => bytes.Clear());
        /// <summary>현재 TYPE 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>실제 하위 호출 순서다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>새 내용물 SID다.</summary>
        public readonly List<int> Born = [];
        /// <summary>서버 요청 전체 인자다.</summary>
        public readonly List<ConstructionConfirmRequest> Requests = [];
        /// <summary>로컬 요청 전체 인자다.</summary>
        public readonly List<ConstructionLocalRequest> Locals = [];
        /// <summary>새 Regular 부모·주기다.</summary>
        public readonly List<(int Sid, float Rate)> Timers = [];
        /// <summary>현재 본체 토큰이다.</summary>
        public readonly uint Token;
        /// <summary>실제 기술 부모다.</summary>
        public readonly int Parent;
        /// <summary>원본 내용물 생성 본체다.</summary>
        public readonly RawContainedCreate Create;
        /// <summary>실제 하위와 현재값 변이를 받는 장치다.</summary>
        public readonly AiDeploymentHooks Hooks;
        /// <summary>현재 전역 getter다.</summary>
        public readonly AiDeploymentGlobals Globals;
        /// <summary>소비할 생산 SID다.</summary>
        public int Product;
        /// <summary>현재 서버 권한이다.</summary>
        public bool Authority = true;
        /// <summary>현재 건설 사제 이동 플래그다.</summary>
        public bool Move;
        /// <summary>할당 실패 경로를 선택한다.</summary>
        public bool Reserve = true;
        /// <summary>QWORD 정밀도를 확인할 현재 시각이다.</summary>
        public double Now = 16777216.25;

        /// <summary>같은 실제 풀에 부모·생산·내용물 생성기를 연결한다.</summary>
        public Scene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            Parent = New();
            Product = New();
            Types[Type].Cost = 123.75f;
            Types[Type].Flags2 = 0x4a00;
            Set(0x260, 2);
            Set(0x2d8, 1000);
            Set(0x224, BitConverter.SingleToUInt32Bits(2.5f));
            Create = new(Pool, new ContainedCreateState(), new ContainedCreateHooks
            {
                Create = (type, flags) =>
                {
                    Assert.Equal(6u, type);
                    Assert.Equal(2u, flags);
                    int sid = New();
                    Pool.AllocatedBytes(sid)[10] = 6;
                    Born.Add(sid);
                    return sid;
                },
                NotifySurface = _ => throw new InvalidOperationException("검사 외 표면 통지"),
                Attach = (sid, parent, flags) =>
                {
                    Assert.Equal(Parent, parent);
                    Assert.Equal(0x10u, flags);
                    Trace.Add("create");
                },
            });
            Globals = new() { Authority = () => Authority, MoveLocalBuilder = value => Move = value, Now = () => Now };
            Hooks = new()
            {
                Product = (token, type) => { Assert.Equal(Token, token); Assert.Equal(Type, type); Trace.Add("product"); return Product; },
                Technology = _ => { Trace.Add("technology"); return (uint)Parent; },
                Destroy = (sid, flags) => { Assert.Equal(Product, sid); Assert.Equal(0u, flags); Product = 0; Trace.Add("destroy"); },
                ReserveRegular = () => { Trace.Add("reserve"); return Reserve; },
                AddRegular = (sid, rate) => { Timers.Add((sid, rate)); Trace.Add("timer"); },
                Request = request => { Assert.True(Move); Requests.Add(request); Trace.Add("server"); },
                LocalPlace = request => { Assert.True(Move); Locals.Add(request); Trace.Add("local"); },
                Demo = () => { Trace.Add("demo"); return false; },
                Assertion = _ => Trace.Add("assert"),
            };
        }
        /// <summary>실제 클라이언트 슬롯을 할당하고 명시 0으로 채운다.</summary>
        public int New()
        {
            int sid = Pool.Allocate(SidAllocation.Client);
            Pool.AllocatedBytes(sid).Clear();
            return sid;
        }
        /// <summary>본체의 현재 DWORD를 읽는다.</summary>
        public uint Get(int offset) => BinaryPrimitives.ReadUInt32LittleEndian(Memory.Bytes(Token).AsSpan(offset, 4));
        /// <summary>본체의 지정 DWORD를 쓴다.</summary>
        public void Set(int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Memory.Bytes(Token).AsSpan(offset, 4), value);
        /// <summary>본체의 현재 QWORD 기한을 읽는다.</summary>
        public double Time(int offset) => BitConverter.UInt64BitsToDouble(Get(offset) | ((ulong)Get(offset + 4) << 32));
        /// <summary>같은 현재 장치로 검사할 본체를 만든다.</summary>
        public RawAiDeployment Make() => new(Memory, Pool, Types, Create, Globals, Hooks);
    }

    /// <summary>생산→대기→비용→서버 요청 순서와 새 내용물 바이트를 대조한다.</summary>
    [Fact]
    public void ServerProductAndCooldownOrder()
    {
        var scene = new Scene();
        scene.Make().Place(scene.Token, Type, 40.25f, 41.5f, 3, 7);
        Assert.Equal(["product", "destroy", "technology", "create", "reserve", "timer", "demo", "server"], scene.Trace);
        int born = Assert.Single(scene.Born);
        Assert.Equal((born, 2.5f), Assert.Single(scene.Timers));
        Assert.Equal((ushort)0x108, BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(born).Slice(20, 2)));
        Assert.Equal((ushort)133, BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(born).Slice(18, 2)));
        Assert.Equal(new ConstructionConfirmRequest(Type, 40.25f, 41.5f, 2, 3, 7, 0, 0, 0, 0), Assert.Single(scene.Requests));
        Assert.Equal(877u, scene.Get(0x2d8));
        Assert.False(scene.Move);
        Assert.Equal(scene.Now + 20, scene.Time(0x5b8));
        Assert.Equal(scene.Now + 20, scene.Time(0x5c0));
    }

    /// <summary>Regular 예약 실패는 비용과 실제 배치를 취소하지 않는다.</summary>
    [Fact]
    public void RegularAllocationFailureStillPlaces()
    {
        var scene = new Scene { Reserve = false };
        scene.Hooks.AddRegular = null;
        scene.Make().Place(scene.Token, Type, 10, 11, 0, 0);
        Assert.Single(scene.Born);
        Assert.Empty(scene.Timers);
        Assert.Single(scene.Requests);
        Assert.Equal(877u, scene.Get(0x2d8));
    }

    /// <summary>생산 없는 일반 타입과 시설의 비용·기한 분기를 비교한다.</summary>
    [Theory]
    [InlineData(0u, 1000u, false)]
    [InlineData(0x4200u, 877u, true)]
    public void MissingProductTypeBranches(uint flags, uint wallet, bool deadline)
    {
        var scene = new Scene { Product = 0 };
        scene.Types[Type].Flags2 = flags;
        scene.Make().Place(scene.Token, Type, 10, 11, 0, 0);
        Assert.Empty(scene.Born);
        Assert.Equal(wallet, scene.Get(0x2d8));
        Assert.Equal(0, scene.Time(0x5b8));
        Assert.Equal(deadline ? scene.Now + 20 : 0, scene.Time(0x5c0));
    }

    /// <summary>비권한 경로는 두 번째 예측 조각으로 로컬 다섯 인자를 보낸다.</summary>
    [Fact]
    public void ClientPredictionUsesSecondContainedForm()
    {
        var scene = new Scene { Authority = false };
        scene.Hooks.Request = null;
        scene.Make().Place(scene.Token, Type, 17.25f, 29.5f, 255, 0x123);
        Assert.Equal(2, scene.Born.Count);
        Assert.Equal(new ConstructionLocalRequest(scene.Born[1], 17.25f, 29.5f, 255, 0x123), Assert.Single(scene.Locals));
        Assert.Empty(scene.Requests);
        Assert.False(scene.Move);
    }

    /// <summary>비용 signed 절삭·NaN·DWORD 감김과 데모 getter 뒤 지갑 재조회를 확인한다.</summary>
    [Fact]
    public void CostTruncationAndDemoCurrentWallet()
    {
        var scene = new Scene();
        RawAiDeployment body = scene.Make();
        scene.Set(0x2d8, 123);
        Assert.True(body.CanAfford(scene.Token, Type));
        scene.Set(0x2d8, 122);
        Assert.False(body.CanAfford(scene.Token, Type));
        scene.Types[Type].Cost = -1.9f;
        body.Charge(scene.Token, Type);
        Assert.Equal(123u, scene.Get(0x2d8));
        scene.Types[Type].Cost = float.NaN;
        body.Charge(scene.Token, Type);
        Assert.Equal(123u, scene.Get(0x2d8));
        scene.Types[Type].Cost = 1;
        scene.Set(0x2d8, 0x80000000);
        body.Charge(scene.Token, Type);
        Assert.Equal(0x7fffffffu, scene.Get(0x2d8));
        scene.Hooks.Demo = () => { scene.Set(0x2d8, 1191); return true; };
        body.Charge(scene.Token, Type);
        Assert.Equal(1191u, scene.Get(0x2d8));
    }

    /// <summary>선택된 미연결은 이미 수행한 비용·생산 교체·이동 플래그를 임의 복구하지 않는다.</summary>
    [Fact]
    public void SelectedMissingEffectsPreservePrefix()
    {
        var scene = new Scene();
        scene.Hooks.Request = null;
        Assert.Throws<InvalidOperationException>(() => scene.Make().Place(scene.Token, Type, 10, 11, 0, 0));
        Assert.True(scene.Move);
        Assert.Single(scene.Born);
        Assert.Equal(877u, scene.Get(0x2d8));
        Assert.Equal(0, scene.Time(0x5b8));
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Make().CanAfford(scene.Token, 188));
        uint small = scene.Memory.Allocate(4);
        Assert.Throws<ArgumentException>(() => scene.Make().Charge(small, Type));
    }

    /// <summary>예약·타이머·요청의 부작용 뒤 주기·비용·owner·시각을 다시 읽는다.</summary>
    [Fact]
    public void CurrentInputsAfterChildEffects()
    {
        var scene = new Scene();
        scene.Hooks.ReserveRegular = () => { scene.Set(0x224, BitConverter.SingleToUInt32Bits(5.5f)); scene.Set(0x260, 3); return true; };
        scene.Hooks.AddRegular = (sid, rate) => { scene.Timers.Add((sid, rate)); scene.Types[Type].Cost = 12.5f; };
        scene.Hooks.Request = request => { scene.Requests.Add(request); scene.Now += 1; };
        scene.Make().Place(scene.Token, Type, 10, 11, 0, 0);
        Assert.Equal(5.5f, Assert.Single(scene.Timers).Rate);
        Assert.Equal(3u, Assert.Single(scene.Requests).Player);
        Assert.Equal(988u, scene.Get(0x2d8));
        Assert.Equal(scene.Now + 20, scene.Time(0x5b8));
    }

    /// <summary>원본 진단에서 TYPE이 바뀌면 새 genus로 비용을 판단한다.</summary>
    [Fact]
    public void AssertionRechecksType()
    {
        var scene = new Scene { Product = 0 };
        scene.Types[Type].Flags2 = 0x70000;
        scene.Hooks.Assertion = _ => scene.Types[Type].Flags2 = 0x4200;
        scene.Make().Place(scene.Token, Type, 10, 11, 0, 0);
        Assert.Equal(877u, scene.Get(0x2d8));
    }
}
