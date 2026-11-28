using Netstorm.Core.Original;

// 전체435220 부모와 구별해 같은 현재 DWORD를 읽는 다섯 하위 몸체만 제공한다.
namespace Netstorm.Client;

/// <summary>ClientMain의 Cursor/MainMenu/FortFile/ScriptMenu/Network 옵션을 실제 장기 소유자에 연결한다.</summary>
public sealed class ClientValueOptions
{
    /// <summary>원문4353ea..43541a에서 유일하게 보존하는 치트 DWORD다.</summary>
    private const uint AllowedCheat = 0x845fed;
    /// <summary>해석 시점에 다시 읽는 같은 실제 설정이다.</summary>
    private readonly ConfigInterface _configuration;
    /// <summary>현재 Player/커서/치트/포트의 실제 소유자다.</summary>
    private readonly ClientGameplayGlobals _game;
    /// <summary>현재 안내/주소의 실제 물리다.</summary>
    private readonly ClientLocalStateGlobals _local;
    /// <summary>현재 같은 지연 분포를 빌린다. 조회 시 연결을 시작하지 않는다.</summary>
    private readonly ClientConnectionGlobals _connections;
    /// <summary>같은 서버 선택/방송 포트/진단 DWORD다.</summary>
    private readonly ClientNsInfoGlobals _nsInfo;
    /// <summary>같은 저장 억제 DWORD이며 bool로 축약하지 않는다.</summary>
    private readonly UIntBox _readOnlyForts;

    /// <summary>같은 Client 물리를 검증하고 빌린다. 구성 자체로 설정/포트/난수/파일을 변경하지 않는다.</summary>
    public ClientValueOptions(ConfigInterface configuration, ClientGameplayGlobals game, ClientLocalStateGlobals local,
        ClientConnectionGlobals connections, ClientNsInfoGlobals nsInfo, UIntBox readOnlyForts)
    {
        ArgumentNullException.ThrowIfNull(configuration); ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(local);
        ArgumentNullException.ThrowIfNull(connections); ArgumentNullException.ThrowIfNull(nsInfo); ArgumentNullException.ThrowIfNull(readOnlyForts);
        if (!ReferenceEquals(local.Gameplay, game) || !ReferenceEquals(connections.LocalState, local))
            throw new ArgumentException("옵션의 같은 현재 Client 소유자 필요", nameof(local));
        _configuration = configuration; _game = game; _local = local; _connections = connections; _nsInfo = nsInfo; _readOnlyForts = readOnlyForts;
    }
    /// <summary>현재 DWORD를 signed ReadInt로 해석해 누락은 보존하고 전체 비트를 그대로 되쓴다.</summary>
    private void Read(string key, UIntBox owner)
    {
        int value = unchecked((int)owner.Value); _configuration.ReadInt(key, ref value); owner.Value = unchecked((uint)value);
    }
    /// <summary>효과를 실행하지 않고 전체 부모의 같은 Config/Client/연결 대여를 검증한다.</summary>
    internal bool Borrows(ConfigInterface configuration, ClientGameplayGlobals game, ClientLocalStateGlobals local, ClientConnectionGlobals connections) =>
        ReferenceEquals(_configuration, configuration) && ReferenceEquals(_game, game) && ReferenceEquals(_local, local) && ReferenceEquals(_connections, connections);
    /// <summary>원문은 해석마다 회전 모드를0으로 쓴 뒤 현재 rotmode를 읽는다.</summary>
    public void InterpretCursor()
    {
        _game.CursorGlobals.RotationMode.Value = 0;
        Read("rotmode", _game.CursorGlobals.RotationMode);
    }
    /// <summary>현재 안내 여부→현재 순번→자동 데모를 읽으며 누락/상위 비트를 보존한다.</summary>
    public void InterpretMainMenu()
    {
        Read("tellTips", _local.TellTips); Read("tipNumber", _local.TipNumber); Read("autoDemo", _local.AutoDemo);
    }
    /// <summary>저장 억제 뒤 실제 NSInfo 세DWORD를 같은 순서로 읽으며 외부 검증을 시작하지 않는다.</summary>
    public void InterpretFortFiles()
    {
        Read("readOnlyForts", _readOnlyForts); _nsInfo.ReadOptions(_configuration);
    }
    /// <summary>특수 슬롯/보조 DWORD를0으로 쓴 뒤 유일한 허용 치트 값만 현재 치트에 남긴다.</summary>
    public void InterpretScriptMenu()
    {
        _game.Player.Special = 0; _game.SpecialAuxiliary.Value = 0;
        Read("userCheat", _game.UserCheat);
        if (_game.UserCheat.Value != AllowedCheat) _game.UserCheat.Value = 0;
    }
    /// <summary>현재 게임 포트→고정64BYTE Test Battle 주소→선택 지연 분포 순서이며 접속/시계/난수를 실행하지 않는다.</summary>
    public void InterpretNetwork()
    {
        Read("gameServerPort", _game.GameServerPort);
        _configuration.ReadStringBytes("testBattleAddress", _local.TestBattleAddress.Span);
        _connections.PacketDelays().SelectLag(_configuration.GetInt("lag"));
    }
}
