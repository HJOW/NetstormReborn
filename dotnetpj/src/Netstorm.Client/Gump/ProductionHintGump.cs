using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 실제 생산 실패 안내의 원문 문자·벽시계·문자열 수명이다.

/// <summary>원본 TextGump 파생0xb8의 실제 확장 주소다.</summary>
public sealed class ProductionHintNode : TextGumpNode
{
    /// <summary>생성자가 쓰지 않는+a4다.</summary>
    public uint Reserved { get; set; }
    /// <summary>게임 속도와 독립인 벽시계 만료 시각이다.</summary>
    public double Until { get; set; }
    /// <summary>복제한 문자열의 소유 토큰이다.</summary>
    public uint Owned { get; set; }
    /// <summary>안내의 입력 영역에 사용하는 실제 기준 노드다.</summary>
    public uint Reference { get; set; }
}

/// <summary>다른 생산/안내 owner도 공유하는 같은 현재 전역이다.</summary>
public sealed class ProductionHintState
{
    /// <summary>00540434의 최초 종류 BYTE25다.</summary>
    public sbyte Kind { get; set; } = 25;
    /// <summary>현재 팔레트의 흰색이다.</summary>
    public uint White { get; set; } = 7;
    /// <summary>현재 팔레트의 검정이다.</summary>
    public uint Black { get; set; }
    /// <summary>마지막 실제 생성 안내의 원문 serial이다.</summary>
    public uint LastSerial { get; set; }
}

/// <summary>기존 문자/스타일/수집/수명과 실제 시계에 연결한다.</summary>
public sealed class ProductionHintHooks
{
    /// <summary>같은 실제 파생 저장소다.</summary>
    public required Func<uint, ProductionHintNode?> Node { get; init; }
    /// <summary>현재 벽시계 초다.</summary>
    public required Func<double> Now { get; init; }
    /// <summary>0xb8 실제 파생 할당이다.</summary>
    public required Func<int, uint> Allocate { get; init; }
    /// <summary>같은 실제 한 줄 문자 생성이다.</summary>
    public required Action<uint> ConstructText { get; init; }
    /// <summary>같은 공통 Kill 예약이다.</summary>
    public required Action<uint> Kill { get; init; }
    /// <summary>원문 C문자열의 실제 복제 소유권이다.</summary>
    public required Func<string, uint> Duplicate { get; init; }
    /// <summary>같은 현재 소유 바이트다.</summary>
    public required Func<uint, string> String { get; init; }
    /// <summary>같은 실제 소유 문자열 반납이다.</summary>
    public required Action<uint> Free { get; init; }
    /// <summary>현재 실제 번역이다.</summary>
    public required Func<string, string> Translate { get; init; }
    /// <summary>현재 기본 문맥의 실제 파싱이다.</summary>
    public required Action<uint, string> Style { get; init; }
    /// <summary>같은 실제 글자 측정/여백이다.</summary>
    public required Action<uint, int, int> Fit { get; init; }
    /// <summary>같은 공통 Idle 등록이다.</summary>
    public required Action<uint, bool> Idle { get; init; }
    /// <summary>같은 현재 경계다.</summary>
    public required Func<uint, ScreenRect> Bounds { get; init; }
    /// <summary>같은 현재 위치다.</summary>
    public required Func<uint, ScreenPoint> Position { get; init; }
    /// <summary>같은 실제 head를 현재 위치에 출력한다.</summary>
    public required Action<uint, ScreenPoint> Draw { get; init; }
    /// <summary>같은 실제 가상 부착이다.</summary>
    public required Action<uint, float, float, uint, uint> Attach { get; init; }
    /// <summary>기존 Text 소멸과 선택 물리 반납이다.</summary>
    public required Func<uint, uint, uint> DeleteText { get; init; }
}

/// <summary>Source ProductionHintGump의 생성/두 안내 wrapper/Idle/Draw/scalar 전체다.</summary>
public sealed class ProductionHintGump
{
    /// <summary>원본10.78의 실제 파생 가상표다.</summary>
    private const uint Vtable = 0x50c2b4;
    /// <summary>같은 실제 등록/루트 전역이다.</summary>
    private readonly GumpCreateState _create;
    /// <summary>같은 현재 종류/팔레트/serial 전역이다.</summary>
    private readonly ProductionHintState _state;
    /// <summary>같은 실제 하위 장치다.</summary>
    private readonly ProductionHintHooks _hooks;
    /// <summary>원문의 모든 실제 경계를 검증하고 빌린다.</summary>
    public ProductionHintGump(GumpCreateState create, ProductionHintState state, ProductionHintHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (create.Collection.Edition != OriginalEdition.Patch1078 || hooks.Node is null || hooks.Now is null || hooks.Allocate is null || hooks.ConstructText is null || hooks.Kill is null || hooks.Duplicate is null || hooks.String is null || hooks.Free is null || hooks.Translate is null || hooks.Style is null || hooks.Fit is null || hooks.Idle is null || hooks.Bounds is null || hooks.Position is null || hooks.Draw is null || hooks.Attach is null || hooks.DeleteText is null) throw new ArgumentException("생산 안내 실제 경계 누락");
        _create = create; _state = state; _hooks = hooks;
    }
    /// <summary>효과 뒤에도 현재 같은 파생 저장소를 다시 읽는다.</summary>
    private ProductionHintNode Node(uint token) => _hooks.Node(token) ?? throw new InvalidOperationException("생산 안내 노드 없음");
    /// <summary>기본 생성→복제/파싱/맞춤→Idle 등록→현재 벽시계+4 순서다.</summary>
    public uint Construct(uint token, string text)
    {
        _hooks.ConstructText(token); Node(token).Vtable = Vtable; Node(token).Until = 0; Node(token).Command = unchecked((uint)(int)_state.Kind);
        uint[] context = [0, 6, _state.White, 0, 0, _state.Black, 1, 1]; context.CopyTo(Node(token).TextContext, 0); Node(token).Reference = 0; Node(token).Owned = _hooks.Duplicate(text);
        _hooks.Style(token, _hooks.String(Node(token).Owned)); _hooks.Fit(token, 6, 4); _hooks.Idle(token, true); Node(token).Until = _hooks.Now() + 4.0; return token;
    }
    /// <summary>입력 기준 영역을 저장하고 y+20의 실제 루트에 부착한 뒤 현재 serial을 게시한다.</summary>
    public uint Show(bool empty, int x, int y, uint reference)
    {
        uint token = _hooks.Allocate(0xb8); if (token == 0) throw new OutOfMemoryException(); Construct(token, _hooks.Translate(empty ? "~0~w~ENone in inventory" : "~0~w~ENot enough Storm Power to build this")); Node(token).Flags |= 0x4000;
        var rect = _hooks.Bounds(reference); var node = Node(token); node.Positioning[2] = BitConverter.SingleToUInt32Bits(rect.Left); node.Positioning[3] = BitConverter.SingleToUInt32Bits(rect.Top); node.Positioning[4] = BitConverter.SingleToUInt32Bits(rect.Right); node.Positioning[5] = BitConverter.SingleToUInt32Bits(rect.Bottom); node.Flags |= 0x8100; node.Depth = 0xffffff9c; node.Reference = reference;
        _hooks.Attach(token, x, unchecked(y + 20), _create.Collection.Root, 0); _state.LastSerial = Node(token).Serial; return token;
    }
    /// <summary>NaN을 만료로 만들지 않는 현재 시각 비교와 실제 Kill이다.</summary>
    public void Idle(uint token) { if (Node(token).Until <= _hooks.Now()) _hooks.Kill(token); }
    /// <summary>빈 head에서도 현재 경계/위치 뒤(3,2) 여백의 실제 스타일 출력을 호출한다.</summary>
    public void Draw(uint token) { _hooks.Bounds(token); var p = _hooks.Position(token); _hooks.Draw(Node(token).Head, new(unchecked(p.X + 3), unchecked(p.Y + 2))); }
    /// <summary>가상표→소유 문자열 반납→Text scalar이며 소유 포인터 필드를0으로 쓰지 않는다.</summary>
    public uint DeleteScalar(uint token, uint flags) { Node(token).Vtable = Vtable; if (Node(token).Owned != 0) _hooks.Free(Node(token).Owned); return _hooks.DeleteText(token, flags); }
}
