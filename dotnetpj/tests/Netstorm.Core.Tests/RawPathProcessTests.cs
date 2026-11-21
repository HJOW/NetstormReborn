using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 같은 실제 공간/Kernel에서 원본 경로 프레임과 자기 삭제를 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>PATCH의60프레임/12도착/63비행 관측을 실제 Host에 재사용한다. CD Host 하위는 별도 미복원이다.</summary>
public sealed class RawPathProcessTests
{
    /// <summary>주석/빈 행을 제외한 기존 관측을 읽는다.</summary>
    internal static string[][] Rows(string name) => File.ReadAllLines(Path.Combine(AppContext.BaseDirectory, "X86", name)).Where(line => line.Length > 0 && !line.StartsWith('#')).Select(line => line.Split('\t')).ToArray();
    /// <summary>문화권에 독립적인 원문 DWORD다.</summary>
    internal static uint U(string text) => uint.Parse(text, CultureInfo.InvariantCulture);
    /// <summary>관측된 float 비트를 그대로 해석한다.</summary>
    internal static float F(string text) => BitConverter.UInt32BitsToSingle(U(text));
    /// <summary>관측된 double 비트를 그대로 해석한다.</summary>
    internal static double D(string text) => BitConverter.UInt64BitsToDouble(ulong.Parse(text, CultureInfo.InvariantCulture));
    /// <summary>두 원문 판본을 선택한다.</summary>
    internal static OriginalEdition Edition(string text) => text == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
    /// <summary>전체 물리 이미지에서 원문 DWORD만 읽는다.</summary>
    internal static uint Get(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw[offset..]);
    /// <summary>명시한 원문 DWORD 입력을 같은 실제 바이트에 쓴다.</summary>
    internal static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);

    /// <summary>원문 PathScene과 같은 실제 등록/삭제/프로세스 소유자를 구성한다.</summary>
    internal sealed class Scene : IDisposable
    {
        /// <summary>선택한 실제 판본이다.</summary>
        public readonly OriginalEdition Edition;
        /// <summary>부모/폼이 함께 사용하는 실제 풀이다.</summary>
        public readonly RawSidPool Pool;
        /// <summary>같은 Pop/Unpop의 현재 공간 해시다.</summary>
        public readonly RawSquidHash Hash = new();
        /// <summary>생성자 주소를 포함한 실제 타입 입력이다.</summary>
        public readonly OriginalTypeRecord[] Types;
        /// <summary>TYPE별 보행 프레임 입력이다.</summary>
        public readonly RiftTypeFrames[] Frames;
        /// <summary>TYPE별 실제 보행 변위 입력이다.</summary>
        public readonly SquidDisplayShape[] Shapes;
        /// <summary>모든 칸에 지면 비트6이 있는 원문 입력이다.</summary>
        public readonly byte[] Spots = Enumerable.Repeat((byte)6, 65536).ToArray();
        /// <summary>실제 공간 등록 처리기다.</summary>
        public readonly RawPop Pop;
        /// <summary>실제 공간 해제 처리기다.</summary>
        public readonly RawUnpop Unpop;
        /// <summary>경로 수명과 프레임을 소유하는 실제 커널이다.</summary>
        public readonly Kernel Kernel = new(32);
        /// <summary>경로와 같은 시각/권한 소유자다.</summary>
        public readonly SquidProcessState Clock = new();
        /// <summary>실제 Form 부착과 파괴 호스트다.</summary>
        public readonly SquidProcessHost Host;
        /// <summary>경로 전역이 빌리는 같은 시계다.</summary>
        public readonly RawPathProcessState State;
        /// <summary>실제 부모 생성 공장이다.</summary>
        public readonly RawSquidFactory Factory;
        /// <summary>원문 관측과 같은 부모5다.</summary>
        public readonly int Parent;
        /// <summary>실제 Kernel에 부착한 경로다.</summary>
        public readonly RawPathProcess Process;
        /// <summary>원문 순서대로 축적한 외부/공간 요청이다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>외부 삭제 효과를 실제 Host가 발생시켰는지 보관한다.</summary>
        public readonly List<SquidDestroyEvent> ExternalDeletion = [];
        /// <summary>원문 가상 도착의 명시 응답이다.</summary>
        public uint ReturnCode = 1002;
        /// <summary>원문 가상 Halt의 명시 응답이다.</summary>
        public uint HaltCode = 10;
        /// <summary>삭제 접두의 trace 필터 입력이다.</summary>
        public bool WillDelete;
        /// <summary>실제 Cleanup이 호출됐는지 나타낸다.</summary>
        public bool Deleted;
        /// <summary>실제 삭제 직전 전체 경로 이미지다.</summary>
        public byte[] FinalImage = [];
        /// <summary>가상 안의 실제 자기 삭제 등 추가 입력이다.</summary>
        public Action<int>? AfterHalt = null;
        /// <summary>가상 도착의 실제 삭제/현재 상태 변경 입력이다.</summary>
        public Action<int>? AfterArrival = null;
        /// <summary>원문 진단의 명시 응답이다.</summary>
        public Action<string, string, uint>? Diagnostic = null;
        /// <summary>같은 실제 Pool/hash/spot/Host로 독립 초기 상태를 만든다.</summary>
        public Scene(OriginalEdition edition, bool priest = false, bool flying = false)
        {
            Edition = edition; Pool = new(32768, false, edition: edition);
            int count = edition == OriginalEdition.Patch1078 ? 188 : 171;
            Types = Enumerable.Range(0, count).Select(index => new OriginalTypeRecord { ConstructorAddress = index is 1 or 2 or 3 or 6 ? OriginalConstructors.Address(edition, index) : 0, FootX = 1, FootY = 1 }).ToArray();
            Frames = Enumerable.Range(0, count).Select(_ => new RiftTypeFrames([])).ToArray(); Shapes = Enumerable.Range(0, count).Select(_ => new SquidDisplayShape()).ToArray();
            var post = new RawPostPop(Pool, Types, new RawPostPopState { Suppressed = true });
            Unpop = new(Pool, Hash, Spots); Pop = new(Pool, Hash, Spots, postPop: post); var destroy = new RawSquidDestroy(Pool, Unpop, Types);
            Host = new(Pool, Types, Kernel, destroy, Clock, asset: new() { Emit = ExternalDeletion.Add, Selected = () => 0 });
            Factory = new(Pool, Types); Host.BindFormFactory(Factory); State = new(Clock);
            int type = priest ? 158 : 134; var codes = new List<RiftFrameCode>();
            // 방향별 두 실제 프레임이 각각 Run.first/count를 구성한다.
            for (int direction = 0; direction < 8; direction++) { codes.Add(new((byte)('A' + direction), (byte)'P', 1, 0)); codes.Add(new((byte)('A' + direction), (byte)'P', 2, 0)); }
            Frames[type] = new(codes); Shapes[type].Loaded = true; Shapes[type].FrameCount = 16;
            // hotspot과 다른 원문 walkStep 필드로 혼동을 검출한다.
            for (int index = 0; index < 16; index++) Shapes[type].Frames.Add(new() { CellWidth = 1, CellHeight = 1, WalkStepX = .3f, WalkStepY = .2f, CellHotspotX = 2, CellHotspotY = 3 });
            Parent = Factory.Create((uint)type, SidAllocation.Client); Assert.Equal(5, Parent); Assert.Equal(RawPopResult.Registered, Pop.Pop(Parent, Types[type], 1, 1, 10, 20)); Assert.Equal(6, Pool.Allocate(SidAllocation.Client));
            var next = new RawPathProcess(Pool, Hash, Types, Frames, Shapes, Spots, State, Hooks(), priest, flying);
            next.Runtime.SetPath([new(10, 20), new(15, 20), new(17, 22)], [0, 0, 2], [0, 1, 3]);
            Process = RawPathProcess.Attach(Host, next, Parent, 0x10)!; Assert.Equal(7, Process.Form()); Trace.Clear();
        }
        /// <summary>가상 응답은 관측 입력이고 공간/Host 삭제는 실제 구현을 사용한다.</summary>
        public RawPathProcessHooks Hooks() => new()
        {
            Unpop = (sid, flags) => { if (!WillDelete) Trace.Add($"unpop,{flags}"); Unpop.Unpop(sid, Types[Pool.Slot(sid)[10]], flags); },
            Pop = (sid, point, flags) => { if (!WillDelete) Trace.Add($"pop,{BitConverter.SingleToUInt32Bits(point.X)},{BitConverter.SingleToUInt32Bits(point.Y)},{flags}"); Assert.Equal(RawPopResult.Registered, Pop.Pop(sid, Types[Pool.Slot(sid)[10]], 1, 1, point.X, point.Y, flags)); },
            Repop = (sid, flags) => { if (!WillDelete) Trace.Add($"repop,{flags}"); Assert.Equal(RawPopResult.Registered, Pop.Pop(sid, Types[Pool.Slot(sid)[10]], 1, 1, BitConverter.UInt32BitsToSingle(Get(Pool.Slot(sid), 14)), BitConverter.UInt32BitsToSingle(Get(Pool.Slot(sid), 18)), flags)); },
            Halt = sid => { Trace.Add($"halt,{HaltCode}"); if (HaltCode == 11) WillDelete = true; AfterHalt?.Invoke(sid); return HaltCode; },
            Arrival = (sid, first, second) => { Trace.Add($"arrival,{first},{second},{ReturnCode}"); if (ReturnCode != 1002) WillDelete = true; if (ReturnCode == 1001) Host.Kill(Process, 0); AfterArrival?.Invoke(sid); return ReturnCode; },
            SoundId = _ => 321,
            StepSound = (_, x, y, sound) => Trace.Add($"sound,{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)},{sound}"),
            ObservationStamp = () => 0x12345678,
            Assertion = (expression, file, line) => { if (Diagnostic is null) throw new InvalidOperationException("검사 진단 미연결"); Diagnostic(expression, file, line); },
            ClearBlocked = (_, _, _, _) => Trace.Add("clearBlocked"),
            Cleanup = (runtime, _, flags) => { Deleted = true; FinalImage = runtime.Bytes.ToArray(); if (ReturnCode != 1001 || HaltCode == 11) Trace.Add($"kill,{flags}"); },
        };
        /// <summary>살아 있는 이미지 또는 실제 Cleanup 당시 이미지의 같은 필드를 읽는다.</summary>
        public string Fields()
        {
            ReadOnlySpan<byte> raw = Pool.Slot(Parent), image = Deleted ? FinalImage : Process.Runtime.Bytes.Span; RawPathLayout layout = Process.Runtime.Layout;
            return string.Join(',', Get(raw, 14), Get(raw, 18), Edition == OriginalEdition.Patch1078 ? Get(raw, 36) : raw[34], Get(image, layout.Current), Get(image, layout.Angle), Get(image, layout.Finished), Deleted ? 1u : 0u);
        }
        /// <summary>원문 효과 구분자와 빈 목록을 보존한다.</summary>
        public string Events() => Trace.Count == 0 ? "-" : string.Join(';', Trace);
        /// <summary>나머지 실제 공간/장치보다 먼저 프로세스를 반납한다.</summary>
        public void Dispose() => Kernel.Clear();
    }

    /// <summary>실제 Kernel의 PATCH 두 클래스60관측과 좌표/프레임/전체 효과가 같다.</summary>
    [Fact]
    public void NativePatchKernelFrames60()
    {
        string[][] rows = Rows("pathruntime-x86.tsv").Where(row => row[0] == "frame" && row[1] == "originals").ToArray(); Assert.Equal(60, rows.Length);
        // 매 원문 행은 독립 초기 공간과 실제 Form7을 사용한다.
        foreach (string[] row in rows) { using var scene = new Scene(Edition(row[1]), U(row[2]) != 0); scene.Process.Runtime.RestoreStep(unchecked((int)U(row[3])), F(row[5])); scene.Clock.Now = D(row[4]); scene.Kernel.RunFrame(); Assert.Equal(row[6], scene.Fields()); Assert.Equal(row[7], scene.Events()); }
    }
    /// <summary>PATCH 세 도착 응답/두 Halt의12관측과 실제 커널 생존이 같다.</summary>
    [Fact]
    public void NativePatchArrival12OwnsActualSelfDeletion()
    {
        string[][] rows = Rows("pathruntime-x86.tsv").Where(row => row[0] == "arrival" && row[1] == "originals").ToArray(); Assert.Equal(12, rows.Length);
        // 1001은 같은 Host.Kill이 삭제하고 결과 뒤에는 저장 이미지만 읽는다.
        foreach (string[] row in rows) { using var scene = new Scene(Edition(row[1]), U(row[2]) != 0); scene.ReturnCode = U(row[3]); scene.HaltCode = U(row[4]); scene.Process.Runtime.RestoreStep(3, 0); scene.Clock.Now = 3; scene.Process.Arrive(); Assert.Equal(row[5], scene.Fields()); Assert.Equal(row[6], scene.Events()); Assert.Equal(scene.ReturnCode == 1002 && scene.HaltCode == 10, scene.Kernel.Size() != 0); }
    }
    /// <summary>비행63관측의20ms 예약과 Halt 선행 순서를 모두 대조한다.</summary>
    [Fact]
    public void NativeFlyFrame63()
    {
        string[][] rows = Rows("flypath-x86.tsv").Where(row => row[0] == "frame").ToArray(); Assert.Equal(63, rows.Length);
        // 예약값과 unordered 시각도 원문 비트 그대로 사용한다.
        foreach (string[] row in rows) { using var scene = new Scene(OriginalEdition.Patch1078, flying: true); scene.Process.Runtime.RestoreStep(unchecked((int)U(row[1])), 0); scene.Clock.Now = D(row[2]); scene.Process.Runtime.NextFrame = D(row[3]); scene.Process.RunFrame(); Assert.Equal(row[4], scene.Fields()); Assert.Equal(row[5], scene.Events()); ulong next = BitConverter.DoubleToUInt64Bits(scene.Process.Runtime.NextFrame); Assert.Equal(U(row[6]), (uint)next); Assert.Equal(U(row[7]), (uint)(next >> 32)); }
    }
    /// <summary>미연결 탐색의 명시 실패와 실제0x5105 동결 등록을 검사한다.</summary>
    [Fact]
    public void MissingSearchAndActualFreezePreserveEarlierState()
    {
        using var scene = new Scene(OriginalEdition.Patch1078, true); int surface = scene.Factory.Create(83, SidAllocation.Client); scene.Hash.Cell(0, 10, 20) = (ushort)surface;
        Assert.Throws<InvalidOperationException>(() => scene.Process.InitializePath(scene.Parent, new(15, 20), new(10, 20))); Assert.Equal(1, scene.Kernel.Size()); Assert.Equal(0, scene.Process.Runtime.Current);
        scene.Types[158].Flags2 |= 0x10000; Assert.NotNull(scene.Host.AddRegular(scene.Parent, 0x5105, 100)); scene.Clock.Now = .5; scene.Kernel.RunFrame(); Assert.Equal(0, scene.Process.Runtime.Current); Assert.Equal("-", scene.Events());
        Assert.Equal(0, RawPathSurface.Cell(.0001f)); Assert.Equal(1f, .0001f + RawPathSurface.CellBias);
    }
}
