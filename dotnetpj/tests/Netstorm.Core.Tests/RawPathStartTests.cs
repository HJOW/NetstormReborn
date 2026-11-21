using System.Buffers.Binary;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using static Netstorm.Core.Tests.RawPathProcessTests;

// 같은 실제 Host/공간에서 시작 부모의 독립 원문 관측을 재생한다.
namespace Netstorm.Core.Tests;

/// <summary>PATCH256입력의 실제 생성/Init/BFS/Prepare/부착/사제 통지를 검사한다.</summary>
public sealed class RawPathStartTests
{
    /// <summary>원본 fixture의 입력 raw 장면이며 결과/분기 기대값을 계산하지 않는다.</summary>
    private sealed class StartScene
    {
        /// <summary>실제 서버/클라이언트 Form을 함께 확보하는 풀이다.</summary>
        internal readonly RawSidPool Pool = new(32768);
        /// <summary>일반 조회/표면 탐색의 같은 해시다.</summary>
        internal readonly RawSquidHash Hash = new();
        /// <summary>현재TYPE와 생성자 입력이다.</summary>
        internal readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = i is 1 or 2 or 3 or 6 ? OriginalConstructors.Address(OriginalEdition.Patch1078, i) : 0 }).ToArray();
        /// <summary>실제 표면 접합 입력 프레임이다.</summary>
        internal readonly RiftTypeFrames[] Frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        /// <summary>이번 시작 검사에서 실행하지 않는 걷기 표시 자료다.</summary>
        internal readonly SquidDisplayShape[] Shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        /// <summary>표면 spot6과 빈칸의 같은 지도다.</summary>
        internal readonly byte[] Spots = new byte[65536];
        /// <summary>Prepare/Host와 같은 시계 및 현재 boss다.</summary>
        internal readonly SquidProcessState Clock = new();
        /// <summary>관측하는 외부 요청의 원문 순서다.</summary>
        internal readonly List<string> Effects = [];
        /// <summary>원본처럼 동일 raw 슬롯과 hash 머리 입력을 준비한다.</summary>
        internal StartScene(uint type, uint profile, bool boss)
        {
            // 원문 입력의5..53을 동일 client 순서로 확보한다.
            for (int sid = 5; sid <= 53; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            Clock.Now = 123.125; Clock.Boss = boss; Types[83].Flags1 = OriginalTypeRecord.Surface; Types[83].Flags2 = 2; Frames[83] = new([new((byte)'A', (byte)'A', 1, 0)]); int next = 6;
            // 같은 여섯 행의 표면을 구성한다.
            for (int y = 9; y < 15; y++)
                // 같은 행의 여섯 표면은 각각 별도 실제SID다.
                for (int x = 9; x < 15; x++) { int sid = next++; Insert(sid, 83, x, y, 0); Pool.RawSlot(sid)[30] = 3; Spots[y * 256 + x] = 6; }
            Insert(50, 122, 13.75f, 13.25f, 1); Insert(51, 122, 13.75f, 13.25f, 1, true); Insert(52, 122, 13.5f, 13.5f, 1);
            if (profile == 6) { Insert(53, 150, 10.75f, 10.25f, 1); Types[150].Flags2 = 0x40000; }
            Span<byte> raw = Pool.RawSlot(5); raw.Clear(); raw[10] = (byte)type; raw[11] = profile == 4 ? (byte)4 : (byte)0; Put(raw, 0, 0x501dd0); Put(raw, 14, BitConverter.SingleToUInt32Bits(9.2f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(9.3f)); raw[34] = 2; raw[33] = 1;
            if (profile != 4) { BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], Hash.Bucket(1, 9.2f, 9.3f)); Hash.Bucket(1, 9.2f, 9.3f) = 5; } Types[type].FootX = Types[type].FootY = 1;
            if (profile is 2 or 6 or 7) { Hash.Cell(0, 10, 10) = 0; Hash.Cell(0, 9, 9) = 0; }
        }
        /// <summary>입력 슬롯을 현재 공간 머리에 연결한다.</summary>
        private void Insert(int sid, int type, float x, float y, int level, bool buried = false)
        {
            Span<byte> raw = Pool.RawSlot(sid); raw.Clear(); raw[10] = (byte)type; raw[40] = buried ? (byte)8 : (byte)0; Put(raw, 14, BitConverter.SingleToUInt32Bits(x)); Put(raw, 18, BitConverter.SingleToUInt32Bits(y)); Types[type].FootX = Types[type].FootY = 1;
            BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], Hash.Bucket(level, x, y)); Hash.Bucket(level, x, y) = (ushort)sid;
        }
    }
    /// <summary>관찰되지 않는 실행 경계는 명시 실패하며 실제 공간/정리 요청만 공급한다.</summary>
    private static RawPathProcessHooks ProcessHooks(StartScene scene, RawPop pop, RawUnpop unpop, RawPathSearch search, bool old) => RawPathSearch.MakeHooks(scene.Pool, search, new()
    {
        Unpop = (sid, flags) => unpop.Unpop(sid, scene.Types[scene.Pool.Slot(sid)[10]], flags),
        Pop = (sid, point, flags) => { if (!old) scene.Effects.Add($"P:{BitConverter.SingleToUInt32Bits(point.X)},{BitConverter.SingleToUInt32Bits(point.Y)},{flags}"); Assert.Equal(RawPopResult.Registered, pop.Pop(sid, scene.Types[scene.Pool.Slot(sid)[10]], 1, 1, point.X, point.Y, flags)); },
        Repop = (sid, flags) => Assert.Equal(RawPopResult.Registered, pop.Pop(sid, scene.Types[scene.Pool.Slot(sid)[10]], 1, 1, BitConverter.UInt32BitsToSingle(Get(scene.Pool.Slot(sid), 14)), BitConverter.UInt32BitsToSingle(Get(scene.Pool.Slot(sid), 18)), flags)),
        Halt = _ => throw new InvalidOperationException("시작 대조의 Halt 미실행"), Arrival = (_, _, _) => throw new InvalidOperationException("시작 대조의 도착 미실행"), SoundId = _ => throw new InvalidOperationException("시작 대조의 발소리 미실행"), StepSound = (_, _, _, _) => throw new InvalidOperationException("시작 대조의 발소리 미실행"), ClearBlocked = (_, _, _, _) => throw new InvalidOperationException("시작 대조의 도착 겹침 미실행"),
        Cleanup = (_, parent, flags) => { Assert.Equal(5, parent); Assert.Equal(old ? 0x10u : 0u, flags); if (old) scene.Effects.Add("K"); },
    });
    /// <summary>원본 PATCH256개의 전체 시작 결과와 실제 Form/전송/프레임/남은 사건이 같다.</summary>
    [Fact]
    public void NativePatchStart256UsesActualBfsHostTransmissionAndFrame()
    {
        string[][] rows = Rows("pathstart-x86.tsv").Where(r => r[0] == "originals").ToArray(); Assert.Equal(256, rows.Length);
        // 실제 CD Host 하위가 미완성이므로 그256입력을 검증 완료로 세지 않는다.
        foreach (string[] row in rows)
        {
            uint[] input = row[1].Split(',').Select(U).ToArray(); uint type = input[0], profile = input[1], pending = input[5]; bool boss = input[2] != 0, legacy = input[3] != 0;
            var scene = new StartScene(type, profile, boss); var pop = new RawPop(scene.Pool, scene.Hash, scene.Spots); var unpop = new RawUnpop(scene.Pool, scene.Hash, scene.Spots); var destroy = new RawSquidDestroy(scene.Pool, unpop, scene.Types); var kernel = new Kernel(32);
            var host = new SquidProcessHost(scene.Pool, scene.Types, kernel, destroy, scene.Clock, asset: new() { Selected = () => 0, Emit = ev => Assert.Equal(SquidDestroyEffect.Transmit, ev.Effect) });
            var search = new RawPathSearch(scene.Pool, scene.Hash, scene.Spots, scene.Types, scene.Frames, new()); var processState = new RawPathProcessState(scene.Clock); var buffer = Enumerable.Repeat((byte)0xa5, 2048).ToArray(); int sent = 0, sentForm = 0, updates = 0, metadataReads = 0;
            host.SetFormTransmit(RawPathTransmission.Make(scene.Pool, scene.Types, new(), buffer, length => { Assert.True(length > 0); Assert.Equal(5, BinaryPrimitives.ReadUInt16LittleEndian(buffer.AsSpan(6))); sentForm = BinaryPrimitives.ReadUInt16LittleEndian(buffer.AsSpan(4)); Assert.True(sentForm >= RawSidPool.ServerFirst); sent++; }));
            if (legacy) { var old = new RawPathProcess(scene.Pool, scene.Hash, scene.Types, scene.Frames, scene.Shapes, scene.Spots, processState, ProcessHooks(scene, pop, unpop, search, true), type == 158); old.Runtime.SetTargets(0x87654321, 0xfedcba98); Assert.NotNull(RawPathProcess.Attach(host, old, 5, 0x10)); }
            // 두 사건을 반복 등록해 시작 통지의 반복 검색/전체 삭제를 검사한다.
            foreach (uint ev in new uint[] { 0x25c, 0x25d })
                // 입력 pending만큼 실제 같은 Host에 등록한다.
                for (uint i = 0; i < pending; i++) Assert.NotNull(host.AddRegular(5, ev, 1));
            var frame = new SquidFrame(scene.Pool, scene.Types, new() { FrameSize = (_, _) => [1, 1], Update = (sid, flags) => { Assert.Equal(5, sid); Assert.Equal(0u, flags); updates++; }, Unpop = (sid, flags) => unpop.Unpop(sid, scene.Types[scene.Pool.Slot(sid)[10]], flags), Pop = (sid, x, y, flags) => Assert.Equal(RawPopResult.Registered, pop.Pop(sid, scene.Types[scene.Pool.Slot(sid)[10]], 1, 1, x, y, flags)) });
            var lookup = new RawContainedLookup(scene.Pool, (form, flags) => host.Kill(host.FromForm(form)!, flags)); var state = new RawPathStartState(scene.Clock) { SnapPriestDestination = profile == 1, Retrying = profile == 7 }; var selection = new RawMovementProcessTypes();
            var start = new RawPathStart(scene.Pool, scene.Hash, scene.Types, host, lookup, frame, state, new()
            {
                ProcessType = parentType => RawMovementProcessTypes.Select(OriginalEdition.Patch1078, scene.Types, selection, parentType),
                Create = priest => { scene.Effects.Add("C"); return new(scene.Pool, scene.Hash, scene.Types, scene.Frames, scene.Shapes, scene.Spots, processState, ProcessHooks(scene, pop, unpop, search, false), priest); },
                Metadata = parent => { Assert.Equal(5, parent); metadataReads++; if (metadataReads == 2) { Assert.Null(host.FindEvent(5, 0x25c)); Assert.Null(host.FindEvent(5, 0x25d)); } return new(.7f, 90, 7); },
                ClearBlocked = (point, parentType, owner) => scene.Effects.Add($"B:{BitConverter.SingleToUInt32Bits(point.X)},{BitConverter.SingleToUInt32Bits(point.Y)},{parentType},{owner}"),
            });
            RawPathPoint destination = profile == 3 ? new(-1.25f, 400.5f) : profile == 5 ? new(9.2f, 9.3f) : new(13.2f, 13.3f); RawPathProcess? result = start.Start(new(5, destination, input[4], 0x12345678, 0x87654321, 0xa5fedcba, 0x13579bdf));
            Assert.Equal(U(row[2]) != 0, result is not null); string[] expectedEffects = row[3].Split(';').Where(e => e is "C" or "K" || e.StartsWith("P:", StringComparison.Ordinal) || e.StartsWith("B:", StringComparison.Ordinal)).ToArray(); Assert.Equal(expectedEffects, scene.Effects);
            if (result is not null)
            {
                Assert.Equal(row[5], RawPathSearchTests.SearchScene.Points(result.Runtime)); RawPathLayout layout = result.Runtime.Layout;
                Assert.Equal(row[4], string.Join(',', new[] { layout.TargetA, layout.TargetB, layout.Helper, 0x186e4, 0x186e8, 0x186dc, 0x186e0 }.Select(offset => Get(result.Runtime.Bytes.Span, offset))));
                Assert.Equal(result.Form(), BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(5)[6..])); Assert.Equal(5, BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(result.Form())[16..])); Assert.Equal((ulong)result.Form(), result.Runtime.ReadBits(4)); Assert.Equal(5ul, result.Runtime.ReadBits(8));
                if (type == 158) { Assert.Equal(1, sent); Assert.Equal(sentForm, result.Form()); Assert.Equal(2, updates); Assert.Null(host.FindEvent(5, 0x25c)); Assert.Null(host.FindEvent(5, 0x25d)); } else { Assert.Equal(0, sent); Assert.True(result.Form() < RawSidPool.ServerFirst); Assert.Equal(0, updates); }
            }
            else { Assert.Equal(0, sent); Assert.Equal(0, updates); Assert.Equal(0, metadataReads); }
            Assert.Equal(U(row[6]), Get(scene.Pool.Slot(5), 36)); int first = 0, second = 0, current = BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(5)[6..]);
            // 현재 종속 체인에 남은 실제 Regular 사건만 센다.
            while (current != 0) { if (host.FromForm(current) is RegularProcess regular) { if (regular.Event() == 0x25c) first++; if (regular.Event() == 0x25d) second++; } current = BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(current)[4..]); }
            Assert.Equal(row[7], $"{first},{second},{(state.Retrying ? 1 : 0)}"); kernel.Clear();
        }
    }
}
