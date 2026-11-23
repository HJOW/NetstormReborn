using System.Buffers.Binary;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using SidAllocation = Netstorm.Core.Simulation.SidAllocation;

// 같은 World/Core의 실제 3b/3c 수신 공장과 공통 송신 경계를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>원문 두 수신 공장의 파생 선택·진단·부착과 writer/송신 선택을 확인한다.</summary>
public sealed class RawCampaignArcherBindingsReceiveTests
{
    /// <summary>같은 실제 World/Core와 명시 TYPE/방향/프레임 입력을 소유한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>같은 전체 물리 저장소다.</summary>
        internal RawSurfaceStorage Storage { get; }
        /// <summary>같은 Pool/Host를 실제로 대여한 월드다.</summary>
        internal RawSurfaceWorld World { get; }
        /// <summary>월드의 참조 보기이며 별도 풀을 만들지 않는다.</summary>
        internal RawWorldCore Core { get; }
        /// <summary>현재 아처 방향/클러스터 자료다.</summary>
        internal ArcherTypeData[] Directions { get; } = Enumerable.Range(0, 188).Select(_ => new ArcherTypeData(20, 80)).ToArray();
        /// <summary>실제 파생이 빌리는 명시 문자 프레임 표다.</summary>
        internal PriestPlainCanonType[] Frames { get; } = Enumerable.Range(0, 188).Select(_ => new PriestPlainCanonType()).ToArray();
        /// <summary>현재 미사일의 보충 자료다.</summary>
        internal MissileTypeData[] Missiles { get; } = Enumerable.Range(0, 188).Select(_ => new MissileTypeData()).ToArray();
        /// <summary>외부에서 빌려주는 아처 현재 시계다.</summary>
        internal ArcherAnimationState AnimationState { get; } = new();
        /// <summary>외부에서 빌려주는 미사일 현재 시계다.</summary>
        internal MissileState MissileState { get; } = new();
        /// <summary>같은 단일 게임 난수다.</summary>
        internal GameRandom Random { get; } = new(123);
        /// <summary>실제 표적 판정과 현재 중심을 제공한다.</summary>
        internal RawGunTarget Target { get; }
        /// <summary>생성 중 호출되지 않아야 하는 외부 효과와 실제 월드 자료를 준비한다.</summary>
        internal Scene()
        {
            OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i), FootX = 1, FootY = 1, Group = i is 128 or 129 or 131 ? 1 : 0 }).ToArray();
            RiftTypeFrames[] frames = types.Select(_ => new RiftTypeFrames([])).ToArray(); SquidDisplayShape[] shapes = types.Select(_ => new SquidDisplayShape()).ToArray();
            Storage = new(OriginalEdition.Patch1078, types, frames, shapes); World = new(OriginalEdition.Patch1078, Storage.Types, 97, Storage, frames); Core = World.Core();
            Target = new(Core.Pool, Core.Hash, Core.Types, types.Select(_ => new GunTargetType()).ToArray(), Core.Shapes, new());
        }
        /// <summary>같은 실제 자료와 외부 시계를 빌려 전투 공장을 만든다.</summary>
        internal RawCampaignArcherBindings Bind() => new(World, Target, Random, Directions, Frames, Missiles, AnimationState, MissileState, new());
        /// <summary>수신 부모 raw와 실제 Host의 기존 Gun을 부착한다.</summary>
        internal GunProcess Gun(byte type)
        {
            int parent = Core.Pool.Allocate(SidAllocation.Client); Span<byte> raw = Core.Pool.AllocatedBytes(parent); raw.Clear(); raw[10] = type; raw[34] = 1;
            BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x501ea0); BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 50); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 60);
            var logic = new RawGunProcess(Core.Pool, Core.Types, new(), new()); var fields = new GunProcessFields(); logic.Initialize(fields, (ushort)parent);
            return Assert.IsType<GunProcess>(Core.Processes.Attach(new GunProcess(logic, fields), 34, parent, 0x10));
        }
        /// <summary>현재 실제 Kernel 사용을 먼저 끝내고 World 대여를 반납한다.</summary>
        public void Dispose() { Core.Kernel.DrainOwnedForShutdown(); World.Dispose(); }
    }
    /// <summary>원문9BYTE 머리·Form·진단 뒤 부모DWORD와 전체 선택 필드를 붙이는 실제 수신 입력이다.</summary>
    private static byte[] FreePacket(int form, uint message, int parent)
    {
        var packet = new byte[25]; packet[0] = 1; packet[1] = (byte)message;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2), 25); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), (uint)form);
        packet[8] = 0xff; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9), (uint)parent);
        BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(17), 2.5); return packet;
    }
    /// <summary>같은 실제 Host에 발사 파생을 붙이고 현재 시각/부모/역할을 확인한다.</summary>
    [Theory]
    [InlineData(71, 0x3b, nameof(ArcherAnimation), 35)]
    [InlineData(128, 0x3b, nameof(CannonFire), 35)]
    [InlineData(128, 0x3c, nameof(GunAnimation), 36)]
    [InlineData(140, 0x3c, nameof(ArcherAnimation), 36)]
    public void FreeReceiveAttachesDerivedProcessToSameHost(byte type, uint message, string expected, byte processType)
    {
        using var scene = new Scene(); RawCampaignArcherBindings binding = scene.Bind(); GunProcess gun = scene.Gun(type);
        binding.ReceiveAnimationPacket(message, FreePacket(16000, message, gun.Parent()));
        SquidProcess animation = scene.Core.Processes.FromForm(16000)!;
        Assert.Equal(expected, animation.GetType().Name); Assert.Equal(gun.Parent(), animation.Parent()); Assert.Equal(processType, scene.Core.Pool.Slot(16000)[10]);
        Assert.Equal(2, scene.Core.Kernel.Size());
    }
    /// <summary>알 수 없는 파생은 같은 Gunanim 진단 뒤 선택 필드 저장 없이 부착을 거부한다.</summary>
    [Fact]
    public void UnknownTypeDiagnosesGunanimAndRejectsAttach()
    {
        using var scene = new Scene(); RawCampaignArcherBindings binding = scene.Bind(); GunProcess gun = scene.Gun(86); int diagnostics = 0;
        scene.Storage.BindDisplayDiagnostics(new(), (text, file, line) =>
        {
            Assert.Equal("false", text); Assert.Equal("\\Ns\\O\\Gunanim.cpp", file); Assert.Equal(0x23bu, line); diagnostics++;
        });
        var failure = Assert.Throws<InvalidOperationException>(() => binding.ReceiveAnimationPacket(0x3b, FreePacket(16000, 0x3b, gun.Parent())));
        Assert.Contains("현재PID 선택필드", failure.Message); Assert.Equal(1, diagnostics); Assert.Null(scene.Core.Processes.FromForm(16000)); Assert.Equal(1, scene.Core.Kernel.Size());
    }
    /// <summary>ff 진단 복귀는 최초 Form을 유지하고 현재 패킷으로 파생을 부착한다.</summary>
    [Fact]
    public void NonFfDiagnosticKeepsCapturedFormAndAttachesAfterReturn()
    {
        using var scene = new Scene(); RawCampaignArcherBindings binding = scene.Bind(); GunProcess gun = scene.Gun(128); byte[] packet = FreePacket(16000, 0x3b, gun.Parent()); packet[8] = 0; int diagnostics = 0;
        scene.Storage.BindDisplayDiagnostics(new(), (text, file, line) =>
        {
            Assert.Equal("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", text); Assert.Equal(@"c:\ns\o\_p_fireanimprocessdata.cpp", file); Assert.Equal(0x42u, line); diagnostics++;
            packet[8] = 0xff;
        });
        binding.ReceiveAnimationPacket(0x3b, packet);
        Assert.Equal(1, diagnostics); Assert.IsType<CannonFire>(scene.Core.Processes.FromForm(16000)); Assert.Equal(35, scene.Core.Pool.Slot(16000)[10]); Assert.Equal(2, scene.Core.Kernel.Size());
    }
    /// <summary>live 수신은 공장/현재 시각을 덮어쓰지 않고 같은 Kernel 객체의 선택 필드만 갱신한다.</summary>
    [Fact]
    public void LiveReceiveUpdatesSelectedTimeFieldWithoutReattaching()
    {
        using var scene = new Scene(); RawCampaignArcherBindings binding = scene.Bind(); GunProcess gun = scene.Gun(128);
        binding.ReceiveAnimationPacket(0x3b, FreePacket(16000, 0x3b, gun.Parent()));
        CannonFire animation = Assert.IsType<CannonFire>(scene.Core.Processes.FromForm(16000)); Assert.Equal(0, animation.Fields.Time());
        var packet = new byte[17]; packet[0] = 1; packet[1] = 0x3b; BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2), 17);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16000); packet[8] = 8; BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(9), 7.5);
        binding.ReceiveAnimationPacket(0x3b, packet);
        Assert.Same(animation, scene.Core.Processes.FromForm(16000)); Assert.Equal(7.5, animation.Fields.Time()); Assert.Equal(2, scene.Core.Kernel.Size());
    }
    /// <summary>live 부모 비트 진단은 복귀 뒤 같은 객체를 유지하고 필드를 바꾸지 않는다.</summary>
    [Fact]
    public void LiveParentBitDiagnosticKeepsSameProcessWithoutFieldChange()
    {
        using var scene = new Scene(); RawCampaignArcherBindings binding = scene.Bind(); GunProcess gun = scene.Gun(71);
        binding.ReceiveAnimationPacket(0x3b, FreePacket(16000, 0x3b, gun.Parent()));
        ArcherAnimation animation = Assert.IsType<ArcherAnimation>(scene.Core.Processes.FromForm(16000)); int diagnostics = 0;
        var packet = new byte[13]; packet[0] = 1; packet[1] = 0x3b; BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2), 13);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16000); packet[8] = 1; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9), (uint)gun.Parent());
        scene.Storage.BindDisplayDiagnostics(new(), (text, file, line) =>
        {
            Assert.Equal("(*(char *)(( (char *)zacket+8) ) & 1) == 0", text); Assert.Equal(@"c:\ns\o\_p_fireanimprocessdata.cpp", file); Assert.Equal(0x4du, line); diagnostics++;
            packet[8] = 0;
        });
        binding.ReceiveAnimationPacket(0x3b, packet);
        Assert.Equal(1, diagnostics); Assert.Same(animation, scene.Core.Processes.FromForm(16000)); Assert.Equal(2, scene.Core.Kernel.Size());
    }
    /// <summary>메시지 범위·머리/Form/SID/부모 범위와 없는 현재 PID를 거부한다.</summary>
    [Fact]
    public void BoundsRejectMessageHeaderFormSidParentAndMissingPid()
    {
        using var scene = new Scene(); RawCampaignArcherBindings binding = scene.Bind(); GunProcess gun = scene.Gun(128);
        Assert.Throws<ArgumentException>(() => binding.ReceiveAnimationPacket(0x3d, FreePacket(16000, 0x3b, gun.Parent())));
        Assert.Throws<ArgumentOutOfRangeException>(() => binding.ReceiveAnimationPacket(0x3b, new byte[8]));
        Assert.Throws<ArgumentOutOfRangeException>(() => binding.ReceiveAnimationPacket(0x3b, FreePacket(0, 0x3b, gun.Parent())));
        Assert.Throws<ArgumentOutOfRangeException>(() => binding.ReceiveAnimationPacket(0x3b, FreePacket(0x10003e80, 0x3b, gun.Parent())));
        Assert.Throws<ArgumentOutOfRangeException>(() => binding.ReceiveAnimationPacket(0x3b, FreePacket(40000, 0x3b, gun.Parent())));
        byte[] dangling = FreePacket(16000, 0x3b, 40000);
        Assert.Throws<ArgumentOutOfRangeException>(() => binding.ReceiveAnimationPacket(0x3b, dangling));
        int liveForm = scene.Core.Pool.Allocate(SidAllocation.Client);
        var live = new byte[9]; live[0] = 1; live[1] = 0x3b; BinaryPrimitives.WriteUInt16LittleEndian(live.AsSpan(2), 9);
        BinaryPrimitives.WriteUInt32LittleEndian(live.AsSpan(4), (uint)liveForm); live[8] = 0;
        var missing = Assert.Throws<InvalidOperationException>(() => binding.ReceiveAnimationPacket(0x3b, live));
        Assert.Contains("현재 PID", missing.Message);
    }
    /// <summary>같은 공통 버퍼의 writer/조회/송신을 공유하고 다른 파생은 기존 fallback으로 넘긴다.</summary>
    [Fact]
    public void TransmissionWritesSameCommonBufferAndFallsBack()
    {
        using var scene = new Scene(); RawCampaignArcherBindings binding = scene.Bind(); GunProcess gun = scene.Gun(128);
        binding.ReceiveAnimationPacket(0x3b, FreePacket(16000, 0x3b, gun.Parent()));
        CannonFire animation = Assert.IsType<CannonFire>(scene.Core.Processes.FromForm(16000));
        byte[]? sent = null; var fallback = new ProcessFormTransmission(); var transmission = binding.Transmission(packet => sent = packet.ToArray(), fallback);
        int length = transmission.WriteData!(animation, false); Assert.True(length > 9);
        byte[] buffer = scene.Storage.GameAux().SquidTransmission.Buffer;
        Assert.Equal(buffer.AsSpan(0, length).ToArray(), transmission.Packet!(length));
        Assert.Equal(0x3b, transmission.Packet!(length)[1]); transmission.Transmit!(length); Assert.Equal(buffer.AsSpan(0, length).ToArray(), sent);
        var other = new SquidProcess(); int calls = 0;
        fallback.WriteData = (process, changed) => { Assert.Same(other, process); calls++; return 7; };
        Assert.Equal(7, transmission.WriteData!(other, true)); Assert.Equal(1, calls);
        Assert.Throws<InvalidOperationException>(() => binding.Transmission(null).WriteData!(other, true));
        Assert.Throws<InvalidOperationException>(() => binding.Transmission(null).WriteFullData!(other, 1));
        Assert.Throws<InvalidOperationException>(() => binding.Transmission(null).Packet!(1));
        Assert.Throws<InvalidOperationException>(() => binding.Transmission(null).Transmit!(1));
    }
    /// <summary>강제 writer는 같은 VT의 실제 헤더를 고르고 버퍼 범위 밖 송신을 거부한다.</summary>
    [Fact]
    public void FullWriterSelectsActualHeaderAndRejectsOutOfRange()
    {
        using var scene = new Scene(); RawCampaignArcherBindings binding = scene.Bind(); GunProcess fire = scene.Gun(71); GunProcess rotate = scene.Gun(140);
        binding.ReceiveAnimationPacket(0x3b, FreePacket(16000, 0x3b, fire.Parent()));
        binding.ReceiveAnimationPacket(0x3c, FreePacket(16001, 0x3c, rotate.Parent()));
        var transmission = binding.Transmission(null);
        ArcherAnimation archer = Assert.IsType<ArcherAnimation>(scene.Core.Processes.FromForm(16000));
        int fireLength = transmission.WriteFullData!(archer, 1); byte[] firePacket = transmission.Packet!(fireLength);
        Assert.Equal(1, firePacket[0]); Assert.Equal(0x3b, firePacket[1]);
        ArcherAnimation crossbow = Assert.IsType<ArcherAnimation>(scene.Core.Processes.FromForm(16001));
        int rotateLength = transmission.WriteFullData!(crossbow, 1); byte[] rotatePacket = transmission.Packet!(rotateLength);
        Assert.Equal(0x3c, rotatePacket[1]);
        Assert.Throws<ArgumentOutOfRangeException>(() => transmission.Packet!(5000));
        var ranged = binding.Transmission(_ => { });
        Assert.Throws<ArgumentOutOfRangeException>(() => ranged.Transmit!(5000));
        Assert.Throws<InvalidOperationException>(() => binding.Transmission(null).Transmit!(rotateLength));
    }
}
