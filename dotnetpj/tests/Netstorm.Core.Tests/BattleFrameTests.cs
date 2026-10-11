using System.Globalization;
using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>화면 프레임의 명령·정지·다리 스캔을 세션 경계에서 원본 x86 및 구체적인 사용자 동작에 대조한다.</summary>
public sealed class BattleFrameTests
{
    /// <summary>원본 서버 SID 범위의 시작 번호.</summary>
    private const int First = SidPool.ServerFirst1078;

    /// <summary>지정한 프레임 시각으로 시작하며 저장 사제가 하나 있거나 빈 세션을 만든다. 모든 칸을 걸을 수 있다.</summary>
    private static BattleSession Create(bool priest = false, double initialFrameSeconds = 0)
    {
        FortMapObject[] objects = priest
            ? [new(5, 5, 0, new FortObject(0, 0, OriginalData.RequireTypes().Find("priest")!, null, null, null, null, null, 1, []))]
            : [];
        var map = new BattleMap(objects, (_, _) => 0);
        var grid = new BridgeGrid((_, _) => true, graphSurfaces: _ => BridgeGrid.MinSurfaceGraphSize, sids: map.Sids);
        return new BattleSession(map, grid, OriginalData.RequireTypes(), initialFrameSeconds: initialFrameSeconds) { CombatEnabled = false };
    }

    /// <summary>원본 기대값의 십진 double 비트 표현을 그대로 읽는다.</summary>
    private static double Bits(string text) => BitConverter.UInt64BitsToDouble(ulong.Parse(text, CultureInfo.InvariantCulture));

    /// <summary>순서가 보존된 실제 프레임 시간열을 넣어 커서·다음 스캔 시각을 원본 기계어 결과와 직접 비교한다.</summary>
    [Theory]
    [InlineData("steady14ms")]
    [InlineData("steady24hz")]
    [InlineData("steady60hz")]
    [InlineData("irregular")]
    public void Advance_ScanMatchesOriginalFrameSequence(string sequence)
    {
        string[][] fixture = X86Fixture.Read("bridgedecay");
        string[] initial = fixture.Single(row => row[0] == "Init" && row[1] == "originals" && row[2] == sequence);
        BattleSession session = Create(initialFrameSeconds: Bits(initial[3]));
        string[][] rows = fixture
            .Where(row => row[0] == "Scan" && row[1] == "originals" && row[2] == sequence).ToArray();
        Assert.NotEmpty(rows);
        // 독립 함수 비교에서 끝내지 않고 같은 시간열을 실제 세션 API로 재생한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(int.Parse(row[8], CultureInfo.InvariantCulture), session.Bridges.ScanCursor);
            session.Advance(Bits(row[4]));
            Assert.Equal(Bits(row[3]), session.FrameSeconds);
            Assert.Equal(int.Parse(row[11], CultureInfo.InvariantCulture), session.Bridges.ScanCursor);
            Assert.Equal(ulong.Parse(row[12], CultureInfo.InvariantCulture), BitConverter.DoubleToUInt64Bits(session.Bridges.NextScanTime));
        }
    }

    /// <summary>14ms 첫 프레임에도 명령은 같은 프레임에서 적용되며 다음 24Hz 틱을 기다리지 않는다.</summary>
    [Fact]
    public void Advance_ProcessesCommandsBeforeFirstLogicTick()
    {
        BattleSession session = Create(priest: true);
        GameEntity priest = Assert.Single(session.Entities);
        session.Submit(new SelectEntityCommand(1, priest.Id));
        session.Submit(new MoveEntityCommand(1, priest.Id, 7, 5));
        Assert.Equal(0, session.Advance(0.014));
        Assert.Equal(priest.Id, session.Player(1).SelectedEntityId);
        Assert.True(session.IsMoving(priest.Id));
        Assert.Equal(0, session.Tick);
        Assert.Equal(First + 11, session.Bridges.ScanCursor);
    }

    /// <summary>정지 중 반복 선택·이동 명령은 시간·이동 진행량·다리 수명을 변경하지 않는다.</summary>
    [Fact]
    public void ProcessCommands_PauseKeepsTimeAndMovementFrozen()
    {
        BattleSession session = Create(priest: true);
        GameEntity priest = Assert.Single(session.Entities);
        session.Submit(new MoveEntityCommand(1, priest.Id, 7, 5));
        session.ProcessCommands();
        Assert.True(session.IsMoving(priest.Id));
        // 여러 번 누르는 동안에도 게임 시각은 늘어나지 않는다.
        for (int i = 0; i < 12; i++)
        {
            session.Submit(new SelectEntityCommand(1, i % 2 == 0 ? priest.Id : 0));
            session.ProcessCommands();
        }
        Assert.Equal(0, session.Tick);
        Assert.Equal(0, session.Seconds);
        Assert.Equal(0, session.FrameSeconds);
        Assert.Equal((5.0, 5.0), session.VisualCell(priest));
        Assert.Equal(First, session.Bridges.ScanCursor);
        Assert.Equal(10, session.Bridges.NextScanTime);
        Assert.Equal(0, session.Player(1).SelectedEntityId);
    }

    /// <summary>붕괴 처리의 실제 대상은 배타적 SID 구간으로 결정되며 틱이 없는 첫 두 화면에도 수명이 갱신된다.</summary>
    [Fact]
    public void Advance_ScansBridgeInItsFrameInterval()
    {
        var map = new BattleMap([], (_, _) => 0);
        var grid = new BridgeGrid((x, _) => x == 9, graphSurfaces: _ => BridgeGrid.MinSurfaceGraphSize, sids: map.Sids);
        var session = new BattleSession(map, grid, OriginalData.RequireTypes());
        // 첫 프레임의 11개 번호를 다른 객체로 채워 다리는 두 번째 프레임에서 검사되게 한다.
        for (int i = 0; i < 11; i++) map.NextId();
        BridgeCellState cell = Assert.Single(grid.Place(new BridgePiece(BridgePatternCatalog.SinglePiece, 1), 10, 5, 1));
        Assert.Equal(First + 11, cell.Sid);
        Assert.Equal(0, session.Advance(0.014));
        Assert.Equal(0, cell.TimeLeft);
        Assert.Equal(0, session.Advance(0.014));
        Assert.Equal(7, cell.TimeLeft);
        Assert.Equal(First + 22, grid.ScanCursor);
    }

    /// <summary>긴 화면 프레임은 한 번의 원본 스캔으로 처리하며 논리 따라잡기 틱 수에 맞춰 반복하지 않는다.</summary>
    [Fact]
    public void Advance_LongFrameScansOnceUsingActualElapsedTime()
    {
        BattleSession session = Create();
        Assert.Equal(8, session.Advance(5));
        Assert.Equal(5, session.FrameSeconds);
        Assert.Equal(First + 4000, session.Bridges.ScanCursor);
        Assert.Equal(10, session.Bridges.NextScanTime);
    }

    /// <summary>같은 명령과 프레임 시간열은 프레임 시각·스캔 상태까지 같은 검사합을 만든다.</summary>
    [Fact]
    public void Advance_SameFrameHistoryIsDeterministic()
    {
        BattleSession first = Create(priest: true), second = Create(priest: true);
        first.Submit(new MoveEntityCommand(1, First, 7, 5));
        second.Submit(new MoveEntityCommand(1, First, 7, 5));
        // 14ms 기본 프레임과 긴 프레임·불규칙 간격을 함께 재생한다.
        foreach (double delta in new[] { 0.014, 0.014, 0.030, 0.008, 0.3, 0.014 })
        {
            first.Advance(delta);
            second.Advance(delta);
        }
        Assert.Equal(first.Checksum(), second.Checksum());
        Assert.Equal(first.DrainEvents(), second.DrainEvents());
    }

    /// <summary>잘못된 화면 간격은 시각·명령·스캔에 적용하기 전에 거부한다.</summary>
    [Theory]
    [InlineData(-0.001)]
    [InlineData(double.NaN)]
    [InlineData(double.PositiveInfinity)]
    public void Advance_InvalidIntervalPreservesState(double elapsed)
    {
        BattleSession session = Create();
        uint before = session.Checksum();
        Assert.Throws<ArgumentOutOfRangeException>(() => session.Advance(elapsed));
        Assert.Equal(before, session.Checksum());
    }
}
