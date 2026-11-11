using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 호스트 세션이 지형 생성 입력 15종의 세션 소유 결선을 원본 기본값으로 묶는지 검사한다.
public sealed class HostTerrainInputsTests
{
    /// <summary>세션이 게임 보조·모드·전투 옵션·진단 값을 원본 기본값으로 소유하는지 확인한다.</summary>
    [Fact]
    public void OwnsTerrainInputSourcesWithOriginalDefaults()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.Equal(1u, session.GameAux.TerrainEnabled);
        Assert.Equal(15, session.GameAux.ArchipelagoMinimum);
        Assert.Equal(35, session.GameAux.ArchipelagoMaximum);
        Assert.Equal(0u, session.GameAux.ArchipelagoPercent);
        Assert.Equal(2, session.GameAux.WorldGeyserMargin);
        Assert.True(session.OwnerMode.Battle);
        Assert.True(session.BridgeState.Mission);
        Assert.Equal(82u, session.BridgeState.BridgeType);
        Assert.Equal(87, session.BattleOptions.Length);
        Assert.Equal(0, session.BattleOptions[5]);
        Assert.Equal(2, session.BattleOptions[6]);
        Assert.Equal(0u, session.AiChecking.Read());
    }

    /// <summary>간격 선택 BYTE가 원본 스택 표대로 칸 수로 바뀌는지 확인한다.</summary>
    [Fact]
    public void WorldGeyserSpacingMatchesOriginalTable()
    {
        Assert.Equal(3, RawWorldGeyserSpacing.Spacing(0));
        Assert.Equal(4, RawWorldGeyserSpacing.Spacing(1));
        Assert.Equal(5, RawWorldGeyserSpacing.Spacing(2));
        Assert.Equal(6, RawWorldGeyserSpacing.Spacing(3));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawWorldGeyserSpacing.Spacing(4));
    }

    /// <summary>입력 묶음이 세션 장기 전역·기록을 공유하는지 확인한다.</summary>
    [Fact]
    public void BuildTerrainGenerationInputsSharesSessionBoxes()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        TerrainGenerationInputs inputs = session.BuildTerrainGenerationInputs();
        Assert.Same(session.GenerationInputs, inputs);
        Assert.True(inputs.Enabled.IsEnabled);
        Assert.True(inputs.Battle.IsEnabled);
        Assert.True(inputs.Mission.IsEnabled);
        Assert.Equal(0, inputs.Layout!());
        Assert.Equal(5, inputs.Spacing!());
        Assert.Equal(2, inputs.Span.Value);
        Assert.Equal(15, inputs.Minimum.Value);
        Assert.Equal(35, inputs.Maximum.Value);
        Assert.Equal(0u, inputs.Percent.Value);
        // 구독자는 Player 기록 +0x2c를 매번 다시 읽는다.
        Assert.Equal(0u, inputs.Subscriber!(1));
        // R6·성장 assertion은 미연결로 두어 GenerateOriginal이 명시 거부한다.
        Assert.Null(inputs.Placement);
        Assert.Null(inputs.LogPercent);
        Assert.Null(inputs.Growth.Assertion);
    }

    /// <summary>세션 값을 바꾸면 입력 묶음이 같은 장기 값을 읽는지 확인한다.</summary>
    [Fact]
    public void BuiltInputsFollowSessionValues()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        TerrainGenerationInputs inputs = session.BuildTerrainGenerationInputs();
        session.GameAux.TerrainEnabled = 0u;
        session.OwnerMode.Battle = false;
        session.BridgeState.Mission = false;
        session.BattleOptions[6] = 3;
        Assert.False(inputs.Enabled.IsEnabled);
        Assert.False(inputs.Battle.IsEnabled);
        Assert.False(inputs.Mission.IsEnabled);
        Assert.Equal(6, inputs.Spacing!());
        Assert.Throws<ArgumentOutOfRangeException>(() => inputs.Subscriber!(0));
    }
}
