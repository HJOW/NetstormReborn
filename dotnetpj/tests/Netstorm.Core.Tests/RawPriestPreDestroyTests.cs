using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 사제 가상 사전 파괴(<see cref="RawPriestPreDestroy"/>)의 경계 계약을 cpppj PriestPreDestroyTests.cpp 의
/// <c>priest_predestroy_guards_and_dispatch</c> 중 10.78 몫으로 확인한다.
/// 범위 밖 번호·잘못된 가상 표·상태 비트·손상된 목록 개수·Carrier 사전 검사 실패가 목록과 슬롯을 바꾸기 전에
/// 거부되는지, 추가 비트 9 가 목록 접근 없이 Carrier 만 부르는지, 분배 래퍼가 사제 사건만 가로채는지 본다.
/// 기준 판본은 10.78 이다 (W03 다리 밖 자산 삭제 재정의 조각, 2026-10-23).
/// </summary>
public sealed class RawPriestPreDestroyTests
{
    /// <summary>검사 대상 사제 번호.</summary>
    private const int Source = 50;

    /// <summary>10.78 실제 사제 클래스의 가상 표 값.</summary>
    private const uint PatchPriestVtable = 0x0050F210;

    /// <summary>사제가 아닌 다른 클래스의 가상 표 값(거부 확인용).</summary>
    private const uint ForeignVtable = 0x005034C8;

    /// <summary>실제 사제 타입 번호.</summary>
    private const byte PriestType = 158;

    /// <summary>원본 관찰의 풀 크기.</summary>
    private const int Capacity = 24000;

    /// <summary>사제 목록의 초기 저장소. 행 1과 같은 여섯 항목이다.</summary>
    private static readonly uint[] InitialEntries = [50, 60, 50, 70, 50, 80];

    /// <summary>한 장면의 풀·상태·기록. cpppj PriestScene 의 10.78 몫이다.</summary>
    private sealed class Scene
    {
        /// <summary>사제 50 이 실제 할당된 클라이언트 풀.</summary>
        public readonly RawSidPool Pool;

        /// <summary>사제 목록 상태.</summary>
        public readonly PriestPostPopState State = new();

        /// <summary>효과 문자열.</summary>
        public readonly List<string> Parts = [];

        /// <summary>Carrier 사전 검사 횟수.</summary>
        public int Validations;

        /// <summary>보호막 조회가 돌려줄 번호.</summary>
        public uint Found;

        /// <summary>보호막 조회 뒤 추가 비트를 바꾸는 값. null 이면 바꾸지 않는다.</summary>
        public string? After;

        /// <summary>클라이언트 풀을 만들고 사제 50 까지 실제 할당한다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(Capacity, false);
            // 검사 대상 50 까지 실제 번호를 확보한다
            while (Pool.Allocate(SidAllocation.Client) != Source)
            {
            }
            Prepare();
        }

        /// <summary>입력 raw·목록을 행 1과 같은 값으로 채운다. 기대 결과는 넣지 않는다.</summary>
        public void Prepare()
        {
            Parts.Clear();
            Validations = 0;
            Found = 0;
            After = null;
            Span<byte> raw = Pool.RawSlot(Source);
            raw.Fill(0xab);
            Put(raw, 0, PatchPriestVtable);
            raw[10] = PriestType;
            raw[11] = 0;
            raw[40] = 0;
            State.Priests.Entries = (uint[])InitialEntries.Clone();
            State.Priests.Count = 0;
        }

        /// <summary>호출 시점의 인자·목록 개수·추가 비트를 기록한다.</summary>
        /// <param name="marker">효과 종류 문자.</param>
        /// <param name="sid">호출 대상 번호.</param>
        /// <param name="flags">플래그 접미사.</param>
        private void Record(char marker, int sid, string flags)
        {
            // 사건 사이에 세미콜론을 둔다
            if (Parts.Count != 0)
            {
                Parts.Add(";");
            }
            Parts.Add($"{marker}:{sid}{flags}:{State.Priests.Count}:{Pool.Slot(Source)[40]}");
        }

        /// <summary>실제 경계를 기록하는 훅 묶음.</summary>
        public PriestPreDestroyHooks Hooks() => new()
        {
            FindForcefield = sid =>
            {
                Record('F', sid, string.Empty);
                // 조회가 추가 비트를 바꾸면 다음 분기가 그 값을 읽는다
                if (After is not null)
                {
                    Pool.RawSlot(Source)[40] = (byte)uint.Parse(After, System.Globalization.CultureInfo.InvariantCulture);
                }
                return Found;
            },
            DestroyForcefield = (sid, flags) => Record('D', sid, ":" + flags),
            ValidateCarrier = (_, _) => Validations++,
            CarrierPre = (sid, flags) => Record('C', sid, ":" + flags),
        };

        /// <summary>현재 목록 저장소 전체를 문자열로 만든다.</summary>
        public string Entries() => string.Join(',', State.Priests.Entries);

        /// <summary>효과 문자열.</summary>
        public string Trace() => string.Concat(Parts);

        /// <summary>슬롯 전체를 소문자 hex 로 만든다.</summary>
        public string SlotHex() => Convert.ToHexStringLower(Pool.Slot(Source).ToArray());
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>거부 장면이 목록·슬롯·효과를 하나도 바꾸지 않는지 확인한다.</summary>
    [Fact]
    public void RejectsRawListAndCarrierBeforeWriting()
    {
        var scene = new Scene();
        var body = new RawPriestPreDestroy(scene.Pool, scene.State, scene.Hooks());
        // 사제가 아닌 번호·풀 밖 번호·상한 밖 번호를 모두 거부한다
        foreach (int sid in new[] { 0, 51, Capacity, 65535 })
        {
            string entries = scene.Entries();
            string hex = scene.SlotHex();
            Assert.ThrowsAny<Exception>(() => body.PreDestroy(sid, 0));
            Assert.Empty(scene.Parts);
            Assert.Equal(0u, scene.State.Priests.Count);
            Assert.Equal(entries, scene.Entries());
            Assert.Equal(hex, scene.SlotHex());
        }
        // 상태 비트 1·8 은 거부한다
        foreach (byte state in new byte[] { 1, 8 })
        {
            scene.Pool.RawSlot(Source)[11] = state;
            Assert.Throws<InvalidOperationException>(() => body.PreDestroy(Source, 0));
            scene.Pool.RawSlot(Source)[11] = 0;
        }
        // 다른 가상 표는 사제 경로가 아니다
        Put(scene.Pool.RawSlot(Source), 0, ForeignVtable);
        Assert.False(body.Handles(Source));
        Assert.Throws<InvalidOperationException>(() => body.PreDestroy(Source, 0));
        Put(scene.Pool.RawSlot(Source), 0, PatchPriestVtable);
        // 손상된 목록 개수는 목록을 쓰기 전에 거부한다
        scene.State.Priests.Count = 7;
        string saved = scene.Entries();
        Assert.Throws<InvalidOperationException>(() => body.PreDestroy(Source, 0));
        Assert.Empty(scene.Parts);
        Assert.Equal(saved, scene.Entries());
        Assert.Equal(7u, scene.State.Priests.Count);
    }

    /// <summary>Carrier 사전 검사가 실패하면 목록·보호막 효과 없이 중단한다.</summary>
    [Fact]
    public void CarrierValidationFailurePreservesList()
    {
        var scene = new Scene();
        scene.State.Priests.Count = 6;
        PriestPreDestroyHooks hooks = scene.Hooks();
        hooks.ValidateCarrier = (_, _) => throw new InvalidOperationException("미복원 Carrier 거부");
        var body = new RawPriestPreDestroy(scene.Pool, scene.State, hooks);
        string entries = scene.Entries();
        Assert.Throws<InvalidOperationException>(() => body.PreDestroy(Source, 0));
        Assert.Empty(scene.Parts);
        Assert.Equal(6u, scene.State.Priests.Count);
        Assert.Equal(entries, scene.Entries());
    }

    /// <summary>추가 비트 9 는 목록을 읽지 않고 Carrier 만 부른다.</summary>
    [Fact]
    public void AbstractPrefixSkipsListAndReadsOnlyCarrier()
    {
        var scene = new Scene();
        scene.State.Priests.Count = 7;
        scene.Pool.RawSlot(Source)[40] = 9;
        var body = new RawPriestPreDestroy(scene.Pool, scene.State, scene.Hooks());
        string entries = scene.Entries();
        body.PreDestroy(Source, 123);
        // 손상된 개수 7 은 접근하지 않으므로 컴파일된 몸체가 Carrier 만 기록한다
        Assert.Equal("C:50:123:7:9", scene.Trace());
        Assert.Equal(7u, scene.State.Priests.Count);
        Assert.Equal(entries, scene.Entries());
        // Carrier 사전 검사는 일반·추상 모두 한 번 부른다
        Assert.Equal(1, scene.Validations);
    }

    /// <summary>일반 사제는 목록을 압축하고 보호막을 한 번 요청한다.</summary>
    [Fact]
    public void OrdinaryPathRemovesAllAndRequestsForcefieldOnce()
    {
        var scene = new Scene();
        scene.State.Priests.Count = 6;
        scene.Found = 12;
        var body = new RawPriestPreDestroy(scene.Pool, scene.State, scene.Hooks());
        body.PreDestroy(Source, 7);
        // 세 번 나온 50 을 모두 빼고 나머지 순서를 유지하며, 비활성 꼬리는 원본처럼 남긴다
        Assert.Equal(3u, scene.State.Priests.Count);
        Assert.Equal("60,70,80,70,50,80", scene.Entries());
        Assert.Equal("F:50:3:0;D:12:0:3:0;C:50:7:3:0", scene.Trace());
        Assert.Equal(1, scene.Validations);
        // 풀 밖 번호는 삭제 요청을 하지 않는다
        scene.Prepare();
        scene.State.Priests.Count = 6;
        scene.Found = Capacity;
        body.PreDestroy(Source, 0);
        Assert.Equal("F:50:3:0;C:50:0:3:0", scene.Trace());
    }

    /// <summary>분배 래퍼는 사제 사건만 가로채고 나머지를 그대로 넘긴다.</summary>
    [Fact]
    public void WrapperInterceptsOnlyPriestPreDestroy()
    {
        var scene = new Scene();
        scene.State.Priests.Count = 6;
        var body = new RawPriestPreDestroy(scene.Pool, scene.State, scene.Hooks());
        var forwarded = new List<SquidDestroyEvent>();
        SquidDestroyHooks hooks = body.WrapHooks(scene.Pool, new SquidDestroyHooks
        {
            Emit = forwarded.Add,
            Selected = () => 3,
        });
        Assert.Equal(3, hooks.Selected!());
        hooks.Emit!(new SquidDestroyEvent(SquidDestroyEffect.PostDestroy, Source, 0, 1));
        Assert.Single(forwarded);
        // 사제 사전 파괴는 이 몸체가 처리하고 후속으로 넘기지 않는다
        hooks.Emit!(new SquidDestroyEvent(SquidDestroyEffect.PreDestroy, Source, 0, 4));
        Assert.Single(forwarded);
        Assert.Equal("F:50:3:0;C:50:4:3:0", scene.Trace());
        // 사제가 아닌 번호의 사전 파괴도 그대로 넘긴다
        hooks.Emit!(new SquidDestroyEvent(SquidDestroyEffect.PreDestroy, 51, 0, 5));
        Assert.Equal(2, forwarded.Count);
        // 다른 풀·후속 효과 누락·필수 훅 누락은 첫 호출 전에 거부한다
        Assert.Throws<ArgumentException>(() => body.WrapHooks(new RawSidPool(Capacity, false), new SquidDestroyHooks { Emit = forwarded.Add }));
        Assert.Throws<ArgumentException>(() => body.WrapHooks(scene.Pool, new SquidDestroyHooks()));
        Assert.Throws<ArgumentException>(() => new RawPriestPreDestroy(scene.Pool, scene.State, new PriestPreDestroyHooks
        {
            FindForcefield = _ => 0,
            DestroyForcefield = (_, _) => { },
            ValidateCarrier = (_, _) => { },
        }));
    }
}
