using System.Buffers.Binary;
using System.Text;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 Host 수신이 같은 공장/공간/장기 전역/현재 Runtime과 연결되는지 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 자료를 쓰는 기존 월드 검사와 같은 세션 구성기를 공유한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>같은 실제 Surface와 미션 공장/사제 공간을 수신 전에 구성한다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, ClientSurfaceGlobals Surface) SquidPacketScene()
    {
        var game = new ClientGameplayGlobals(); game.Authority.Server = true;
        var local = new ClientLocalStateGlobals(game, new()); var surface = new ClientSurfaceGlobals(game, new(local), new Kernel(), () => 0);
        var (session, types) = Scene(surface); session.BuildPriestWorldDevices();
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) => throw new InvalidOperationException($"객체 패킷 진단:{text} {file}:{line:x}"));
        return (session, types, surface);
    }
    /// <summary>영어0과 치환 없는 실제 입력의 설명 이름 장치다.</summary>
    private static RawSquidName SquidPacketNames(HostSession session, IReadOnlyList<OriginalTypeRecord> types, Func<ReadOnlyMemory<byte>, byte[]>? substitute = null) =>
        new(session.Pool, types, new() { Language = () => 0, Substitute = substitute ?? (text => text.ToArray()) });
    /// <summary>단일35BYTE의 실제 sid/TYPE/owner/좌표를 직접 작성하는 원문 입력이다.</summary>
    private static byte[] SquidPacket(int sid, byte type, byte owner = 1)
    {
        var packet = new byte[35]; BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), (ushort)sid);
        BinaryPrimitives.WriteSingleLittleEndian(packet.AsSpan(6, 4), 50); BinaryPrimitives.WriteSingleLittleEndian(packet.AsSpan(10, 4), 60);
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(14, 2), owner);
        packet[16] = type; packet[17] = 0x10; packet[18] = owner; return packet;
    }
    /// <summary>실제 사제 Pop이 만든35BYTE를 다른 실제 Host에 수신해 같은 공간/소유자/회복Form까지 등록한다.</summary>
    [Fact]
    public void NativeSquidPacketUsesActualFactoryOwnerSpatialPopAndRegular()
    {
        var (source, sourceTypes) = Scene(); source.GameAux.SquidTransmission.Compress = 0; int sid = PlacePriest(source, sourceTypes, 0, 9);
        byte[] packet = source.WorldPackets.Last(row => row[1] == 0x2c && BinaryPrimitives.ReadUInt16LittleEndian(row.AsSpan(4, 2)) == sid).ToArray();
        var (target, types, surface) = SquidPacketScene(); target.BindSquidPacketDevices(surface.Storage, SquidPacketNames(target, types), new());
        TerrainStampHooks stamp = target.BuildTerrainStampHooks(types); int ground = stamp.Create!(97, 1); stamp.Pop!(ground, 50, 60, 0x02000061);
        target.SetWorldLoadingDepth(1); target.ReceiveFixedGameplayPacket(0x433c30, packet); target.SetWorldLoadingDepth(0);
        Assert.Equal(158, target.Pool.Slot(sid)[10]); Assert.Equal(1, target.Pool.Slot(sid)[34]); Assert.Equal(0, target.Pool.Slot(sid)[11] & 5);
        Assert.Equal(1u, target.SidLists.Priests.Count); Assert.Equal((uint)sid, target.SidLists.Priests.Items()[0]); Assert.NotNull(target.PlayerProcesses.FindEvent(sid, 602));
        Assert.Equal(1u, target.TotalMade.GlobalCounts[158]); Assert.Equal(0u, target.GameAux.SupportTypeWasBox.Value);
        Assert.NotEqual(0u, BinaryPrimitives.ReadUInt32LittleEndian(target.Pool.Slot(sid))); source.ReleasePlayerProcesses(); target.ReleasePlayerProcesses();
    }
    /// <summary>NoMake가 같은 Core 장부를 읽고 설명 치환 뒤 현재 Player 이름으로 실제 BYTE Tell을 실행한다.</summary>
    [Fact]
    public void NativeSquidNoMakeReadsCurrentNameAfterRealDescriptionEffects()
    {
        var (session, types, surface) = SquidPacketScene(); types[158].TechBitIndex = 0; int sid = 30000;
        session.Gameplay.CoreData.Records().AsSpan(26, 4).Clear();
        "old\0"u8.CopyTo(session.PlayerRecords.AsSpan(192)); byte[] packet = SquidPacket(sid, 158); byte[]? told = null;
        var names = SquidPacketNames(session, types, text => { "new\0"u8.CopyTo(session.PlayerRecords.AsSpan(192)); return text.ToArray(); });
        session.BindSquidPacketDevices(surface.Storage, names, new() { Tell = text => { told = text.ToArray(); throw new InvalidOperationException("tell-stop"); } });
        Assert.Equal("tell-stop", Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x433c30, packet)).Message);
        Assert.Equal("PlayerNoMake(new," + Encoding.Latin1.GetString(ExtendedText.EncodeDisplayText(types[158].Description)) + ")", Encoding.Latin1.GetString(told!));
        Assert.Equal(0u, session.LocalState.PlayerNoMakeWarnings[158]); Assert.Equal(158u, session.GameAux.SupportTypeWasBox.Value); Assert.Equal(1, session.Pool.Slot(sid)[34]);
        session.ReleasePlayerProcesses();
    }
    /// <summary>실제 안내가 owner를 바꾸면 저장한 경고 마스크는 유지하고 현재 Player에400 표식을 기록한다.</summary>
    [Fact]
    public void NativeSquidNoMakeMarksCurrentPlayerAndKeepsEarlierOwnerMask()
    {
        var (session, types, surface) = SquidPacketScene(); types[158].TechBitIndex = 0; int sid = 30000;
        session.Gameplay.CoreData.Records().AsSpan(26, 4).Clear(); session.PlayerRecords.AsSpan(180 + 0x54, 4).Clear(); session.PlayerRecords.AsSpan(360 + 0x54, 4).Clear();
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { Tell = _ => session.Pool.AllocatedBytes(sid)[34] = 2 });
        byte[] packet = SquidPacket(sid, 158); packet[34] = 1;
        session.ReceiveFixedGameplayPacket(0x433c30, packet);
        Assert.Equal(2u, session.LocalState.PlayerNoMakeWarnings[158]); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x54, 4)));
        Assert.Equal(0x400u, BinaryPrimitives.ReadUInt32LittleEndian(session.PlayerRecords.AsSpan(360 + 0x54, 4))); Assert.Equal(0u, session.GameAux.SupportTypeWasBox.Value);
        session.ReleasePlayerProcesses();
    }
    /// <summary>techBitIndex 진단 복귀 뒤 같은 현재 Core 장부를 읽고 불필요 Tell을 요구하지 않는다.</summary>
    [Fact]
    public void NativeSquidTechnologyUsesCurrentCoreRecordsAfterDiagnostic()
    {
        var (session, types, surface) = SquidPacketScene(); types[158].TechBitIndex = 32; int sid = 30000; int diagnostics = 0;
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) =>
        {
            Assert.Equal("techBitIndex >= 0 && techBitIndex < 32", text); Assert.Equal(@"\Ns\O\Ch.cpp", file); Assert.Equal(0x164u, line); diagnostics++;
            BinaryPrimitives.WriteUInt32LittleEndian(session.Gameplay.CoreData.Records().AsSpan(26, 4), 1);
        });
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        TerrainStampHooks stamp = session.BuildTerrainStampHooks(types); int ground = stamp.Create!(97, 1); stamp.Pop!(ground, 50, 60, 0x02000061);
        byte[] packet = SquidPacket(sid, 158); BinaryPrimitives.WriteInt16LittleEndian(packet.AsSpan(27, 2), checked((short)session.Reward.TypeHitPoints(158)));
        session.SetWorldLoadingDepth(1); session.ReceiveFixedGameplayPacket(0x433c30, packet); session.SetWorldLoadingDepth(0);
        Assert.Equal(1, diagnostics); Assert.Equal(0u, session.LocalState.PlayerNoMakeWarnings[158]); Assert.Equal(0u, session.GameAux.SupportTypeWasBox.Value); session.ReleasePlayerProcesses();
    }
    /// <summary>4bh와4ah는 같은 장기 두 상자를 쓰며 실제 검증 로그 중 전체 owner를 반납해도 현재 호출 수명이 유지된다.</summary>
    [Fact]
    public void NativeSquidVerificationUsesSameGlobalsAndPinsActualOwner()
    {
        var (session, types, surface) = SquidPacketScene(); int sid = session.BuildPlayerWorldHooks().Create!(158, 0); var first = new byte[6];
        BinaryPrimitives.WriteUInt16LittleEndian(first.AsSpan(4, 2), (ushort)sid); int logs = 0;
        using var owner = HostMissionRuntime.Create(session, surface.Storage);
        // 수신 장치는 Runtime 공개 전에 연결하는 것이 원문 조립 순서지만 실행 중 값을 바꾸지 않는 연결도 같은 풀만 허용한다.
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()
        {
            Log = (level, text) => { logs++; Assert.Equal(0x1d, level); Assert.Contains($"VERIFY {sid}", text); Assert.True(session.Gameplay.RuntimeCallActive(session)); owner.Dispose(); Assert.False(owner.Released); },
        });
        using (session.Gameplay.PinCurrentRuntime(session)) session.ReceiveFixedGameplayPacket(0x4326e0, first);
        Assert.Equal((uint)sid, session.GameAux.VerifySidBox.Value); Assert.Equal((uint)sid, session.GameAux.PreviousVerifySidBox.Value);
        var free = new byte[7]; first.CopyTo(free, 0);
        using (session.Gameplay.PinCurrentRuntime(session)) session.ReceiveFixedGameplayPacket(0x432710, free);
        Assert.True(logs > 0); Assert.True(owner.Released); Assert.Equal(0u, session.GameAux.VerifySidBox.Value); Assert.Equal((uint)sid, session.GameAux.PreviousVerifySidBox.Value);
        session.ReleasePlayerProcesses();
    }
    /// <summary>압축 decoder가 갱신한 같은 owner 캐시를 직접 공통 수신으로 넘기며 패킷 종류를 재분배하지 않는다.</summary>
    [Fact]
    public void NativeSquidCompressedPacketCallsCommonBodyAndKeepsOwnerCacheOnFailure()
    {
        var (session, types, surface) = SquidPacketScene(); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        byte[] packet = [0xff, 0x70, 0, 0, 0x30, 0x75, 0x10, 0x01, 2, 158, 2]; session.GameAux.VerifySidBox.Value = 30000;
        // 검증 분기는 TYPE와owner 비교만 하므로 존재하지 않는 Pop 장치로 우회하지 않는다.
        var failure = Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x434080, packet)); Assert.Contains("선택된 실제 장치", failure.Message);
        Assert.Equal(2u, session.GameAux.SquidTransmission.LastOwner); Assert.Equal(30000u, session.GameAux.VerifySidBox.Value); session.ReleasePlayerProcesses();
    }
    /// <summary>이름/저장소가 다른 풀을 빌리거나 실제 World 반납 뒤이면 결선과 고정 수신을 거부한다.</summary>
    [Fact]
    public void NativeSquidBindingRequiresSameWorldAndRejectsRetiredSession()
    {
        var (session, types, surface) = SquidPacketScene(); RawSquidName names = SquidPacketNames(session, types);
        var wrong = new RawSquidName(new RawSidPool(32768), types, new());
        Assert.Throws<ArgumentException>(() => session.BindSquidPacketDevices(surface.Storage, wrong, new()));
        session.BindSquidPacketDevices(surface.Storage, names, new()); session.ReleasePlayerProcesses();
        Assert.Throws<InvalidOperationException>(() => session.BindSquidPacketDevices(surface.Storage, names, new()));
        Assert.Throws<ObjectDisposedException>(() => session.ReceiveFixedGameplayPacket(0x4326e0, ReadOnlyMemory<byte>.Empty));
    }
}
