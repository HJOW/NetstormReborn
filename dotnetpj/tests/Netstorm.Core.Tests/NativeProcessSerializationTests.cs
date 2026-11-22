using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 같은 Native Host의 기본 writer와 실제 공통 버퍼/현재 헤더/송신 예약을 확인한다.
namespace Netstorm.Core.Tests;

/// <summary>기존 Dais/Path 본체를 실제 장기 저장소 기본 전송으로 호출한다.</summary>
public sealed class NativeProcessSerializationTests
{
    /// <summary>표/진단과 실제 전송 콜백을 갖는 장기 저장소다. World는 만들지 않는다.</summary>
    private static RawSurfaceStorage Storage(Action<Memory<byte>>? broadcast = null)
    {
        var types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i), FootX = 1, FootY = 1 }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        storage.BindTransmissionDevices(new() { Broadcast = packet => broadcast?.Invoke(packet), Connected = _ => false, Send = (_, _) => { } });
        storage.BindGraphTrace((_, _) => { }); storage.BindDisplayDiagnostics(new StringBox(), (_, _, _) => { });
        storage.BindDisplayDevices(new() { View = () => new() { Zoom = 65536 }, Suppressed = () => false, Invalidate = (_, _) => { } });
        return storage;
    }

    /// <summary>실제 현재 Host 장치 앞 등록은 외부 Kernel 결합을 막지 않고 Packet 크기 계약은 현재 버퍼 범위다.</summary>
    [Fact]
    public void NativeDefaultTransmissionWaitsForExternalKernelAndChecksPacketBounds()
    {
        var storage = Storage(); var kernel = new Kernel(16); storage.Processes().BindDevices(kernel, new FrameTimeBox());
        ProcessFormTransmission device = storage.Processes().Host().Transmission()!; byte[] buffer = storage.GameAux().SquidTransmission.Buffer;
        buffer[0] = 37; Assert.Equal(new byte[] { 37 }, device.Packet!(1)); Assert.Empty(device.Packet(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => device.Packet(-1)); Assert.Throws<ArgumentOutOfRangeException>(() => device.Packet(buffer.Length + 1));
        Assert.Same(kernel, storage.Processes().Kernel()); Assert.False(storage.Borrowed());
    }

    /// <summary>최초 dirty·현재 헤더·부모/force의 조건과 같은 전체 버퍼의 미작성 tail을 보존한다.</summary>
    [Fact]
    public void DaisDefaultsReadCurrentHeaderAndPreserveUnwrittenTail()
    {
        var storage = Storage(); var process = new DaisProcess(); process.SetIdentity(72, 80, 0);
        BinaryPrimitives.WriteUInt64LittleEndian(process.Fields.Bytes.AsSpan(32), 0x123456789abcdef0);
        ProcessFormTransmission device = storage.Processes().Host().Transmission()!; byte[] buffer = storage.GameAux().SquidTransmission.Buffer;
        Array.Fill(buffer, (byte)0xa5); Assert.Equal(17, device.WriteData!(process, false));
        Assert.Equal(0x6a, buffer[1]); Assert.Equal(1, buffer[0]); Assert.Equal(72u, BinaryPrimitives.ReadUInt32LittleEndian(buffer.AsSpan(4)));
        Assert.Equal(0x123456789abcdef0ul, BinaryPrimitives.ReadUInt64LittleEndian(buffer.AsSpan(9))); Assert.Equal(0xa5, buffer[17]); Assert.Equal(0, process.Fields.Bytes[24]);
        byte[] first = device.Packet!(17); buffer[0] = 0x81; Assert.Equal(1, first[0]);
        storage.GameAux().DaisProcessHeader[0] = 0x70; storage.GameAux().DaisProcessHeader[1] = 0x81;
        Assert.Equal(21, device.WriteFullData!(process, 0x10000)); Assert.Equal(0x81, buffer[0]); Assert.Equal(0x70, buffer[1]);
        Assert.Equal(80u, BinaryPrimitives.ReadUInt32LittleEndian(buffer.AsSpan(9))); Assert.Equal(0xff, buffer[8]); Assert.Equal(0xa5, buffer[21]);
        Assert.Equal(9, device.WriteData(process, false)); Assert.Equal(0, buffer[8]);
    }

    /// <summary>같은 writer 뒤 길이는 양수 관문에만 사용하며 observer는 실제 전체2048BYTE를 수정할 수 있다.</summary>
    [Fact]
    public void NativeTransmitPassesSameWholeReservationAfterWriter()
    {
        RawSurfaceStorage? storage = null; int sends = 0;
        storage = Storage(packet => { sends++; Assert.Equal(2048, packet.Length); packet.Span[2047] = 37; Assert.Equal(37, storage!.GameAux().SquidTransmission.Buffer[2047]); });
        ProcessFormTransmission device = storage.Processes().Host().Transmission()!;
        int length = device.WriteData!(new DaisProcess(), false); device.Transmit!(0); Assert.Equal(0, sends);
        device.Transmit(length); Assert.Equal(1, sends); Assert.Equal(37, storage.GameAux().SquidTransmission.Buffer[2047]);
    }

    /// <summary>직렬화에 불필요한 게임 효과를 호출하면 실패하도록 실제 Path 객체의 외부 경계를 마련한다.</summary>
    private static RawPathProcess PathProcess(RawSurfaceStorage storage) => new(storage.Pool, storage.Hash, storage.Types, storage.Frames, storage.Shapes, storage.Spots,
        new RawPathProcessState(storage.Processes().State()), new()
        {
            Unpop = (_, _) => throw new InvalidOperationException("writer의 불필요한 Unpop"), Repop = (_, _) => throw new InvalidOperationException("writer의 불필요한 Repop"),
            Pop = (_, _, _) => throw new InvalidOperationException("writer의 불필요한 Pop"), Halt = _ => throw new InvalidOperationException("writer의 불필요한 Halt"),
            Arrival = (_, _, _) => throw new InvalidOperationException("writer의 불필요한 Arrival"), SoundId = _ => throw new InvalidOperationException("writer의 불필요한 SoundId"),
            StepSound = (_, _, _, _) => throw new InvalidOperationException("writer의 불필요한 Sound"), ClearBlocked = (_, _, _, _) => throw new InvalidOperationException("writer의 불필요한 Clear"),
            Cleanup = (_, _, _) => throw new InvalidOperationException("writer의 불필요한 Cleanup"),
        }, false);

    /// <summary>기존 PE의 profile0 PATCH32관측 전체2048BYTE가 일반/전체 Native 기본 writer와 모두 같다.</summary>
    [Fact]
    public void PathDefaultsMatchIndependentPeWholeBuffers()
    {
        var storage = Storage(); RawPathProcess process = PathProcess(storage); process.SetIdentity(32700, 5, 0); process.OnAttached();
        ProcessFormTransmission device = storage.Processes().Host().Transmission()!; byte[] buffer = storage.GameAux().SquidTransmission.Buffer; int cases = 0;
        // 독립 PE가 받은 프로필0의 장르/헤더/최초 시각 조합만 입력으로 재현한다. 기대 패킷은 계산하지 않는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "pathdata-x86.tsv")))
        {
            if (!line.StartsWith("originals\t0\t", StringComparison.Ordinal)) continue;
            string[] row = line.Split('\t'); uint genus = uint.Parse(row[2]); uint variant = uint.Parse(row[3]);
            storage.Types[74].Flags2 = genus; storage.Pool.RawSlot(5)[10] = 74;
            process.Runtime.SetPath([new(10, 20)], [0], [BitConverter.UInt64BitsToDouble(ulong.Parse(row[5]))]);
            process.Runtime.SetCommandFields(0x12340000, 0x87654321, 0xa5, default);
            RawPathDataHeader header = storage.GameAux().PathPacketHeader; header.PacketId = (byte)(0x30 + variant); header.PacketFlags = variant != 0 ? (byte)0x81 : (byte)0xfe; header.PacketLength = variant != 0 ? 0u : 0x8123u;
            byte fill = variant != 0 ? (byte)0xa5 : (byte)0x5a; byte[] expected = Convert.FromHexString(row[7]); int length = int.Parse(row[6]);
            Array.Fill(buffer, fill); Assert.Equal(length, device.WriteData!(process, true)); Assert.Equal(expected, buffer);
            Array.Fill(buffer, fill); Assert.Equal(length, device.WriteFullData!(process, uint.MaxValue)); Assert.Equal(expected, buffer); cases++;
        }
        Assert.Equal(32, cases);
    }

    /// <summary>다른 Pool의 Path는 어떤 접두도 쓰기 전에 거부하고 같은 풀의 미이식 파생도 명시 실패한다.</summary>
    [Fact]
    public void ForeignPathAndUnknownDerivedCannotUseRecursiveFallback()
    {
        var storage = Storage(); RawPathProcess foreign = PathProcess(Storage()); byte[] buffer = storage.GameAux().SquidTransmission.Buffer; Array.Fill(buffer, (byte)0xa5);
        Assert.Throws<ArgumentException>(() => storage.Processes().Host().Transmission()!.WriteData!(foreign, false)); Assert.All(buffer, value => Assert.Equal(0xa5, value));
        Assert.Throws<InvalidOperationException>(() => storage.WriteDefaultForm(new SquidProcess(), false, 0)); Assert.All(buffer, value => Assert.Equal(0xa5, value));
    }

    /// <summary>현재 Runtime이 부분 전송을 제공하면 기본 writer로 빈 슬롯을 채우지 않으며 반납 뒤 기본으로 돌아온다.</summary>
    [Fact]
    public void ActualNativeHostKeepsWholeCurrentTransmissionSelection()
    {
        var storage = Storage(); SquidProcessHost host = storage.Processes().Host();
        var runtime = new ProcessHostRuntimeBindings { Destroy = storage.BaseDestroy(), Transmit = new() { Packet = _ => [99] } };
        using (RawProcessHostStorage.Use lease = storage.Processes().Bind(runtime))
        {
            Assert.Null(host.Transmission()!.WriteData); Assert.Equal(new byte[] { 99 }, host.Transmission()!.Packet!(1));
        }
        Assert.Equal(17, host.Transmission()!.WriteData!(new DaisProcess(), false));
    }

    /// <summary>실제 Dais 부착 writer/observer와 Form→현재 PID→전체 writer→대상 버퍼의 실제 Host 경로를 통과한다.</summary>
    [Fact]
    public void AttachedDaisUsesActualPidWriterAndCommonFormPacket()
    {
        int sends = 0; var storage = Storage(_ => sends++);
        int parent = storage.Pool.Allocate(SidAllocation.Server); storage.Pool.RawSlot(parent).Clear(); storage.Pool.RawSlot(parent)[10] = 74;
        var kernel = new Kernel(16); storage.Processes().BindDevices(kernel, new FrameTimeBox()); SquidProcessHost host = storage.Processes().Host();
        var dais = new DaisProcess(); Assert.Same(dais, host.Attach(dais, storage.GameAux().DaisProcessTypeBox.Value, parent, 0x80));
        Assert.Equal(1, sends); Assert.Same(dais, host.FromForm(dais.Form())); Assert.Same(dais, kernel.Get(kernel.Find(dais)));
        byte[] packet = Enumerable.Repeat((byte)0xa5, 2048).ToArray();
        Assert.Equal(21, host.WriteFormPacket(dais.Form(), 1, packet, storage.GameAux().SquidTransmission));
        Assert.Equal((uint)dais.Form(), BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(4)));
        Assert.Equal((uint)parent, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(9))); Assert.Equal(0xa5, packet[21]); Assert.Equal(1, sends);
    }

    /// <summary>Gun flags0의 최초 writer와 부착 뒤 통지 순서/현재 헤더/전체 FormPacket을 같은 실제 Host에서 확인한다.</summary>
    [Fact]
    public void AttachedGunUsesCurrentNativeHeaderAndWriterBeforeAttachedNotice()
    {
        int sends = 0; var storage = Storage(_ => sends++); storage.Types[128].Group = 1;
        int parent = storage.Pool.Allocate(SidAllocation.Server); storage.Pool.RawSlot(parent).Clear(); storage.Pool.RawSlot(parent)[10] = 128;
        var kernel = new Kernel(16); storage.Processes().BindDevices(kernel, new FrameTimeBox()); SquidProcessHost host = storage.Processes().Host();
        var state = new GunProcessState { Boss = false, Now = 10, SharedWindArcherType = storage.GameAux().WindArcherTypeBox, SharedThunderCannonType = storage.GameAux().ThunderCannonTypeBox, SharedRainCannonType = storage.GameAux().RainCannonTypeBox };
        var logic = new RawGunProcess(storage.Pool, storage.Types, state, new());
        GunProcess process = Assert.IsType<GunProcess>(RawGunProcess.CreateGunProcess(host, logic, (ushort)parent, storage.GameAux().GunProcessTypeBox.Value));
        Assert.Equal(1, sends); Assert.Equal(0, process.Fields.Dirty()); Assert.Equal(0u, process.Fields.Stage()); Assert.Equal(process.Form(), process.Fields.Form()); Assert.Equal(parent, process.Fields.Parent());
        byte[] buffer = storage.GameAux().SquidTransmission.Buffer; Assert.Equal(31, BinaryPrimitives.ReadUInt16LittleEndian(buffer.AsSpan(2))); Assert.Equal(0x3a, buffer[1]); Assert.Equal(0xff, buffer[8]);
        kernel.RunFrame(); Assert.Equal(1u, process.Fields.Stage());
        BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(28), 3); process.Fields.Bytes[8] = 8; host.RefreshForm(process);
        Assert.Equal(2, sends); Assert.Equal(3u, process.Fields.Stage()); Assert.Equal(13, BinaryPrimitives.ReadUInt16LittleEndian(buffer.AsSpan(2))); Assert.Equal(3u, BinaryPrimitives.ReadUInt32LittleEndian(buffer.AsSpan(9)));
        byte[] header = storage.GameAux().GunProcessHeader; header[0] = 0x73; header[1] = 0x81; storage.GameAux().WindArcherTypeBox.Value = 141; storage.GameAux().GunProcessTypeBox.Value = 44; storage.GameAux().ResetForGeneralWorld();
        Assert.Same(header, storage.GameAux().GunProcessHeader); Assert.Equal(141u, state.ReadWindArcherType()); Assert.Equal(44u, storage.GameAux().GunProcessTypeBox.Value);
        byte[] target = Enumerable.Repeat((byte)0xa5, 2048).ToArray(); Assert.Equal(31, host.WriteFormPacket(process.Form(), 1, target, storage.GameAux().SquidTransmission));
        Assert.Equal(0x81, target[0]); Assert.Equal(0x73, target[1]); Assert.Equal((uint)process.Form(), BinaryPrimitives.ReadUInt32LittleEndian(target.AsSpan(4))); Assert.Equal((uint)parent, BinaryPrimitives.ReadUInt32LittleEndian(target.AsSpan(9)));
        Assert.Equal(0xa5, target[31]); Assert.Equal(3u, process.Fields.Stage()); Assert.Equal(2, sends); Assert.Same(process, host.FromForm(process.Form())); Assert.Same(process, kernel.Get(kernel.Find(process)));
        kernel.DrainOwnedForShutdown();
    }

    /// <summary>기본 표 flags8 부착 뒤 Rain 전문화와 같은 Kernel/현재 Native writer의 전체45BYTE를 확인한다.</summary>
    [Fact]
    public void AttachedFlyerUsesActualConstructorOrderKernelAndCurrentNativeWriter()
    {
        int sends = 0, frames = 0; var storage = Storage(_ => sends++); int parent = storage.Pool.Allocate(SidAllocation.Server); storage.Pool.RawSlot(parent).Clear(); storage.Pool.RawSlot(parent)[10] = 144;
        var kernel = new Kernel(16); storage.Processes().BindDevices(kernel, new FrameTimeBox()); SquidProcessHost host = storage.Processes().Host(); FlyerProcess? process = null; var events = new List<string>();
        var body = new RawFlyerProcess(storage.Pool, storage.Types, new(() => true, () => 42, storage.GameAux().FlyerProcessTypeBox), new()
        {
            Attach = (fields, into, type, flags) =>
            {
                Assert.NotNull(process); Assert.Same(process.Fields, fields); Assert.Equal(0x509cc8u, BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes)); Assert.Equal(8u, flags);
                Assert.Same(process, host.Attach(process, type, into, flags));
            },
            Frame = fields => { Assert.Same(process!.Fields, fields); frames++; }, Notify = (_, message, first, second) => events.Add($"notify:{message}:{first}:{second}"), ParentDying = _ => events.Add("parent"),
        });
        process = new(body, new FlyerProcessFields(0xa5)); body.Construct(process.Fields, (ushort)parent, 23, 0x3f400000, 0.5f);
        try
        {
            Assert.Equal(1, sends); Assert.Equal(31, storage.Pool.Slot(process.Form())[10]); Assert.Equal(process.Form(), process.Fields.Form()); Assert.Equal(parent, process.Fields.Parent()); Assert.Equal(0, process.Fields.Bytes[8]);
            byte[] common = storage.GameAux().SquidTransmission.Buffer; Assert.Equal(45, BinaryPrimitives.ReadUInt16LittleEndian(common.AsSpan(2))); Assert.Equal(1, common[0]); Assert.Equal(0x39, common[1]);
            RawFlyerProcess.SpecializeRain(process.Fields); kernel.RunFrame(); Assert.Equal(1, frames); process.Notify(2, 3, 4); process.OnParentDying(parent); Assert.Equal(new[] { "notify:2:3:4", "parent" }, events);
            storage.GameAux().FlyerProcessHeader[0] = 0x73; storage.GameAux().FlyerProcessHeader[1] = 0x81; storage.GameAux().FlyerProcessTypeBox.Value = 44; storage.GameAux().ResetForGeneralWorld(); Assert.Equal(44u, storage.GameAux().FlyerProcessTypeBox.Value);
            byte[] packet = Enumerable.Repeat((byte)0xa7, 64).ToArray(); Assert.Equal(45, host.WriteFormPacket(process.Form(), 1, packet, storage.GameAux().SquidTransmission)); Assert.Equal(0x81, packet[0]); Assert.Equal(0x73, packet[1]); Assert.Equal(0xa7, packet[45]);
            var copy = new FlyerProcessFields(); RawFlyerProcess.InitializeReceived(copy, 1); Assert.Equal((uint)parent, RawFlyerProcess.ReadData(copy, packet)); Assert.Equal(23, copy.Home()); Assert.Equal(42, BinaryPrimitives.ReadDoubleLittleEndian(copy.Bytes.AsSpan(32)));
            Assert.Same(process, host.FromForm(process.Form())); Assert.Same(process, kernel.Get(kernel.Find(process))); Assert.Equal(1, sends);
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }

    /// <summary>실제 훨리베이스 flags8 부착/Kernel/현재 헤더와 같은 Native 전체 상태 패킷을 연결한다.</summary>
    [Fact]
    public void AttachedSunAviaryUsesCurrentNativeHeaderAndSameHostFields()
    {
        int sends = 0; var storage = Storage(_ => sends++); int parent = storage.Pool.Allocate(SidAllocation.Server); storage.Pool.RawSlot(parent).Clear(); storage.Pool.RawSlot(parent)[10] = 72;
        var kernel = new Kernel(16); storage.Processes().BindDevices(kernel, new FrameTimeBox()); SquidProcessHost host = storage.Processes().Host();
        var body = new RawSunAviaryProcess(storage.Pool, new(() => true, () => 0, () => 42, storage.GameAux().SunAviaryProcessTypeBox, storage.GameAux().SunFlyerTypeBox), new()
        {
            SetFrame = (sid, frame) => BinaryPrimitives.WriteInt32LittleEndian(storage.Pool.RawSlot(sid)[36..], frame), Sound = (_, _, name) => Assert.Equal("apStart.wav", name),
        });
        var fields = new SunAviaryProcessFields(0xa5); body.Initialize(fields); var process = new SunAviaryProcess(body, fields);
        try
        {
            Assert.Same(process, host.Attach(process, storage.GameAux().SunAviaryProcessTypeBox.Value, parent, 8)); Assert.Equal(1, sends); Assert.Equal(0, process.Fields.Dirty()); Assert.Equal(process.Form(), process.Fields.Form()); Assert.Equal(parent, process.Fields.Parent()); Assert.Equal(0xff, fields.Dirty());
            byte[] common = storage.GameAux().SquidTransmission.Buffer; Assert.Equal(25, BinaryPrimitives.ReadUInt16LittleEndian(common.AsSpan(2))); Assert.Equal(1, common[0]); Assert.Equal(0x38, common[1]);
            kernel.RunFrame(); Assert.Equal(8u, process.Fields.Stage()); Assert.Equal(27u, BinaryPrimitives.ReadUInt32LittleEndian(storage.Pool.Slot(parent)[36..]));
            byte[] header = storage.GameAux().SunAviaryHeader; header[0] = 0x73; header[1] = 0x81; storage.GameAux().SunAviaryProcessTypeBox.Value = 44; storage.GameAux().SunFlyerTypeBox.Value = 111; storage.GameAux().ResetForGeneralWorld();
            Assert.Same(header, storage.GameAux().SunAviaryHeader); Assert.Equal(44u, storage.GameAux().SunAviaryProcessTypeBox.Value); Assert.Equal(111u, storage.GameAux().SunFlyerTypeBox.Value);
            byte[] packet = Enumerable.Repeat((byte)0xa7, 2048).ToArray(); Assert.Equal(25, host.WriteFormPacket(process.Form(), 1, packet, storage.GameAux().SquidTransmission)); Assert.Equal(0x81, packet[0]); Assert.Equal(0x73, packet[1]); Assert.Equal(0xa7, packet[25]);
            var copy = new SunAviaryProcessFields(); Assert.Equal((uint)parent, RawAviaryProcessData.Read(copy.Bytes, packet)); Assert.Equal(42, copy.Time()); Assert.Equal(8u, copy.Stage()); Assert.Equal(1, sends); Assert.Same(process, host.FromForm(process.Form())); Assert.Same(process, kernel.Get(kernel.Find(process)));
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }

    /// <summary>실제 건설 부착/현재 헤더/일반 준비 보존과 같은 Host 전체 FormPacket을 새 codec에 연결한다.</summary>
    [Fact]
    public void AttachedConstructionUsesCurrentHeaderAndSameNativeFormPacket()
    {
        int sends = 0; var storage = Storage(_ => sends++); SquidProcessHost host = storage.Processes().Host();
        int parent = storage.Pool.Allocate(SidAllocation.Server); storage.Pool.RawSlot(parent).Clear(); storage.Pool.RawSlot(parent)[10] = 74;
        RawConstructionProcess logic = RawConstructionDataTests.Logic(storage.Pool, storage.Types, new(), host);
        var process = new ConstructionProcess(logic, new() { Dirty = 255, Start = 1, End = 8, Sound = 77, Additional = 0x5a });
        Assert.Same(process, host.Attach(process, storage.GameAux().ConstructionProcessTypeBox.Value, parent, 0x80));
        Assert.Equal(1, sends); Assert.Equal(0, process.Fields.Dirty);
        byte[] buffer = storage.GameAux().SquidTransmission.Buffer; Assert.Equal(1, buffer[0]); Assert.Equal(0x37, buffer[1]);
        var copy = new ConstructionProcessFields(); Assert.Equal((uint)parent, RawConstructionData.Read(copy, buffer)); Assert.Equal(1, copy.Start); Assert.Equal(8, copy.End); Assert.Equal(0x5a, copy.Additional);
        byte[] header = storage.GameAux().ConstructionHeader; header[0] = 0x73; header[1] = 0x81; storage.GameAux().ResetForGeneralWorld();
        Assert.Same(header, storage.GameAux().ConstructionHeader); Assert.Equal(new byte[] { 0x73, 0x81 }, header);
        byte[] target = Enumerable.Repeat((byte)0xa5, 2048).ToArray();
        Assert.Equal(30, host.WriteFormPacket(process.Form(), 1, target, storage.GameAux().SquidTransmission));
        Assert.Equal(0x81, target[0]); Assert.Equal(0x73, target[1]); Assert.Equal((uint)process.Form(), BinaryPrimitives.ReadUInt32LittleEndian(target.AsSpan(4)));
        Assert.Equal((uint)parent, BinaryPrimitives.ReadUInt32LittleEndian(target.AsSpan(9))); Assert.Equal(0xa5, target[30]); Assert.Equal(1, sends);
    }
}
