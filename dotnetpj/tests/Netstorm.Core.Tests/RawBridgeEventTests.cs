using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 다리 사건 처리기(<see cref="RawBridgeEvents"/>)의 보호 경로와 분기 계약을 확인한다.
/// 원본 기대값 재생(X86RawBridgeEventTests)은 사건 반환·효과·두 슬롯을 대조하고,
/// 이 검사들은 합성 입력으로 권한·진단·하드 프레임·글자 없음·예외 시 프레임 복구를 고정한다 (W02 다리 사건 조각, 2026-10-20).
/// </summary>
public sealed class RawBridgeEventTests
{
    /// <summary>다리 번호.</summary>
    private const int Bridge = 50;

    /// <summary>새 객체 번호.</summary>
    private const int Born = 60;

    /// <summary>다리 타입 번호.</summary>
    private const uint BridgeType = 82;

    /// <summary>값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>관찰과 같은 14개 프레임 코드를 만든다.</summary>
    private static List<FrameCode> Frames()
    {
        const string letters = "JKALMNOJKLBJPPCD";
        var codes = new List<FrameCode>();
        // 방향 글자 14개를 차례로 둔다
        for (int i = 0; i < letters.Length; i++)
        {
            codes.Add(new FrameCode(letters[i], 'P', i + 1, (FrameCodeFlags)0));
        }
        return codes;
    }

    /// <summary>픽스처 장면이다. 다리 슬롯은 현재 프레임 'J'(0번)다.</summary>
    private sealed class Scene
    {
        /// <summary>슬롯 풀.</summary>
        public readonly RawSidPool Pool;

        /// <summary>공간 해시.</summary>
        public readonly RawSquidHash Hash;

        /// <summary>점유 지도.</summary>
        public readonly byte[] Spots;

        /// <summary>타입 표.</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>프레임 표.</summary>
        public readonly List<IReadOnlyList<FrameCode>> FrameTable;

        /// <summary>효과 기록.</summary>
        public readonly List<string> Events = [];

        /// <summary>이웃 탐색 결과(0 이면 이웃 없음).</summary>
        public int Neighbor;

        /// <summary>이웃 탐색이 던질 예외(있으면).</summary>
        public Exception? NeighborError;

        /// <summary>다음 생성 번호.</summary>
        public int BornSid = Born;

        /// <summary>번호 5~74번을 확보하고 다리 슬롯을 채운다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(24000, false);
            // 원본 관찰의 번호 범위를 확보한다
            for (int id = 5; id <= 74; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
            Hash = new RawSquidHash();
            Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            FrameTable = [];
            for (int i = 0; i < Types.Length; i++)
            {
                FrameTable.Add([]);
            }
            FrameTable[(int)BridgeType] = Frames();
            // 다리 슬롯: 타입·부모·단어·좌표·프레임·플래그
            Span<byte> raw = Pool.AllocatedBytes(Bridge);
            raw.Clear();
            raw[RawSidPool.TypeOffset] = (byte)BridgeType;
            Put(raw, 8, 7, 2);
            Put(raw, 12, 0x40, 2);
            Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f), 4);
            Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f), 4);
            Put(raw, 36, 0, 4);
            raw[0x22] = 3;
            raw[0x28] = 0x10;
            Pool.AllocatedBytes(Born).Clear();
        }

        /// <summary>효과 기록과 함께 처리기를 만든다.</summary>
        /// <param name="state">전역 상태.</param>
        public RawBridgeEvents Events_(BridgeEventState state) => new(Pool, state, new BridgeEventHooks
        {
            Frames = type => FrameTable[(int)type],
            FirstNeighbor = sid =>
            {
                Events.Add($"F:{sid}");
                // 설정된 예외가 있으면 그대로 던진다
                if (NeighborError is not null)
                {
                    throw NeighborError;
                }
                return Neighbor;
            },
            NotifySurface = sid => Events.Add($"N:{sid}"),
            Create = type =>
            {
                Events.Add($"C:{type}");
                Span<byte> born = Pool.AllocatedBytes(BornSid);
                born.Clear();
                born[RawSidPool.TypeOffset] = (byte)type;
                return BornSid;
            },
            Destroy = (sid, flags) => Events.Add($"D:{sid}:{flags}"),
            SetOwner = (sid, owner) => Events.Add($"O:{sid}:{owner}"),
            Pop = (sid, x, y, flags) => Events.Add($"P:{sid}:{flags}"),
        });
    }

    /// <summary>
    /// 처리기는 일곱 효과가 모두 있어야 한다.
    /// </summary>
    [Fact]
    public void ConstructorRequiresAllHooks()
    {
        var scene = new Scene();
        Assert.ThrowsAny<Exception>(() => new RawBridgeEvents(scene.Pool, new BridgeEventState(), new BridgeEventHooks()));
        var hooks = new BridgeEventHooks
        {
            Frames = type => [],
            FirstNeighbor = sid => 0,
            NotifySurface = sid => { },
            Create = type => Born,
            Destroy = (sid, flags) => { },
            SetOwner = (sid, owner) => { },
        };
        Assert.ThrowsAny<Exception>(() => new RawBridgeEvents(scene.Pool, new BridgeEventState(), hooks));
    }

    /// <summary>
    /// 처리하지 않는 사건은 payload를 그대로 돌려준다.
    /// </summary>
    [Fact]
    public void UnknownEventReturnsPayload()
    {
        var scene = new Scene();
        var bridge = scene.Events_(new BridgeEventState());
        Assert.Equal(12.5f, bridge.Handle(Bridge, 0x1234, 0, 12.5f));
        Assert.Empty(scene.Events);
    }

    /// <summary>
    /// 권한이 없으면 파괴도 변환도 하지 않고 끝(0)을 돌려준다.
    /// </summary>
    [Fact]
    public void WithoutAuthorityEventsEnd()
    {
        var scene = new Scene();
        var bridge = scene.Events_(new BridgeEventState { Authority = false });
        Assert.Equal(RawBridgeEvents.EventEnd, bridge.Handle(Bridge, RawBridgeEvents.DestroyEvent, 0, 0f));
        Assert.Equal(RawBridgeEvents.EventEnd, bridge.Handle(Bridge, RawBridgeEvents.FallEvent, 0, 0f));
        Assert.Empty(scene.Events);
        // 직접 변환도 권한이 없으면 아무것도 바꾸지 않는다
        byte[] before = scene.Pool.Bytes.ToArray();
        bridge.ConvertEnd(Bridge, 100f, 100f);
        Assert.Empty(scene.Events);
        Assert.Equal(before, scene.Pool.Bytes.ToArray());
    }

    /// <summary>
    /// 진단 플래그가 켜져 있으면 변환을 하지 않고 유지(−1)만 돌려준다.
    /// </summary>
    [Fact]
    public void DebugKeepSkipsConversion()
    {
        var scene = new Scene();
        var bridge = scene.Events_(new BridgeEventState { DebugKeep = true });
        Assert.Equal(RawBridgeEvents.EventKeep, bridge.Handle(Bridge, RawBridgeEvents.FallEvent, 0, 0f));
        Assert.Empty(scene.Events);
    }

    /// <summary>
    /// 객체 타입 미만(form)은 변환하지 않고 거부한다.
    /// </summary>
    [Fact]
    public void FormTypeIsRejected()
    {
        var scene = new Scene();
        scene.Pool.RawSlot(Bridge)[RawSidPool.TypeOffset] = 5;
        var bridge = scene.Events_(new BridgeEventState());
        Assert.Throws<InvalidOperationException>(() => bridge.ConvertEnd(Bridge, 100f, 100f));
    }

    /// <summary>
    /// 현재 프레임이 표를 벗어나면 거부한다.
    /// </summary>
    [Fact]
    public void FrameOutOfRangeIsRejected()
    {
        var scene = new Scene();
        Put(scene.Pool.RawSlot(Bridge), 36, 99, 4);
        var bridge = scene.Events_(new BridgeEventState());
        var error = Assert.Throws<ArgumentOutOfRangeException>(() => bridge.ConvertEnd(Bridge, 100f, 100f));
        Assert.Equal("bridge", error.ParamName);
    }

    /// <summary>
    /// 이웃 탐색이 던지면 임시 프레임을 되돌리고 그대로 던진다.
    /// </summary>
    [Fact]
    public void NeighborErrorRestoresFrame()
    {
        var scene = new Scene();
        scene.NeighborError = new InvalidOperationException("이웃 실패");
        var bridge = scene.Events_(new BridgeEventState());
        // 세로 칸 J 이고 목표 y 가 더 크므로 N 으로 바꾼 뒤 이웃 탐색에서 실패한다
        Assert.Throws<InvalidOperationException>(() => bridge.ConvertEnd(Bridge, 20f, 100f));
        uint frame = (uint)scene.Pool.Slot(Bridge)[36] | ((uint)scene.Pool.Slot(Bridge)[37] << 8);
        Assert.Equal(0u, frame);
    }

    /// <summary>
    /// 이웃이 없으면 다리를 지우고 새 객체를 만들지 않는다.
    /// </summary>
    [Fact]
    public void NoNeighborDestroysBridge()
    {
        var scene = new Scene();
        scene.Neighbor = 0;
        var bridge = scene.Events_(new BridgeEventState());
        bridge.ConvertEnd(Bridge, 20f, 100f);
        Assert.Contains("F:50", scene.Events);
        Assert.Contains("D:50:0", scene.Events);
        Assert.DoesNotContain("C:82", scene.Events);
    }

    /// <summary>
    /// 하드 프레임이면 수명·알림까지만 하고 새 객체를 만들지 않는다.
    /// </summary>
    [Fact]
    public void HardFrameStopsBeforeCreate()
    {
        var scene = new Scene();
        scene.Neighbor = 70;
        // 현재 프레임을 하드로 둔다(0번을 하드로 바꾼다)
        scene.FrameTable[(int)BridgeType] = [new FrameCode('J', 'P', 1, (FrameCodeFlags)0x40), new FrameCode('N', 'P', 2, (FrameCodeFlags)0x40)];
        var bridge = scene.Events_(new BridgeEventState());
        bridge.ConvertEnd(Bridge, 20f, 100f);
        Assert.Contains("N:50", scene.Events);
        Assert.DoesNotContain("C:82", scene.Events);
        // 수명 비트는 (5×8−8)=0x20 으로 맞춘다
        Assert.Equal(0x20, scene.Pool.Slot(Bridge)[12] & 0x78);
    }

    /// <summary>
    /// 방향 글자가 J·K 가 아니면 아무것도 하지 않는다.
    /// </summary>
    [Fact]
    public void OtherSideLettersDoNothing()
    {
        var scene = new Scene();
        scene.Neighbor = 70;
        scene.FrameTable[(int)BridgeType] = [new FrameCode('A', 'P', 1, (FrameCodeFlags)0)];
        var bridge = scene.Events_(new BridgeEventState());
        bridge.ConvertEnd(Bridge, 20f, 100f);
        Assert.Empty(scene.Events);
    }

    /// <summary>
    /// 변환할 끝 칸 글자가 타입에 없으면 거부하고 임시 프레임을 되돌린다.
    /// 타입 단어 알림은 다리 타입 번호와 같을 때만 나간다.
    /// </summary>
    [Fact]
    public void MissingLetterAndWordNotifyFollowContracts()
    {
        // 끝 칸 글자가 없으면 거부한다
        {
            var scene = new Scene();
            scene.Neighbor = 70;
            scene.FrameTable[(int)BridgeType] = [new FrameCode('J', 'P', 1, (FrameCodeFlags)0)];
            var bridge = scene.Events_(new BridgeEventState());
            Assert.ThrowsAny<Exception>(() => bridge.ConvertEnd(Bridge, 20f, 100f));
            uint frame = (uint)scene.Pool.Slot(Bridge)[36];
            Assert.Equal(0u, frame);
        }
        // 타입 단어 알림은 다리 타입 번호와 같을 때만 나간다
        {
            var scene = new Scene();
            scene.Neighbor = 70;
            var bridge = scene.Events_(new BridgeEventState { BridgeType = 999 });
            bridge.ConvertEnd(Bridge, 20f, 100f);
            Assert.DoesNotContain("N:60", scene.Events);
            Assert.Contains("N:50", scene.Events);
        }
    }

    /// <summary>
    /// 정상 변환은 프레임·소유자·단어·부모·플래그를 옮기고 이전 칸을 지운 뒤 새 칸을 등록한다.
    /// </summary>
    [Fact]
    public void ConversionCopiesFieldsAndRepops()
    {
        var scene = new Scene();
        scene.Neighbor = 70;
        var bridge = scene.Events_(new BridgeEventState { BridgeType = BridgeType });
        bridge.ConvertEnd(Bridge, 20f, 100f);
        // 임시 프레임 뒤 복구되고, 새 객체에는 N 프레임(5번)·소유자 3·수명 단어·부모 7·플래그 0x10 이 간다
        ReadOnlySpan<byte> born = scene.Pool.Slot(Born);
        Assert.Equal(5u, (uint)born[36]);
        Assert.Equal(0x20, born[12] & 0x78);
        Assert.Equal(7, born[8] | (born[9] << 8));
        Assert.Equal(0x10, born[0x28] & 0x10);
        Assert.Contains("D:50:0", scene.Events);
        Assert.Contains("P:60:0", scene.Events);
        // 소유자는 외부 효과로 넘긴다(다리 슬롯의 3번)
        Assert.Contains("O:60:3", scene.Events);
        // 다리 슬롯은 지워지되 새 객체의 표면 알림은 다리 타입일 때만 나간다
        Assert.Contains("N:60", scene.Events);
    }
}
