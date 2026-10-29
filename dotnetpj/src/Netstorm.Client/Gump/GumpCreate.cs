using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 원본 Gump 생성과 등록의 공유 저장소 영역이다.

/// <summary>원본 기본 Gump 0xa0바이트의 의미별 필드. 표시 필드는 복제하지 않고 기반 노드와 공유한다.
/// 근거: cpppj/src/client/GumpCreate.h. 원본이 쓰지 않는 Form/Untouched도 보존한다.</summary>
public class GumpCreateNode : GumpCollectionNode
{
    /// <summary>+0 원본 가상 표 식별자. 호스트 주소로 실행하지 않는다.</summary>
    public uint Vtable { get; set; }
    /// <summary>+8 정렬 플래그.</summary>
    public uint AlignFlags { get; set; }
    /// <summary>+0x10 이전 형제 토큰.</summary>
    public uint Previous { get; set; }
    /// <summary>+0x1c 등록 표 번호.</summary>
    public uint Id { get; set; }
    /// <summary>+0x20 동작 명령.</summary>
    public uint Command { get; set; }
    /// <summary>+0x28 기본 생성자가 쓰지 않는 Form 포인터 식별자.</summary>
    public uint Form { get; set; }
    /// <summary>+0x2c 생성 일련번호. 미등록 생성도 받는다.</summary>
    public uint Serial { get; set; }
    /// <summary>+0x34 부모에 상대적인 깊이 입력.</summary>
    public uint SecondaryDepth { get; set; }
    /// <summary>+0x38 배율. 기본 생성은 1.0이다.</summary>
    public float Scale { get; set; }
    /// <summary>+0x3c 배율 전 가로 크기.</summary>
    public float BaseWidth { get; set; }
    /// <summary>+0x40 배율 전 세로 크기.</summary>
    public float BaseHeight { get; set; }
    /// <summary>+0x54~0x68 위치/속도 입력의 실제 DWORD 비트 6개.</summary>
    public uint[] Positioning { get; } = new uint[6];
    /// <summary>+0x74 기본 생성자가 쓰지 않는 필드.</summary>
    public uint Untouched { get; set; }
    /// <summary>+0x78 기본 텍스트 포인터 식별자.</summary>
    public uint TextPointer { get; set; }
    /// <summary>+0x7c 보조 텍스트 포인터 식별자.</summary>
    public uint AlternateTextPointer { get; set; }
    /// <summary>+0x80~0x9c 텍스트 문맥 DWORD8개.</summary>
    public uint[] TextContext { get; } = new uint[8];

    /// <summary>같은 노드 객체를 유지하며 초기화 대상만0으로 만든다. Form/+74는 원본처럼 쓰지 않는다.</summary>
    internal void ClearConstructedFields()
    {
        Vtable = Flags = AlignFlags = Next = Previous = Parent = Child = Id = Command = Object = Serial = Depth = SecondaryDepth = 0;
        Scale = BaseWidth = BaseHeight = Width = Height = X = Y = 0;
        Array.Clear(Positioning); Cached = default; TextPointer = AlternateTextPointer = 0; Array.Clear(TextContext);
    }
}

/// <summary>Gump 등록 표/카운터/화면 크기와 같은 루트 수집 전역을 소유한다.</summary>
public sealed class GumpCreateState
{
    /// <summary>원본 등록 표의 고정 칸 수.</summary>
    public const int RegistryCapacity = 1000;
    /// <summary>기본 Gump의 요청 할당 크기.</summary>
    public const int BaseSize = 0xa0;
    /// <summary>첫 빈 칸부터 쓰는 토큰 등록 표.0번은 실제 루트다.</summary>
    public uint[] Entries { get; } = new uint[RegistryCapacity];
    /// <summary>원본 등록 개수. 표의 점유 수를 재계산하지 않는다.</summary>
    public uint Count { get; set; }
    /// <summary>모든 생성에서 DWORD로 증가하는 일련번호.</summary>
    public uint Serial { get; set; }
    /// <summary>할당 뒤 현재 값으로 읽는 논리 화면 폭.</summary>
    public int ScreenWidth { get; set; }
    /// <summary>할당 뒤 현재 값으로 읽는 논리 화면 높이.</summary>
    public int ScreenHeight { get; set; }
    /// <summary>동일한 노드 저장소를 수집기에 제공하는 현재 전역.</summary>
    public GumpCollectionState Collection { get; } = new();
}

/// <summary>생성할 안정된 기본/파생 노드와 new 요청에 대응하는 토큰을 공급한다.</summary>
public sealed class GumpCreateHooks
{
    /// <summary>같은 노드 객체를 반환하는 저장소 조회.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; set; }
    /// <summary>요청 크기의 새 저장소 토큰.0은 원본 new 실패이며 호출 중 현재 전역을 바꿀 수 있다.</summary>
    public Func<int, uint>? Allocate { get; set; }
}

/// <summary>기본 Gump 초기화·등록과 화면 크기의 루트 생성을 복원한다.
/// 근거: cpppj/src/client/GumpCreate.cpp, 10.78 004642c0/004663d0·CD00492da0/00492b30.</summary>
public sealed class GumpCreate
{
    /// <summary>현재 등록/화면 전역. 생성기보다 오래 살아야 한다.</summary>
    private readonly GumpCreateState _state;
    /// <summary>안정된 노드 저장소와 할당 경계.</summary>
    private readonly GumpCreateHooks _hooks;

    /// <summary>현재 생성 상태와 필수 노드 조회를 빌린다. 할당은 루트 생성에서만 요구한다.</summary>
    public GumpCreate(GumpCreateState state, GumpCreateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Node is null) throw new ArgumentException("Gump 생성 저장소 필요", nameof(hooks));
        _state = state; _hooks = hooks;
    }

    /// <summary>준비된 노드의 기본 필드를 초기화한다. 미등록 bit0x100000은 표/개수를 건너뛰며 serial은 항상 증가한다.</summary>
    public uint Construct(uint token, uint command, float width, float height, uint flags)
    {
        GumpCreateNode node = (token != 0 ? _hooks.Node!(token) : null) ?? throw new InvalidOperationException("Gump 생성 토큰/저장소 없음");
        if ((flags & 0x100000) == 0 && unchecked((int)(_state.Count + 1U)) > 999) throw new ArgumentException("Gump 원본 개수 한도 초과");
        node.ClearConstructedFields();
        node.Vtable = _state.Collection.Edition == OriginalEdition.Patch1078 ? 0x50ac70U : 0x5048a8U;
        node.Command = command; node.BaseWidth = node.Width = width; node.BaseHeight = node.Height = height;
        node.Flags = flags | 8; node.Scale = 1; node.Depth = 0xffffffd8;
        node.TextContext[6] = node.TextContext[7] = 1;
        _state.Serial = unchecked(_state.Serial + 1); node.Serial = _state.Serial;
        if ((node.Flags & 0x100000) == 0)
        {
            // 첫 빈 칸만 쓴다. 표가 가득 차면 원본처럼 Id0으로 반환하고 개수는 늘린다.
            for (int index = 0; index < _state.Entries.Length; index++)
            {
                if (_state.Entries[index] != 0) continue;
                _state.Entries[index] = token; node.Id = (uint)index; break;
            }
            _state.Count = unchecked(_state.Count + 1);
        }
        _state.Collection.Root = _state.Entries[0]; return token;
    }

    /// <summary>표를 비운 뒤 new0xa0→현재 화면 크기로 기본 생성→루트 갱신한다. count/serial/override/이전 노드는 유지한다.</summary>
    public uint InitializeRoot()
    {
        if (_hooks.Allocate is null) throw new ArgumentException("Gump 루트 할당 경계 필요");
        Array.Clear(_state.Entries); _state.Collection.Root = 0;
        uint token = _hooks.Allocate(GumpCreateState.BaseSize);
        if (token != 0) Construct(token, 0, _state.ScreenWidth, _state.ScreenHeight, 1);
        _state.Entries[0] = token; _state.Collection.Root = token; return token;
    }

    /// <summary>같은 기본/파생 노드를 복제 없이 수집기에 빌린다. 월드 투영은 현재 게임 경계로 별도 공급한다.</summary>
    public GumpCollectionHooks CollectionHooks(Func<uint, ScreenPoint>? project = null) => new()
    {
        Node = token => token != 0 ? _hooks.Node!(token) : null, Project = project,
    };
}
