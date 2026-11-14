using System.Buffers.Binary;
using Netstorm.Core.Display;
using Netstorm.Core.Original;

// 실제 Unpop/World 소비자의 도달 장치와 현재 상태/부분 변경을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>독립 PE 기대값과 구별한 실제 공장/공간/현재 장치 검사다.</summary>
public sealed partial class SquidUnpopParentCurrentTests
{
    /// <summary>선검사가 호출되면 실패하고 실제 늦은 Update만 관측하는 외부 표시 장치다.</summary>
    private sealed class DisplayDevice : IRawUnpopDisplay
    {
        /// <summary>실제 늦은 표시 소비 위치의 효과다.</summary>
        public Action? Updated { get; set; }
        /// <summary>실제 Update 호출 수다.</summary>
        public int Updates { get; private set; }
        /// <summary>공통 Unpop이 선검사로 우회하지 않아야 한다.</summary>
        public void Validate(ReadOnlySpan<byte> slot, uint flags) => throw new InvalidOperationException("원문에는 없는 표시 선검사");
        /// <summary>선택된 실제 표시 효과를 호출 끝까지 보유한다.</summary>
        public void Update(ReadOnlySpan<byte> slot, uint flags) { Updates++; Updated?.Invoke(); }
    }

    /// <summary>같은 풀의 실제 미니맵 소비에 외부 입력 효과를 제공한다.</summary>
    private sealed class QueueDevice : IRawMinimapQueue
    {
        /// <summary>같은 물리 풀이다.</summary>
        public RawSidPool Pool { get; }
        /// <summary>미니맵이 관측하는 현재 효과다.</summary>
        private readonly Action<int> _enqueue;
        /// <summary>풀과 실제 입력 효과를 결합한다.</summary>
        public QueueDevice(RawSidPool pool, Action<int> enqueue) { Pool = pool; _enqueue = enqueue; }
        /// <summary>도달한 현재 소비 효과를 실행한다.</summary>
        public void Enqueue(int sid) => _enqueue(sid);
    }

    /// <summary>실제 공장으로 만든 기본 자산과 같은 공간/현재 TYPE 표다.</summary>
    private sealed class DeviceScene
    {
        /// <summary>실제 물리 풀이다.</summary>
        public RawSidPool Pool { get; } = new(24000);
        /// <summary>동일한 네 공간 해시다.</summary>
        public RawSquidHash Hash { get; } = new();
        /// <summary>실제 전체 점유 BYTE 지도다.</summary>
        public byte[] Spots { get; } = new byte[65536];
        /// <summary>현재 TYPE의 같은 물리 칸들이다.</summary>
        public OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>동일한 실제 공간 해제다.</summary>
        public RawUnpop Unpop { get; }
        /// <summary>실제 공장이 할당/초기화한 자산이다.</summary>
        public int Sid { get; }
        /// <summary>명시 공간 등록 입력을 공급하며 Pop 전체를 대신 실행했다고 세지 않는다.</summary>
        public DeviceScene(uint genus = 1, int footprint = 1, IRawUnpopDisplay? display = null)
        {
            Types[74].Flags2 = genus; Types[74].FootX = Types[74].FootY = footprint;
            Sid = new RawSquidFactory(Pool, Types).Create(74); Pool.RawSlot(Sid)[11] = 0;
            BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(Sid)[14..], BitConverter.SingleToUInt32Bits(15f));
            BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(Sid)[18..], BitConverter.SingleToUInt32Bits(20f));
            Hash.Bucket(0, 15, 20) = (ushort)Sid; Array.Fill(Spots, (byte)0xff); Unpop = new(Pool, Hash, Spots, display);
        }
    }

    /// <summary>현재 미지원 표시를 선택하기 전의 void/점유/해시 쓰기를 실제 공간 부모가 유지한다.</summary>
    [Fact]
    public void ActualUnsupportedDisplayKeepsEarlierVoidSpotsAndHashRemoval()
    {
        var s = new DeviceScene(); BinaryPrimitives.WriteUInt32LittleEndian(s.Pool.RawSlot(s.Sid), 0xdeadbeef);
        Assert.Throws<InvalidOperationException>(() => s.Unpop.Unpop(s.Sid, s.Types[74]));
        Assert.Equal(4, s.Pool.Slot(s.Sid)[11]); Assert.Equal(0xfe, s.Spots[20 * 256 + 15]); Assert.Equal(0, s.Hash.Bucket(0, 15, 20));
    }

    /// <summary>실제 표시의 구성 선검사를 부르지 않고 공간 제거 뒤 Update를 실행한다.</summary>
    [Fact]
    public void ActualDisplayRunsOnlyAfterConsumedSpaceChanges()
    {
        var display = new DisplayDevice(); var s = new DeviceScene(display: display);
        display.Updated = () => { Assert.Equal(4, s.Pool.Slot(s.Sid)[11]); Assert.Equal(0xfe, s.Spots[20 * 256 + 15]); Assert.Equal(0, s.Hash.Bucket(0, 15, 20)); };
        s.Unpop.Unpop(s.Sid, s.Types[74]); Assert.Equal(1, display.Updates);
    }

    /// <summary>실제 표면 중간 옆면을 소비할 때만 누락을 실패시키고 그 앞의 점유와 void를 보존한다.</summary>
    [Fact]
    public void ActualMissingSelectedSideKeepsEarlierCellsAndVoid()
    {
        var s = new DeviceScene(0x4010, 3); s.Hash.Cell(0, 13, 19) = 70;
        Assert.Throws<InvalidOperationException>(() => s.Unpop.Unpop(s.Sid, s.Types[74]));
        Assert.Equal(4, s.Pool.Slot(s.Sid)[11]); Assert.Equal(s.Sid, s.Hash.Bucket(0, 15, 20));
        Assert.Equal(0xef, s.Spots[18 * 256 + 13]); Assert.Equal(0xef, s.Spots[19 * 256 + 13]); Assert.Equal(0xff, s.Spots[19 * 256 + 14]);
    }

    /// <summary>실제 현재 표 재선택/처음 물리 칸·진단 함수 사본/저장 BYTE와 현재 checking을 구별한다.</summary>
    [Fact]
    public void ActualCurrentTypesAndPinnedDiagnosticKeepSavedByteAndPhysicalType()
    {
        var s = new DeviceScene(6, 3); uint checking = 1; int oldCalls = 0, newCalls = 0;
        var next = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); next[71].Flags2 = 1; next[71].FootX = next[71].FootY = 2;
        next[74].FootX = next[74].FootY = 1; Array.Fill(s.Spots, (byte)0x0e);
        s.Unpop.BindDiagnostics(new() { Types = s.Types, Checking = new RawCheckingFlag(() => checking), Assertion = (expression, file, line) =>
        {
            Assert.Equal(0x617u, line); Assert.Equal(71, s.Pool.Slot(s.Sid)[10]); oldCalls++; checking = 0; Array.Fill(s.Spots, (byte)0xff);
        } });
        s.Unpop.SetMinimapQueue(new QueueDevice(s.Pool, sid =>
        {
            Assert.Equal(s.Sid, sid); s.Types[74].FootX = s.Types[74].FootY = 2; s.Pool.RawSlot(s.Sid)[10] = 71;
            s.Unpop.BindDiagnostics(new() { Types = next, Checking = new RawCheckingFlag(() => checking), Assertion = (_, _, _) => newCalls++ });
        }));
        s.Unpop.Unpop(s.Sid, new OriginalTypeRecord { FootX = 100, FootY = 100 });
        Assert.Equal(1, oldCalls); Assert.Equal(0, newCalls); Assert.Equal(0u, checking);
        Assert.Equal(0x0e, s.Spots[19 * 256 + 14]); Assert.Equal(0xfe, s.Spots[19 * 256 + 15]); Assert.Equal(0xfe, s.Spots[20 * 256 + 14]); Assert.Equal(0xff, s.Spots[19 * 256 + 13]);
    }

    /// <summary>호출 시작의 종속 함수는 유지하고 현재 contained 분기는 해시/표시를 실행하지 않는다.</summary>
    [Fact]
    public void ActualContainedFunctionIsPinnedUntilParentReturns()
    {
        var display = new DisplayDevice(); var s = new DeviceScene(6, 2, display); int oldCalls = 0, newCalls = 0;
        s.Unpop.BindContainedUnpop(sid => { Assert.Equal(s.Sid, sid); oldCalls++; });
        s.Unpop.SetMinimapQueue(new QueueDevice(s.Pool, _ => { s.Pool.RawSlot(s.Sid)[11] |= 8; s.Unpop.BindContainedUnpop(_ => newCalls++); }));
        s.Unpop.Unpop(s.Sid, s.Types[74]); Assert.Equal(1, oldCalls); Assert.Equal(0, newCalls); Assert.Equal(0, display.Updates);
        Assert.Equal(12, s.Pool.Slot(s.Sid)[11]); Assert.Equal(s.Sid, s.Hash.Bucket(0, 15, 20));
    }

    /// <summary>실제 World의 공간 해제가 같은 Client의 현재 복귀 진단에 연결된다.</summary>
    [Fact]
    public void ActualWorldUnpopUsesCurrentClientDiagnosticBeforeFreeReturn()
    {
        var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToList(); var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToList();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes); int assertions = 0;
        storage.BindDisplayDiagnostics(new StringBox(), (expression, file, line) => { Assert.Equal("!isForm()", expression); Assert.Equal(@"\Ns\O\Squid.cpp", file); Assert.Equal(0x5d8u, line); assertions++; });
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97, storage, frames); RawWorldCore core = world.Core(); core.Pool.RawSlot(16000)[10] = 0; core.Pool.RawSlot(16000)[11] = 1;
        core.Unpop.Unpop(16000, types[74]); Assert.Equal(1, assertions); Assert.Equal(1, core.Pool.Slot(16000)[11]);
    }
}
