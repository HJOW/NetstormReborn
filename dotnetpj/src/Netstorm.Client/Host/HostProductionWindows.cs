using System.Buffers.Binary;
using System.Text;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // 같은 실제 생산 부모와 공통 UI의 현재 장치를 조립한다.

/// <summary>CombatGump의 실제 자식 포인터와 단일 유지 상태다.</summary>
public sealed class ProductionRootNode : GumpCreateNode
{
    /// <summary>+a0의 상단 배경이다.</summary>
    public uint Top { get; set; }
    /// <summary>+a4의 다리 배경이다.</summary>
    public uint Middle { get; set; }
    /// <summary>+a8의 텔레포트 배경이다.</summary>
    public uint Bottom { get; set; }
    /// <summary>+ac의 실제 예약 필드다.</summary>
    public uint ReservedAC { get; set; }
    /// <summary>+b0의 다리 격자다.</summary>
    public uint BridgeGrid { get; set; }
    /// <summary>+b4의 텔레포트 격자다.</summary>
    public uint TeleportGrid { get; set; }
    /// <summary>+b8의 실제 예산이다.</summary>
    public uint Budget { get; set; }
    /// <summary>+bc의 실제 구분선이다.</summary>
    public uint Separator { get; set; }
    /// <summary>+c0의 예약 필드다.</summary>
    public uint ReservedC0 { get; set; }
    /// <summary>+c4의 예약 필드다.</summary>
    public uint ReservedC4 { get; set; }
    /// <summary>+c8/d8/dc/e0를 유지 본체와 같은 객체로 빌린다.</summary>
    public ProductionMaintenancePanel Panel { get; } = new();
    /// <summary>+d0의 실제 두 번째 시계다.</summary>
    public double SecondTimer { get; set; }
}

/// <summary>43e490의 같은 TextGump 파생과 실제 예산 필드다.</summary>
public sealed class ProductionBudgetNode : TextGumpNode
{
    /// <summary>+a4의 현재 표시 금액이다.</summary>
    public int Amount { get; set; }
    /// <summary>+a8의 다음 점멸 벽시각이다.</summary>
    public double Until { get; set; }
    /// <summary>+b0의 남은 점멸 횟수다.</summary>
    public int Flash { get; set; }
}

/// <summary>실제 UI 장치와 선택 위치에서만 요구하는 현재 Runtime 효과다.</summary>
public sealed class HostProductionDevices
{
    /// <summary>현재 실제 벽시계다.</summary>
    public required Func<double> Now { get; init; }
    /// <summary>현재 실제 이름 음향이다.</summary>
    public required Action<string> Sound { get; init; }
    /// <summary>현재 실제 미션 알림이다.</summary>
    public required Action<string> Tell { get; init; }
    /// <summary>현재 실제 설정과 원문 누락 기본값이다.</summary>
    public required Func<string, int, int> ConfigInt { get; init; }
    /// <summary>현재 실제 번역이며 생성 중 선택 위치에서 요구한다.</summary>
    public Func<string, string>? Translate { get; init; }
    /// <summary>같은 공통 UI 구조 변경 통지다.</summary>
    public required Action Changed { get; init; }
    /// <summary>같은 메뉴의 실제 기본 도움말 처리다.</summary>
    public required Func<uint, uint, uint, uint, uint> BaseHandle { get; init; }
    /// <summary>실제 격자/항목의 현재 기본 재그리기다.</summary>
    public required Action<uint, bool> Redraw { get; init; }
    /// <summary>현재 내용물 Form의 실제 Native/Session 부착이다.</summary>
    public Func<uint, uint, uint>? BindForm { get; set; }
    /// <summary>실제 ClientProductionSync이며 Worldless/borrowed 관문도 같은 장치가 보존한다.</summary>
    public Action? Synchronize { get; set; }
    /// <summary>현재 Session의 실제 dirty/생산/동기화 갱신이다.</summary>
    public Func<bool>? UpdateContents { get; set; }
    /// <summary>같은 현재 패널/갱신 함수를 빌리는 실제 Session 유지 본체 생성이다.</summary>
    public Func<ProductionMaintenancePanel, Action<uint>, RawProductionMaintenance>? Maintenance { get; set; }
    /// <summary>현재 실제 커서 취소다.</summary>
    public Action? CancelCursor { get; set; }
    /// <summary>현재 실제 내용물 취득이다.</summary>
    public Action<uint, uint>? Take { get; set; }
    /// <summary>같은 내용물/현재 Affordable를 실제 정보 메뉴에 연결한다.</summary>
    public Func<InventoryItemNode, ScreenPoint, Func<uint, bool>, uint>? About { get; set; }
    /// <summary>구분선이 선택한 실제 현재 마우스 폴링이다.</summary>
    public Func<uint, InputEvent>? Poll { get; set; }
    /// <summary>구분선이 선택한 실제 커서 종류다.</summary>
    public Action<uint>? Cursor { get; set; }
    /// <summary>현재 실제 controlled 메뉴 개수다.</summary>
    public Func<uint>? MenuInstances { get; set; }
    /// <summary>같은 실제 팝업 정리다.</summary>
    public Action? ClosePopups { get; set; }
}

/// <summary>Source RawProductionControls의 실제 공통 트리/자식/예산/현재 owner 조립이다.</summary>
public sealed partial class HostProductionWindows : IDisposable
{
    /// <summary>원문 float을 승격한 점멸 간격이다.</summary>
    private const double FlashInterval = 0.15000000596046448;
    /// <summary>같은 실제 트리다.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>같은 실제 문자/스타일 owner다.</summary>
    private readonly HostHtmlText _text;
    /// <summary>실제 현재 물리/전역 문맥이다.</summary>
    private readonly ProductionControlContext _context;
    /// <summary>같은 장기 생산 전역이다.</summary>
    private readonly ProductionMaintenanceGlobals _globals;
    /// <summary>현재 실제 외부 효과다.</summary>
    private readonly HostProductionDevices _devices;
    /// <summary>실제 같은 부모 파생이다.</summary>
    private readonly ProductionRootNode _root = new();
    /// <summary>실제 같은 예산 파생이다.</summary>
    private readonly ProductionBudgetNode _budget = new();
    /// <summary>생성/재부착이 같은 실제 격자 본체다.</summary>
    private InventoryGump _grid = null!;
    /// <summary>같은 실제 내용물 항목 본체다.</summary>
    private InventoryItemGump _item = null!;
    /// <summary>같은 실제 이미지/픽셀 적중 본체다.</summary>
    private ImageGump _image = null!;
    /// <summary>현재 실제 이미지 전역이다.</summary>
    private readonly ImageGumpState _imageState = new() { ShadowOffset = new(5, 5) };
    /// <summary>같은 실제 구분선 기하다.</summary>
    private HudDividerGump _divider = null!;
    /// <summary>같은 실제 구분선 입력이다.</summary>
    private HudDividerInput _dividerInput = null!;
    /// <summary>같은 실제 안내 본체다.</summary>
    private ProductionHintGump _hint = null!;
    /// <summary>생산 안내의 실제 소유 문자열이다.</summary>
    private readonly Dictionary<uint, string> _hintStrings = [];
    /// <summary>단조 발급하는 원문 문자열 별칭이다.</summary>
    private uint _hintStringSerial = 0x70000000;
    /// <summary>같은 패널 상태를 빌린 실제 유지 본체다.</summary>
    private RawProductionMaintenance? _maintenance;
    /// <summary>현재 실제 부모 토큰이다.</summary>
    private uint _token;
    /// <summary>생성 중 추가 config 조회를 억제하는 호스트 범위다.</summary>
    private bool _constructing;
    /// <summary>실제 최초 옵션에서 정한 슬롯 한도다.</summary>
    private uint _slotLimit;
    /// <summary>현재 원문 항목 정책의 입력 금지 DWORD다.</summary>
    private uint _inputDisabled = 0;
    /// <summary>현재 마지막 항목 종류다.</summary>
    private readonly UIntBox _lastType = new();
    /// <summary>같은 현재 변환표를 색표 설치 때 교체한다.</summary>
    private ProductionColorMaps _colors = new();
    /// <summary>같은 현재 명암표다.</summary>
    private GumpShadeMaps _shades = new();
    /// <summary>독립 문맥의 기존 색표 대체 저장소다. 실제 Client는 OwnerColorTables를 빌린다.</summary>
    private readonly Dictionary<uint, OwnerColorMaps> _localColors = [];
    /// <summary>현재 그림 호출 동안만 빌리는 이미지 출력이다.</summary>
    private ImageGumpHooks? _activeImage;
    /// <summary>현재 그림 호출 동안만 빌리는 질감 장치다.</summary>
    private GumpBackgroundDraw? _activeBackground;
    /// <summary>현재 외부 출력 등록이다.</summary>
    private uint _drawing;
    /// <summary>이전 공통 반납 장치다.</summary>
    private Action<uint> _releaseBase = null!;
    /// <summary>출력/가상 슬롯 대여가 끝났는지다.</summary>
    private bool _disposed;
    /// <summary>실제 선택한 하위만 현재 호출 위치에서 요구한다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("생산 부모 실제 현재 장치 미연결");
    /// <summary>공통 owner만 검증/연결하며 Construct/Open은 호출자가 원문 위치에서 선택한다.</summary>
    public HostProductionWindows(GumpButtonTree tree, HostHtmlText text, ProductionControlContext context, ProductionMaintenanceGlobals globals, HostProductionDevices devices)
    {
        ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(text); ArgumentNullException.ThrowIfNull(context); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(devices);
        if (!ReferenceEquals(tree, text.Tree) || context.Storage is null || context.Gameplay is null || context.Players is null || context.Options is null || context.Options.Length != 87 || context.Storage.Pool.Edition != OriginalEdition.Patch1078 || context.Storage.Source() is not { } assets || !text.SharesPictureTypes(assets) || !ReferenceEquals(context.Storage.Authority(), context.Gameplay.Authority) || !ReferenceEquals(context.Storage.GameAux(), context.Gameplay.GameAux)) throw new ArgumentException("생산 부모의 실제 공통 트리/물리/자산 연결 불일치");
        if (devices.Now is null || devices.Sound is null || devices.Tell is null || devices.ConfigInt is null || devices.Changed is null || devices.BaseHandle is null || devices.Redraw is null) throw new ArgumentException("생산 부모 실제 UI 장치 누락");
        _tree = tree; _text = text; _context = context; _globals = globals; _devices = devices; BuildChildren(text.RequireProductionAssets(assets)); Bind();
    }
    /// <summary>같은 현재 생산 Root 토큰이다.</summary>
    public uint RootToken => _token;
    /// <summary>같은 실제 owner의 현재 파생만 조회한다.</summary>
    public GumpCreateNode? Node(uint token) => ReferenceEquals(_tree.NodeOwner(token), this) ? _tree.Node(token) : null;
    /// <summary>같은 실제 현재 Text 파생만 조회한다.</summary>
    private TextGumpNode? Text(uint token) => Node(token) as TextGumpNode;
    /// <summary>현재 등록/부분 생성 저장소가 남았는지 실제 owner 장부를 읽는다.</summary>
    public bool HasNodes => _tree.OwnedNodes(this).Length != 0;
    /// <summary>같은 실제 원문 WORD다.</summary>
    private uint Word(uint sid, int offset) => BinaryPrimitives.ReadUInt16LittleEndian(_context.Storage.Pool.Slot(unchecked((ushort)sid))[offset..]);
    /// <summary>같은 실제 owner로만 파생을 할당한다.</summary>
    private uint Allocate(GumpCreateNode node) => _tree.WithNodeOwner(this, () => _tree.AllocateNode(node));
    /// <summary>현재 같은 자식 우선 Kill을 실행한다.</summary>
    private void ClearChildren(uint token) { while (_tree.Node(token)!.Child != 0) _tree.Lifetime.Kill(_tree.Node(token)!.Child); }
    /// <summary>현재 원문 전역을 읽고 생성 범위 밖에서만 설정을 다시 읽는다.</summary>
    private void Prepare() { _imageState.LocalOwner = _context.Gameplay.Player.BattleOwner; _globals.BridgeType = _context.Storage.Links.BridgeType; if (!_constructing) _globals.StockEnabled = unchecked((uint)_devices.ConfigInt("useProductTimers", 1)); }
    /// <summary>같은 실제 SP를 World 없는 Client에서도 읽는다.</summary>
    private float Money(uint owner) => BitConverter.UInt32BitsToSingle(_context.Sp is { } sp ? sp.Store.Get(owner) : _context.Gameplay.RequireSession().Sp.Get(owner));
    /// <summary>현재 실제 기술은 known0일 때만 SID0이다.</summary>
    private uint Technology() => _context.Gameplay.Player.PlayerKnown == 0 ? 0 : PlayerField(_context.Gameplay.Player.BattleOwner, 0x9c);
    /// <summary>현재 실제9행 기록의 DWORD만 읽는다.</summary>
    private uint PlayerField(uint owner, int offset)
    {
        var records = _context.Players.Records.Span; long at = (long)owner * 180 + offset; if (owner >= 9 || offset < 0 || offset > 176 || at + 4 > records.Length) throw new ArgumentOutOfRangeException(nameof(owner), "생산 현재 Player DWORD 없음"); return BinaryPrimitives.ReadUInt32LittleEndian(records[(int)at..]);
    }
    /// <summary>같은 실제 스타일 체인을 변경한다.</summary>
    private void Style(uint token, string value) { var node = Text(token) ?? throw new InvalidOperationException("생산 실제 Text 없음"); _text.Style.Parse(ref node.Head, Encoding.Latin1.GetBytes(value), node.TextContext); }
}
