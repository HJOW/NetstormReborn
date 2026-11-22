namespace Netstorm.Client.Gump; // 같은 실제 NSInfo 자식과 현재 지도 모드를 다루는 영역이다.

/// <summary>원본0xd8BYTE NSInfo의 전체 실제 자식 포인터다.</summary>
public sealed class HudNsInfoNode : GumpCreateNode
{
    /// <summary>+a0의 모든 하위 패널이 붙는 실제 배경이다.</summary>
    public uint Background { get; set; }
    /// <summary>+a4..c0의 현재 PlayerInfo8개다.</summary>
    public uint[] Players { get; } = new uint[8];
    /// <summary>+c4의 현재 참가자 목록이다.</summary>
    public uint Roster { get; set; }
    /// <summary>+c8의 현재 서버/zone 제목이다.</summary>
    public uint Title { get; set; }
    /// <summary>+cc의 위 구분 배경이다.</summary>
    public uint UpperDivider { get; set; }
    /// <summary>+d0의 아래 구분 배경이다.</summary>
    public uint LowerDivider { get; set; }
    /// <summary>+d4의 현재 BattleInfo다.</summary>
    public uint Battle { get; set; }
}

/// <summary>복제하지 않는 실제 현재 모드/PlayerKnown 조회다.</summary>
public sealed class HudNsInfoModeView
{
    /// <summary>00594fc8의 현재 메타맵 모드다.</summary>
    public required Func<uint> Metamap { get; init; }
    /// <summary>00595344의 현재 플레이어 인식 값이다.</summary>
    public required Func<uint> PlayerKnown { get; init; }
}

/// <summary>같은 NSInfo/배경 주소와 공통/가상 부착을 제공한다.</summary>
public sealed class HudNsInfoHooks
{
    /// <summary>고정this의 실제 확장 주소다.</summary>
    public required Func<uint, HudNsInfoNode?> Node { get; init; }
    /// <summary>현재 자식의 같은 기본 주소다.</summary>
    public required Func<uint, GumpCreateNode?> Base { get; init; }
    /// <summary>00466750의 실제 공통 부착이다.</summary>
    public required Action<uint, float, float, uint, uint> BaseAttach { get; init; }
    /// <summary>현재 배경의 실제 가상+18이다.</summary>
    public required Action<uint> Push { get; init; }
    /// <summary>현재 배경의 실제 가상+20이다.</summary>
    public required Action<uint, float, float, uint, uint> Attach { get; init; }
    /// <summary>일반 관문 뒤에만 요구하는 전체 실제 메타맵 배치다.</summary>
    public Action<uint>? MetamapLayout { get; init; }
}

/// <summary>cpppj/src/client/HudNsInfo.cpp의0047e9d0 일반 부착과 현재 모드 관문이다.</summary>
public sealed class HudNsInfo
{
    /// <summary>실제 같은 현재 모드의 함수 슬롯이다.</summary>
    private readonly HudNsInfoModeView _globals;
    /// <summary>같은 실제 기본/가상 부착 장치다.</summary>
    private readonly HudNsInfoHooks _hooks;
    /// <summary>실제 일반 앞단에 도달하는 장치만 생성 시 검증한다.</summary>
    public HudNsInfo(HudNsInfoModeView globals, HudNsInfoHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        if (globals.Metamap is null || globals.PlayerKnown is null || hooks.Node is null || hooks.Base is null || hooks.BaseAttach is null || hooks.Push is null || hooks.Attach is null) throw new ArgumentException("NSInfo의 같은 노드/트리 연결 필요");
        _globals = globals; _hooks = hooks;
    }
    /// <summary>호출 중 같은 주소를 유지하는 실제 NSInfo를 요구한다.</summary>
    private HudNsInfoNode Node(uint token) => (token != 0 ? _hooks.Node(token) : null) ?? throw new InvalidOperationException("NSInfo 실제 저장소 없음");
    /// <summary>현재 배경을 매 접근 위치에서 같은 공통 저장소로 조회한다.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? _hooks.Base(token) : null) ?? throw new InvalidOperationException("NSInfo 현재 배경 없음");
    /// <summary>기본 부착→현재 배경 Push→축별 현재 포인터/크기→가상 부착→모드/known 관문 순서다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        var node = Node(token); _hooks.BaseAttach(token, x, y, parent, mode); _hooks.Push(node.Background);
        float width = node.BaseWidth; var horizontal = Base(node.Background); horizontal.BaseWidth = width; horizontal.Width = (float)((double)width * horizontal.Scale);
        var vertical = Base(node.Background); float height = node.BaseHeight; vertical.BaseHeight = height; vertical.Height = (float)((double)height * vertical.Scale);
        _hooks.Attach(node.Background, 0, 0, token, 0);
        if (_globals.Metamap() == 0 || _globals.PlayerKnown() == 0) return;
        (_hooks.MetamapLayout ?? throw new InvalidOperationException("NSInfo 전체 메타맵 배치 미연결"))(token);
    }
}
