// 로컬 미션 Zone 설치가 단일 미션 바인딩을 원본과 같이 기록하는지 검증한다.
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Core.Tests;

// Zone 설치의 Player 기록·현재 상태·전역·효과 순서를 검증하는 테스트 클래스다. 2026-11-12 R4 실제 호스트·월드 조립(0.402)에서 추가됐다.
public sealed class LocalMissionZoneInstallTests
{
    /// <summary>thewarbegins 단일 바인딩과 같은 1클라이언트 Zone을 만든다. 2026-11-12 R4 실제 호스트·월드 조립(0.402)에서 추가됐다.</summary>
    /// <param name="handle">클라이언트 이름이다.</param>
    /// <param name="subscriber">클라이언트 구독자 번호다.</param>
    /// <returns>헤더 15바이트+슬롯 71바이트 Zone이다.</returns>
    private static byte[] SingleClientZone(string handle, uint subscriber)
    {
        var zone = new byte[LocalMissionZone.ZoneHead + LocalMissionZone.ZoneClient];
        zone[14] = 1;
        int offset = LocalMissionZone.ZoneHead;
        zone[offset] = 0;
        // 구독자 DWORD를 little endian으로 쓴다.
        for (int index = 0; index < 4; ++index)
        {
            zone[offset + 1 + index] = unchecked((byte)(subscriber >> (8 * index)));
        }

        byte[] name = System.Text.Encoding.Latin1.GetBytes(handle);
        Array.Copy(name, 0, zone, offset + 9, name.Length);
        return zone;
    }

    /// <summary>기록 효과를 모으는 Zone 훅을 만든다. 2026-11-12 R4 실제 호스트·월드 조립(0.402)에서 추가됐다.</summary>
    /// <param name="stored">받을 이름표 기록이다.</param>
    /// <param name="served">받을 서브 포트 기록이다.</param>
    /// <param name="assertions">받을 진단 기록이다.</param>
    private static LocalMissionZoneHooks RecordingHooks(
        List<(uint Subscriber, string Name)> stored, List<uint> served, List<string> assertions) =>
        new()
        {
            LoginTimeout = () => 30,
            Seconds = () => 0.0,
            StoreName = (subscriber, name) => stored.Add((subscriber, name)),
            Serve = port => served.Add(port),
            Assertion = assertions.Add,
        };

    /// <summary>단일 미션 바인딩이 구독자·flags 0x08·이름·현재 상태를 원본과 같이 쓰는지 확인한다.</summary>
    [Fact]
    public void InstallsSingleMissionBindingLikeTheWarBegins()
    {
        byte[] zone = SingleClientZone("Youpson", 1u);
        var players = new byte[LocalMissionZone.PlayerCount * LocalMissionZone.PlayerBytes];
        var player = new PlayerWorldState { TemplateOwner = 0u, BattleOwner = 0u };
        var globals = new LocalMissionZoneGlobals();
        var stored = new List<(uint Subscriber, string Name)>();
        var served = new List<uint>();
        var assertions = new List<string>();
        LocalMissionZone.Install(zone, players, player, globals,
            RecordingHooks(stored, served, assertions), 6799u);
        // 1번 기록에 구독자 1과 flags 0x08이 있다.
        Assert.Equal(1u, System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(players.AsSpan(180 + 0x2c, 4)));
        Assert.Equal(0x08u, System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(players.AsSpan(180 + 0x50, 4)));
        // 이름은 NUL까지 복사된다.
        Assert.Equal("Youpson", System.Text.Encoding.Latin1.GetString(players, 180 + 12, 7));
        Assert.Equal(0, players[180 + 12 + 7]);
        // 현재 Player 상태가 1번을 가리킨다.
        Assert.Equal(1u, player.TemplateOwner);
        Assert.Equal(1u, player.BattleOwner);
        Assert.Equal(1u, player.PlayerKnown);
        // 서버·방장 전역과 이름표·서브 효과가 남는다.
        Assert.True(globals.Server);
        Assert.True(globals.Boss);
        Assert.Equal([(1u, "Youpson")], stored);
        Assert.Equal([6799u], served);
        Assert.Empty(assertions);
        // 2번 기록은 손대지 않는다.
        Assert.Equal(0u, System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(players.AsSpan(360 + 0x2c, 4)));
        Assert.Equal(0u, System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(players.AsSpan(360 + 0x50, 4)));
    }

    /// <summary>빈 입력·범위 밖 입력을 명시 오류·진단으로 거부하는지 확인한다.</summary>
    [Fact]
    public void InstallGuardsRanges()
    {
        var stored = new List<(uint Subscriber, string Name)>();
        var served = new List<uint>();
        var assertions = new List<string>();
        LocalMissionZoneHooks hooks = RecordingHooks(stored, served, assertions);
        Assert.Throws<ArgumentException>(() => LocalMissionZone.Install(
            new byte[15], new byte[100], new PlayerWorldState(), new LocalMissionZoneGlobals(), hooks, 6799u));
        // 소유자 번호 밖 슬롯은 범위 오류로 거부한다.
        byte[] badOwner = SingleClientZone("Youpson", 1u);
        badOwner[LocalMissionZone.ZoneHead] = 8;
        Assert.Throws<ArgumentOutOfRangeException>(() => LocalMissionZone.Install(
            badOwner, new byte[1620], new PlayerWorldState(), new LocalMissionZoneGlobals(), hooks, 6799u));
        // 클라이언트 수 0은 진단을 보고하고 기록 없이 서브만 시작한다.
        var empty = new byte[15];
        var players = new byte[1620];
        LocalMissionZone.Install(empty, players, new PlayerWorldState(),
            new LocalMissionZoneGlobals(), hooks, 6799u);
        Assert.Equal(["numClients > 0 && numClients < 9"], assertions);
        Assert.Equal([6799u], served);
        Assert.All(players, value => Assert.Equal(0, value));
    }
}
