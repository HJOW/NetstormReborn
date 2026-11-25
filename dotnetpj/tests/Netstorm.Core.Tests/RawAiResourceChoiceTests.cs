// AI 자원 선택의 실제 원문·절삭·그래프/목록 효과 순서를 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 자원 선택을 검사하는 클래스다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.
// 근거: cpppj tests/AiResourceChoiceTests.cpp의 12건 전부다.
public sealed class RawAiResourceChoiceTests
{
    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>같은 실제 풀/본체/등록 목록과 명시 하위 효과를 준비한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    private sealed class AiResourceScene
    {
        /// <summary>현재 슬롯과 실제 본체/목록 소유자다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>실제 본체 저장소다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x4b000000u, bytes => bytes.Fill(0));

        /// <summary>실제 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>현재 무효 그래프 값이다.</summary>
        public uint Invalid = 254;

        /// <summary>현재 TYPE 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>명시 입력의 역할 SID다.</summary>
        public Sid Actor;

        /// <summary>명시 입력의 역할 SID다.</summary>
        public Sid Temple;

        /// <summary>명시 입력의 역할 SID다.</summary>
        public Sid First;

        /// <summary>명시 입력의 역할 SID다.</summary>
        public Sid Second;

        /// <summary>명시 입력의 역할 SID다.</summary>
        public Sid Third;

        /// <summary>명시 그래프 하위의 결과다.</summary>
        public readonly Dictionary<ushort, uint> Graphs = [];

        /// <summary>선택된 실제 하위다.</summary>
        public AiResourceChoiceHooks Hooks = new();

        /// <summary>원문 효과/순서 관찰이다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>명시 상태 입력이다.</summary>
        public int Crystals;

        /// <summary>이동한 목표다.</summary>
        public Sid Moved;

        /// <summary>현재 신전 중심이다.</summary>
        public RawPathPoint Center = new(20, 30);

        /// <summary>다음에 할당할 SID 번호다.</summary>
        private ushort _nextSid = 5;

        /// <summary>판단 결과를 계산하지 않고 원문과 선택된 하위만 공급한다.</summary>
        public AiResourceScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            Actor = New(133);
            Temple = New(75);
            First = New(80);
            Second = New(80);
            Third = New(80);
            Types[80].Flags2 = 0x10000000;
            Field(0x294, Temple.Value);
            Field(0x298, 7);
            Field(0x248, 0x10);
            Field(0xb00, 1);
            Hooks.Center = sid =>
            {
                Assert.Equal(Temple, sid);
                Trace.Add("center");
                return Center;
            };
            Hooks.Graph = sid =>
            {
                Trace.Add("graph:" + sid.Value);
                return Graphs.TryGetValue(sid.Value, out uint found) ? found : 0u;
            };
            Hooks.Crystals = sid =>
            {
                Assert.Equal(Actor, sid);
                Trace.Add("crystals");
                return Crystals;
            };
            Hooks.Target = (brain, sid, target) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(Actor, sid);
                Moved = target;
                Trace.Add("target");
            };
            Hooks.Home = (brain, sid) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(Actor, sid);
                Trace.Add("home");
            };
            Hooks.CaptureLog = (brain, sid) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("capture:" + sid.Value);
            };
            Hooks.DisconnectedLog = (brain, sid) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("disconnected:" + sid.Value);
            };
            Hooks.Assertion = text =>
            {
                Assert.Equal("s.isValid() && (s.getGenus()&gGEYSER)", text);
                Trace.Add("assert");
            };
        }

        /// <summary>실제 확보한 payload만 명시 0으로 준비한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
        /// <param name="type">쓸 TYPE 번호다.</param>
        /// <returns>확보한 SID다.</returns>
        public Sid New(byte type)
        {
            ushort sid = _nextSid++;
            Span<byte> raw = Pool.RawSlot(sid);
            raw.Fill(0);
            raw[RawSidPool.TypeOffset] = type;
            return new Sid(sid);
        }

        /// <summary>지정 현재 본체 DWORD만 쓴다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>실제 현재 본체 DWORD를 읽는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => Get(Memory.Bytes(Token), at);

        /// <summary>현재 실제 슬롯 위치를 입력한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
        /// <param name="sid">위치를 쓸 SID다.</param>
        /// <param name="x">쓸 가로 float다.</param>
        /// <param name="y">쓸 세로 float다.</param>
        public void Position(Sid sid, float x, float y)
        {
            Span<byte> raw = Pool.RawSlot(sid.Value);
            Put(raw, 14, BitConverter.SingleToUInt32Bits(x));
            Put(raw, 18, BitConverter.SingleToUInt32Bits(y));
        }

        /// <summary>새 실제 목록에 입력을 쓰고 count/cap/포인터를 같은 본체에 연결한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
        /// <param name="at">목록 기록 위치다.</param>
        /// <param name="values">쓸 SID 목록이다.</param>
        /// <param name="capacity">목록 용량이다.</param>
        /// <returns>확보한 backing 토큰이다.</returns>
        public uint List(int at, Sid[] values, uint capacity = 8)
        {
            uint pointer = Memory.Allocate((int)capacity * 4);
            int offset = 0;
            // 명시 입력 순서대로 DWORD를 쓰는 반복문이다.
            foreach (Sid value in values)
            {
                Put(Memory.Bytes(pointer), offset, value.Value);
                offset += 4;
            }

            Field(at, pointer);
            Field(at + 4, capacity);
            Field(at + 8, (uint)values.Length);
            return pointer;
        }

        /// <summary>같은 실제 저장소/현재 전역을 빌린다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
        /// <returns>검사할 자원 선택 몸체다.</returns>
        public RawAiResourceChoice Make() => new(Memory, Pool, Types, () => Invalid, Hooks);
    }

    /// <summary>신전이 없으면 NaN/부호 0을 포함한 fallback 원문을 유지하고 center를 요구하지 않는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void HomeCenterRawFallbackAndCurrentTemple()
    {
        var scene = new AiResourceScene();
        scene.Field(0x294, 0);
        scene.Field(0x268, 0x7fc01234u);
        scene.Field(0x26c, 0x80000000u);
        scene.Hooks.Center = null;
        RawPathPoint point = scene.Make().HomeCenter(scene.Token);
        Assert.Equal(0x7fc01234u, BitConverter.SingleToUInt32Bits(point.X));
        Assert.Equal(0x80000000u, BitConverter.SingleToUInt32Bits(point.Y));
        scene.Field(0x294, scene.Temple.Value);
        scene.Hooks.Center = sid =>
        {
            Assert.Equal(scene.Temple, sid);
            scene.Field(0x294, scene.First.Value);
            return new RawPathPoint(7, 8);
        };
        RawPathPoint current = scene.Make().HomeCenter(scene.Token);
        Assert.Equal(7f, current.X);
        Assert.Equal(8f, current.Y);
        Assert.Equal(scene.First.Value, scene.Field(0x294));
    }

    /// <summary>소수 거리의 정수 절삭 동률은 먼저 방문한 항목이며 65536 경계는 선택하지 않는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void TruncatedDistanceTieAndInitialLimit()
    {
        var scene = new AiResourceScene();
        scene.List(0x330, [scene.First, scene.Second, scene.Third]);
        scene.Position(scene.First, 2.2f, 0);
        scene.Position(scene.Second, 2, 0);
        scene.Position(scene.Third, 256, 0);
        Assert.Equal(scene.First, scene.Make().Select(scene.Token, 0x330, new RawPathPoint(0, 0), 254, 254));
        Assert.Equal(["graph:7", "graph:8", "graph:9"], scene.Trace);
        scene.List(0x330, [scene.Third]);
        Assert.Equal(new Sid(0), scene.Make().Select(scene.Token, 0x330, new RawPathPoint(0, 0), 254, 254));
        scene.Field(0x338, 0xffffffffu);
        scene.Hooks = new AiResourceChoiceHooks();
        Assert.Equal(new Sid(0), scene.Make().Select(scene.Token, 0x330, new RawPathPoint(0, 0), 254, 254));
    }

    /// <summary>모든 후보의 그래프를 읽고 제외/필수 그래프는 현재 무효 sentinel과 비교한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void GraphFiltersAndCurrentInvalidSentinel()
    {
        var scene = new AiResourceScene();
        scene.List(0x330, [scene.First, scene.Second, scene.Third]);
        scene.Position(scene.First, 1, 0);
        scene.Position(scene.Second, 3, 0);
        scene.Position(scene.Third, 2, 0);
        scene.Graphs[scene.First.Value] = 9;
        scene.Graphs[scene.Second.Value] = 7;
        scene.Graphs[scene.Third.Value] = 8;
        Assert.Equal(scene.Second, scene.Make().Select(scene.Token, 0x330, new RawPathPoint(0, 0), 9, 7));
        Assert.Equal(3, scene.Trace.Count);
        scene.Invalid = 7;
        Assert.Equal(scene.Third, scene.Make().Select(scene.Token, 0x330, new RawPathPoint(0, 0), 9, 7));
    }

    /// <summary>그래프 효과가 현재 포인터/count/좌표를 바꿔도 이번 거리·후보는 이미 저장되어 있다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void GraphEffectRereadsCurrentList()
    {
        var scene = new AiResourceScene();
        scene.List(0x330, [scene.First, scene.Third]);
        scene.Position(scene.First, 1, 0);
        scene.Position(scene.Second, 2, 0);
        scene.Hooks.Graph = sid =>
        {
            scene.Trace.Add("graph:" + sid.Value);
            if (sid == scene.First)
            {
                scene.Position(scene.First, 100, 0);
                scene.List(0x330, [scene.Third, scene.Second]);
            }

            return 7u;
        };
        Assert.Equal(scene.First, scene.Make().Select(scene.Token, 0x330, new RawPathPoint(0, 0), 254, 7));
        Assert.Equal(["graph:7", "graph:8"], scene.Trace);
        scene.Trace.Clear();
        scene.Hooks.Graph = sid =>
        {
            Assert.Equal(scene.Third, sid);
            scene.Field(0x338, 0);
            return 7u;
        };
        Assert.Equal(scene.Third, scene.Make().Select(scene.Token, 0x330, new RawPathPoint(0, 0), 254, 7));
        Assert.Equal(0u, scene.Field(0x338));
    }

    /// <summary>원본 무효 ftol 하위 DWORD 0을 보존하며 잘못된 물리 목록은 성공으로 바꾸지 않는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void NonfiniteFtolAndPhysicalBounds()
    {
        var scene = new AiResourceScene();
        scene.List(0x330, [scene.First, scene.Second]);
        scene.Position(scene.First, float.NaN, 0);
        Assert.Equal(scene.First, scene.Make().Select(scene.Token, 0x330, new RawPathPoint(0, 0), 254, 254));
        scene.Field(0x330, 0xdeadbeefu);
        Assert.ThrowsAny<Exception>(() => scene.Make().Select(scene.Token, 0x330, new RawPathPoint(0, 0), 254, 254));
    }

    /// <summary>관문 탈락은 내용물도 읽지 않고 크리스털/수집 능력 탈락은 실제 귀환만 호출한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CollectionGatesAndCarriedCrystal()
    {
        var scene = new AiResourceScene();
        scene.Field(0x294, 0);
        scene.Hooks = new AiResourceChoiceHooks();
        scene.Make().CollectOrHome(scene.Token, scene.Actor);
        scene.Field(0x294, scene.Temple.Value);
        scene.Field(0x298, 254);
        scene.Make().CollectOrHome(scene.Token, scene.Actor);
        Assert.Empty(scene.Trace);
        var carried = new AiResourceScene();
        carried.Crystals = 1;
        carried.Make().CollectOrHome(carried.Token, carried.Actor);
        Assert.Equal(["crystals", "home"], carried.Trace);
        carried.Trace.Clear();
        carried.Crystals = 0;
        carried.Field(0x248, 0);
        carried.Make().CollectOrHome(carried.Token, carried.Actor);
        Assert.Equal(["crystals", "home"], carried.Trace);
    }

    /// <summary>현재 신전 중심→actor 그래프→간헐천 목록→실제 이동 순서다. 목표가 없으면 귀환한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CollectionUsesHomeCenterAndActualMovement()
    {
        var scene = new AiResourceScene();
        scene.List(0x330, [scene.First, scene.Second]);
        scene.Position(scene.First, 21, 30);
        scene.Position(scene.Second, 20, 32);
        scene.Graphs[scene.Actor.Value] = 7;
        scene.Graphs[scene.First.Value] = 8;
        scene.Graphs[scene.Second.Value] = 7;
        scene.Make().CollectOrHome(scene.Token, scene.Actor);
        Assert.Equal(scene.Second, scene.Moved);
        Assert.Equal(["crystals", "center", "graph:5", "graph:7", "graph:8", "target"], scene.Trace);
        scene.Trace.Clear();
        scene.Field(0x338, 0);
        scene.Make().CollectOrHome(scene.Token, scene.Actor);
        Assert.Equal(["crystals", "center", "graph:5", "home"], scene.Trace);
    }

    /// <summary>내용물 효과 뒤 능력을 다시 읽으며 actor 그래프 효과 뒤 현재 sentinel을 인자로 쓴다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CollectionEffectsUseCurrentCapabilityAndSentinel()
    {
        var scene = new AiResourceScene();
        scene.Hooks.Crystals = _ =>
        {
            scene.Field(0x248, 0);
            return 0;
        };
        scene.Make().CollectOrHome(scene.Token, scene.Actor);
        Assert.Equal(["home"], scene.Trace);
        scene.Trace.Clear();
        scene.Field(0x248, 0x10);
        scene.Hooks.Crystals = _ => 0;
        scene.List(0x330, [scene.First]);
        scene.Hooks.Graph = sid =>
        {
            if (sid == scene.Actor)
            {
                scene.Invalid = 7;
                return 7u;
            }

            return 8u;
        };
        scene.Make().CollectOrHome(scene.Token, scene.Actor);
        Assert.Equal(scene.First, scene.Moved);
        Assert.Equal(["center", "target"], scene.Trace);
    }

    /// <summary>비행 판정은 현재 TYPE만 보며 중복은 로그도 생략하고 full은 로그를 유지한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CaptureQueueFlyingDuplicateCapacityAndNoTimer()
    {
        var scene = new AiResourceScene();
        uint backing = scene.List(0x384, [scene.First], 2);
        scene.Types[80].Flags2 |= 0x20000u;
        scene.Pool.RawSlot(scene.First.Value)[11] = 3;
        Assert.True(scene.Make().HasFlyingCaptureCarrier(scene.Token));
        byte[] before = scene.Memory.Bytes(scene.Token).ToArray();
        scene.Make().RememberCaptureCarrier(scene.Token, scene.First);
        Assert.Empty(scene.Trace);
        scene.Make().RememberCaptureCarrier(scene.Token, scene.Actor);
        Assert.Equal(2u, scene.Field(0x38c));
        Assert.Equal(scene.Actor.Value, Get(scene.Memory.Bytes(backing), 4));
        scene.Make().RememberCaptureCarrier(scene.Token, scene.Second);
        Assert.Equal(2u, scene.Field(0x38c));
        Assert.Equal(["capture:5", "capture:8"], scene.Trace);
        byte[] after = scene.Memory.Bytes(scene.Token);
        Assert.True(before.AsSpan(0x5e0, 8).SequenceEqual(after.AsSpan(0x5e0, 8)));
        scene.Field(0x38c, 0);
        scene.Field(0x388, 0);
        scene.Field(0x384, 0xdeadbeefu);
        scene.Field(0xb00, 0);
        scene.Hooks = new AiResourceChoiceHooks();
        Assert.False(scene.Make().HasFlyingCaptureCarrier(scene.Token));
        scene.Make().RememberCaptureCarrier(scene.Token, scene.Actor);
    }

    /// <summary>끊긴 중복을 모두 안정 제거하며 대상 목록 full은 제거를 되돌리지 않는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void DisconnectedStableRemovalAndFullDestination()
    {
        var scene = new AiResourceScene();
        uint source = scene.List(0x330, [scene.First, scene.First, scene.Second, scene.Third]);
        uint destination = scene.List(0x348, [scene.Third], 2);
        scene.Graphs[scene.First.Value] = 8;
        scene.Graphs[scene.Second.Value] = 7;
        scene.Graphs[scene.Third.Value] = 9;
        scene.Make().ReclassifyDisconnected(scene.Token);
        Assert.Equal(1u, scene.Field(0x338));
        Assert.Equal(scene.Second.Value, Get(scene.Memory.Bytes(source), 0));
        Assert.Equal(2u, scene.Field(0x350));
        Assert.Equal(scene.Third.Value, Get(scene.Memory.Bytes(destination), 0));
        Assert.Equal(scene.First.Value, Get(scene.Memory.Bytes(destination), 4));
        Assert.Equal(["graph:7", "disconnected:7", "graph:8", "graph:9", "disconnected:9"], scene.Trace);
        Assert.Equal(scene.Third.Value, Get(scene.Memory.Bytes(source), 12));
    }

    /// <summary>SID 0 진단이 반환하면 무효 그래프를 사용하고 로그 누락이면 이동 전 원문을 유지한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void DisconnectedAssertionAndSelectedMissingLog()
    {
        var zero = new AiResourceScene();
        zero.List(0x330, [new Sid(0)]);
        zero.Field(0x298, 254);
        zero.Hooks.Graph = null;
        zero.Make().ReclassifyDisconnected(zero.Token);
        Assert.Equal(["assert"], zero.Trace);
        Assert.Equal(1u, zero.Field(0x338));
        var missing = new AiResourceScene();
        uint backing = missing.List(0x330, [missing.First]);
        missing.Graphs[missing.First.Value] = 8;
        missing.Hooks.DisconnectedLog = null;
        Assert.ThrowsAny<Exception>(() => missing.Make().ReclassifyDisconnected(missing.Token));
        Assert.Equal(1u, missing.Field(0x338));
        Assert.Equal(missing.First.Value, Get(missing.Memory.Bytes(backing), 0));
    }

    /// <summary>공용 내용물 총수는 전체 WORD mask와 DWORD kind를 비교하고 free/dead 후보를 임의 제외하지 않는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CountContainedUsesFullWordAndDwordFilters()
    {
        var scene = new AiResourceScene();
        var state = new ContainedFinderState();
        Span<byte> actor = scene.Pool.RawSlot(scene.Actor.Value);
        Span<byte> first = scene.Pool.RawSlot(scene.First.Value);
        Span<byte> second = scene.Pool.RawSlot(scene.Second.Value);
        Span<byte> third = scene.Pool.RawSlot(scene.Third.Value);
        BinaryPrimitives.WriteUInt16LittleEndian(actor.Slice(6, 2), scene.First.Value);
        BinaryPrimitives.WriteUInt16LittleEndian(first.Slice(4, 2), scene.Second.Value);
        BinaryPrimitives.WriteUInt16LittleEndian(second.Slice(4, 2), scene.Third.Value);
        first[10] = 6;
        second[10] = 6;
        third[10] = 6;
        first[11] = 3;
        BinaryPrimitives.WriteUInt16LittleEndian(first.Slice(18, 2), 154);
        BinaryPrimitives.WriteUInt16LittleEndian(first.Slice(20, 2), 0x120);
        BinaryPrimitives.WriteUInt16LittleEndian(second.Slice(18, 2), 154);
        BinaryPrimitives.WriteUInt16LittleEndian(second.Slice(20, 2), 0x20);
        BinaryPrimitives.WriteUInt16LittleEndian(third.Slice(18, 2), 155);
        BinaryPrimitives.WriteUInt16LittleEndian(third.Slice(20, 2), 0x100);
        Assert.Equal(2, RawContainedFinder.CountContained(scene.Pool, state, scene.Actor.Value, 0x20, 154));
        Assert.Equal(2, RawContainedFinder.CountContained(scene.Pool, state, scene.Actor.Value, 0x100, 0));
        Assert.Equal(3, RawContainedFinder.CountContained(scene.Pool, state, scene.Actor.Value, 0, 0));
        Assert.Equal(0, RawContainedFinder.CountContained(scene.Pool, state, scene.Actor.Value, 0, 0x10000));
    }
}
