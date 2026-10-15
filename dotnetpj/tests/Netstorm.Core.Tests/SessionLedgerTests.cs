using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 세션이 공통 postPop 통계(원본 Totalmade 장부)를 원본 규칙대로 유지하는지 검사한다: 완성한 객체는 등록하고, 삭제 때는 현재 수만 내리며,
/// 맵에 처음부터 있던 객체는 장부에 없어 삭제해도 감기지 않는다. 장부 표 자체의 기계어 대조는 <see cref="X86TotalMadeTests"/>가 맡는다.
/// </summary>
/// <remarks>2026-10-15 감사 D14 에서 추가.</remarks>
public sealed class SessionLedgerTests
{
    /// <summary>타입 이름의 런타임 번호.</summary>
    private static int Index(string name) => OriginalData.RequireTypes().Find(name)!.RuntimeIndex;

    /// <summary>같은 섬 영역에 실제 타입을 두는 오브젝트.</summary>
    private static FortMapObject Object(string name, int owner, int x, int y)
    {
        TypeInfo type = OriginalData.RequireTypes().Find(name)!;
        return new FortMapObject(x, y, 0, new FortObject(0, 0, type, null, null, null, null, null, owner, []));
    }

    /// <summary>튜토리얼 없는 작은 세션: 사제와 신전을 가진 사람 플레이어. 회수 금지는 풀어 둔다.</summary>
    /// <param name="objects">추가 오브젝트</param>
    private static BattleSession Create(params FortMapObject[] objects)
    {
        FortMapObject[] all = [Object("priest", 1, 10, 10), Object("windVortex", 1, 14, 10), .. objects];
        var session = new BattleSession(new BattleMap(all, (_, _) => 0), new BridgeGrid((_, _) => true, (_, _) => false),
            OriginalData.RequireTypes(), startStormPower: 50000) { DenySalvage = false };
        return session;
    }

    /// <summary>맵에 처음부터 있던 객체는 장부에 세지 않고, 그 객체를 회수해도 현재 수가 감기지 않는다(원본 장부 초기화 뒤 상태).</summary>
    [Fact]
    public void InitialObjects_AreNotCountedAndDoNotUnderflowWhenRemoved()
    {
        BattleSession session = Create(Object("sunFactory", 1, 30, 10));
        int factory = Index("sunFactory");
        Assert.Equal((0u, 0u, 0u), (session.Ledger.Local(factory), session.Ledger.Built(factory), session.Ledger.Global(factory)));
        GameEntity initial = session.Entities.Single(e => e.Kind == ObjectKind.Workshop);
        session.Submit(new SalvageCommand(1, initial.Id));
        session.RunTicks(1);
        Assert.DoesNotContain(session.Entities, e => e.Id == initial.Id);
        Assert.Equal((0u, 0u, 0u), (session.Ledger.Local(factory), session.Ledger.Built(factory), session.Ledger.Global(factory)));
    }

    /// <summary>완공한 건물은 현재 수·누적 제작 수·전체 현재 수에 하나씩 등록되고, 회수하면 현재 수와 전체 수만 내려가 누적 제작 수는 남는다.</summary>
    [Fact]
    public void CompletedBuilding_IsRegisteredAndSalvageLowersOnlyLiveCounts()
    {
        BattleSession session = Create();
        int factory = Index("sunFactory");
        // 워크샾을 지어 완공 틱까지 진행한다(사제가 걸어가 도착한 뒤 건설한다). 신전은 섬에 하나만 둘 수 있다.
        (int x, int y) = SessionData.FindCell((cx, cy) => session.CheckBuilding(1, "sunFactory", cx, cy).Allowed);
        session.Submit(new ConstructBuildingCommand(1, "sunFactory", x, y));
        session.RunTicks(1);
        GameEntity built = session.Entities.Single(e => e.Kind == ObjectKind.Workshop && e.Footprint.AnchorX == x && e.Footprint.AnchorY == y);
        Assert.Equal((0u, 0u, 0u), (session.Ledger.Local(factory), session.Ledger.Built(factory), session.Ledger.Global(factory)));
        SessionData.RunUntilComplete(session, built);
        Assert.Equal((1u, 1u, 1u), (session.Ledger.Local(factory), session.Ledger.Built(factory), session.Ledger.Global(factory)));
        Assert.Equal(1, session.BuiltCount("sunFactory"));
        Assert.Equal(1, session.BuiltForGenus(TypeFlagBits.Factory));
        session.Submit(new SalvageCommand(1, built.Id));
        session.RunTicks(1);
        Assert.Equal((0u, 1u, 0u), (session.Ledger.Local(factory), session.Ledger.Built(factory), session.Ledger.Global(factory)));
        Assert.Equal(1, session.BuiltCount("sunFactory"));
    }

    /// <summary>놓은 다리 칸은 칸마다 세 장부에 등록되고 누적 부류 합(다리 4)이 읽는다. 모르는 타입 이름의 누적 수는 0이다.</summary>
    [Fact]
    public void PlacedBridgeCells_AreRegisteredPerCell()
    {
        BattleSession session = SessionData.FromMission("tutorial1");
        PlayerState player = session.Player(1);
        int bridge = Index(ProductionDeck.BridgeType);
        Assert.Equal(0, session.BuiltForGenus(TypeFlagBits.Bridge));
        int placed = SessionData.PlaceBridgePiece(session, player);
        Assert.True(placed > 0);
        Assert.Equal(placed, session.BuiltForGenus(TypeFlagBits.Bridge));
        Assert.Equal(((uint)placed, (uint)placed, (uint)placed), (session.Ledger.Local(bridge), session.Ledger.Built(bridge), session.Ledger.Global(bridge)));
        Assert.Equal(0, session.BuiltCount("noSuchType"));
    }

    /// <summary>검사합은 장부의 값을 포함해 같은 명령을 같은 순서로 넣은 두 세션은 같고 장부가 다르면 다르다.</summary>
    [Fact]
    public void Checksum_IncludesLedgerTables()
    {
        BattleSession a = SessionData.FromMission("tutorial1");
        BattleSession b = SessionData.FromMission("tutorial1");
        Assert.Equal(a.Checksum(), b.Checksum());
        b.Ledger.LocalSecondaryCounts[Index(ProductionDeck.BridgeType)] = 1;
        Assert.NotEqual(a.Checksum(), b.Checksum());
        b.Ledger.LocalSecondaryCounts[Index(ProductionDeck.BridgeType)] = 0;
        b.Ledger.Allow(Index("sunFactory"), 0);
        Assert.NotEqual(a.Checksum(), b.Checksum());
    }
}
