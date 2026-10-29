using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 기본 등록/트리와 같은 저장소를 쓰는 배경 파생 생성 영역이다.

/// <summary>0xb8바이트 배경 Gump의 추가 필드다. 기반 위치/등록은 복제하지 않는다.
/// 근거: cpppj/src/client/GumpBackgroundCreate.h.</summary>
public sealed class GumpBackgroundNode : GumpCreateNode
{
    /// <summary>+a0 셰이프 토큰. 호스트 주소로 역참조하지 않는다.</summary>
    public uint Shape { get; set; }
    /// <summary>+a4 생성 때 읽은 signed WORD 폭의 부호 확장 값.</summary>
    public int TileWidth { get; set; }
    /// <summary>+a8 생성 때 읽은 signed WORD 높이의 부호 확장 값.</summary>
    public int TileHeight { get; set; }
    /// <summary>+ac 현재 원본 타입 번호.</summary>
    public uint Type { get; set; }
    /// <summary>+b0 현재 논리/물리 프레임 번호.</summary>
    public uint Frame { get; set; }
    /// <summary>+b4 질감/단색/체크/테두리 플래그.</summary>
    public uint BackgroundFlags; // 버튼이 원본 +b4 주소처럼 ref로 공유하므로 속성 대신 같은 저장 필드를 제공한다.
}

/// <summary>현재 타입 저장소의 안정된 토큰과 SHP 번호다. 프레임 콜백 중 바뀐 Shape도 다시 읽는다.</summary>
public sealed class GumpBackgroundType
{
    /// <summary>호스트 타입 저장소 조회 토큰.</summary>
    public uint Token { get; init; }
    /// <summary>현재 +dc SHP 식별자.</summary>
    public uint Shape { get; set; }
}

/// <summary>프레임 추가 헤더의 signed WORD 치수. 같은 헤더 객체를 안정되게 제공할 수 있다.</summary>
public sealed class GumpBackgroundFrame
{
    /// <summary>+18 폭.</summary>
    public short Width { get; init; }
    /// <summary>+1a 높이.</summary>
    public short Height { get; init; }
}

/// <summary>같은 파생 노드와 실제 타입/SHP 프레임을 공급하는 생성 경계다.</summary>
public sealed class GumpBackgroundCreateHooks
{
    /// <summary>기본 생성기의 노드와 동일한 객체를 반환한다.</summary>
    public Func<uint, GumpBackgroundNode?>? Node { get; set; }
    /// <summary>현재 타입. type0이면 호출하지 않는다.</summary>
    public Func<uint, GumpBackgroundType?>? Type { get; set; }
    /// <summary>타입 토큰·abstract0·현재 노드 frame을 읽는 안정된 헤더.</summary>
    public Func<uint, uint, uint, GumpBackgroundFrame?>? Frame { get; set; }
}

/// <summary>기본 생성 뒤 판본별 배경 필드 쓰기와 현재 자산 조회 순서를 보존한다.
/// 근거: cpppj/src/client/GumpBackgroundCreate.cpp, 004655d0/CD00494640.</summary>
public sealed class GumpBackgroundCreate
{
    /// <summary>현재 등록/판본 전역.</summary>
    private readonly GumpCreateState _state;
    /// <summary>같은 노드의 기본 생성/등록.</summary>
    private readonly GumpCreate _factory;
    /// <summary>원본 signed BYTE 색 전역의 현재 값. C++ 참조 전역을 C# 조회자로 옮긴다.</summary>
    private readonly Func<sbyte> _color;
    /// <summary>실제 노드/타입/프레임 경계.</summary>
    private readonly GumpBackgroundCreateHooks _hooks;

    /// <summary>현재 전역/생성기와 필수 노드만 요구한다. 타입/프레임은 실제 도달한 분기에서 요구한다.</summary>
    public GumpBackgroundCreate(GumpCreateState state, GumpCreate factory, Func<sbyte> color, GumpBackgroundCreateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(color); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Node is null) throw new ArgumentException("배경 Gump 저장소 필요"); _state = state; _factory = factory; _color = color; _hooks = hooks;
    }

    /// <summary>기본/파생 저장소가 같은 객체인지 검사한 뒤 현재 색/필드/헤더를 쓴다. type0의 미작성 셰이프/치수는 보존한다.</summary>
    public uint Construct(uint token, float width, float height, uint backgroundFlags, uint flags, uint type, uint frame)
    {
        GumpBackgroundNode? node = token != 0 ? _hooks.Node!(token) : null;
        if (node is null || !ReferenceEquals(_factory.CollectionHooks().Node!(token), node)) throw new InvalidOperationException("배경 Gump 토큰/공유 저장소 없음");
        _factory.Construct(token, 0, width, height, flags); bool patch = _state.Collection.Edition == OriginalEdition.Patch1078;
        node.Vtable = patch ? 0x50ae40U : 0x5048e8U; node.Command = unchecked((uint)(int)_color());
        node.BackgroundFlags = backgroundFlags; node.Type = type; node.Frame = frame;
        if (type != 0)
        {
            if (_hooks.Type is null || _hooks.Frame is null) throw new ArgumentException("배경 타입/SHP 프레임 경계 필요");
            GumpBackgroundType asset = _hooks.Type(type) ?? throw new InvalidOperationException("배경 타입 저장소 없음");
            if (!patch) node.Shape = asset.Shape;
            GumpBackgroundFrame header = _hooks.Frame(asset.Token, 0, node.Frame) ?? throw new InvalidOperationException("배경 SHP 프레임 헤더 없음");
            node.TileWidth = header.Width; node.TileHeight = header.Height; if (patch) node.Shape = asset.Shape;
        }
        return token;
    }
}
