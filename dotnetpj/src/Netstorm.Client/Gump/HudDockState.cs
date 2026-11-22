using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 같은 공통 Gump 저장소와 HUD 지도 전역을 빌리는 영역이다.

/// <summary>원문 0x11cBYTE 구분선의 배경 기반과 축·제한·고정10슬롯이다.</summary>
public sealed class HudDividerNode : GumpBackgroundNode
{
    /// <summary>+b8의 크기 연동/드래그 제한/커서 플래그.</summary>
    public uint Axis { get; set; }
    /// <summary>+bc의 signed X 하한.</summary>
    public int MinX { get; set; }
    /// <summary>+c0의 signed X 상한.</summary>
    public int MaxX { get; set; }
    /// <summary>+c4의 signed Y 하한.</summary>
    public int MinY { get; set; }
    /// <summary>+c8의 signed Y 상한.</summary>
    public int MaxY { get; set; }
    /// <summary>+cc~118의 현재 serial/관계 플래그 쌍. 중복 serial을 허용한다.</summary>
    public (uint Serial, uint Flags)[] Slaves { get; } = new (uint, uint)[10];
}

/// <summary>생성 중인 실제 HUD this의 현재 필드를 조회할 때 사용하는 종류다.</summary>
public enum HudDockPart
{
    /// <summary>생성 중인 실제 HUD 부모.</summary>
    Root,
    /// <summary>+a0 세로 구분선.</summary>
    Vertical,
    /// <summary>+a4 미니맵 구분선.</summary>
    MinimapDivider,
    /// <summary>+a8 채팅 구분선.</summary>
    ChatDivider,
    /// <summary>+ac 미니맵.</summary>
    Minimap,
    /// <summary>+b0 생산 패널.</summary>
    Production,
    /// <summary>+b8 채팅 창.</summary>
    Chat,
    /// <summary>+c0 지식 패널.</summary>
    Knowledge,
}

/// <summary>원문 화면/지도 전역을 공통 트리·현재 Deck·미니맵 갱신 owner와 공유한다.
/// 생성자는 값을 초기화하지 않으며 실제 HUD 사건만 지도 영역을 작성한다.</summary>
public sealed class HudDockGlobals
{
    /// <summary>현재 화면 크기를 소유한 같은 생성 전역.</summary>
    private readonly GumpCreateState _create;
    /// <summary>지도 중앙 정렬과 Renderer 공급자가 함께 읽는 같은 트리 전역.</summary>
    private readonly GumpTreeState _tree;
    /// <summary>0054db14의 같은 장기 전체 Deck 모드.</summary>
    private readonly UIntBox _showHud;
    /// <summary>005949d4의 같은 장기 미니맵 갱신 DWORD.</summary>
    private readonly UIntBox _fullRefresh;
    /// <summary>실제 공통 화면/트리와 두 DWORD owner를 빌린다. 사본이나 기본 viewport를 만들지 않는다.</summary>
    public HudDockGlobals(GumpCreateState create, GumpTreeState tree, UIntBox showHud, UIntBox fullRefresh)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(tree);
        ArgumentNullException.ThrowIfNull(showHud); ArgumentNullException.ThrowIfNull(fullRefresh);
        _create = create; _tree = tree; _showHud = showHud; _fullRefresh = fullRefresh;
    }
    /// <summary>005c78f0의 현재 signed 화면 너비.</summary>
    public int ScreenWidth => _create.ScreenWidth;
    /// <summary>005c78f4의 현재 signed 화면 높이.</summary>
    public int ScreenHeight => _create.ScreenHeight;
    /// <summary>0054db14의 현재 Deck 표시 관문.</summary>
    public uint ShowHud => _showHud.Value;
    /// <summary>005ca99c의 현재 지도 원점 X.</summary>
    public int MapLeft { get; set; }
    /// <summary>005ca9a0의 현재 지도 원점 Y.</summary>
    public int MapTop { get; set; }
    /// <summary>005ca9b4의 현재 지도 폭.</summary>
    public int MapWidth { get; set; }
    /// <summary>005ca9b8의 현재 지도 높이.</summary>
    public int MapHeight { get; set; }
    /// <summary>005ca9bc~c8의 float32 지도 LTRB.</summary>
    public float[] MapFloatRect { get; } = new float[4];
    /// <summary>005ca9cc~d8의 정수 지도 LTRB. 공통 정렬기의 같은 저장소다.</summary>
    public ScreenRect MapRect { get => _tree.MapView; set => _tree.MapView = value; }
    /// <summary>005ca9a4~b0의 좌측 하단 제외 영역.</summary>
    public ScreenRect LeftBottomRect { get; set; }
    /// <summary>005949d4의 같은 미니맵 갱신 DWORD.</summary>
    public uint FullRefresh { get => _fullRefresh.Value; set => _fullRefresh.Value = value; }
    /// <summary>00542674의 숨기기 전 채팅 구분선 Y.</summary>
    public float SavedChatY { get; set; }
    /// <summary>전체 UI 조립이 동일한 화면/지도 트리 owner를 빌리는지 값 변경 없이 확인한다.</summary>
    internal bool BorrowsView(GumpCreateState create, GumpTreeState tree) => ReferenceEquals(_create, create) && ReferenceEquals(_tree, tree);
}

/// <summary>같은 기본/파생 노드와 실제 가상 호출·현재 화면 효과를 공급하는 경계다.</summary>
public sealed class HudDockHooks
{
    /// <summary>C++ 장치 구조체의 값 복사처럼 함수 슬롯만 복사하고 실제 callback/상태 owner는 공유한다.</summary>
    internal HudDockHooks CopyDevices() => (HudDockHooks)MemberwiseClone();
    /// <summary>현재 공통 기본 노드.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; set; }
    /// <summary>같은 객체의 구분선 확장.</summary>
    public Func<uint, HudDividerNode?>? Divider { get; set; }
    /// <summary>004655d0의 배경 생성. flags=1은 연결자가 그대로 공급한다.</summary>
    public Action<uint, float, float, uint, uint, uint>? ConstructBackground { get; set; }
    /// <summary>004653c0의 활성 등록.</summary>
    public Action<uint, uint>? Activate { get; set; }
    /// <summary>00466750의 기본 부착.</summary>
    public Action<uint, float, float, uint, uint>? BaseAttach { get; set; }
    /// <summary>가상+18의 실제 분리.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>가상+20의 실제 파생 부착.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>가상+10의 공통 Move.</summary>
    public Action<uint, float, float, uint>? Move { get; set; }
    /// <summary>00464020의 현재 serial 해결.</summary>
    public Func<uint, uint>? ResolveSerial { get; set; }
    /// <summary>00464e90의 실제 부모 사건 전달.</summary>
    public Action<uint, uint, uint, uint>? Bubble { get; set; }
    /// <summary>00466710의 팝업 정리.</summary>
    public Action? ClosePopups { get; set; }
    /// <summary>00499370의 x/y/폭/높이/플래그 dirty 요청.</summary>
    public Action<int, int, int, int, uint>? Invalidate { get; set; }
    /// <summary>004973c0의 실제 화면/카메라 후처리.</summary>
    public Action? FinishResize { get; set; }
    /// <summary>슬롯 한도 line83의 정상 복귀 가능한 원문 진단.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>00464990의 기본 입력 처리.</summary>
    public Func<uint, uint, uint, uint>? BaseHandle { get; set; }
    /// <summary>0042f250의 현재 채팅 표시 설정.</summary>
    public Action<bool>? ChatVisible { get; set; }
    /// <summary>0042f270의 현재 채팅 입력 표시.</summary>
    public Action<bool>? ChatInput { get; set; }
}
