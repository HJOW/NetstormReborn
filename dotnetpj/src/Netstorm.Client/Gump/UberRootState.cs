namespace Netstorm.Client.Gump; // 원문 전체 HUD 부모의 같은 기본/파생 저장소와 실제 장치 경계다.

/// <summary>004c7160의0xc8BYTE Uber 부모. 예약 두 필드는 생성자가 보존한다.</summary>
public sealed class UberRootNode : GumpCreateNode
{
    /// <summary>+a0 세로 구분선.</summary>
    public uint Vertical { get; set; }
    /// <summary>+a4 미니맵 구분선.</summary>
    public uint MinimapDivider { get; set; }
    /// <summary>+a8 채팅 구분선.</summary>
    public uint ChatDivider { get; set; }
    /// <summary>+ac 미니맵.</summary>
    public uint Minimap { get; set; }
    /// <summary>+b0 생산 패널.</summary>
    public uint Production { get; set; }
    /// <summary>+b4 원문 미작성 예약 DWORD.</summary>
    public uint ReservedB4 { get; set; }
    /// <summary>+b8 채팅 창.</summary>
    public uint Chat { get; set; }
    /// <summary>+bc 원문 미작성 예약 DWORD.</summary>
    public uint ReservedBC { get; set; }
    /// <summary>+c0 지식 패널.</summary>
    public uint Knowledge { get; set; }
    /// <summary>+c4 배경 메시지 GIF 창. 메시지 없는 생성에서는 보존한다.</summary>
    public uint Message { get; set; }
}

/// <summary>실제 원본 전역과 전체 자식 공장/가상 수명만 빌린다. 등록표나 노드를 복제하지 않는다.</summary>
public sealed class UberRootHooks
{
    /// <summary>같은 고정 this의 실제 파생.</summary>
    public Func<uint, UberRootNode?>? Node { get; set; }
    /// <summary>실제 세 구분선의 같은 확장 객체.</summary>
    public Func<uint, HudDividerNode?>? Divider { get; set; }
    /// <summary>기존/새 부모와 모든 실제 자식의 기본 주소.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>종류 BYTE/화면/TYPE/frame/메시지/명령을 실제 소비 위치에서 읽는다.</summary>
    public Func<uint, uint>? Read { get; set; }
    /// <summary>배경/프레임 전역 세 곳의 실제 쓰기.</summary>
    public Action<uint, uint>? Write { get; set; }
    /// <summary>0xc8 부모/0x11c 구분선/0xb0 GIF의 실제 파생 저장소.</summary>
    public Func<int, uint>? Allocate { get; set; }
    /// <summary>같은 GumpCreate.Construct.</summary>
    public Action<uint, uint, float, float, uint>? ConstructBase { get; set; }
    /// <summary>같은 HudDividerGump.Construct 전체 본체.</summary>
    public Action<uint, uint, int, int, uint, uint, uint>? ConstructDivider { get; set; }
    /// <summary>실제 미니맵 전체 공장.</summary>
    public Func<uint>? CreateMinimap { get; set; }
    /// <summary>실제 생산 패널 전체 공장.</summary>
    public Func<uint>? CreateProduction { get; set; }
    /// <summary>실제 지식 패널 전체 공장.</summary>
    public Func<uint>? CreateKnowledge { get; set; }
    /// <summary>실제 채팅 창 전체 공장.</summary>
    public Func<uint>? CreateChat { get; set; }
    /// <summary>4437f0은 실제1BYTE RET다. 선택적 호출 관측만 제공한다.</summary>
    public Action? EmptyObserver { get; set; }
    /// <summary>42f540의 현재 기술 표시 갱신.</summary>
    public Action? RefreshTechnology { get; set; }
    /// <summary>4662b0의 같은 공통 지연 삭제 정리.</summary>
    public Action? Cleanup { get; set; }
    /// <summary>각 실제 파생의 가상+20 부착.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>현재 위치/기준점의 가상+10 Move.</summary>
    public Action<uint, float, float, uint>? Move { get; set; }
    /// <summary>현재 링크의 가상+18 분리.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>004c4790의 실제 고정 슬롯 등록.</summary>
    public Action<uint, uint, uint>? RegisterSlave { get; set; }
    /// <summary>0049a840의 현재 TYPE 조회. 이 부모는 반환값을 읽지 않는다.</summary>
    public Func<uint, uint>? Type { get; set; }
    /// <summary>Chat Window 번역 포인터.</summary>
    public Func<string, uint>? Translate { get; set; }
    /// <summary>00464ed0의 같은 번역 포인터 보관.</summary>
    public Action<uint, uint>? Help { get; set; }
    /// <summary>00464f80의 chatView 도움말 키 보관.</summary>
    public Action<uint, string>? HelpWord { get; set; }
    /// <summary>00565e70 현재 C문자열의 같은 원본 포인터 토큰.</summary>
    public Func<uint>? SavedMessage { get; set; }
    /// <summary>현재 C문자열의 대소문자 무시 비교.</summary>
    public Func<uint, string, bool>? EqualText { get; set; }
    /// <summary>0041a030의 현재 이름/BackgroundGifSpec 전체 창 생성.</summary>
    public Action<uint, uint>? ConstructMessage { get; set; }
    /// <summary>459c60의 원문9DWORD 인자로 실제 조건 표현식을 작성한다.</summary>
    public Func<uint[], uint>? FormatMessage { get; set; }
    /// <summary>41ab10의 완성 표현식 검사.</summary>
    public Func<uint, uint>? TestMessage { get; set; }
    /// <summary>4b6100의 실제 현재 메시지 방문 기록.</summary>
    public Action<uint>? VisitMessage { get; set; }
    /// <summary>419e10의 실제 GIF 사건/인자.</summary>
    public Action<uint, uint, uint, uint>? MessageEvent { get; set; }
    /// <summary>466600의 같은 공통 overlay 등록.</summary>
    public Action<uint, bool>? Overlay { get; set; }
    /// <summary>4652f0의 같은 현재 자식 우선 깊이 쓰기.</summary>
    public Action<uint, uint>? SetDepth { get; set; }
    /// <summary>4666a0의 이전 부모 Kill/Cleanup과 실제 등록0 교체.</summary>
    public Action<uint>? ReplaceRoot { get; set; }
    /// <summary>004c7880이 기존 부모 비영일 때 먼저 선택하는00466480.</summary>
    public Action<uint>? Kill { get; set; }
}
