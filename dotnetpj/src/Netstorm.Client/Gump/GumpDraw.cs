using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // 같은 Gump 트리의 현재 클립·가상 Draw·중첩 복구 영역이다.

/// <summary>원본 half-open 논리 화면 전역과 양 끝 포함 VFX 전역이다. 투영/Draw가 현재 값을 바꿀 수 있다.</summary>
public sealed class GumpDrawState
{
    /// <summary>끝 제외 논리 클립.</summary>
    public ScreenRect Clip { get; set; }
    /// <summary>끝 포함 VFX 클립. 논리 클립과 다르게 초기화된 값도 그대로 읽는다.</summary>
    public ScreenRect VfxClip { get; set; }
}

/// <summary>같은 노드의 실제 파생 가상 Draw 경계다. 자식은 표시 목록에서 별도 호출한다.</summary>
public sealed class GumpDrawHooks
{
    /// <summary>가상+0x24로 전달할 실제 대상 토큰.</summary>
    public Action<uint>? Draw { get; set; }
}

/// <summary>현재 논리 영역만 저장하고 정상/예외에서 두 클립을 함께 복구한다.
/// 근거: cpppj/src/client/GumpDraw.cpp 의 GumpClipScope. C++ RAII를 C# using/IDisposable로 옮기며 같은 보호 객체의 중복 복구는 막는다.</summary>
public sealed class GumpClipScope : IDisposable
{
    /// <summary>공유 현재 클립 저장소.</summary>
    private readonly GumpDrawState _state;
    /// <summary>경계 조회 뒤의 실제 논리 클립.</summary>
    private readonly ScreenRect _saved;
    /// <summary>같은 객체를 중복 Dispose해도 한 번만 복구하는 소유 상태.</summary>
    private bool _disposed;

    /// <summary>현재 논리 클립을 저장하고 bounds의 네 좌표를 각 경계 안으로 제한한다. 빈/역전도 보존한다.</summary>
    public GumpClipScope(GumpDrawState state, ScreenRect bounds)
    {
        ArgumentNullException.ThrowIfNull(state); _state = state; _saved = state.Clip;
        GumpDraw.SetGumpDrawClip(state, ScreenGeometry.ClipScreenRect(bounds, _saved));
    }

    /// <summary>가상 Draw가 바꾼 현재 클립 대신 저장한 논리 영역과 그 VFX 끝점을 복구한다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        GumpDraw.SetGumpDrawClip(_state, _saved); _disposed = true;
    }
}

/// <summary>같은 수집기의 현재 flags/경계로 기본 Gump 가상 Draw를 임시 클립에 연결한다.
/// 근거: cpppj/src/client/GumpDraw.h·GumpDraw.cpp, 10.78 00466310. 공통 kind2 분배 바인딩은 R5에서 연결한다.</summary>
public sealed class GumpDraw
{
    /// <summary>임시 ClipGump를 만드는 원본 flags+4 비트.</summary>
    public const uint DrawClip = 0x00400000;
    /// <summary>현재 노드의 위치/flags/캐시를 조회하는 실제 수집기.</summary>
    private readonly GumpCollection _collection;
    /// <summary>공유 논리/VFX 클립.</summary>
    private readonly GumpDrawState _state;
    /// <summary>실제 파생 가상 Draw.</summary>
    private readonly GumpDrawHooks _hooks;

    /// <summary>같은 수집기/상태/가상 처리기를 빌린다. 처리기는 일부 클립 쓰기 전에 생성 시 요구한다.</summary>
    public GumpDraw(GumpCollection collection, GumpDrawState state, GumpDrawHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Draw is null) throw new ArgumentException("Gump 가상 Draw 필요", nameof(hooks));
        _collection = collection; _state = state; _hooks = hooks;
    }

    /// <summary>원본004a1580처럼 두 전역을 함께 쓴다. right/bottom-1의 signed DWORD 감김과 빈/역전 입력도 보존한다.</summary>
    public static void SetGumpDrawClip(GumpDrawState state, ScreenRect clip)
    {
        ArgumentNullException.ThrowIfNull(state); state.Clip = clip;
        state.VfxClip = new(clip.Left, clip.Top, unchecked(clip.Right - 1), unchecked(clip.Bottom - 1));
    }

    /// <summary>현재 flags에 보호 비트가 있을 때만 경계/캐시를 읽는다. 빈 클립에서도 가상 Draw를 호출하며 visible/dead/자식 필터는 추가하지 않는다.</summary>
    public void Draw(uint token)
    {
        if ((_collection.Flags(token) & DrawClip) != 0)
        {
            ScreenRect bounds = _collection.Bounds(token); using var guard = new GumpClipScope(_state, bounds); _hooks.Draw!(token); return;
        }
        _hooks.Draw!(token);
    }
}
