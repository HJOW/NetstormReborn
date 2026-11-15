using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 현재 Graph 전체 물리와 콜백 재진입을 원본 관측에 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj GraphLiveCheckingTests·GraphDetachLiveTests의 원본608행과 실제 World 결선을 검사한다.</summary>
public sealed class GraphLiveCurrentTests
{
    /// <summary>원문 좌표/번호/행/stack과 외부 콜백의 명시 입력만 구성하는 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 원문 판본이다. CD는 기존50바이트/188TYPE 관리 하네스로 관측 필드를 투영한다.</summary>
        public readonly OriginalEdition Edition;
        /// <summary>같은 실제 원본 풀이다.</summary>
        public readonly RawSidPool Pool;
        /// <summary>현재 일반 해시/표면 지도다.</summary>
        public readonly RawSquidHash Hash = new();
        /// <summary>현재 내부 bit8 입력이다.</summary>
        public readonly byte[] Spots = new byte[65536];
        /// <summary>현재 TYPE와 독립 snapshot 입력이다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>원본 AP 코드 한 개씩을 제공한다.</summary>
        public readonly IReadOnlyList<FrameCode>[] Frames = Enumerable.Range(0, 188).Select(_ => (IReadOnlyList<FrameCode>)Array.Empty<FrameCode>()).ToArray();
        /// <summary>원문 callback이 직접 바꾸는 장기 기록·전체 stack이다.</summary>
        public readonly RawGraphStorage Storage = new();
        /// <summary>원문과 같은 실제 할당5..10이다.</summary>
        public readonly int[] Ids = new int[6];
        /// <summary>위 물리를 직접 빌리는 제품 실행기다.</summary>
        public readonly RawGraph Graph;
        /// <summary>현재 전체 checking DWORD다.</summary>
        public uint Checking;
        /// <summary>현재 활성 DWORD다.</summary>
        public uint Enabled = 1;
        /// <summary>현재 분할 정책 DWORD다.</summary>
        public uint Rebuild;
        /// <summary>원문 콜백 변경 종류다.</summary>
        public readonly uint Action;
        /// <summary>현재 장면 이름이다.</summary>
        public readonly string Name;
        /// <summary>현재 로그 호출 횟수다.</summary>
        private uint _traces;
        /// <summary>현재 시계 호출 횟수다.</summary>
        private uint _clocks;
        /// <summary>원문 동기 진단/로그/시계의 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>현재 판본 graph BYTE 위치다.</summary>
        private int Offset => Edition == OriginalEdition.Patch1078 ? 30 : 28;
        /// <summary>삭제 장면과 Add/Flood 장면을 구분한다.</summary>
        private readonly bool _detach;

        /// <summary>기대값을 계산하지 않고 원본 실행기에 넣었던 명시 입력만 옮긴다.</summary>
        public Scene(string[] row, bool detach)
        {
            Edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072; Pool = new(32768, edition: Edition);
            Checking = X86Fixture.UInt(row[2]); Action = X86Fixture.UInt(row[3]); _detach = detach;
            string[] key = row[1].Split(':'); Name = key[0]; if (detach) { Rebuild = X86Fixture.UInt(key[1]); Enabled = Name == "Disabled" ? 0u : 1u; }
            (float X, float Y)[] positions = [(20, 20), (19, 20), (21, 20), (22, 20), (20, 21), (20, 19)];
            byte[] numbers = [row[1] == "Flood" ? (byte)3 : RawGraph.Invalid, 3, 4, 4, 5, 6];
            // 원문 번호·타입·발자국·코드와 실제 위치를 순서대로 준비한다.
            for (int index = 0; index < Ids.Length; index++)
            {
                Ids[index] = Pool.Allocate(SidAllocation.Client); Assert.Equal(index + 5, Ids[index]); byte type = (byte)(75 + index);
                Types[type].FootX = Types[type].FootY = 1; Types[type].Flags1 = OriginalTypeRecord.Surface; Types[type].Flags2 = 4;
                Frames[type] = new[] { new FrameCode('A', 'P', 1, 0) }; Span<byte> raw = Pool.RawSlot(Ids[index]); raw.Clear(); raw[10] = type;
                raw[11] = detach && index == 0 ? (byte)2 : (byte)0;
                raw[Offset] = detach ? (Name == "Lookup" && index != 0 ? (byte)4 : (byte)3) : numbers[index];
                float x = index == 0 && Name == "AddZero" ? 0 : positions[index].X, y = Name == "Lookup" && index == 5 ? 20 : positions[index].Y;
                BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y); Hash.Cell(0, (int)x, (int)y) = (ushort)Ids[index];
            }
            if (Name == "AddVoid") Pool.RawSlot(Ids[0])[11] = 4;
            if (Name == "Lookup") BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(Ids[5])[4..], (ushort)Ids[0]);
            if (Name == "Missing") Hash.Cell(0, 20, 20) = 0;
            Types[81].FootX = Types[81].FootY = 3; Types[81].Flags1 = OriginalTypeRecord.Surface; Types[81].Flags2 = 8; Frames[81] = new[] { new FrameCode('A', 'P', 1, 0) };
            // 미사용 행의 reserved도 고유한 원문 입력이다.
            for (int index = 0; index < Storage.Records.Length; index++) Storage.Records[index].Reserved = (short)(100 + index);
            if (detach) { Storage.Records[3].Surfaces = 6; Storage.Records[3].InUse = Name == "Inactive" ? (short)0 : (short)2; Storage.Records[4].Surfaces = 6; Storage.Records[4].InUse = 1; }
            else
            {
                // 원문 장부의 불일치를 보정하지 않고 그대로 입력한다.
                foreach ((int index, short surfaces, short used) in new[] { (3, (short)2, (short)2), (4, (short)2, (short)1), (5, (short)1, (short)1), (6, (short)1, (short)1), (7, (short)0, (short)1) })
                    Storage.Records[index] = new() { Surfaces = surfaces, InUse = used, Reserved = (short)(100 + index) };
            }
            Storage.Records[254].Surfaces = Storage.Records[254].InUse = detach && Action == 5 ? (short)0 : (short)0x7dfd;
            // 전체 오래된 stack 입력은 사용하지 않는 끝 DWORD까지 고유하게 둔다.
            for (int index = 0; index < Storage.Stack.Length; index++) Storage.Stack[index] = 0xa0000000u + (uint)index;
            Graph = new(Pool, Hash, Spots, Types, Storage, Frames, GraphRecovery.FullPool);
            Graph.BindDiagnostics(new() { Checking = new(() => Checking), Assertion = AssertCurrent }); Graph.BindTrace(Trace);
            if (detach) { Graph.BindDetachState(new(() => Enabled), new(() => Rebuild)); Graph.BindClock(Clock); }
        }

        /// <summary>선택된 원문 진단 뒤 지정된 현재 행·checking·슬롯 입력만 바꾼다.</summary>
        private void AssertCurrent(string expression, string file, uint line)
        {
            Events.Add($"a:{line}:{expression}:{file}");
            if (_detach)
            {
                if (line == 0x59 && Action is 9 or 10) { Pool.RawSlot(Ids[0])[11] = 1; if (Action == 10) Pool.RawSlot(Ids[0])[10] = 82; }
                if (Action is 2 or 5 && line == 0x68) { Checking = 0; if (Action == 5) Enabled = 0; }
            }
            else if (Action == 1)
            {
                Checking = 0; Storage.Records[3] = new() { Surfaces = 9, InUse = 0, Reserved = -310 }; Pool.RawSlot(Ids[0])[11] = 0;
                BinaryPrimitives.WriteSingleLittleEndian(Pool.RawSlot(Ids[0])[14..], 20f); BinaryPrimitives.WriteSingleLittleEndian(Pool.RawSlot(Ids[0])[18..], 20f);
            }
            else if (Action == 2)
            {
                Pool.RawSlot(Ids[0])[11] = 0;
                if (line is 0x100 or 0xc2) { Pool.RawSlot(Ids[2])[Offset] = 5; Storage.Records[5].Surfaces = 3; }
                else if (line == 0x113) { Storage.Records[5].Surfaces = 1; Storage.Records[4].Surfaces = 77; }
            }
            else if (Action == 4 && line == 0x68) Checking = 0;
        }

        /// <summary>현재 로그 횟수 뒤 후속 후보·행·stack·지도만 원문 입력대로 바꾼다.</summary>
        private void Trace(int level, string text)
        {
            Events.Add($"l:{level}:{text[..text.IndexOf(' ')]}"); if (++_traces != 1) return;
            if (_detach)
            {
                if (Action == 1) { Pool.RawSlot(Ids[2])[Offset] = Pool.RawSlot(Ids[4])[Offset] = 3; Storage.Records[3] = new() { Surfaces = 91, InUse = 2, Reserved = -321 }; Storage.Stack[4095] = 0xdeadbeef; Rebuild = 1; }
                else if (Action == 2) Storage.Records[254].Surfaces = Storage.Records[254].InUse = 0;
                else if (Action == 4) Pool.RawSlot(Ids[2])[11] = 1;
                else if (Action == 6) Hash.Cell(0, 22, 20) = 0;
                else if (Action == 8) Rebuild = 0;
            }
            else if (Action == 3)
            {
                byte number = Pool.Slot(Ids[0])[Offset]; Pool.RawSlot(Ids[2])[Offset] = Pool.RawSlot(Ids[4])[Offset] = number;
                if (number < 251) Storage.Records[number].Surfaces = 91; Storage.Stack[4095] = 0xdeadbeef;
            }
            else if (Action == 4) Storage.Records[254].Surfaces = Storage.Records[254].InUse = 0;
            else if (Action == 5) Hash.Cell(0, 22, 20) = 0;
            else if (Action == 6) Pool.RawSlot(Ids[2])[11] = 1;
        }

        /// <summary>원문의 두 시각 읽기에서 지정된 현재 정책/위치만 바꾼다.</summary>
        private double Clock()
        {
            Events.Add("t"); _clocks++;
            if (_clocks == 1 && Action == 3)
            {
                Rebuild = 1; Enabled = 0; BinaryPrimitives.WriteSingleLittleEndian(Pool.RawSlot(Ids[0])[14..], 200f); BinaryPrimitives.WriteSingleLittleEndian(Pool.RawSlot(Ids[0])[18..], 200f);
                Storage.Records[3] = new() { Surfaces = 91, InUse = 2, Reserved = -321 };
            }
            else if (_clocks == 1 && Action == 7) Rebuild = 0;
            return 17.25;
        }

        /// <summary>255개 원문6BYTE 행을 padding 없이 직렬화한다.</summary>
        public byte[] Records()
        {
            byte[] bytes = new byte[255 * 6];
            // 각 행의 세 WORD를 같은 순서로 보존한다.
            for (int index = 0; index < Storage.Records.Length; index++)
            {
                GraphRecord record = Storage.Records[index]; BinaryPrimitives.WriteInt16LittleEndian(bytes.AsSpan(index * 6), record.Surfaces);
                BinaryPrimitives.WriteInt16LittleEndian(bytes.AsSpan(index * 6 + 2), record.InUse); BinaryPrimitives.WriteInt16LittleEndian(bytes.AsSpan(index * 6 + 4), record.Reserved);
            }
            return bytes;
        }
        /// <summary>사용/미사용4096DWORD 전체를 직렬화한다.</summary>
        public byte[] Stack() => Storage.Stack.SelectMany(BitConverter.GetBytes).ToArray();
        /// <summary>원문SID5..10 전체 슬롯을 비교한다. CD는 관리 하네스의 앞36BYTE만 원본 폭으로 투영한다.</summary>
        public byte[] Slots() => Ids.SelectMany(sid => Pool.Slot(sid)[..(Edition == OriginalEdition.Patch1078 ? 50 : 36)].ToArray()).ToArray();
    }

    /// <summary>긴 원본 바이트 차이는 처음 달라진 위치만 출력하고 기대값을 재계산하지 않는다.</summary>
    private static void SameHex(string expected, byte[] bytes, string context)
    {
        string actual = Convert.ToHexStringLower(bytes); int first = Enumerable.Range(0, Math.Min(expected.Length, actual.Length)).FirstOrDefault(index => expected[index] != actual[index], -1);
        Assert.True(expected == actual, $"{context}: 처음 다른16진수 위치={first}, 길이={expected.Length}/{actual.Length}");
    }

    /// <summary>실제 부모·중첩 진단/로그의 전체 기록·stack·슬롯과 사건을 원본168행에 대조한다.</summary>
    [Theory]
    [InlineData("graph-live-checking", 144)]
    [InlineData("graph-live-free-callback", 24)]
    public void NativeParentsAndCallbackMutations(string fixture, int expectedRows)
    {
        int observations = 0;
        // 모든 입력의 전체 물리와 현재 DWORD를 검사한다.
        foreach (string[] row in X86Fixture.Read(fixture))
        {
            var scene = new Scene(row, false); string result = "-";
            if (row[1].StartsWith("Add")) scene.Graph.Add(scene.Ids[0]); else if (row[1] == "Flood") result = scene.Graph.Flood(scene.Ids[0], 7).ToString(); else scene.Graph.Rebuild(row[1] == "Rebuild1");
            string context = string.Join('/', row.Take(4)); Assert.Equal(row[4], result); SameHex(row[5], scene.Records(), context + "/records"); SameHex(row[6], scene.Stack(), context + "/stack");
            SameHex(row[7], scene.Slots(), context + "/slots"); Assert.Equal(X86Fixture.UInt(row[8]), scene.Checking); Assert.Equal(row[9], scene.Events.Count == 0 ? "-" : string.Join('|', scene.Events)); observations++;
        }
        Assert.Equal(expectedRows, observations);
    }

    /// <summary>현재 삭제 정책·시계·값 원천과 영역 진단 뒤 free/retype를 원본440행에 대조한다.</summary>
    [Theory]
    [InlineData(false, 432)]
    [InlineData(true, 8)]
    public void NativeDetachAndRegionSetter(bool region, int expectedRows)
    {
        int observations = 0;
        // 원본이 낸 같은 실제SID의 모든 물리와 현재 정책을 대조한다.
        foreach (string[] row in X86Fixture.Read(region ? "graph-region-free" : "graph-detach-live"))
        {
            var scene = new Scene(row, true);
            if (region) { scene.Storage.Records[3].Surfaces = scene.Storage.Records[3].InUse = 0; scene.Spots[20 * 256 + 20] = 8; scene.Graph.InvalidateRegion(scene.Ids[0]); }
            else scene.Graph.Detach(scene.Ids[0], scene.Rebuild != 0, scene.Name == "Override" ? (byte)9 : (byte)1, scene.Name == "Override" ? (byte)81 : null);
            string context = string.Join('/', row.Take(4)); SameHex(row[5], scene.Records(), context + "/records"); SameHex(row[6], scene.Stack(), context + "/stack"); SameHex(row[7], scene.Slots(), context + "/slots");
            Assert.Equal(X86Fixture.UInt(row[8]), scene.Checking); Assert.Equal(row[9], scene.Events.Count == 0 ? "-" : string.Join('|', scene.Events));
            if (!region) { Assert.Equal(X86Fixture.UInt(row[10]), scene.Enabled); Assert.Equal(X86Fixture.UInt(row[11]), scene.Rebuild); } observations++;
        }
        Assert.Equal(expectedRows, observations);
    }

    /// <summary>실제 World Graph는 같은 장기 배열·FullPool 복구·Client 로그를 사용한다.</summary>
    [Fact]
    public void ActualWorldUsesSharedPhysicalGraphAndStorageTrace()
    {
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        types[74].Flags1 = OriginalTypeRecord.Surface; types[74].Flags2 = 4; types[74].FootX = types[74].FootY = 1;
        RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray(); frames[74] = new([new RiftFrameCode((byte)'A', (byte)'P', 1, 0)]);
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray());
        var traces = new List<string>(); storage.BindGraphTrace((level, text) => { Assert.Equal(0x20, level); traces.Add(text); });
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 83, storage, frames); RawWorldCore core = world.Core(); int sid = core.Pool.Allocate(SidAllocation.Client);
        core.Pool.RawSlot(sid).Clear(); core.Pool.RawSlot(sid)[10] = 74; core.Pool.RawSlot(sid)[30] = 254;
        BinaryPrimitives.WriteSingleLittleEndian(core.Pool.RawSlot(sid)[14..], 20); BinaryPrimitives.WriteSingleLittleEndian(core.Pool.RawSlot(sid)[18..], 20); core.Hash.Cell(0, 20, 20) = (ushort)sid;
        // 실제 표를 소진시켜 같은 현재 물리에서 복구하게 한다.
        for (int index = 0; index < RawGraph.Count; index++) storage.Graph.Records[index] = new() { InUse = 1, Reserved = (short)(100 + index) };
        core.Graph.Add(sid); Assert.Same(storage.Graph.Records, core.Graph.Records); Assert.Same(storage.Graph.Stack, core.Graph.FloodStack);
        Assert.Equal(1, core.Pool.Slot(sid)[30]); Assert.Equal(1, storage.Graph.Records[1].Surfaces); Assert.Equal(3, traces.Count);
        Assert.All(traces, text => Assert.EndsWith(" graphs in use\n", text));
    }

    /// <summary>영역 통지의 실제 두 후보·현재 기록·진단을 제공하며 최종 효과는 계산하지 않는다.</summary>
    private static (RawSidPool Pool, RawGraphStorage Storage, RawGraph Graph, int First, int Second, int Source) RegionScene()
    {
        var pool = new RawSidPool(32768); var hash = new RawSquidHash(); var spots = new byte[65536]; var storage = new RawGraphStorage(); RawGraphStorage.ResetTable(storage);
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        types[73].FootX = 2; types[73].FootY = 1; types[74].Flags1 = OriginalTypeRecord.Surface; types[74].FootX = types[74].FootY = 1;
        int first = pool.Allocate(SidAllocation.Client), second = pool.Allocate(SidAllocation.Client), source = pool.Allocate(SidAllocation.Client);
        // 발자국2칸과 각 한 칸 표면의 원문 raw 입력이다.
        foreach (int sid in new[] { first, second, source })
        {
            pool.RawSlot(sid).Clear(); pool.RawSlot(sid)[10] = sid == source ? (byte)73 : (byte)74;
            BinaryPrimitives.WriteSingleLittleEndian(pool.RawSlot(sid)[14..], sid == first ? 20 : 21); BinaryPrimitives.WriteSingleLittleEndian(pool.RawSlot(sid)[18..], 20);
        }
        pool.RawSlot(first)[30] = 3; pool.RawSlot(second)[30] = 4;
        storage.Records[3] = new() { Reserved = 971 }; storage.Records[4] = new() { Reserved = 972 }; storage.Records[5] = new() { Surfaces = 3, InUse = 1, Reserved = 973 };
        hash.Cell(0, 20, 20) = (ushort)first; hash.Cell(0, 21, 20) = (ushort)second; spots[20 * 256 + 20] = spots[20 * 256 + 21] = 8;
        return (pool, storage, new(pool, hash, spots, types, storage), first, second, source);
    }

    /// <summary>영역 진단이 바꾼 현재 행·checking·다음 후보 번호를 그대로 소비하며 Validate는 진단을 실행하지 않는다.</summary>
    [Fact]
    public void ActualRegionUsesCurrentRecordAndFollowingSlot()
    {
        var s = RegionScene(); uint checking = 0x80000000; int assertions = 0;
        s.Graph.BindDiagnostics(new() { Checking = new(() => checking), Assertion = (expression, file, line) =>
        {
            Assert.Equal("inUse", expression); Assert.Equal(@"\Ns\O\Graph.cpp", file); Assert.Equal(0x59u, line); assertions++;
            s.Storage.Records[3] = new() { Surfaces = 2, InUse = 1, Reserved = 991 }; s.Pool.RawSlot(s.Second)[30] = 5; checking = 0;
        } });
        byte[] before = s.Pool.Bytes.ToArray(); s.Graph.ValidatePostPop(s.Source, true, false); Assert.Equal(0, assertions); Assert.Equal(before, s.Pool.Bytes.ToArray());
        s.Graph.PostPop(s.Source, true, false); Assert.Equal(1, assertions); Assert.Equal(0u, checking);
        Assert.Equal((short)1, s.Storage.Records[3].Surfaces); Assert.Equal((short)991, s.Storage.Records[3].Reserved); Assert.Equal((short)2, s.Storage.Records[5].Surfaces);
        Assert.Equal(RawGraph.Invalid, s.Pool.Slot(s.First)[30]); Assert.Equal(RawGraph.Invalid, s.Pool.Slot(s.Second)[30]); Assert.All(s.Storage.Stack, value => Assert.Equal(0u, value));
    }

    /// <summary>선택된 영역 진단 장치가 없으면 첫 행/graph BYTE 쓰기 전에 멈추며 사전 검사는 장치를 요구하지 않는다.</summary>
    [Fact]
    public void MissingSelectedRegionDiagnosticKeepsFirstWriteBoundary()
    {
        var s = RegionScene(); s.Graph.BindDiagnostics(new() { Checking = new(0x80000000u) }); byte[] before = s.Pool.Bytes.ToArray();
        s.Graph.ValidatePostPop(s.Source, true, false); Assert.Throws<InvalidOperationException>(() => s.Graph.InvalidateRegion(s.Source));
        Assert.Equal(before, s.Pool.Bytes.ToArray()); Assert.Equal((short)0, s.Storage.Records[3].Surfaces); Assert.Equal((short)0, s.Storage.Records[3].InUse);
    }

    /// <summary>공유 생성은 기록/stack을 초기화하지 않으며 독립 snapshot과 명시 현재 TYPE의 소유권을 구분한다.</summary>
    [Fact]
    public void SharedConstructionPreservesEveryPhysicalValue()
    {
        var storage = new RawGraphStorage(); Array.Fill(storage.Records, new GraphRecord { Surfaces = -3, InUse = 7, Reserved = -91 }); Array.Fill(storage.Stack, 0xfedcba98);
        var pool = new RawSidPool(32768); var hash = new RawSquidHash(); var spots = new byte[65536];
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        var graph = new RawGraph(pool, hash, spots, types, storage); Assert.Same(storage.Records, graph.Records); Assert.Same(storage.Stack, graph.FloodStack);
        Assert.All(storage.Records, value => Assert.Equal((short)-3, value.Surfaces)); Assert.All(storage.Stack, value => Assert.Equal(0xfedcba98u, value));
        types[74].Flags1 = OriginalTypeRecord.Surface; Assert.Throws<ArgumentException>(() => graph.ValidateTypes(types)); graph.BindCurrentTypes(types); graph.ValidateTypes(types);
        graph.BindCurrentTypes([]); Assert.Throws<ArgumentException>(() => graph.ValidateTypes(types));
        Assert.Throws<ArgumentException>(() => graph.BindCurrentTypes(types.Take(187).ToArray()));
    }

    /// <summary>실제 삭제의 비활성 관문은 잘못된 this/정책 인자를 읽기 전에 자연 반환한다.</summary>
    [Fact]
    public void DisabledDetachSkipsUnselectedSourceAndPolicy()
    {
        var s = RegionScene(); byte[] before = s.Pool.Bytes.ToArray(); GraphRecord[] records = s.Storage.Records.ToArray();
        s.Graph.BindDetachState(new(false), new(false)); s.Graph.Detach(-1, removedSurfaces: 2);
        Assert.Equal(before, s.Pool.Bytes.ToArray()); Assert.Equal(records, s.Storage.Records);
    }
}
