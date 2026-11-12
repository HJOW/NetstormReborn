// 섬 런타임의 Player 기록 바인딩(+0x78 소용돌이 사슬 전제)을 검사하는 네임스페이스다.
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 섬 런타임의 Player 기록 바인딩 검사를 담는 클래스다. 2026-11-12 0.477에서 단일 미션 +0x78 정정(전투섬 배정 갈고리 호출 제거·섬 런타임 바인딩)용으로 추가했다.
public sealed class HostIslandFirstRegionTests
{
    /// <summary>섬 런타임이 세션 Player 기록을 공유하는지 확인한다. 소용돌이→섬 소유자→첫 지역 사슬의 전제다.</summary>
    /// <remarks>셀 없는 배치라 bounds 진단이 발생하므로 진단만 무시한다. 배치 유효성은 섬·빌더 검사에서 다룬다.</remarks>
    [Fact]
    public void BuildIslandsSharesPlayerRecords()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        var players = new MissionPlayers();
        session.StartPlayers(players, "You");
        RawIslandRuntime islands = session.BuildIslands(new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => throw new InvalidOperationException("조립 검사는 Residence owner를 요청하지 않는다"),
            Assertion = _ => { },
        });
        uint island = PlaceSingleIsland(session, islands);
        islands.Owner((int)island, 1u);
        // 바인딩됐으면 세션 Player 기록의 첫 지역에 섬 번호가 들어간다.
        Assert.Equal(0, session.PlayerRecords[1 * 180 + 0x78]);
        Assert.Equal(0, session.PlayerRecords[1 * 180 + 0x79]);
        // 같은 세션 모드 표가 아니라 Player 기록에 썼다.
        Assert.Equal(127, session.IslandModes.PlayerRegions[1]);
    }

    /// <summary>첫 지역은 처음 정한 섬을 유지하고 0번 소유자는 쓰지 않는지 확인한다.</summary>
    [Fact]
    public void FirstRegionKeepsFirstIslandAndSkipsOwnerZero()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        var players = new MissionPlayers();
        session.StartPlayers(players, "You");
        RawIslandRuntime islands = session.BuildIslands(new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        uint island = PlaceSingleIsland(session, islands);
        islands.Owner((int)island, 1u);
        islands.Owner((int)island, 1u);
        Assert.Equal(0, session.PlayerRecords[1 * 180 + 0x78]);
        islands.Owner((int)island, 0u);
        // 0번 소유자는 첫 지역을 쓰지 않으므로 9기록이 그대로다.
        for (int owner = 0; owner <= 8; owner++)
        {
            int expected = owner == 1 ? 0 : 127;
            Assert.Equal(expected, session.PlayerRecords[owner * 180 + 0x78]);
        }
    }

    /// <summary>하나의 섬을 세션 월드에 놓고 번호를 돌려준다. 생성·지형 없이 bounds·목록 등록까지만 한다.</summary>
    /// <param name="session">섬 런타임의 소유 세션이다.</param>
    /// <param name="islands">세션이 만든 섬 런타임이다.</param>
    /// <returns>놓인 섬 번호다.</returns>
    private static uint PlaceSingleIsland(HostSession session, RawIslandRuntime islands)
    {
        var islandState = new RawIslandBuilderState();
        var builder = new RawIslandBuilder(
            session.World, session.World, islands, session.WorldFrames,
            () => session.GameAux.PuzzlePieceType, islandState, _ => { });
        byte[] record = [0x11, 0x09, 0x54, 0, 0, 0];
        return builder.Place(record, 0u, true);
    }
}
