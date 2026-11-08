using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 자산 루트의 다리 가상 삭제 재정의(<see cref="RawSquidDestroyDispatch"/>)를 cpppj DestroyDispatchTests.cpp 의
/// 계약대로 확인한다. Graph 표면 수 4/5 경계·signed 음수·hard/보통 프레임·debugKeep·Graph 비활성·단락 평가·
/// 무효 Graph/프레임의 쓰기 전 예외·서로 다른 풀/공간 거부를 다룬다.
/// 기준 판본은 10.78 이며, CD 10.72 의 프레임 배치 갈래는 옮기지 않았다 (W03 자산 삭제 분배 조각, 2026-10-23).
/// </summary>
public sealed class RawSquidDestroyDispatchTests
{
    /// <summary>다리 타입 번호. cpppj DestroyDispatchTests.cpp 와 같은 원본 타입이다.</summary>
    private const int BridgeType = 82;

    /// <summary>10.78 다리 클래스의 가상 표 값(원본 kPatchBridgeVtable).</summary>
    private const uint BridgeVtable = 0x005034C8;

    /// <summary>상태 바이트의 빈 슬롯 비트.</summary>
    private const byte FreeBit = 1;

    /// <summary>가상 삭제 판정 장면. 공통 pre/post 깊이만 완료하는 최소 삭제 연결이다.</summary>
    private sealed class Scene
    {
        /// <summary>삭제 대상 슬롯 풀(서버 풀, 32,768칸).</summary>
        public readonly RawSidPool Pool;

        /// <summary>Pop·Unpop·Graph 가 공유하는 4단계 공간 해시.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>Pop·Unpop·Graph 가 공유하는 65,536칸 점유 지도.</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 번호 → 타입 레코드(10.78 188개, 전부 기본값).</summary>
        public readonly OriginalTypeRecord[] Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];

        /// <summary>타입 번호 → 프레임 코드 목록. 다리 타입에 보통·hard 두 프레임을 둔다.</summary>
        public readonly FrameCode[][] Frames = new FrameCode[OriginalConstructors.TypeCount][];

        /// <summary>Graph 초기 표 255 레코드. 0번 레코드의 표면 수를 입력으로 쓴다.</summary>
        public readonly GraphRecord[] Records = new GraphRecord[RawGraph.TableSize];

        /// <summary>현재 실행 상태. 시험에서 교체하면 다음 호출부터 반영된다.</summary>
        public BridgeDecayMode Mode = BridgeDecayMode.SinglePlayer;

        /// <summary>자산 뿌리의 공간 해제기.</summary>
        public readonly RawUnpop Unpop;

        /// <summary>공통 삭제 본체.</summary>
        public readonly RawSquidDestroy Base;

        /// <summary>같은 풀의 이웃·프레임 탐색기.</summary>
        public readonly RawSquidNeighbors Neighbors;

        /// <summary>같은 풀·타입 표의 Graph.</summary>
        public readonly RawGraph Graph;

        /// <summary>검사 대상 분배기.</summary>
        public readonly RawSquidDestroyDispatch Dispatch;

        /// <summary>공통 pre/post 깊이만 완료하고 나머지 효과를 세는 콜백 묶음.</summary>
        public readonly SquidDestroyHooks Hooks;

        /// <summary>지울 다리 루트 번호.</summary>
        public readonly int Source;

        /// <summary>바깥 효과가 나간 횟수. 거부 장면에서는 0 이어야 한다.</summary>
        public int Calls;

        /// <summary>
        /// 그래프 표면 수·프레임 hard 여부·Graph 연결 여부로 장면을 만든다. 나머지 필드는 cpppj 장면과 같다.
        /// </summary>
        /// <param name="surfaces">Graph 0번 레코드의 signed WORD 표면 수.</param>
        /// <param name="hard">현재 프레임(1번)이 hard 인지. 거짓이면 0번 보통 프레임을 쓴다.</param>
        /// <param name="graphEnabled">Graph 를 분배기에 연결할지. 거짓이면 표면 수 0 으로 본다.</param>
        public Scene(short surfaces, bool hard, bool graphEnabled = true)
        {
            Pool = new RawSidPool(32768, true);
            // 타입 번호 전체에 빈 레코드와 빈 프레임 목록을 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
                Frames[i] = [];
            }
            Types[BridgeType].Flags1 = 0x802;
            Types[BridgeType].Flags2 = 4;
            Types[BridgeType].FootX = 1;
            Types[BridgeType].FootY = 1;
            Frames[BridgeType] = [new FrameCode('K', 'P', 1, FrameCodeFlags.None), new FrameCode('K', 'P', 2, FrameCodeFlags.Hard)];
            Records[0] = new GraphRecord { Surfaces = surfaces, InUse = 1, Reserved = 23 };
            Unpop = new RawUnpop(Pool, Hash, Spots);
            Base = new RawSquidDestroy(Pool, Unpop, Types);
            Neighbors = new RawSquidNeighbors(Pool, Hash, Spots, Types, Frames);
            Graph = new RawGraph(Pool, Hash, Spots, Types, Records, null, Frames);
            Dispatch = new RawSquidDestroyDispatch(Base, Neighbors, () => Mode, graphEnabled ? Graph : null);
            var factory = new RawSquidFactory(Pool, Types);
            Source = factory.Create(BridgeType);
            Span<byte> raw = Pool.AllocatedBytes(Source);
            Write(raw, 0, BridgeVtable, 4);
            Write(raw, 36, hard ? 1U : 0U, 4);
            raw[30] = 0;
            Hooks = new SquidDestroyHooks
            {
                Emit = OnEffect,
                Selected = () => 0,
            };
        }

        /// <summary>
        /// 공통 삭제가 넘긴 플래그를 보존하고 pre/post 두 깊이의 완료 계약을 수행한다.
        /// client 비트가 켜진 0x1234 라서 전송 효과는 나오지 않는다.
        /// </summary>
        /// <param name="effect">나간 삭제 효과.</param>
        private void OnEffect(SquidDestroyEvent effect)
        {
            Calls++;
            switch (effect.Effect)
            {
                case SquidDestroyEffect.PreDestroy:
                    Assert.Equal(0x1234u, effect.Flags);
                    Base.CompletePreDestroy();
                    break;
                case SquidDestroyEffect.PostDestroy:
                    Base.CompletePostDestroy();
                    break;
                default:
                    Assert.Equal(SquidDestroyEffect.Transmit, effect.Effect);
                    break;
            }
        }

        /// <summary>
        /// 한 번의 판정 결과를 대조한다. 참이면 실제 반납·두 콜백, 거짓이면 슬롯·풀·콜백의 완전 보존을 확인한다.
        /// 두 경우 모두 Graph 표·flood 스택과 공통 깊이가 그대로여야 한다.
        /// </summary>
        /// <param name="expected">분배기가 돌려줘야 하는 값.</param>
        public void Check(bool expected)
        {
            string before = Convert.ToHexStringLower(Pool.Bytes.ToArray());
            uint freeBefore = Pool.FreeCount;
            uint[] stack = [.. Graph.FloodStack];
            Assert.Equal(expected, Dispatch.Destroy(Source, 0x1234, Hooks));
            if (expected)
            {
                Assert.NotEqual(0, Pool.Slot(Source)[11] & FreeBit);
                Assert.Equal(freeBefore + 1, Pool.FreeCount);
                Assert.Equal(2, Calls);
            }
            else
            {
                Assert.Equal(before, Convert.ToHexStringLower(Pool.Bytes.ToArray()));
                Assert.Equal(freeBefore, Pool.FreeCount);
                Assert.Equal(0, Calls);
            }
            Assert.Equal(Records[0].Surfaces, Graph.Records[0].Surfaces);
            Assert.Equal(Records[0].Reserved, Graph.Records[0].Reserved);
            Assert.Equal(stack, Graph.FloodStack);
            Assert.Equal(0u, Base.PreDepth);
            Assert.Equal(0u, Base.PostDepth);
        }

        /// <summary>슬롯에 값을 리틀 엔디언으로 쓴다.</summary>
        /// <param name="bytes">대상 슬롯 바이트.</param>
        /// <param name="offset">쓰기 시작 위치.</param>
        /// <param name="value">쓸 값.</param>
        /// <param name="width">쓰는 바이트 수.</param>
        public static void Write(Span<byte> bytes, int offset, uint value, int width)
        {
            // 낮은 바이트부터 폭만큼 쓴다
            for (int i = 0; i < width; i++)
            {
                bytes[offset + i] = (byte)(value >> (8 * i));
            }
        }
    }

    /// <summary>4/5 경계·signed 음수·hard/보통·debugKeep·Graph 비활성을 각각 판정한다.</summary>
    [Fact]
    public void BridgeThresholdHardDebugAndDisabledGraph()
    {
        new Scene(4, true).Check(true);
        new Scene(5, true).Check(false);
        new Scene(-1, true).Check(true);
        new Scene(5, false).Check(true);
        var debug = new Scene(5, false);
        debug.Mode = debug.Mode with { DebugKeep = true };
        debug.Check(false);
        new Scene(5, true, false).Check(true);
    }

    /// <summary>editor·권한 없음·abstract·매립은 무효 Graph·프레임 조회를 모두 건너뛴다.</summary>
    [Fact]
    public void ShortCircuitsGraphAndFrameReads()
    {
        // 네 종류의 첫 조건 실패를 각각 독립 장면으로 검사한다
        for (int gate = 0; gate < 4; gate++)
        {
            var scene = new Scene(5, true);
            scene.Pool.AllocatedBytes(scene.Source)[30] = RawGraph.Invalid;
            Scene.Write(scene.Pool.AllocatedBytes(scene.Source), 36, 99, 4);
            if (gate == 0)
            {
                scene.Mode = scene.Mode with { Editor = true };
            }
            else if (gate == 1)
            {
                scene.Mode = scene.Mode with { Authority = false };
            }
            else
            {
                scene.Pool.AllocatedBytes(scene.Source)[40] |= (byte)(gate == 2 ? 1 : 8);
            }
            scene.Check(true);
        }
        var small = new Scene(4, true);
        Scene.Write(small.Pool.AllocatedBytes(small.Source), 36, 99, 4);
        small.Check(true);
    }

    /// <summary>살아 있는 raw 가상 표·프레임을 매번 읽고, 공통 본체 직접 호출과 구별한다.</summary>
    [Fact]
    public void ReadsLiveVtableFrameAndKeepsDirectBaseCall()
    {
        var frame = new Scene(5, true);
        frame.Check(false);
        Scene.Write(frame.Pool.AllocatedBytes(frame.Source), 36, 0, 4);
        frame.Check(true);

        var ordinary = new Scene(5, true);
        Scene.Write(ordinary.Pool.AllocatedBytes(ordinary.Source), 0, OriginalConstructors.BaseVtable, 4);
        ordinary.Check(true);

        var direct = new Scene(5, true);
        direct.Base.Destroy(direct.Source, 0x1234, direct.Hooks);
        Assert.NotEqual(0, direct.Pool.Slot(direct.Source)[11] & FreeBit);
    }

    /// <summary>무효 Graph·범위 밖 Graph·범위 밖 프레임은 원본 접근 오류 대신 쓰기 전 예외로 남긴다.</summary>
    [Fact]
    public void InvalidGraphAndFramePreserveRawState()
    {
        var scene = new Scene(5, true);
        scene.Pool.AllocatedBytes(scene.Source)[30] = RawGraph.Invalid;
        string before = Convert.ToHexStringLower(scene.Pool.Bytes.ToArray());
        Assert.Throws<InvalidOperationException>(() => scene.Dispatch.Destroy(scene.Source, 0x1234, scene.Hooks));
        Assert.Equal(before, Convert.ToHexStringLower(scene.Pool.Bytes.ToArray()));
        Assert.Equal(0, scene.Calls);

        scene.Pool.AllocatedBytes(scene.Source)[30] = 255;
        string badGraph = Convert.ToHexStringLower(scene.Pool.Bytes.ToArray());
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Dispatch.Destroy(scene.Source, 0x1234, scene.Hooks));
        Assert.Equal(badGraph, Convert.ToHexStringLower(scene.Pool.Bytes.ToArray()));
        Assert.Equal(0, scene.Calls);

        scene.Pool.AllocatedBytes(scene.Source)[30] = 0;
        Scene.Write(scene.Pool.AllocatedBytes(scene.Source), 36, 99, 4);
        string badFrame = Convert.ToHexStringLower(scene.Pool.Bytes.ToArray());
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Dispatch.Destroy(scene.Source, 0x1234, scene.Hooks));
        Assert.Equal(badFrame, Convert.ToHexStringLower(scene.Pool.Bytes.ToArray()));
        Assert.Equal(0, scene.Calls);
    }

    /// <summary>서로 다른 풀·다른 공간 해시의 Graph 연결은 가상 삭제를 실행하기 전에 거부한다.</summary>
    [Fact]
    public void RejectsMismatchedPoolAndGraphSpace()
    {
        var first = new Scene(5, true);
        var second = new Scene(5, true);
        Assert.Throws<ArgumentException>(() => new RawSquidDestroyDispatch(first.Base, second.Neighbors, first.Mode));
        Assert.Throws<ArgumentException>(() => new RawSquidDestroyDispatch(first.Base, first.Neighbors, first.Mode, second.Graph));
        // 같은 풀·같은 타입 표라도 공간 해시·점유 지도가 다르면 거부한다
        var otherGraph = new RawGraph(first.Pool, new RawSquidHash(), first.Spots, first.Types, first.Records, null, first.Frames);
        Assert.Throws<ArgumentException>(() => new RawSquidDestroyDispatch(first.Base, first.Neighbors, first.Mode, otherGraph));
    }

    /// <summary>
    /// 자산 공통 분배(<see cref="RawSquidFormDispatch"/>)의 재정의 경계로 그대로 꽂히는지 확인한다.
    /// 거부되면 Form 분배도 거짓을 보존하고 아무 효과를 내지 않아야 한다.
    /// </summary>
    [Fact]
    public void ComposesWithFormDispatchAsAssetOverride()
    {
        var scene = new Scene(5, true);
        var effects = new List<SquidDestroyEffect>();
        var assets = new SquidDestroyHooks
        {
            Emit = effect =>
            {
                effects.Add(effect.Effect);
                if (effect.Effect == SquidDestroyEffect.PreDestroy)
                {
                    scene.Base.CompletePreDestroy();
                }
                else if (effect.Effect == SquidDestroyEffect.PostDestroy)
                {
                    scene.Base.CompletePostDestroy();
                }
            },
            Selected = () => 0,
        };
        var forms = new RawSquidFormDispatch(scene.Pool, scene.Base, assets, scene.Dispatch.Destroy);
        Assert.False(forms.Destroy(scene.Source, 0x1234));
        Assert.Empty(effects);
        Assert.Equal(0, scene.Pool.Slot(scene.Source)[11] & FreeBit);
        // 현재 프레임을 보통으로 바꾸면 같은 장면이 공통 본체로 넘어가 실제로 반납된다
        Scene.Write(scene.Pool.AllocatedBytes(scene.Source), 36, 0, 4);
        Assert.True(forms.Destroy(scene.Source, 0x1234));
        Assert.Equal(new[] { SquidDestroyEffect.PreDestroy, SquidDestroyEffect.PostDestroy }, effects);
        Assert.NotEqual(0, scene.Pool.Slot(scene.Source)[11] & FreeBit);
        Assert.Equal(0u, scene.Base.PreDepth);
        Assert.Equal(0u, scene.Base.PostDepth);
    }

    /// <summary>현행 모드 묶음용 188개 타입 목록을 만든다. 2026-11-07 F2d 실행 연결 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> BoundTypes()
    {
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord { FootX = 1, FootY = 1 }).ToList();
        types[82].Flags1 = TypeFlag1.Surface;
        types[82].Flags2 = TypeFlag2.Bridge;
        return types;
    }

    /// <summary>현행 모드 묶음용 한 코드짜리 프레임 표를 만든다. 2026-11-07 F2d 실행 연결 이식에서 추가됐다.</summary>
    private static List<RiftTypeFrames> BoundFrames() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new RiftTypeFrames([])).ToList();

    /// <summary>현행 모드 묶음용 분배기를 만든다. 2026-11-07 F2d 실행 연결 이식에서 추가됐다.</summary>
    private static (RawSquidDestroyDispatch Dispatch, RawSquidDestroy Base, RawSidPool Pool, RawSquidHash Hash) DispatchScene(BridgeDecayMode mode)
    {
        var pool = new RawSidPool(32768);
        var hash = new RawSquidHash();
        var spots = new byte[65536];
        var types = BoundTypes();
        var frames = BoundFrames();
        frames[82] = new RiftTypeFrames([new RiftFrameCode((byte)'J', (byte)'P', 0, 0x40)]);
        var neighbors = new RawSquidNeighbors(pool, hash, spots, types, new RiftFrameCodeTables(frames));
        var unpop = new RawUnpop(pool, hash, spots);
        var baseDestroy = new RawSquidDestroy(pool, unpop, types);
        var records = new GraphRecord[SurfaceGraph.TableSize];
        records[0] = new GraphRecord { Surfaces = 9, InUse = 1 };
        var graph = new RawGraph(pool, hash, spots, types, records, new uint[SurfaceGraph.FloodSize], null, GraphRecovery.FullPool);
        return (new RawSquidDestroyDispatch(baseDestroy, neighbors, mode, graph), baseDestroy, pool, hash);
    }

    /// <summary>현행 모드 묶음용 다리 슬롯을 만든다. 2026-11-07 F2d 실행 연결 이식에서 추가됐다.</summary>
    private static int DispatchSlot(RawSidPool pool, RawSquidHash hash)
    {
        int sid = pool.Allocate();
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = 82;
        WriteSlot(raw, 0, 0x005034C8u, 4);
        WriteSlot(raw, 36, 0u, 4);
        WriteSlot(raw, 14, BitConverter.SingleToUInt32Bits(10f), 4);
        WriteSlot(raw, 18, BitConverter.SingleToUInt32Bits(10f), 4);
        WriteSlot(raw, 12, 3u * 8u, 2);
        raw[30] = 0;
        hash.MutableEntries(0)[10 * 256 + 10] = (ushort)sid;
        return sid;
    }

    /// <summary>현행 모드 묶음용 삭제 훅 묶음을 만든다. 2026-11-07 F2d 실행 연결 이식에서 추가됐다.</summary>
    private static SquidDestroyHooks DispatchHooks(RawSquidDestroy @base) => new()
    {
        Selected = () => 0,
        Emit = effect =>
        {
            if (effect.Effect == SquidDestroyEffect.PreDestroy)
            {
                @base.CompletePreDestroy();
            }
            else if (effect.Effect == SquidDestroyEffect.PostDestroy)
            {
                @base.CompletePostDestroy();
            }
        },
    };

    /// <summary>단단한 프레임은 편집기 없이 막히는지 검증한다. 2026-11-07 F2d 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void DestroyBlocksHardFrameWithoutEditor()
    {
        (RawSquidDestroyDispatch dispatch, RawSquidDestroy @base, RawSidPool pool, RawSquidHash hash) = DispatchScene(new BridgeDecayMode { Editor = false, Authority = true });
        int sid = DispatchSlot(pool, hash);
        Assert.False(dispatch.Destroy(sid, 0u, DispatchHooks(@base)));
        Assert.Equal(0, pool.Slot(sid)[RawSidPool.StateOffset] & 2);
    }

    /// <summary>묶은 편집기가 차단을 푸는지 검증한다. 2026-11-07 F2d 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundEditorUnblocksHardFrame()
    {
        (RawSquidDestroyDispatch dispatch, RawSquidDestroy @base, RawSidPool pool, RawSquidHash hash) = DispatchScene(new BridgeDecayMode { Editor = false, Authority = true });
        dispatch.BindCurrentModes(() => new RawAuthorityState(), () => 1u);
        int sid = DispatchSlot(pool, hash);
        Assert.True(dispatch.Destroy(sid, 0u, DispatchHooks(@base)));
        Assert.NotEqual(0, pool.Slot(sid)[RawSidPool.StateOffset] & 2);
    }

    /// <summary>묶은 비권한이 차단을 푸는지 검증한다. 2026-11-07 F2d 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundNoAuthorityUnblocksHardFrame()
    {
        (RawSquidDestroyDispatch dispatch, RawSquidDestroy @base, RawSidPool pool, RawSquidHash hash) = DispatchScene(new BridgeDecayMode { Editor = false, Authority = true });
        dispatch.BindCurrentModes(() => new RawAuthorityState { Boss = false });
        int sid = DispatchSlot(pool, hash);
        Assert.True(dispatch.Destroy(sid, 0u, DispatchHooks(@base)));
        Assert.NotEqual(0, pool.Slot(sid)[RawSidPool.StateOffset] & 2);
    }

    /// <summary>빠진 owner를 거부하는지 검증한다. 2026-11-07 F2d 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindCurrentModesRejectsNull()
    {
        (RawSquidDestroyDispatch dispatch, _, _, _) = DispatchScene(new BridgeDecayMode());
        Assert.Throws<ArgumentNullException>(() => dispatch.BindCurrentModes(null!));
    }

    /// <summary>슬롯 바이트를 리틀 엔디언으로 쓰는 검사 보조다. 2026-11-07 F2d 실행 연결 이식에서 추가됐다.</summary>
    private static void WriteSlot(Span<byte> raw, int offset, uint value, int width)
    {
        for (int i = 0; i < width; i++)
        {
            raw[offset + i] = (byte)(value >> (8 * i));
        }
    }
}
