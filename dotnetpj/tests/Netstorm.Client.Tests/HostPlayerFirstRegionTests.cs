// 호스트 세션의 Player 첫 지역(+0x78) 로그인 초기화(9기록 127)를 검사하는 네임스페이스다.
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// Player 첫 지역(+0x78) 초기화를 검사하는 클래스다. 2026-11-12 0.473에서 R4 Start 순서 잔여(비참가자 127) 재현용으로 추가했다.
public sealed class HostPlayerFirstRegionTests
{
    /// <summary>StartPlayers가 9기록의 첫 지역을 모두 127로 두는지 확인한다.</summary>
    [Fact]
    public void StartPlayersInitializesAllFirstRegionsTo127()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        var players = MissionPlayersForFirstRegion();
        session.StartPlayers(players, "You");
        // 0~8번 9기록의 +0x78 DWORD가 모두 127(LE 7F 00 00 00)이어야 한다.
        for (int owner = 0; owner <= 8; owner++)
        {
            int offset = owner * HostSession.PlayerRecordBytes / 9 + 0x78;
            Assert.Equal(127, session.PlayerRecords[offset]);
            Assert.Equal(0, session.PlayerRecords[offset + 1]);
            Assert.Equal(0, session.PlayerRecords[offset + 2]);
            Assert.Equal(0, session.PlayerRecords[offset + 3]);
        }
    }

    /// <summary>첫 지역 초기화가 이름·동맹 쓰기를 그대로 두는지 확인한다.</summary>
    [Fact]
    public void StartPlayersKeepsNamesAndAlliancesWithFirstRegion()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        var players = MissionPlayersForFirstRegion();
        session.StartPlayers(players, "You");
        // 1번 이름이 핸들로 들어가고 첫 지역도 127이어야 한다.
        Assert.Equal((byte)'Y', session.PlayerRecords[1 * 180 + 0xc]);
        Assert.Equal(127, session.PlayerRecords[1 * 180 + 0x78]);
        // 동맹 대각선은 그대로 1이어야 한다.
        Assert.Equal(1u, session.Alliances[1 * 9 + 1]);
    }

    /// <summary>첫 지역 검사용 최소 미션 플레이어 목록을 만든다.</summary>
    /// <returns>1번 인간·2번 전산 활성, 나머지는 비활성인 목록이다.</returns>
    private static MissionPlayers MissionPlayersForFirstRegion()
    {
        var list = new MissionPlayers();
        list.Players[1].Active = true;
        list.Players[1].Computer = false;
        list.Players[1].Name = "You";
        list.Players[1].StormPower = 1000;
        list.Players[2].Active = true;
        list.Players[2].Computer = true;
        list.Players[2].Name = "AI";
        list.Players[2].StormPower = 1000;
        return list;
    }
}
