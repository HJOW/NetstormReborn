using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using static Netstorm.Core.Tests.RawPathProcessTests;

// 원문 애니메이션/준비와 실제 Host 수명/전송 선택을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>독립 프레임 관측과 실제 같은 객체의 전송/삭제 연결을 검증한다.</summary>
public sealed class RawPathIntegrationTests
{
    /// <summary>각 판본의608읽기/1408정지 관측을 전체 슬롯/가상 순서와 비교한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    public void NativeAnimation2016PerEdition(string edition)
    {
        string[][] rows = Rows("pathanimation-x86.tsv"); var codes = new Dictionary<uint, RiftTypeFrames>();
        // 원문 입력 코드 표는 네 바이트 단위로 해석하며 기대번호를 만들지 않는다.
        foreach (string[] row in rows.Where(row => row[0] == "Codes"))
        {
            byte[] bytes = Convert.FromHexString(row[2]); var list = new List<RiftFrameCode>();
            // 원문 입력 표의 side/variant/number/flags 순서를 유지한다.
            for (int index = 0; index < bytes.Length; index += 4) list.Add(new(bytes[index], bytes[index + 1], bytes[index + 2], bytes[index + 3]));
            codes.Add(U(row[1]), new(list));
        }
        bool patch = edition == "originals"; var pool = new RawSidPool(24000, false, edition: Edition(edition)); RiftTypeFrames[] frames = Enumerable.Range(0, 96).Select(_ => new RiftTypeFrames([])).ToArray(); int reads = 0, stops = 0;
        // 같은 원문 SID50을 실제로 할당한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        // 독립 PE의 판본별 관측을 바꾸지 않고 읽는다.
        foreach (string[] row in rows.Where(row => row[0] != "Codes" && row[1] == edition))
        {
            RiftTypeFrames current = codes[U(row[2])]; int frame = int.Parse(row[3], System.Globalization.CultureInfo.InvariantCulture);
            if (row[0] == "Read") { Assert.Equal(int.Parse(row[4], System.Globalization.CultureInfo.InvariantCulture), RawPathAnimation.Side(current, frame)); Assert.Equal(int.Parse(row[5], System.Globalization.CultureInfo.InvariantCulture), RawPathAnimation.Direction(current, frame)); reads++; continue; }
            Span<byte> raw = pool.AllocatedBytes(50); raw.Clear(); Put(raw, 0, 0x11010000); raw[10] = 82; Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f));
            if (patch) Put(raw, 36, unchecked((uint)frame)); else raw[34] = unchecked((byte)frame); raw[patch ? 40 : 35] = (byte)U(row[4]); raw[patch ? 34 : 32] = (byte)U(row[5]); frames[82] = current; var trace = new List<string>();
            // 두 효과는 원문 CPU의 관찰 경계이며 실제 frame 저장은 제품 몸체가 한다.
            uint Frame() => patch ? Get(pool.Slot(50), 36) : pool.Slot(50)[34];
            var animation = new RawPathAnimation(pool, frames, new()
            {
                Unpop = (sid, flags) => trace.Add($"U:{sid}:{flags}:{Frame()}"),
                Repop = (sid, flags) => trace.Add($"P:{sid}:{Get(pool.Slot(sid), 14)}:{Get(pool.Slot(sid), 18)}:{flags}:{Frame()}"),
            });
            animation.Stop(50); Assert.Equal(row[6], string.Join(';', trace)); Assert.Equal(row[7], Convert.ToHexString(pool.Slot(50)[..(patch ? 50 : 36)]).ToLowerInvariant()); stops++;
        }
        Assert.Equal(608, reads); Assert.Equal(1408, stops);
    }
    /// <summary>실제 가상 Prepare48입력에서 PATCH override/CD 무시와 모든 시각 비트가 같다.</summary>
    [Fact]
    public void NativeVirtualPrepare48()
    {
        string[][] rows = Rows("pathprepare-x86.tsv"); Assert.Equal(48, rows.Length);
        // 실제 Process의 같은 시계와 네 원문 꼭짓점을 사용한다.
        foreach (string[] row in rows)
        {
            OriginalEdition edition = Edition(row[0]); var pool = new RawSidPool(32768, edition: edition); var hash = new RawSquidHash(); int count = edition == OriginalEdition.Patch1078 ? 188 : 171;
            OriginalTypeRecord[] types = Enumerable.Range(0, count).Select(_ => new OriginalTypeRecord()).ToArray(); RiftTypeFrames[] frames = Enumerable.Range(0, count).Select(_ => new RiftTypeFrames([])).ToArray(); SquidDisplayShape[] shapes = Enumerable.Range(0, count).Select(_ => new SquidDisplayShape()).ToArray(); var state = new RawPathProcessState(new SquidProcessState { Now = D(row[4]) });
            // Prepare는 Host/공간/부모 가상을 실행하지 않는다. 잘못 선택하면 즉시 실패하는 원문 검사 경계다.
            void Unexpected() => throw new InvalidOperationException("Prepare에서 외부 효과가 선택됨");
            var hooks = new RawPathProcessHooks { Unpop = (_, _) => Unexpected(), Repop = (_, _) => Unexpected(), Pop = (_, _, _) => Unexpected(), Halt = _ => throw new InvalidOperationException("Prepare의 Halt"), Arrival = (_, _, _) => throw new InvalidOperationException("Prepare의 Arrival"), SoundId = _ => throw new InvalidOperationException("Prepare의 SoundId"), StepSound = (_, _, _, _) => Unexpected(), ClearBlocked = (_, _, _, _) => Unexpected(), Cleanup = (_, _, _) => Unexpected() };
            var process = new RawPathProcess(pool, hash, types, frames, shapes, new byte[65536], state, hooks, false); process.Runtime.SetPath([new(10, 20), new(13.1f, 24.7f), new(15.2f, 19.8f), new(23.9f, 27.6f)], [7, 0, 4, 6]); process.Prepare(F(row[1]), F(row[2]), F(row[3])); string[] expected = row[5].Split(',');
            // 원문53비트 double 저장 네 곳을 전부 대조한다.
            for (int index = 0; index < 4; index++) Assert.Equal(ulong.Parse(expected[index], System.Globalization.CultureInfo.InvariantCulture), BitConverter.DoubleToUInt64Bits(process.Runtime.Time(index)));
        }
    }
    /// <summary>같은 실제 Host의 자기 삭제 뒤 외부 checking owner와 진단만 살아 있다.</summary>
    [Fact]
    public void ActualHostHaltDeletionRetainsExternalCurrentChecking()
    {
        // 상위 비트까지 포함한 세 현재 진단값을 같은 삭제 경로에 공급한다.
        foreach (uint initial in new uint[] { 0, 0x80000000, uint.MaxValue })
        {
            using var scene = new Scene(OriginalEdition.Patch1078); uint checking = initial, diagnostics = 0; scene.State.Checking = new RawCheckingFlag(() => checking); scene.HaltCode = 0;
            scene.AfterHalt = parent => { Assert.Equal(scene.Parent, parent); scene.Host.Kill(scene.Process, 0); };
            scene.Diagnostic = (expression, file, line) => { Assert.Equal("result == haltALREADY_DEAD", expression); Assert.Equal(@"\Ns\O\Pathprocess.cpp", file); Assert.Equal(0x2d5u, line); Assert.Equal(0, scene.Kernel.Size()); Assert.True(scene.Deleted); diagnostics++; checking = 0; };
            scene.Process.RunFrame(); Assert.Equal(initial == 0 ? 0u : 1u, diagnostics); Assert.Equal(0u, checking); Assert.True(scene.Deleted); Assert.Equal(0, scene.Kernel.Size());
        }
    }
    /// <summary>실제 Host 삭제 뒤 저장 부모의 현재 metadata로 한 번 겹침을 정리한다.</summary>
    [Fact]
    public void ActualHostArrivalDeletionAndDiagnosticResumeClearOnce()
    {
        // 정상/비정상 반환과 살아 있는 가상/자기 삭제를 각각 구별한다.
        for (int variant = 0; variant < 4; variant++)
        {
            using var scene = new Scene(OriginalEdition.Patch1078); uint checking = variant == 0 ? 0u : 0x80000000; scene.State.Checking = new RawCheckingFlag(() => checking); scene.ReturnCode = variant < 2 ? 1000u : 999u; var diagnostics = new List<uint>();
            int candidate = scene.Pool.Allocate(SidAllocation.Client); Span<byte> raw = scene.Pool.AllocatedBytes(candidate); raw.Clear(); raw[10] = 1; Put(raw, 14, BitConverter.SingleToUInt32Bits(10)); Put(raw, 18, BitConverter.SingleToUInt32Bits(20)); scene.Types[1].Flags2 = 0x200; scene.Hash.Cell(0, 10, 20) = (ushort)candidate; scene.Spots[20 * 256 + 10] |= 8;
            scene.Diagnostic = (expression, file, line) => { Assert.Equal(@"\Ns\O\Pathprocess.cpp", file); Assert.True(line is 0x28d or 0x290); diagnostics.Add(line); if (line == 0x290) { Assert.True(scene.Deleted); Assert.Equal(0, scene.Kernel.Size()); } checking = 0; };
            if (variant < 3) scene.AfterArrival = _ => scene.Host.Kill(scene.Process, 0);
            scene.Process.Arrive(); Assert.True(scene.Deleted); Assert.Equal(0u, checking); Assert.Equal(0, scene.Kernel.Size()); Assert.Equal(1, scene.Trace.Count(value => value == "clearBlocked")); Assert.Equal(variant == 0 ? [] : variant == 1 ? new uint[] { 0x290 } : new uint[] { 0x28d }, diagnostics);
        }
    }
    /// <summary>원문 마지막 writer 선택은 fallback/full/Packet/Transmit에 공유된다.</summary>
    [Fact]
    public void ActualPathTransmissionSharesHeaderBufferAndFallbackSelection()
    {
        using var scene = new Scene(OriginalEdition.Patch1078); byte[] bytes = Enumerable.Repeat((byte)0xa5, 2048).ToArray(); var header = new RawPathDataHeader(); var events = new List<string>();
        var fallback = new ProcessFormTransmission { WriteData = (_, changed) => { events.Add("other:" + changed); return 2; }, WriteFullData = (_, force) => { events.Add("full:" + force); return 3; }, Packet = size => Enumerable.Repeat((byte)9, size).ToArray(), Transmit = size => events.Add("other-send:" + size) };
        ProcessFormTransmission codec = RawPathTransmission.Make(scene.Pool, scene.Types, header, bytes, size => events.Add("path-send:" + size), fallback);
        int size = codec.WriteData!(scene.Process, true); Assert.True(size > 24); Assert.Equal(0x30, bytes[1]); Assert.Equal(bytes[..size], codec.Packet!(size)); codec.Transmit!(size);
        Assert.Equal(2, codec.WriteData(new SquidProcess(), false)); Assert.Equal(new byte[] { 9, 9 }, codec.Packet(2)); codec.Transmit(2);
        header.PacketId = 0x35; header.PacketFlags = 0x81; Assert.Equal(size, codec.WriteFullData!(scene.Process, uint.MaxValue)); Assert.Equal(0x35, codec.Packet(size)[1]); Assert.Equal(1, codec.Packet(size)[0]);
        Assert.Equal(3, codec.WriteFullData(new SquidProcess(), 5)); codec.Transmit(3); Assert.Equal(new[] { "path-send:" + size, "other:False", "other-send:2", "full:5", "other-send:3" }, events);
    }
    /// <summary>Native writer 선택은 typed Pool/Form/부모 guard를 먼저 실행하지 않는다.</summary>
    [Fact]
    public void NativePathWriterBypassesTypedGuardAndReceivesExactArguments()
    {
        using var scene = new Scene(OriginalEdition.Patch1078); var header = new RawPathDataHeader(); byte[] buffer = new byte[32]; var seen = new List<(bool, uint)>();
        var unattached = new RawPathProcess(scene.Pool, scene.Hash, scene.Types, scene.Frames, scene.Shapes, scene.Spots, scene.State, scene.Hooks(), false);
        var otherPool = new RawSidPool(32768); ProcessFormTransmission codec = RawPathTransmission.Make(otherPool, scene.Types, header, buffer, _ => { }, nativeWriter: (process, bytes, changed, force) => { Assert.Same(unattached, process); Assert.True(bytes.Span.SequenceEqual(buffer)); seen.Add((changed, force)); bytes.Span[0] = 71; return 1; });
        Assert.Equal(1, codec.WriteData!(unattached, true)); Assert.Equal(71, codec.Packet!(1)[0]); Assert.Equal(1, codec.WriteFullData!(unattached, 0xfedcba98)); Assert.Equal(new[] { (true, 0u), (false, 0xfedcba98u) }, seen);
        var typed = RawPathTransmission.Make(otherPool, scene.Types, header, buffer, _ => { }); Assert.Throws<ArgumentException>(() => typed.WriteData!(unattached, false));
    }
}
