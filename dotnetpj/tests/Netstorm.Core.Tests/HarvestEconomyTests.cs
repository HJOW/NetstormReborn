using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>튜토리얼 1의 가이저 연결·사제 운반·결정당 Storm Power와 유닛별 이동 속도를 검사한다.</summary>
public sealed class HarvestEconomyTests
{
    /// <summary>원본 파일에 가이저가 없는 튜토리얼 1도 연결 가능한 연습 가이저를 만든다.</summary>
    [Fact]
    public void TutorialOne_GeyserRequiresBridgeThenPaysPerDelivery()
    {
        BattleSession session = SessionData.FromMission("tutorial1");
        GameEntity geyser = Assert.Single(session.Entities, e => e.Kind == ObjectKind.Geyser);
        GameEntity priest = Assert.Single(session.Entities, e => e.Kind == ObjectKind.Priest && e.Owner == 1);
        Assert.Equal(0, session.Player(1).StormPower);

        session.Submit(new HarvestGeyserCommand(1, geyser.Id));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.NoRoute);

        ConnectPracticeGeyser(session, geyser);
        session.Submit(new HarvestGeyserCommand(1, geyser.Id));
        session.RunTicks(1);
        Assert.DoesNotContain(session.DrainEvents(), e => e.Kind == SessionEventKind.CommandRejected);

        // 사제가 가이저와 신전을 왕복해 결정 세 개를 전달할 때까지 정확히 틱을 진행한다.
        int delivered = 0;
        for (int tick = 0; tick < session.TicksPerSecond * 180 && delivered < 3; tick++)
        {
            session.RunTicks(1);
            delivered += session.DrainEvents().Count(e => e.Kind == SessionEventKind.CrystalDelivered);
        }
        Assert.Equal(3, delivered);
        Assert.Equal(3 * StormPower.CrystalValue, session.Player(1).StormPower);
        Assert.Equal(0, priest.CarriedCrystals);
    }

    /// <summary>F4부터 다리 8·19칸, 가이저 연결, 첫 전달, 600 SP까지 튜토리얼 1의 단계가 넘어간다.</summary>
    [Fact]
    public void TutorialOne_StagesReachMissionAccomplished()
    {
        BattleSession session = SessionData.FromMission("tutorial1");
        GameEntity geyser = Assert.Single(session.Entities, e => e.Kind == ObjectKind.Geyser);
        session.Submit(new ReturnHomeCommand(1));
        session.RunTicks(1);
        Assert.Equal('B', session.Tutorial!.Stage);

        // 생산 창의 조각을 명령 경로(집기 → 놓기)로 놓아 원본 8·19칸 기준을 채운다.
        // 원본은 놓은 칸의 누적 수(FUN_004c2500(4))를 보므로 누적 합으로 단계를 확인한다.
        PlayerState builder = session.Player(1);
        int built = 0;
        bool sawC = false;
        while (built < 19)
        {
            built += SessionData.PlaceBridgePiece(session, builder);
            // 누적 8칸을 처음 넘긴 놓기에 단계 C 가 된다.
            if (built >= 8 && !sawC)
            {
                Assert.Equal('C', session.Tutorial.Stage);
                sawC = true;
            }
        }
        Assert.True(sawC);
        Assert.Equal('D', session.Tutorial.Stage);

        ConnectPracticeGeyser(session, geyser);
        session.RunTicks(1);
        Assert.Equal('E', session.Tutorial.Stage);
        session.Submit(new HarvestGeyserCommand(1, geyser.Id));
        session.RunTicks(1);
        // 수확 왕복이 세 번 끝나면 원본 스크립트의 600 SP 완료 단계 G가 된다.
        for (int tick = 0; tick < session.TicksPerSecond * 180 && !session.Tutorial.Finished; tick++)
        {
            session.RunTicks(1);
        }
        Assert.Equal('G', session.Tutorial.Stage);
        Assert.True(session.Tutorial.Finished);
        Assert.Equal(600, session.Player(1).StormPower);
    }

    /// <summary>이동 속도는 사제 공통값이 아니라 Priest·Golem·Balloon·Sail Skater·Crystal Crab 타입마다 읽는다.</summary>
    [Fact]
    public void MovementRate_UsesEachUnitType()
    {
        var types = OriginalData.RequireTypes();
        Assert.Equal(1.8, MovementRate.CellsPerSecond(types.Find("priest")!));
        Assert.Equal(2.0, MovementRate.CellsPerSecond(types.Find("sunWalker")!));
        Assert.Equal(1.9, MovementRate.CellsPerSecond(types.Find("sunBalloon")!));
        Assert.Equal(3.4, MovementRate.CellsPerSecond(types.Find("windWalker")!));
        Assert.Equal(2.4, MovementRate.CellsPerSecond(types.Find("rainWalker")!));
    }

    /// <summary>연습 가이저의 받침과 본섬 사이 네 칸에 가로 다리를 차례로 잇는다.</summary>
    private static void ConnectPracticeGeyser(BattleSession session, GameEntity geyser)
    {
        int padLeft = geyser.Footprint.Left - 1;
        int y = geyser.Footprint.Top + 1;
        // 동쪽 해안에서 받침 앞까지 한 칸짜리 수평 조각을 잇는다.
        for (int x = padLeft - 4; x < padLeft; x++)
        {
            var piece = new BridgePiece(BridgePatternCatalog.SinglePiece, 1);
            BridgePlacementCheck check = session.Bridges.Check(piece, x, y, 1);
            Assert.True(check.Allowed, $"연습 가이저 다리 ({x}, {y}): {check.Problem}");
            session.Bridges.Place(piece, x, y, 1);
        }
    }
}
