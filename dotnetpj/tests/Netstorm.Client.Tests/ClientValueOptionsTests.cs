using Netstorm.Core.Original;

// 실제 Client 장기 소유자를 다섯 값 해석과 함께 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>누락/상위 비트/현재 값 재조회와 네트워크 문자열 실패의 접두를 검증한다.</summary>
public sealed class ClientValueOptionsTests
{
    /// <summary>동일 Config/게임/연결 소유자와 억제 DWORD를 구성한다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 설정 객체이며 변경 뒤 같은 소비자가 재조회한다.</summary>
        public readonly ConfigInterface Config = new(null);
        /// <summary>현재 Player/회전/치트/포트 소유자다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>현재 안내와 주소 물리다.</summary>
        public readonly ClientLocalStateGlobals Local;
        /// <summary>같은 현재 지연 분포다.</summary>
        public readonly ClientConnectionGlobals Connections;
        /// <summary>같은 현재 서버 정보다.</summary>
        public readonly ClientNsInfoGlobals NsInfo = new();
        /// <summary>저장 관문이 빌리는 실제 DWORD다.</summary>
        public readonly UIntBox ReadOnly = new();
        /// <summary>검사 중 같은 소유자를 계속 빌리는 다섯 본체다.</summary>
        public readonly ClientValueOptions Options;
        /// <summary>구성 자체가 값/소켓/시각을 소비하지 않는 동일 Client를 조립한다.</summary>
        public Scene()
        {
            Local = new(Game, new()); Connections = new(Local);
            Options = new(Config, Game, Local, Connections, NsInfo, ReadOnly);
        }
    }
    /// <summary>회전 누락은0으로 재설정하지만 안내/파일/포트 누락은 현재 전체 DWORD를 유지한다.</summary>
    [Fact]
    public void MissingOptionsRespectEachOriginalResetPolicy()
    {
        var scene = new Scene(); scene.Game.CursorGlobals.RotationMode.Value = 17;
        scene.Local.TellTips.Value = 0x80000001; scene.Local.TipNumber.Value = uint.MaxValue; scene.Local.AutoDemo.Value = 0x81234567;
        scene.ReadOnly.Value = 0x80000000; scene.NsInfo.ServerSelection.Value = -17; scene.Game.GameServerPort.Value = 0xfedcba98;
        scene.Local.TestBattleAddress.Span.Fill(0xa5);
        scene.Options.InterpretCursor(); scene.Options.InterpretMainMenu(); scene.Options.InterpretFortFiles(); scene.Options.InterpretNetwork();
        Assert.Equal(0u, scene.Game.CursorGlobals.RotationMode.Value); Assert.Equal(0x80000001u, scene.Local.TellTips.Value);
        Assert.Equal(uint.MaxValue, scene.Local.TipNumber.Value); Assert.Equal(0x81234567u, scene.Local.AutoDemo.Value);
        Assert.Equal(0x80000000u, scene.ReadOnly.Value); Assert.Equal(-17, scene.NsInfo.ServerSelection.Value); Assert.Equal(0xfedcba98u, scene.Game.GameServerPort.Value);
        Assert.All(scene.Local.TestBattleAddress.ToArray(), value => Assert.Equal(0xa5, value));
    }
    /// <summary>설정의 signed 상위 비트와 수정 후 재조회가 같은 실제 필드에 반영된다.</summary>
    [Fact]
    public void ConfiguredValuesUseCurrentOwnersAndKeepFullBits()
    {
        var scene = new Scene();
        scene.Config.Set("rotmode", "-2147483648"); scene.Config.Set("tellTips", "-1"); scene.Config.Set("tipNumber", "-2"); scene.Config.Set("autoDemo", "-3");
        scene.Config.Set("readOnlyForts", "-4"); scene.Config.Set("curServer", "-5"); scene.Config.Set("rootServerBroadcastPort", "-6"); scene.Config.Set("passedDiags", "-7");
        scene.Config.Set("gameServerPort", "-8"); scene.Config.Set("testBattleAddress", "é.host"); scene.Config.Set("lag", "2");
        scene.Options.InterpretCursor(); scene.Options.InterpretMainMenu(); scene.Options.InterpretFortFiles(); scene.Options.InterpretNetwork();
        Assert.Equal(0x80000000u, scene.Game.CursorGlobals.RotationMode.Value); Assert.Equal(uint.MaxValue, scene.Local.TellTips.Value);
        Assert.Equal(unchecked((uint)-2), scene.Local.TipNumber.Value); Assert.Equal(unchecked((uint)-3), scene.Local.AutoDemo.Value);
        Assert.Equal(unchecked((uint)-4), scene.ReadOnly.Value); Assert.Equal(-5, scene.NsInfo.ServerSelection.Value);
        Assert.Equal(-6, scene.NsInfo.RootServerBroadcastPort.Value); Assert.Equal(-7, scene.NsInfo.PassedDiags.Value); Assert.Equal(unchecked((uint)-8), scene.Game.GameServerPort.Value);
        Assert.Equal(new byte[] { 0xe9, 46, 104, 111, 115, 116, 0 }, scene.Local.TestBattleAddress.Span[..7].ToArray());
        Assert.Equal(new sbyte[] { 0, 0, 0, 0, 1, -1 }, scene.Connections.PacketDelays().Distribution);
        scene.Config.Set("gameServerPort", "42"); scene.Config.Set("lag", "0"); scene.Options.InterpretNetwork();
        Assert.Equal(42u, scene.Game.GameServerPort.Value); Assert.Equal(new sbyte[] { 0, 0, 0, 0, 1, -1 }, scene.Connections.PacketDelays().Distribution);
    }
    /// <summary>특수 두 필드는 항상0이며 현재 치트는 원문의 단일 허용값만 유지한다.</summary>
    [Theory]
    [InlineData("8675309", 0x845fedu)]
    [InlineData("-1", 0u)]
    [InlineData("0", 0u)]
    [InlineData(null, 0x845fedu)]
    public void ScriptMenuResetsSpecialFieldsAndValidatesCheat(string? configured, uint expected)
    {
        var scene = new Scene(); scene.Game.Player.Special = 0x80000000; scene.Game.SpecialAuxiliary.Value = 17; scene.Game.UserCheat.Value = 0x845fed;
        if (configured is not null) scene.Config.Set("userCheat", configured);
        scene.Options.InterpretScriptMenu();
        Assert.Equal(0u, scene.Game.Player.Special); Assert.Equal(0u, scene.Game.SpecialAuxiliary.Value); Assert.Equal(expected, scene.Game.UserCheat.Value);
    }
    /// <summary>주소 복사 실패 시 이미 쓴 포트/64BYTE를 보존하고 뒤 지연 분포는 선택하지 않는다.</summary>
    [Fact]
    public void NetworkAddressFailureKeepsEarlierWritesAndSkipsLag()
    {
        var scene = new Scene(); scene.Config.Set("gameServerPort", "7001"); scene.Config.Set("testBattleAddress", new string('X', 65)); scene.Config.Set("lag", "3");
        sbyte[] before = scene.Connections.PacketDelays().Distribution;
        Assert.Throws<ArgumentOutOfRangeException>(scene.Options.InterpretNetwork);
        Assert.Equal(7001u, scene.Game.GameServerPort.Value); Assert.All(scene.Local.TestBattleAddress.ToArray(), value => Assert.Equal((byte)'X', value));
        Assert.Same(before, scene.Connections.PacketDelays().Distribution);
    }
    /// <summary>서로 다른 Client의 현재 값 조합은 구성 시 거부한다.</summary>
    [Fact]
    public void DifferentClientOwnersAreRejectedBeforeInterpretation()
    {
        var scene = new Scene(); var other = new ClientLocalStateGlobals(new(), new());
        Assert.Throws<ArgumentException>(() => new ClientValueOptions(scene.Config, scene.Game, other, scene.Connections, scene.NsInfo, scene.ReadOnly));
    }
}
