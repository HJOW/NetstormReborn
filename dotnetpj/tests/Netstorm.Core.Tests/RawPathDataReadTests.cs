using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 실제 Pool/Runtime과 변경 가능한 패킷으로 Source 수신 순서를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>선행 저장과 선택된 장치/현재 재조회를 독립 writer 관측 범위와 구별한다.</summary>
public sealed partial class RawPathDataTests
{
    /// <summary>실제 슬롯을 할당하고 수신이 읽는 부모TYPE/공간 좌표만 입력한다.</summary>
    private static (RawSidPool Pool, OriginalTypeRecord[] Types, int Parent) Scene(uint genus = 0)
    {
        var pool = new RawSidPool(32768); int parent = pool.Allocate(); Span<byte> raw = pool.AllocatedBytes(parent); raw[10] = 70; raw[11] = 0;
        BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(14, 4), 100); BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(18, 4), 100);
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); types[70].Flags2 = genus; return (pool, types, parent);
    }
    /// <summary>고정 머리와 선택 payload만 만든다. 선언 길이/메시지는 수신 본체의 추가 guard로 쓰지 않는다.</summary>
    private static byte[] Packet(int parent, ushort nodes, params byte[] payload)
    {
        byte[] packet = new byte[24 + payload.Length]; packet[0] = 0x8f; packet[1] = 0xff; Word(packet, 2, 1); Word(packet, 6, (ushort)parent); Word(packet, 11, 0x1234); Word(packet, 13, nodes); packet[15] = 10; packet[16] = 20; payload.CopyTo(packet, 24); return packet;
    }
    /// <summary>선택되는 실제 콜백의 간단한 입력이며 trace를 필요할 때 수집한다.</summary>
    private static RawPathDataReadHooks ReadHooks(List<string>? trace = null) => new()
    {
        Metadata = _ => (2, 90), Now = () => 0, ClearAnimation = _ => { }, Trace = text => trace?.Add(text),
    };
    /// <summary>기존 count/current/angle을 보존하고 direction0와 시간/꼬리만 원문 순서대로 갱신한다.</summary>
    [Fact]
    public void ReadCompressedAppendsAndOrdersMetadataClockAndClear()
    {
        var scene = Scene(); var runtime = new RawPathRuntime(OriginalEdition.Patch1078); runtime.SetPath([new(1, 1)], [7], [9]); runtime.RestoreStep(1, 30, true); runtime.SetTargets(11, 22); runtime.SetCommandFields(9, 10, 11, default);
        byte[] packet = Packet(scene.Parent, 2, 0x12, 0x0c); Dword(packet, 17, unchecked((uint)-8)); var trace = new List<string>(); var order = new List<string>(); int reads = 0;
        new RawPathData(OriginalEdition.Patch1078).Read(runtime, packet, scene.Pool, scene.Types, new()
        {
            Trace = trace.Add, Metadata = parent => { Assert.Equal(scene.Parent, parent); order.Add("metadata"); return ++reads == 1 ? (123, 90) : (2, 777); },
            Now = () => { order.Add("now"); return 3; }, ClearAnimation = parent => { Assert.Equal(scene.Parent, parent); order.Add("clear"); },
        });
        Assert.Equal(5, runtime.Count); Assert.Equal(1, runtime.Current); Assert.Equal(30f, runtime.Angle); Assert.False(runtime.Finished); Assert.Equal(0u, runtime.TargetA); Assert.Equal(0u, runtime.TargetB); Assert.Equal(0x1234u, runtime.Helper);
        Assert.Equal(1f, runtime.Point(0).X); Assert.Equal(10f, runtime.Point(1).X); Assert.Equal(12f, runtime.Point(2).X); Assert.Equal(20f, runtime.Point(3).Y); Assert.Equal(21f, runtime.Point(4).Y);
        Assert.Equal(new[] { 2, 2, 2, 4, 4 }, Enumerable.Range(0, 5).Select(runtime.Direction)); Assert.Equal(-1.0, runtime.Time(0));
        Assert.Equal(new[] { "metadata", "metadata", "now", "clear" }, order); Assert.Equal(6, trace.Count); Assert.Equal("[0] (10,20) , 2\n", trace[0]); Assert.Equal("[4] (12,21) , 4\n", trace[^1]);
    }
    /// <summary>압축 count는 진입 값을 보존하되 run payload와 trace 함수는 콜백 뒤 현재 값을 쓴다.</summary>
    [Fact]
    public void ReadCompressedKeepsCountButReadsPayloadAndTraceAfterCallback()
    {
        var scene = Scene(); var runtime = new RawPathRuntime(OriginalEdition.Patch1078); byte[] packet = Packet(scene.Parent, 1, 0x0a); var trace = new List<string>(); RawPathDataReadHooks hooks = ReadHooks();
        hooks.Trace = text => { trace.Add("first:" + text); Word(packet, 13, 0); packet[24] = 0x0c; hooks.Trace = changed => trace.Add("next:" + changed); };
        new RawPathData(OriginalEdition.Patch1078).Read(runtime, packet, scene.Pool, scene.Types, hooks);
        Assert.Equal(4, runtime.Count); Assert.Equal(10f, runtime.Point(3).X); Assert.Equal(21f, runtime.Point(3).Y); Assert.Equal(2, runtime.Direction(0)); Assert.Equal(4, runtime.Direction(3));
        Assert.Contains("next:n[0] 4 | 1\n", trace); Assert.Equal(0, ReadWord(packet, 13));
    }
    /// <summary>직접 좌표의 홀수 진단은 초기 append 뒤이며 정상 복귀 뒤 현재 nodeCount를 사용한다.</summary>
    [Fact]
    public void ReadDirectOddDiagnosticSeesAppendThenChangesCurrentCount()
    {
        var scene = Scene(0x100000); var runtime = new RawPathRuntime(OriginalEdition.Patch1078); byte[] packet = Packet(scene.Parent, 1, 30, 40); int calls = 0; RawPathDataReadHooks hooks = ReadHooks(); hooks.Trace = null;
        hooks.Assertion = (expression, file, line) => { calls++; Assert.Equal(1, runtime.Count); Assert.Equal(0x1234u, runtime.Helper); Assert.Equal("(zwp->nodeCount & 1) == 0", expression); Assert.Equal(@"\Ns\O\Pathprocess.cpp", file); Assert.Equal(0x1cau, line); Word(packet, 13, 2); scene.Types[70].Flags2 = 0; };
        new RawPathData(OriginalEdition.Patch1078).Read(runtime, packet, scene.Pool, scene.Types, hooks);
        Assert.Equal(1, calls); Assert.Equal(2, runtime.Count); Assert.Equal(30f, runtime.Point(1).X); Assert.Equal(40f, runtime.Point(1).Y);
    }
    /// <summary>직접 좌표의 두번째 BYTE 부족은 선행 첫점/필드 저장을 되돌리지 않는다.</summary>
    [Fact]
    public void ReadMissingSelectedPayloadKeepsInitialPointAndFields()
    {
        var scene = Scene(0x100000); var runtime = new RawPathRuntime(OriginalEdition.Patch1078); runtime.SetTargets(11, 22); runtime.SetFinished(true); byte[] packet = Packet(scene.Parent, 2, 30);
        Assert.Throws<ArgumentOutOfRangeException>(() => new RawPathData(OriginalEdition.Patch1078).Read(runtime, packet, scene.Pool, scene.Types, new()));
        Assert.Equal(1, runtime.Count); Assert.Equal(10f, runtime.Point(0).X); Assert.Equal(20f, runtime.Point(0).Y); Assert.False(runtime.Finished); Assert.Equal(0u, runtime.TargetA); Assert.Equal(0u, runtime.TargetB); Assert.Equal(0x1234u, runtime.Helper);
    }
    /// <summary>signed 음수 압축 count도 원문의 count!=0 마지막 append를 보존한다.</summary>
    [Fact]
    public void ReadNegativeRunCountStillAppendsFinalPoint()
    {
        var scene = Scene(); var runtime = new RawPathRuntime(OriginalEdition.Patch1078); byte[] packet = Packet(scene.Parent, ushort.MaxValue); var trace = new List<string>();
        new RawPathData(OriginalEdition.Patch1078).Read(runtime, packet, scene.Pool, scene.Types, ReadHooks(trace)); Assert.Equal(2, runtime.Count); Assert.Equal(2, trace.Count); Assert.Equal(10f, runtime.Point(1).X); Assert.Equal(20f, runtime.Point(1).Y);
    }
    /// <summary>용량 진단 직전에 마지막 point/direction/time/count를 실제 저장한다.</summary>
    [Fact]
    public void ReadCapacityDiagnosticKeepsTheAlreadyWrittenLastSlot()
    {
        var scene = Scene(0x100000); var runtime = new RawPathRuntime(OriginalEdition.Patch1078); runtime.SetPath(new RawPathPoint[4999], new int[4999]); byte[] packet = Packet(scene.Parent, 0); int calls = 0;
        Assert.Throws<InvalidOperationException>(() => new RawPathData(OriginalEdition.Patch1078).Read(runtime, packet, scene.Pool, scene.Types, new()
        {
            Assertion = (expression, file, line) => { calls++; Assert.Equal(5000, runtime.Count); Assert.Equal("nVert < MAX_PATH_LENGTH", expression); Assert.Equal(@"c:\ns\o\path.h", file); Assert.Equal(0x16u, line); throw new InvalidOperationException("관측 중단"); },
        }));
        Assert.Equal(1, calls); Assert.Equal(5000, runtime.Count); Assert.Equal(10f, runtime.Point(4999).X); Assert.Equal(20f, runtime.Point(4999).Y); Assert.Equal(0.0, runtime.Time(4999));
    }
    /// <summary>의식 콜백 뒤 현재 부모 좌표/현재 stage/현재 count를 각각 재조회한다.</summary>
    [Fact]
    public void ReadStageCallbacksRequeryParentStageAndLastPointIndex()
    {
        var scene = Scene(0x100000); int target = scene.Pool.Allocate(); scene.Pool.AllocatedBytes(target)[11] = 0; var runtime = new RawPathRuntime(OriginalEdition.Patch1078); byte[] packet = Packet(scene.Parent, 0); Word(packet, 21, (ushort)target); packet[23] = 2; var stages = new List<uint>(); RawPathDataReadHooks hooks = ReadHooks();
        hooks.StagePoint = (destination, stage) =>
        {
            Assert.Equal(target, destination); stages.Add(stage);
            if (stages.Count == 1)
            {
                Assert.Equal(1u, stage); Span<byte> parent = scene.Pool.AllocatedBytes(scene.Parent); BinaryPrimitives.WriteSingleLittleEndian(parent.Slice(14, 4), 20); BinaryPrimitives.WriteSingleLittleEndian(parent.Slice(18, 4), 30);
                runtime.SetCommandFields(runtime.Helper, (uint)target, 4, default); return new(20, 30);
            }
            Assert.Equal(4u, stage); runtime.SetPath([runtime.Point(0), new(999, 999)], [0, 0]); return new(50, 60);
        };
        new RawPathData(OriginalEdition.Patch1078).Read(runtime, packet, scene.Pool, scene.Types, hooks);
        Assert.Equal(new uint[] { 1, 4 }, stages); Assert.Equal(2, runtime.Count); Assert.Equal(20f, runtime.Point(0).X); Assert.Equal(30f, runtime.Point(0).Y); Assert.Equal(50f, runtime.Point(1).X); Assert.Equal(60f, runtime.Point(1).Y);
    }
    /// <summary>머리/판본 계약 실패는 runtime의 선행 상태까지 건드리지 않는다.</summary>
    [Fact]
    public void ReadRejectsMissingPhysicalHeaderAndWrongEditionBeforeWrites()
    {
        var scene = Scene(); var runtime = new RawPathRuntime(OriginalEdition.Patch1078, heapFill: 0xa5); byte[] before = runtime.Bytes.ToArray(); var codec = new RawPathData(OriginalEdition.Patch1078);
        Assert.Throws<ArgumentOutOfRangeException>(() => codec.Read(runtime, new byte[23], scene.Pool, scene.Types, new())); Assert.Equal(before, runtime.Bytes.ToArray());
        Assert.Throws<ArgumentException>(() => new RawPathData(OriginalEdition.Cd1072).Read(runtime, Packet(scene.Parent, 0), scene.Pool, scene.Types, new())); Assert.Equal(before, runtime.Bytes.ToArray());
    }
    /// <summary>같은 장기 헤더의 BYTE/DWORD를 일반 World Reset이 다시 초기화하지 않는다.</summary>
    [Fact]
    public void CurrentHeaderHasBssDefaultsAndSurvivesGeneralWorldReset()
    {
        var aux = new RawGameAuxState(); RawPathDataHeader header = aux.PathPacketHeader; Assert.Equal(0x30, header.PacketId); Assert.Equal(1, header.PacketFlags); Assert.Equal(0u, header.PacketLength);
        header.PacketId = 0x31; header.PacketFlags = 0xff; header.PacketLength = 0xffff0000; aux.ResetForGeneralWorld(); Assert.Same(header, aux.PathPacketHeader); Assert.Equal(0x31, header.PacketId); Assert.Equal(0xff, header.PacketFlags); Assert.Equal(0xffff0000u, header.PacketLength);
    }
}
