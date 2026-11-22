using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Missile = Netstorm.Core.Original.Missile;

// 원문 두 진단의 현재 DWORD/필드 재조회와 실제 자기 삭제를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>실제 효과 뒤 현재 상태와 원문 진단 위치를 검증한다.</summary>
public sealed class RawMissileCheckingTests
{
    /// <summary>현재 원문DWORD만 쓴다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>현재 원문DWORD를 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int at) => BinaryPrimitives.ReadUInt32LittleEndian(raw[at..]);
    /// <summary>원문 float 좌표 두 개를 기록한다.</summary>
    private static void Point(Span<byte> raw, int at, float x, float y) { Put(raw, at, BitConverter.SingleToUInt32Bits(x)); Put(raw, at + 4, BitConverter.SingleToUInt32Bits(y)); }
    /// <summary>비충돌 직선과 종료에 필요한 독립 입력만 준비한다.</summary>
    private sealed class Scene
    {
        /// <summary>진단과 Host가 함께 쓰는 실제 풀이다.</summary>
        internal RawSidPool Pool { get; }
        /// <summary>현재 원문 TYPE와 생성자 자료다.</summary>
        internal OriginalTypeRecord[] Types { get; }
        /// <summary>현재 사거리와 피해 자료다.</summary>
        internal MissileTypeData[] Data { get; }
        /// <summary>두 진단의 같은 현재 owner다.</summary>
        internal MissileState State { get; } = new() { Now = 1 };
        /// <summary>두 진단 사이에서 바뀌는 원문 이미지다.</summary>
        internal MissileFields Fields { get; } = new();
        /// <summary>현재 외부 효과의 선언이다.</summary>
        internal MissileHooks Hooks { get; } = new();
        /// <summary>효과/진단의 실제 순서다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>후반 관문 뒤에만 쓰는 위치다.</summary>
        internal int Previous { get; }
        /// <summary>원문과 같은 세 자산 및 후반 재조회 대상을 확보한다.</summary>
        internal Scene(bool patch = true)
        {
            OriginalEdition edition = patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072; Pool = new(24000, false, edition: edition); Types = Enumerable.Range(0, patch ? 188 : 171).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(edition, i) }).ToArray(); Data = Types.Select(_ => new MissileTypeData()).ToArray(); Previous = patch ? 120 : 96;
            // 진단과 후반 피해가 재해석하는 번호를 확보한다.
            for (int sid = 5; sid <= 16; sid++) { Assert.Equal(sid, Pool.Allocate(SidAllocation.Client)); Pool.AllocatedBytes(sid).Clear(); }
            Pool.AllocatedBytes(10)[10] = 128; Pool.AllocatedBytes(11)[10] = 71; Pool.AllocatedBytes(12)[10] = 80; Pool.RawSlot(12)[34] = 3; Types[128].Group = 1; Types[0].Group = 7; Data[0].DamagePerSecond = 10;
            byte[] raw = Fields.Bytes; Put(raw, 4, 13); Put(raw, 8, 12); Put(raw, 20, 10); Put(raw, 24, 11); Point(raw, patch ? 56 : 40, 10, 10); Point(raw, patch ? 72 : 56, 20, 10); Point(raw, patch ? 64 : 48, 20, 10); Point(raw, Previous, -10000, -10000); BinaryPrimitives.WriteDoubleLittleEndian(raw.AsSpan(patch ? 104 : 80), 2); BinaryPrimitives.WriteDoubleLittleEndian(raw.AsSpan(patch ? 112 : 88), 0);
            Hooks.Unpop = (sid, flags) => { Assert.Equal(0u, flags); Events.Add("U" + sid); }; Hooks.Pop = (sid, x, y, flags) => { Assert.Equal((ushort)12, sid); Assert.Equal(10, y); Assert.Equal(0u, flags); Events.Add("P" + BitConverter.SingleToUInt32Bits(x)); }; Hooks.TargetBounds = (sid, _) => { Assert.Equal((ushort)11, sid); Events.Add("B"); return [100, 100, 101, 101]; }; Hooks.Center = sid => { Assert.Equal((ushort)10, sid); Events.Add("C"); return [10, 10]; }; Hooks.Refresh = sid => Events.Add("Q" + sid); Hooks.Destroy = (sid, flags) => { Assert.Equal(0u, flags); Events.Add("X" + sid); }; Hooks.Hit = (_, _) => throw new InvalidOperationException("미선택 직접 피해"); Hooks.Damage = (sid, amount, owner, attacker) => { Assert.Equal((ushort)11, sid); Assert.Equal(6, amount); Assert.Equal((byte)3, owner); Assert.Equal((ushort)0, attacker); Events.Add("D"); };
        }
        /// <summary>원문 항상참 Baseprocess의 두 식/파일/행을 구별한다.</summary>
        internal void Diagnostic(string expression, string file, uint line) { Assert.Equal(@"\Ns\O\Baseprocess.cpp", file); Assert.Equal(line == 0x69 ? "formSid == 0 || form.isValid()" : "attachToSid == 0 || s.isValid()", expression); Assert.Contains(line, new uint[] { 0x69, 0x6c }); Events.Add("A" + line); }
        /// <summary>각 실행에서 같은 현재 입력 주소를 빌린다.</summary>
        internal void Run() => new RawMissile(Pool, Types, Data, State, Hooks).Frame(Fields);
    }
    /// <summary>상위 DWORD와 첫 진단 복귀 뒤 현재 시각/부모 및 같은 관문 계속을 확인한다.</summary>
    [Fact]
    public void FirstDiagnosticRepairsFullDwordBeforeSlotLookup()
    {
        var scene = new Scene(); scene.State.Checking.Set(0x80000000u); Put(scene.Fields.Bytes, 4, 0x10005); Put(scene.Fields.Bytes, 8, 0x10006); Put(scene.Fields.Bytes, 20, 65535); scene.Hooks.Assertion = (expression, file, line) => { scene.Diagnostic(expression, file, line); if (line == 0x69) { Put(scene.Fields.Bytes, 4, 0); scene.State.Checking.Set(0); scene.State.Now = .5; } else { Put(scene.Fields.Bytes, 8, 12); Put(scene.Fields.Bytes, 20, 10); } }; scene.Run(); Assert.Equal(new[] { "A105", "A108", "U12", "P1095237632", "B", "C" }, scene.Events); Assert.Equal(12.5f, BitConverter.UInt32BitsToSingle(Read(scene.Fields.Bytes, scene.Previous)));
    }
    /// <summary>후반 현재 발사자를 재해석하면서 삭제할 표시는 첫 SID를 유지한다.</summary>
    [Fact]
    public void LateDiagnosticReloadsFiringAndKeepsFirstDisplay()
    {
        var scene = new Scene(); var pop = scene.Hooks.Pop!; scene.Hooks.Pop = (sid, x, y, flags) => { pop(sid, x, y, flags); scene.State.Checking.Set(0x80000000u); Put(scene.Fields.Bytes, 4, 0x10005); }; scene.Hooks.Assertion = (expression, file, line) => { scene.Diagnostic(expression, file, line); Assert.Equal(-10000, BitConverter.UInt32BitsToSingle(Read(scene.Fields.Bytes, scene.Previous))); Put(scene.Fields.Bytes, 4, 0); Put(scene.Fields.Bytes, 8, 14); Put(scene.Fields.Bytes, 20, 14); scene.Pool.AllocatedBytes(11)[11] = 2; }; scene.Run(); Assert.Equal(new[] { "U12", "P1097859072", "B", "C", "A105", "D", "X12" }, scene.Events); Assert.Equal(15, BitConverter.UInt32BitsToSingle(Read(scene.Fields.Bytes, scene.Previous)));
    }
    /// <summary>후반 장치 누락은 이미 수행한 이동/빗나감 소비를 되돌리지 않는다.</summary>
    [Fact]
    public void LateMissingDiagnosticRetainsPriorEffects()
    {
        var scene = new Scene(); scene.Data[128].Range = 40; Put(scene.Fields.Bytes, 40, 1); var pop = scene.Hooks.Pop!; scene.Hooks.Pop = (sid, x, y, flags) => { pop(sid, x, y, flags); scene.State.Checking.Set(1); Put(scene.Fields.Bytes, 8, 24000); }; Assert.Throws<InvalidOperationException>(scene.Run); Assert.Equal(new[] { "U12", "P1097859072", "B", "C" }, scene.Events); Assert.Equal(0u, Read(scene.Fields.Bytes, 40)); Assert.Equal(1u, Read(scene.Fields.Bytes, 44)); Assert.Equal(-10000, BitConverter.UInt32BitsToSingle(Read(scene.Fields.Bytes, scene.Previous))); scene.Hooks.Pop = pop; Put(scene.Fields.Bytes, 8, 12); scene.Events.Clear(); scene.Run(); Assert.Equal(15, BitConverter.UInt32BitsToSingle(Read(scene.Fields.Bytes, scene.Previous))); Assert.Equal(4, scene.Events.Count);
    }
    /// <summary>무효 좌표 조기 종료는 후반 진단이나 이전 좌표 쓰기를 하지 않는다.</summary>
    [Fact]
    public void InvalidPositionSkipsLateGateAndPreviousWrite()
    {
        var scene = new Scene(); scene.State.Checking.Set(1); Put(scene.Fields.Bytes, 4, 24000); Put(scene.Fields.Bytes, 40, 1); BinaryPrimitives.WriteDoubleLittleEndian(scene.Fields.Bytes.AsSpan(104), 0); BinaryPrimitives.WriteDoubleLittleEndian(scene.Fields.Bytes.AsSpan(112), 0); scene.Hooks.Assertion = (expression, file, line) => { scene.Diagnostic(expression, file, line); Put(scene.Fields.Bytes, 4, 0); }; scene.Hooks.ImpactSound = (_, _, _) => { scene.Events.Add("S"); Put(scene.Fields.Bytes, 4, 24000); }; scene.Run(); Assert.Equal(new[] { "A105", "S", "X12" }, scene.Events); Assert.Equal(-10000, BitConverter.UInt32BitsToSingle(Read(scene.Fields.Bytes, scene.Previous)));
    }
    /// <summary>CD는 PATCH checking/진단을 호출하지 않는다.</summary>
    [Fact]
    public void CdFrameSkipsPatchChecking()
    {
        var scene = new Scene(false); scene.State.Checking.Set(uint.MaxValue); Put(scene.Fields.Bytes, 4, 0x10005); scene.Hooks.Assertion = (_, _, _) => throw new InvalidOperationException("CD 미선택 진단"); scene.Run(); Assert.Equal(new[] { "U12", "P1097859072", "B", "C", "Q10", "X12" }, scene.Events);
    }
    /// <summary>이동 중 현재 checking0은 후반 진단을 건너뛴다.</summary>
    [Fact]
    public void CurrentFlagDisabledDuringPopSkipsLateGate()
    {
        var scene = new Scene(); scene.State.Checking.Set(0x80000000u); var pop = scene.Hooks.Pop!; scene.Hooks.Pop = (sid, x, y, flags) => { pop(sid, x, y, flags); scene.State.Checking.Set(0); Put(scene.Fields.Bytes, 4, 24000); }; scene.Run(); Assert.Equal(new[] { "U12", "P1097859072", "B", "C", "Q10", "X12" }, scene.Events); Assert.Equal(24000u, Read(scene.Fields.Bytes, 4));
    }
    /// <summary>실제 Host가 첫/후반 진단 중 삭제하면 raw 후속 접근과 중복 종료를 멈춘다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void ActualHostDiagnosticDeletionStopsFields(bool late)
    {
        var scene = new Scene(); var kernel = new Kernel(); var destroy = new RawSquidDestroy(scene.Pool, new RawUnpop(scene.Pool, new RawSquidHash(), new byte[65536]), scene.Types); var host = new SquidProcessHost(scene.Pool, scene.Types, kernel, destroy, new(), asset: new() { Selected = () => 0 }); Missile? process = null;
        try
        {
            scene.Hooks.Assertion = (expression, file, line) => { scene.Diagnostic(expression, file, line); host.Kill(process!, 0x10); process = null; }; var pop = scene.Hooks.Pop!; scene.Hooks.Pop = (sid, x, y, flags) => { pop(sid, x, y, flags); scene.State.Checking.Set(0x80000000u); Put(process!.Fields.Bytes, 4, 0x10005); }; var logic = new RawMissile(scene.Pool, scene.Types, scene.Data, scene.State, scene.Hooks); process = Assert.IsType<Missile>(host.Attach(new Missile(logic, scene.Fields), 28, 12, 0x10)); int form = process.Form(); if (!late) { scene.State.Checking.Set(0x80000000u); Put(process.Fields.Bytes, 4, 0x10005); } process.RunFrame(); Assert.Null(process); Assert.Equal(0, kernel.Size()); Assert.Null(host.FromForm(form)); Assert.True((scene.Pool.Slot(form)[11] & 1) != 0); Assert.Equal(0, scene.Pool.Slot(12)[11] & 3); Assert.Equal(late ? new[] { "U12", "P1097859072", "B", "C", "A105" } : new[] { "A105" }, scene.Events); Assert.Equal(0u, destroy.PreDepth); Assert.Equal(0u, destroy.PostDepth);
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }
}
