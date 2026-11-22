using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Net;

// 기존 세 PE의5340개 관측과 실제 건설 Host 수신을 C# 출력으로 기대값을 만들지 않고 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>상태 codec의 모든 비트·현재 필드·captured Form과 실제 Kernel 수명을 검증한다.</summary>
public sealed class RawConstructionDataTests
{
    /// <summary>두 입력 DWORD의 비트를 산술 없이 double로 묶는다.</summary>
    private static double Time(uint low, uint high) => BitConverter.UInt64BitsToDouble(((ulong)high << 32) | low);
    /// <summary>전체 의미 필드를 독립 관측의 일곱 정수 형식으로 만든다.</summary>
    private static string Fields(ConstructionProcessFields fields)
    {
        ulong start = BitConverter.DoubleToUInt64Bits(fields.Start), end = BitConverter.DoubleToUInt64Bits(fields.End);
        return string.Join(':', new uint[] { fields.Dirty, (uint)start, (uint)(start >> 32), (uint)end, (uint)(end >> 32), fields.Sound, fields.Additional });
    }
    /// <summary>선택한 실제 본체가 소비하지 않을 공간/소리 경계만 준비한다. Source Replay의 같은 입력이다.</summary>
    internal static RawConstructionProcess Logic(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, ConstructionProcessState state, SquidProcessHost? host = null) =>
        new(pool, types, state, new()
        {
            Reserve = () => true, Effect = (_, _) => { }, LoadSound = _ => 0x15000040, ReserveSound = () => false, AttachSound = (_, _, _, _) => { },
            Attach = (_, _, _, _) => throw new InvalidOperationException("codec 래퍼의 미선택 게임 부착"),
            Refresh = () => { }, UpdateDisplay = _ => { }, RemoveContained = (_, _) => { }, PlayAt = (_, _, _, _) => { }, ReadySound = _ => 0, Play = _ => { },
            Unpop = (sid, _) => pool.RawSlot(sid)[11] |= 4, Repop = (sid, _) => pool.RawSlot(sid)[11] &= 0xfb,
            Push = (_, _, _, _) => { }, MaxHitPoints = _ => 100,
        }, host);

    /// <summary>실제 같은 풀/커널/건설 본체에 수신/래퍼를 붙이는 독립 장면이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>관측 입력과 같은 판본/24000슬롯의 물리다.</summary>
        public RawSidPool Pool { get; }
        /// <summary>실제 원문 생성자 주소를 갖는 현재 TYPE 표다.</summary>
        public OriginalTypeRecord[] Types { get; }
        /// <summary>실제 프로세스를 소유하는 Kernel이다.</summary>
        public Kernel Kernel { get; } = new();
        /// <summary>현재 같은 Form/PID/부모를 처리한다.</summary>
        public SquidProcessHost Host { get; }
        /// <summary>같은 현재 건설 시각/TYPE다.</summary>
        public ConstructionProcessState State { get; } = new();
        /// <summary>실제 건설 진행/완료 본체다.</summary>
        public RawConstructionProcess Body { get; }
        /// <summary>Source DataScene처럼 부모20000을 직접 입력하고 Host/진행 본체를 연결한다.</summary>
        public Scene(OriginalEdition edition)
        {
            Pool = new(24000, false, edition: edition);
            Types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(edition, i) }).ToArray();
            var destroy = new RawSquidDestroy(Pool, new RawUnpop(Pool, new RawSquidHash(), new byte[65536]), Types);
            Host = new(Pool, Types, Kernel, destroy, new()); Body = Logic(Pool, Types, State, Host);
            Span<byte> parent = Pool.RawSlot(20000); parent.Clear(); parent[10] = 100; parent[edition == OriginalEdition.Patch1078 ? 40 : 35] = 1;
        }
        /// <summary>게임 효과를 발생시키지 않고 Kernel의 실제 소유권만 결정적으로 반납한다.</summary>
        public void Dispose() => Kernel.DrainOwnedForShutdown();
    }

    /// <summary>각 PE의1780행 전체 필드·사건·부모·packet/raw를 독립 출력과 비교한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void WholeCodecMatchesIndependentPeObservations(string editionName)
    {
        OriginalEdition edition = editionName == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        var pool = new RawSidPool(24000, false, edition: edition); var state = new ConstructionProcessState(); int count = 0;
        // 세 PE의 기존 입력을 판본별로 읽으며 출력 패킷 생성 규칙은 재구현하지 않는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "constructiondata-x86.tsv")))
        {
            if (!line.StartsWith(editionName + "\t", StringComparison.Ordinal)) continue;
            string[] row = line.Split('\t'); Assert.Equal(9, row.Length);
            uint[] input = row[2].Split(',').Select(value => uint.Parse(value, CultureInfo.InvariantCulture)).ToArray(); Assert.Equal(18, input.Length);
            var fields = new ConstructionProcessFields { Dirty = (byte)input[0], Start = Time(input[5], input[6]), End = Time(input[7], input[8]), Sound = 0x15000040, Additional = (byte)input[9] };
            byte[] packet = Convert.FromHexString(row[6]); var events = new List<string>(); string output = "-"; state.ProcessType = input[16];
            int form = row[1] == "Receive" ? (ushort)input[1] : 90; Span<byte> raw = pool.RawSlot(form); raw.Fill(0xa7); raw[11] = (byte)input[12]; BinaryPrimitives.WriteUInt32LittleEndian(raw[18..], input[13]);
            if (row[1] == "Write") _ = RawConstructionData.Write(fields, packet, input[1], input[2], input[3] != 0, new() { Protocol = (byte)input[10], Message = (byte)input[11] });
            else if (row[1] == "Wrapper")
            {
                if (edition == OriginalEdition.Patch1078)
                {
                    using var scene = new Scene(edition);
                    if (input[1] == 6) Assert.NotNull(scene.Host.AddRegular(20000, 1, 0));
                    ConstructionProcess process = Assert.IsType<ConstructionProcess>(scene.Host.Attach(new ConstructionProcess(scene.Body, fields), 30, 20000, 0x10));
                    Assert.Equal((int)input[1], process.Form()); _ = RawConstructionData.WriteProcess(process, packet, input[4] != 0, input[3] != 0); fields = process.Fields;
                }
                else
                {
                    // CD Host의 삭제/171타입 경계는 미복원이다. 래퍼가 읽는 실제 객체 필드/식별자만 원문 입력으로 둔다.
                    var cdTypes = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToArray();
                    using var process = new ConstructionProcess(Logic(pool, cdTypes, state), fields);
                    process.SetIdentity((int)input[1], 20000, 0);
                    _ = RawConstructionData.WriteProcess(process, packet, input[4] != 0, input[3] != 0); fields = process.Fields;
                }
            }
            else if (row[1] is "Read" or "ReadNull") output = RawConstructionData.Read(row[1] == "ReadNull" ? null : fields, packet).ToString(CultureInfo.InvariantCulture);
            else
            {
                var hooks = new ConstructionDataHooks
                {
                    Allocate = () =>
                    {
                        events.Add("A:56"); if (input[15] == 1) state.ProcessType = 44;
                        if (input[15] == 2) { BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 91); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9), uint.MaxValue); }
                        return fields;
                    },
                    Attach = (_, type, parent, captured, flags) => events.Add($"J:{type}:{parent}:{captured}:{flags}"),
                    Lookup = pid => { events.Add($"K:{pid}"); return input[14] != 0 ? fields : null; },
                };
                new RawConstructionData(pool, state, hooks).Receive(packet);
            }
            Assert.Equal(row[3], events.Count == 0 ? "-" : string.Join(';', events)); Assert.Equal(row[4], Fields(fields)); Assert.Equal(row[5], output);
            Assert.Equal(Convert.FromHexString(row[7]), packet);
            // CD의 원문 슬롯36BYTE와 관리 풀의50BYTE를 구별하고 원문 밖의 추가 영역도 보존하는지 확인한다.
            byte[] expectedRaw = Convert.FromHexString(row[8]); Assert.Equal(expectedRaw, raw[..expectedRaw.Length].ToArray());
            Assert.All(raw[expectedRaw.Length..].ToArray(), value => Assert.Equal(0xa7, value)); count++;
        }
        Assert.Equal(1780, count);
    }

    /// <summary>부족한 쓰기는 dirty/접두 전에 거부하고 읽기는 이미 완료한 앞 필드의 부분 효과를 보존한다.</summary>
    [Fact]
    public void TruncatedWriteAndReadKeepTheirDifferentMutationContracts()
    {
        var fields = new ConstructionProcessFields { Dirty = 2, Start = 1, End = 2, Sound = 77, Additional = 0x5a }; string before = Fields(fields);
        byte[] packet = Enumerable.Repeat((byte)0xcc, 30).ToArray();
        // 최대 길이30보다 짧은 모든 폭에서 force의 ff 선저장도 일어나지 않는다.
        for (int length = 0; length < 30; length++)
        {
            Assert.Throws<ArgumentException>(() => RawConstructionData.Write(fields, packet.AsSpan(0, length), 90, 20000, true));
            Assert.Equal(before, Fields(fields)); Assert.All(packet, value => Assert.Equal(0xcc, value));
        }
        packet[8] = 6; BinaryPrimitives.WriteUInt64LittleEndian(packet.AsSpan(9), 0x7ff8123456789abc);
        Assert.Throws<ArgumentException>(() => RawConstructionData.Read(fields, packet.AsSpan(0, 17)));
        Assert.Equal(0x7ff8123456789abcul, BitConverter.DoubleToUInt64Bits(fields.Start)); Assert.Equal(2, fields.End); Assert.Equal(2, fields.Dirty); Assert.Equal(77u, fields.Sound);
        packet[8] = 0x80; Assert.Equal(0u, RawConstructionData.Read(null, packet));
    }

    /// <summary>실제 수신 새 객체/기존 PID 갱신은 같은 Kernel과 명시한 서버 Form을 사용하며 잘린 부모는 부착되지 않는다.</summary>
    [Fact]
    public void OwnedReceiveUsesSameActualHostAndRejectsWideParentBeforeAttach()
    {
        using var scene = new Scene(OriginalEdition.Patch1078); var receiver = new RawConstructionData(scene.Pool, scene.State, scene.Body, scene.Host);
        var fields = new ConstructionProcessFields { Dirty = 255, Start = 0, End = 8, Sound = 77, Additional = 0x5a }; byte[] packet = new byte[30];
        uint form = RawSidPool.ServerFirst + 50u; _ = RawConstructionData.Write(fields, packet, form, 20000, true); receiver.Receive(packet);
        ConstructionProcess process = Assert.IsType<ConstructionProcess>(scene.Host.FromForm((int)form)); Assert.Equal((int)form, process.Form()); Assert.Equal(20000, process.Parent());
        Assert.Equal(1, scene.Kernel.Size()); Assert.Equal(255, process.Fields.Dirty); Assert.Equal(8, process.Fields.End); Assert.Equal(0u, process.Fields.Sound); Assert.Equal(0x5a, process.Fields.Additional);
        var partial = new ConstructionProcessFields { Dirty = 4, End = 15 }; int length = RawConstructionData.Write(partial, packet, form, 0, false);
        process.Fields.Dirty = 0x88; receiver.Receive(packet.AsMemory(0, length)); Assert.Equal(15, process.Fields.End); Assert.Equal(0x88, process.Fields.Dirty);
        uint otherForm = form + 1; fields.Dirty = 255; _ = RawConstructionData.Write(fields, packet, otherForm, 0x1004e20, true);
        Assert.Throws<ArgumentOutOfRangeException>(() => receiver.Receive(packet)); Assert.Equal(1, scene.Kernel.Size()); Assert.Null(scene.Host.FromForm((int)otherForm));
    }

    /// <summary>실제0x37표 등록과 버전/기록/깊이 경유 수신이 같은 서버 Form/현재 PID에 도달한다.</summary>
    [Fact]
    public void RegisteredPacketDispatchReceivesNewAndPartialConstruction()
    {
        using var scene = new Scene(OriginalEdition.Patch1078); var receiver = new RawConstructionData(scene.Pool, scene.State, scene.Body, scene.Host);
        var events = new List<string>();
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, new()
        {
            Now = () => 42, Record = (connection, _, length) => events.Add($"{connection.Depth}:{length}"),
            Terminate = _ => throw new IOException("미선택 terminate"), Close = _ => throw new IOException("미선택 close"), Release = _ => throw new IOException("미선택 release"),
        });
        dispatch.ObservePacket(0x37, (_, _) => throw new IOException("제거되어야 할 이전 observer")); dispatch.Version(0x37, 9, 7);
        dispatch.RegisterConstructionData(receiver); var registration = dispatch.Registration(0x37);
        Assert.Equal(1, registration.Version); Assert.Equal(0, registration.Mismatch); Assert.Equal(ushort.MaxValue, registration.Length);
        var connection = new PacketConnection { Sender = 17, Received = 11, Heartbeat = 22 };
        uint form = RawSidPool.ServerFirst + 50u; byte[] packet = new byte[30];
        var fields = new ConstructionProcessFields { Dirty = 255, Start = 0, End = 8, Additional = 0x5a };
        _ = RawConstructionData.Write(fields, packet, form, 20000, true, new() { Protocol = 0x81, Message = 0x37 });
        Assert.True(dispatch.Dispatch(connection, packet)); ConstructionProcess process = Assert.IsType<ConstructionProcess>(scene.Host.FromForm((int)form));
        Assert.Equal((int)form, process.Form()); Assert.Equal(20000, process.Parent()); Assert.Equal(1, scene.Kernel.Size());
        Assert.Equal(42u, connection.Received); Assert.Equal(22u, connection.Heartbeat); Assert.Equal(0u, connection.Depth); Assert.Equal(new[] { "0:30" }, events);
        fields.Dirty = 4; fields.End = 15; int length = RawConstructionData.Write(fields, packet, form, 0, false);
        Assert.True(dispatch.Dispatch(connection, packet.AsMemory(0, length))); Assert.Equal(15, process.Fields.End); Assert.Equal(new[] { "0:30", "0:17" }, events);
    }
}
