using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using static Netstorm.Core.Tests.RawPathProcessTests;

// 합성 입력의 실제 프레임 몸체를 독립 x86 전체 이미지/효과와 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>워커/벌프/벌룬의 공유 전역과 부동소수점 저장을 검증한다.</summary>
public sealed class RawPathFrameNativeTests
{
    /// <summary>원문 zlib Adler32와 같은 전체 이미지 검산이다.</summary>
    private static uint Adler(ReadOnlySpan<byte> bytes)
    {
        uint a = 1, b = 0;
        // 미초기화 영역도 포함해 원문 전체 바이트를 소비한다.
        foreach (byte value in bytes) { a = (a + value) % 65521; b = (b + a) % 65521; }
        return (b << 16) | a;
    }
    /// <summary>명시된 SID50을 실제 할당하고 원문50BYTE를 준비한다.</summary>
    private static RawSidPool Pool(uint vtable, byte type, uint frame = 0)
    {
        var pool = new RawSidPool(32768, false);
        // 실제 프레임 쓰기가 허용되는 원문 부모 번호를 확보한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        Span<byte> raw = pool.AllocatedBytes(50); raw.Clear(); Put(raw, 0, vtable); raw[10] = type; Put(raw, 14, BitConverter.SingleToUInt32Bits(10)); Put(raw, 18, BitConverter.SingleToUInt32Bits(20)); Put(raw, 36, frame); return pool;
    }
    /// <summary>원문 메타 입력의 물리 순서로 여러 LetterRun을 구성한다.</summary>
    private static RiftTypeFrames Codes(params (char Side, int Count)[] runs)
    {
        var codes = new List<RiftFrameCode>();
        // 방향 구간과 그 안의 프레임은 원문 입력 순서다.
        foreach (var run in runs)
            // number0부터의 메타 입력이며 결과 프레임을 계산하지 않는다.
            for (int frame = 0; frame < run.Count; frame++) codes.Add(new((byte)run.Side, 0, (byte)frame, 0));
        return new(codes);
    }
    /// <summary>빈 원문 효과 목록 표기를 보존한다.</summary>
    private static string Events(List<string> trace) => trace.Count == 0 ? "-" : string.Join(';', trace);
    /// <summary>원문 좌표 두 DWORD의 효과 인자를 기록한다.</summary>
    private static string XY(RawPathPoint p) => $"{BitConverter.SingleToUInt32Bits(p.X)}:{BitConverter.SingleToUInt32Bits(p.Y)}";
    /// <summary>같은 실제 부모 좌표를 현재 슬롯에서 조회한다.</summary>
    private static RawPathPoint Position(RawSidPool pool) => new(BitConverter.UInt32BitsToSingle(Get(pool.Slot(50), 14)), BitConverter.UInt32BitsToSingle(Get(pool.Slot(50), 18)));
    /// <summary>실제 Pop 외부 경계가 현재 슬롯의 두 좌표만 변경한다.</summary>
    private static void StorePosition(RawSidPool pool, RawPathPoint p) { Put(pool.AllocatedBytes(50), 14, BitConverter.SingleToUInt32Bits(p.X)); Put(pool.AllocatedBytes(50), 18, BitConverter.SingleToUInt32Bits(p.Y)); }

    /// <summary>두 x87 정밀도348관측의 전체 경로/부모/공유counter가 같다.</summary>
    [Fact]
    public void NativeWalker348()
    {
        string[][] rows = Rows("walker-path-x86.tsv"); Assert.Equal(348, rows.Length);
        // 각 행의 원문 입력만 해석하며 기대값은 fixture에서 읽는다.
        foreach (string[] row in rows)
        {
            uint[] a = row[0].Split(',').Select(U).ToArray(); Assert.Equal(12, a.Length); Assert.Equal(row[1..5], row[5..9]);
            if (a[0] == 0) { byte[] image = a[2] != 0 ? new RawPathRuntime(OriginalEdition.Patch1078, false, (byte)a[1], walker: true).Bytes.ToArray() : Enumerable.Repeat((byte)a[1], 100080).ToArray(); Assert.Equal("N:100080", row[1]); Assert.Equal(U(row[2]), Adler(image)); continue; }
            RawSidPool pool = Pool(0x12200000, (byte)a[3], a[5]); var path = new RawPathRuntime(OriginalEdition.Patch1078, false, walker: true); path.SetPath([new(0, 0)], [0]); path.SetAttachment(0, 50); path.RestoreStep(0, 0, a[9] == 2);
            double[] times = [0, 10, Math.BitIncrement(10), BitConverter.UInt64BitsToDouble(0x7ff8000000000000), Math.BitDecrement(10), double.PositiveInfinity, double.NegativeInfinity]; path.NextFrame = times[a[10]];
            RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray(); frames[156] = a[11] != 0 ? Codes(('C', 3), ('A', 5), ('D', 3), ('B', 7)) : Codes(('A', 8), ('B', 8)); var trace = new List<string>(); uint counter = a[6];
            RawWalkerFrame.Run(pool, 50, path, 10, frames, 137, 156, ref counter, new()
            {
                Immobile = () => { trace.Add("I"); return a[9] == 1; }, Halt = () => { trace.Add("H"); return a[9] == 3; },
                Step = () => { trace.Add("S"); path.WriteBits(path.Layout.Angle, a[4]); return new(new(10f + BitConverter.UInt32BitsToSingle(a[7]), 20f + BitConverter.UInt32BitsToSingle(a[8])), BitConverter.UInt32BitsToSingle(a[4]), a[9] == 4); },
                GroundStop = p => { trace.Add("G:" + XY(p)); return a[9] == 5; }, Arrive = () => { trace.Add("A"); path.SetFinished(true); },
                Unpop = (sid, flags) => { Assert.Equal(50, sid); trace.Add("U:" + flags); }, Pop = (sid, p, flags) => { Assert.Equal(50, sid); trace.Add($"P:{XY(p)}:{flags}"); StorePosition(pool, p); },
            }, new());
            Assert.True(row[1] == Events(trace), $"입력 {row[0]} 기대 {row[1]} 실제 {Events(trace)}"); Assert.Equal(U(row[2]), Adler(path.Bytes.Span)); Assert.Equal(row[3], Convert.ToHexString(pool.Slot(50)).ToLowerInvariant()); Assert.Equal(U(row[4]), counter);
        }
    }
    /// <summary>벌프314관측의 회전 helper/보행/난수/현재 좌표 음향이 같다.</summary>
    [Fact]
    public void NativeBulf314()
    {
        string[][] rows = Rows("bulf-path-x86.tsv"); Assert.Equal(314, rows.Length);
        // 두 정밀도의 출력 중복도 검사하지만 관측 수로 중복 계산하지 않는다.
        foreach (string[] row in rows)
        {
            uint[] a = row[0].Split(',').Select(U).ToArray(); Assert.Equal(row[1..5], row[5..9]);
            if (a[0] == 0) { byte[] image = a[2] != 0 ? new RawPathRuntime(OriginalEdition.Patch1078, false, (byte)a[1], bulf: true).Bytes.ToArray() : Enumerable.Repeat((byte)a[1], 100080).ToArray(); Assert.Equal("N:100080", row[1]); Assert.Equal(U(row[2]), Adler(image)); continue; }
            RawSidPool pool = Pool(0x12200000, 135, a[3]); var path = new RawPathRuntime(OriginalEdition.Patch1078, false, bulf: true); path.SetPath([new(0, 0)], [unchecked((int)a[4])]); path.SetAttachment(0, 50); path.RestoreStep(0, BitConverter.UInt32BitsToSingle(a[1]), a[8] == 2);
            RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray(); frames[135] = Codes(('A', 8), ('B', 8), ('C', 8), ('D', 8), ('E', a[11] != 0 ? 24 : 8), ('F', a[11] != 0 ? 24 : 8)); SquidDisplayShape[] shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
            // 원본 메타 입력은 모든 현재 프레임의 같은 walkStepX다.
            for (int index = 0; index < (a[11] != 0 ? 80 : 48); index++) shapes[135].Frames.Add(new() { WalkStepX = BitConverter.UInt32BitsToSingle(a[7]) });
            var trace = new List<string>(); uint result = 0;
            if (a[0] == 2) result = unchecked((uint)RawBulfFrame.TurningFrame(frames[135], BitConverter.UInt32BitsToSingle(a[2]), a[4]));
            else RawBulfFrame.Run(pool, 50, path, frames, shapes, new()
            {
                Immobile = () => { trace.Add("I"); return a[8] == 1; }, Halt = () => { trace.Add("H"); return a[8] == 3; },
                Step = () => { trace.Add("S"); float angle = BitConverter.UInt32BitsToSingle(a[2]); path.RestoreStep(0, angle); return new(new(10f + BitConverter.UInt32BitsToSingle(a[5]), 20f + BitConverter.UInt32BitsToSingle(a[6])), angle, a[8] == 4); },
                Arrive = () => { trace.Add("A"); path.SetFinished(true); }, Unpop = (sid, flags) => { Assert.Equal(50, sid); trace.Add("U:" + flags); },
                Pop = (sid, p, flags) => { Assert.Equal(50, sid); trace.Add($"P:{XY(p)}:{flags}"); StorePosition(pool, a[10] != 0 ? new(33, 44) : p); },
                RandomInt = (low, high) => { trace.Add($"R:{low}:{high}"); return unchecked((int)a[9]); }, Sound = (x, y, name) => trace.Add($"W:{XY(new(x, y))}:{name}:0"),
            }, new());
            Assert.True(row[1] == Events(trace), $"입력 {row[0]} 기대 {row[1]} 실제 {Events(trace)}"); Assert.Equal(U(row[2]), Adler(path.Bytes.Span)); Assert.Equal(row[3], Convert.ToHexString(pool.Slot(50)).ToLowerInvariant()); Assert.Equal(U(row[4]), result);
        }
    }
    /// <summary>벌룬688관측의 전체 이미지/관찰 전역/높이/화염 뒤 현재 좌표가 같다.</summary>
    [Fact]
    public void NativeBalloon688()
    {
        string[][] rows = Rows("balloon-path-x86.tsv"); Assert.Equal(688, rows.Length);
        // 생성/Init/프레임을 원문 모드에 따라 각각 실행한다.
        foreach (string[] row in rows)
        {
            uint[] a = row[0].Split(',').Select(U).ToArray(); Assert.Equal(row[1..6], row[6..11]);
            if (a[0] == 0) { byte[] image = a[2] != 0 ? new RawPathRuntime(OriginalEdition.Patch1078, false, (byte)a[1], balloon: true).Bytes.ToArray() : Enumerable.Repeat((byte)a[1], 100080).ToArray(); Assert.Equal("N:100080", row[1]); Assert.Equal(U(row[2]), Adler(image)); Assert.Equal(a[2], U(row[3])); continue; }
            RawSidPool pool = Pool(0x5040c0, (byte)a[3]); System.Buffers.Binary.BinaryPrimitives.WriteUInt16LittleEndian(pool.AllocatedBytes(50)[6..], 7); pool.AllocatedBytes(50)[30] = (byte)a[2]; var path = new RawPathRuntime(OriginalEdition.Patch1078, false, balloon: true); path.SetAttachment(7, 50); var observation = new RawPathObservation(); var trace = new List<string>();
            if (a[0] == 1) { path.SetPath(new RawPathPoint[a[1]], new int[a[1]]); path.InitializeFlight(new(10, 20), new(BitConverter.UInt32BitsToSingle(a[4]), BitConverter.UInt32BitsToSingle(a[5])), 44, (byte)a[2]); }
            else
            {
                RawPathPoint[] points = a[8] == 2 ? [new(10, 20), new(10, 20), new(10, 20), new(10, 20)] : [new(10, 20), new(10, 16), new(14, 16), new(14, 20)]; path.SetPath(points, [0, 0, 2, 2], a[8] == 1 ? [0, 1, 1.00001, 1.00002] : [0, 1, 3, 4]); path.RestoreStep(unchecked((int)a[1]), 0, a[6] == 1); double now = BitConverter.UInt32BitsToSingle(a[4]);
                RawBalloonFrame.Run(pool, 50, path, () => now, 111, new()
                {
                    Halt = () => { trace.Add("H"); return a[6] == 2; }, Step = () => path.Step(now, Position(pool), observation, () => trace.Add($"S:{XY(Position(pool))}:123")),
                    Arrive = () => { trace.Add("A"); path.SetFinished(true); }, Unpop = (sid, flags) => { trace.Add($"U:{sid}:{flags}"); if (a[7] == 2) pool.AllocatedBytes(50)[30] = 99; },
                    Pop = (_, p, flags) => { trace.Add($"P:{XY(p)}:{flags}"); StorePosition(pool, p); }, Flame = (x, y) => { trace.Add($"F:{XY(new(x, y))}:0"); if (a[7] == 1) StorePosition(pool, new(30, 40)); },
                }, new());
            }
            Assert.True(row[1] == Events(trace), $"입력 {row[0]} 기대 {row[1]} 실제 {Events(trace)}"); Assert.Equal(U(row[2]), Adler(path.Bytes.Span)); Assert.Equal(row[3], Convert.ToHexString(pool.Slot(50)).ToLowerInvariant()); Assert.Equal(U(row[4]), observation.Flags); Assert.Equal(U(row[5]), observation.Stamp);
        }
    }
    /// <summary>Halt/Step/지면/Unpop에서 삭제되면 그 뒤 Pop을 호출하지 않는다.</summary>
    [Fact]
    public void WalkerDeletionStopsAtEachRealEffectBoundary()
    {
        // Source의 네 삭제 위치를 각각 새 watch/이미지로 검사한다.
        for (int stage = 0; stage < 4; stage++)
        {
            RawSidPool pool = Pool(0x12200000, 137); var path = new RawPathRuntime(OriginalEdition.Patch1078, false, walker: true); var watch = new PathLifeWatch(); int[] calls = new int[6]; uint counter = 0;
            RawWalkerFrame.Run(pool, 50, path, 10, [], 137, 156, ref counter, new()
            {
                Immobile = () => false, Halt = () => { calls[0]++; if (stage == 0) watch.Alive = false; return false; },
                Step = () => { calls[1]++; if (stage == 1) watch.Alive = false; return new(new(10, 20), 0, false); }, GroundStop = _ => { calls[2]++; if (stage == 2) watch.Alive = false; return false; },
                Arrive = () => calls[3]++, Unpop = (_, _) => { calls[4]++; if (stage == 3) watch.Alive = false; }, Pop = (_, _, _) => calls[5]++,
            }, watch);
            Assert.False(watch.Alive); Assert.Equal(new[] { 1, stage >= 1 ? 1 : 0, stage >= 2 ? 1 : 0, 0, stage == 3 ? 1 : 0, 0 }, calls);
        }
    }
}
