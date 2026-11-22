using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 같은 현재 내용물과 상위 메뉴 작성기를 연결한다.

/// <summary>원문 종류/명령의 독립 값과 실제 Client의 현재 별칭을 구별한다.</summary>
public sealed class InventoryAboutState
{
    /// <summary>독립 입력의 회수 제외 클래스다.</summary>
    public uint SuppressedClass { get; set; } = 122;
    /// <summary>독립 입력의 개수0 제단 보정 종류다.</summary>
    public uint None { get; set; } = 164;
    /// <summary>독립 입력의 개수0 다이스 보정 종류다.</summary>
    public uint Dais { get; set; } = 165;
    /// <summary>독립 입력의 전액 회수 너겟 종류다.</summary>
    public uint FullValue { get; set; } = 154;
    /// <summary>현재 실제 회수 명령이다.</summary>
    public uint SalvageCommand { get; set; } = 0x0200005d;
    /// <summary>현재 실제 Quick Help 명령이다.</summary>
    public uint HelpCommand { get; set; } = 0x02000017;
    /// <summary>실제 현재00541240의 클래스 별칭이다.</summary>
    public Func<uint>? SharedSuppressedClass { get; set; }
    /// <summary>실제 현재005412e8의 제단 TYPE다.</summary>
    public Func<uint>? SharedAltarType { get; set; }
    /// <summary>실제 현재005412ec의 다이스 TYPE다.</summary>
    public Func<uint>? SharedDaisType { get; set; }
    /// <summary>실제 현재005412c0의 너겟 TYPE다.</summary>
    public Func<uint>? SharedNuggetType { get; set; }
    /// <summary>편집 관문 뒤 같은 현재 제외 클래스를 읽는다.</summary>
    public uint ReadSuppressedClass() => SharedSuppressedClass?.Invoke() ?? SuppressedClass;
    /// <summary>비용 조회 뒤 현재 제단 종류를 읽는다.</summary>
    public uint AltarType() => SharedAltarType?.Invoke() ?? None;
    /// <summary>현재 다이스 종류다.</summary>
    public uint DaisType() => SharedDaisType?.Invoke() ?? Dais;
    /// <summary>현재 너겟 종류다.</summary>
    public uint NuggetType() => SharedNuggetType?.Invoke() ?? FullValue;
}

/// <summary>실제 현재 조회와 메뉴 작성/가상 부착을 공급한다.</summary>
public sealed class InventoryAboutHooks
{
    /// <summary>실제 fancy 메뉴 시작이다.</summary>
    public required Func<uint> Begin { get; init; }
    /// <summary>현재 등록 루트다.</summary>
    public required Func<uint> Root { get; init; }
    /// <summary>현재 템플릿 소유자다.</summary>
    public required Func<uint> TemplateOwner { get; init; }
    /// <summary>현재 편집 DWORD의 참/거짓이다.</summary>
    public required Func<bool> Editor { get; init; }
    /// <summary>같은 실제 비용표의 정수다.</summary>
    public required Func<uint, int> Cost { get; init; }
    /// <summary>기존 현재 SP/편집/비용 비교다.</summary>
    public required Func<uint, bool> Affordable { get; init; }
    /// <summary>NULL과 빈 문자열을 구별하는 현재 도움말이다.</summary>
    public required Func<string?> HelpWord { get; init; }
    /// <summary>공통 실제 Idle 등록이다.</summary>
    public required Action<uint, bool> Idle { get; init; }
    /// <summary>같은 내용물 Form 연결이다.</summary>
    public required Action<uint, uint> Reference { get; init; }
    /// <summary>기존 설명 작성의 네 인자다.</summary>
    public required Action<uint, uint, uint, uint> Describe { get; init; }
    /// <summary>원문 조회 위치의 현재 번역이다.</summary>
    public required Func<string, string> Translate { get; init; }
    /// <summary>번역하지 않는 실제 정수 한 개 형식화다.</summary>
    public required Func<string, int, string> Print { get; init; }
    /// <summary>실제 메뉴 제목 머리다.</summary>
    public required Action<string> Header { get; init; }
    /// <summary>실제 원문 열기 음향이다.</summary>
    public required Action<string> Sound { get; init; }
    /// <summary>NULL/명령/인자/선택 비트를 보존하는 메뉴 항목 작성이다.</summary>
    public required Action<string?, uint, uint, bool, bool, bool> Entry { get; init; }
    /// <summary>현재 controlled 메뉴의 실제 가상 부착이다.</summary>
    public required Action<ScreenPoint, uint> Attach { get; init; }
}

/// <summary>Source InventoryAboutMenu의43d330 전체와 현재 별칭/회수 산술이다.</summary>
public sealed class InventoryAboutMenu
{
    /// <summary>같은 실제 내용물 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>같은 현재 명령/별칭 owner다.</summary>
    private readonly InventoryAboutState _state;
    /// <summary>실제 메뉴/조회/음향 장치다.</summary>
    private readonly InventoryAboutHooks _hooks;
    /// <summary>모든 실제 선택 경계를 검증하고 PATCH 물리를 빌린다.</summary>
    public InventoryAboutMenu(RawSidPool pool, InventoryAboutState state, InventoryAboutHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || hooks.Begin is null || hooks.Root is null || hooks.TemplateOwner is null || hooks.Editor is null || hooks.Cost is null || hooks.Affordable is null || hooks.HelpWord is null || hooks.Idle is null || hooks.Reference is null || hooks.Describe is null || hooks.Translate is null || hooks.Print is null || hooks.Header is null || hooks.Sound is null || hooks.Entry is null || hooks.Attach is null) throw new ArgumentException("생산 정보 실제 경계 누락");
        _pool = pool; _state = state; _hooks = hooks;
    }
    /// <summary>효과 뒤 같은 현재 내용물의 비정렬 WORD를 다시 읽는다.</summary>
    private uint Word(int sid, int offset) => BinaryPrimitives.ReadUInt16LittleEndian(_pool.Slot(sid)[offset..]);
    /// <summary>정수 비용을 float으로 저장한 뒤 넓은 합/quarter와 signed64 절삭 하위DWORD를 보존한다.</summary>
    private int Refund(uint kind) { float cost = _hooks.Cost(kind); double sum = kind == _state.AltarType() || kind == _state.DaisType() ? 0 : cost; if (kind != _state.NuggetType()) sum *= 0.25; return unchecked((int)(long)sum); }
    /// <summary>설명→비용/부족 머리→회수/Help→현재 루트 부착→음향의 실제 순서다.</summary>
    public uint Open(InventoryItemNode item, ScreenPoint point)
    {
        ArgumentNullException.ThrowIfNull(item); int content = unchecked((ushort)item.Content); _pool.Slot(content); uint menu = _hooks.Begin(); _hooks.Idle(menu, true); _hooks.Reference(menu, item.Content);
        uint owner = _hooks.TemplateOwner(); _hooks.Describe(Word(content, 18), 0, owner, uint.MaxValue); uint kind = Word(content, 18); string costPattern = _hooks.Translate("Cost: ~i%d~[IfortGump.3]~.\n"); int cost = _hooks.Cost(kind); _hooks.Header(_hooks.Print(costPattern, cost));
        if (!_hooks.Affordable((uint)content)) _hooks.Header(_hooks.Translate("~r~ENeed more Storm Power to Build!\n"));
        if (!_hooks.Editor() && (Word(content, 20) & 0x20) != 0 && _pool.Slot(content)[10] != _state.ReadSuppressedClass()) { string pattern = _hooks.Translate("Salvage for %d~[IfortGump.3]"); int refund = Refund(Word(content, 18)); string label = _hooks.Print(pattern, refund); _hooks.Entry(label, _state.SalvageCommand, item.Object, true, true, false); }
        uint helpKind = Word(content, 18); string? help = _hooks.HelpWord(); _hooks.Entry(help, _state.HelpCommand, helpKind, true, true, false); _hooks.Attach(point, _hooks.Root()); _hooks.Sound("opengump.wav"); return 1;
    }
}
