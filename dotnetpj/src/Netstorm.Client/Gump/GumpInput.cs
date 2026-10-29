using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // OS 입력 앞의 원본 캡처/dialog/overlay 전달 영역이다.

/// <summary>overlay가 호출자 사건 대신 사용하는 현재 사건 코드 전역이다.</summary>
public sealed class GumpInputState
{
    /// <summary>overlay 가상 처리기에 전달할 별도 사건 코드.</summary>
    public uint OverlayEvent { get; set; }
}

/// <summary>직접 가상 처리와 dialog 한도 로그의 외부 경계. 같은 노드/캡처/표를 바꿀 수 있다.</summary>
public sealed class GumpInputHooks
{
    /// <summary>현재 토큰의 안정된 생성 노드.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; set; }
    /// <summary>현재 대상/사건/인자의 가상 처리기. 전체 DWORD 반환을 유지한다.</summary>
    public Func<uint, uint, uint, uint, uint>? Handle { get; set; }
    /// <summary>dialog 한도에 도달할 때 원본 로그 코드0x13을 전달한다.</summary>
    public Action<uint>? Overflow { get; set; }
}

/// <summary>같은 Gump 저장소의 캡처/dialog 우선 전달과 overlay의 실제 커서 선택을 결합한다.
/// 근거: cpppj/src/client/GumpInput.h·GumpInput.cpp. 캡처 실패 뒤 재전달하지 않고 성공 뒤 현재 캡처를 읽는다.</summary>
public sealed class GumpInput
{
    /// <summary>현재 등록 루트/수집 override.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 캡처/dialog/overlay 표.</summary>
    private readonly GumpLifetimeState _lifetime;
    /// <summary>현재 overlay 사건 전역.</summary>
    private readonly GumpInputState _state;
    /// <summary>같은 노드의 현재 위치/경계 처리기.</summary>
    private readonly GumpCollection _collection;
    /// <summary>같은 노드의 실제 필터/선택/재귀 전달.</summary>
    private readonly GumpEvents _events;
    /// <summary>안정된 노드와 직접 가상 처리 경계.</summary>
    private readonly GumpInputHooks _hooks;

    /// <summary>같은 생성/수명/위치/사건 상태를 빌린다. 노드 조회는 필수다.</summary>
    public GumpInput(GumpCreateState create, GumpLifetimeState lifetime, GumpInputState state, GumpCollection collection, GumpEvents events, GumpInputHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(events); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Node is null) throw new ArgumentException("Gump 입력 저장소 필요", nameof(hooks));
        _create = create; _lifetime = lifetime; _state = state; _collection = collection; _events = events; _hooks = hooks;
    }

    /// <summary>실제 필드/가상 처리에 도달한 토큰만 원본 유효 포인터 계약으로 검사한다.</summary>
    private GumpCreateNode Node(uint token) => (token != 0 ? _hooks.Node!(token) : null) ?? throw new InvalidOperationException("Gump 입력 토큰 없음");

    /// <summary>캡처/overlay 대상에 기본 필터 없이 직접 전달하며 raw 반환을 보존한다.</summary>
    private uint Handle(uint token, uint eventCode, uint first, uint second)
    {
        Node(token); return (_hooks.Handle ?? throw new ArgumentException("Gump 입력 처리기 필요"))(token, eventCode, first, second);
    }

    /// <summary>현재 캡처가 비어 있을 때만 토큰을 기록한다. 토큰0도 원본처럼 허용한다.</summary>
    public void Capture(uint token) { if (_lifetime.Focus == 0) _lifetime.Focus = token; }

    /// <summary>enabled이고 현재 override가 비었을 때만 설정하며 그 외에는 기존 객체와 무관하게 해제한다.</summary>
    public void SetOverride(uint token, bool enabled) => _create.Collection.OverrideHead = enabled && _create.Collection.OverrideHead == 0 ? token : 0;

    /// <summary>중복은 거짓, 새 등록은 참이다. 가득 찬 표는 중복 검색 전에 원본 한도 로그를 호출한다.</summary>
    public bool AddDialog(uint token)
    {
        if (unchecked((int)_lifetime.DialogCount) < 0) throw new ArgumentOutOfRangeException(nameof(token), "Gump dialog 개수 음수");
        if (_lifetime.DialogCount >= _lifetime.Dialogs.Length)
        {
            (_hooks.Overflow ?? throw new ArgumentException("Gump dialog 한도 로그 필요"))(0x13); return false;
        }
        // 활성 구간만 검색하므로 미사용 뒤 칸의 같은 토큰은 새 등록을 막지 않는다.
        for (uint i = 0; i < _lifetime.DialogCount; i++) if (_lifetime.Dialogs[i] == token) return false;
        _lifetime.Dialogs[_lifetime.DialogCount++] = token; _lifetime.CurrentDialog = token; return true;
    }

    /// <summary>현재 캡처는 직접 전달하고 실패하면 끝낸다. 성공하면 콜백 뒤 현재 캡처를 읽고 지운다. 캡처가 없으면 dialog/루트로 재귀 전달한다.</summary>
    public uint Route(uint eventCode, uint first, uint second, bool includeHidden = false)
    {
        if (_lifetime.Focus != 0)
        {
            if (Handle(_lifetime.Focus, eventCode, first, second) != 0)
            {
                uint result = _lifetime.Focus; _lifetime.Focus = 0; return result;
            }
            return 0;
        }
        uint target = _lifetime.CurrentDialog != 0 ? _lifetime.CurrentDialog : _create.Entries[0];
        return target != 0 ? _events.Dispatch(target, eventCode, first, second, includeHidden) : 0;
    }

    /// <summary>자신부터 현재 부모 체인의 visible만 검사한다. dead/void는 이 부모 검사에서 제외하지 않는다.</summary>
    private bool ParentsVisible(uint token)
    {
        var seen = new HashSet<uint>();
        // 현재 부모 링크를 따라가며 손상 순환만 거부한다.
        for (uint current = token; current != 0; current = Node(current).Parent)
        {
            if (!seen.Add(current)) throw new InvalidOperationException("Gump overlay 부모 순환");
            if ((Node(current).Flags & 1) == 0) return false;
        }
        return true;
    }

    /// <summary>표 순서로 visible 부모/경계/실제 전체 Pick을 확인하고 첫 일치 처리기의0을 포함한 raw 반환을 보존한다.
    /// eventCode는 원본에서도 사용하지 않으며 별도 OverlayEvent를 전달한다.</summary>
    public uint Overlay(uint eventCode, ScreenPoint point)
    {
        // 뒤 칸은 앞 대상의 위치/선택 콜백 이후 현재 값을 읽는다.
        foreach (uint token in _lifetime.Overlay)
        {
            if (token == 0) continue; uint flags = Node(token).Flags;
            if ((flags & 1) == 0 || (flags & 0x10000) != 0 || !ParentsVisible(token)) continue;
            ScreenRect rect = _collection.Bounds(token);
            if (rect.Left <= point.X && point.X < rect.Right && rect.Top <= point.Y && point.Y < rect.Bottom && _events.Pick(point, 0) == token)
                return Handle(token, _state.OverlayEvent, unchecked((uint)point.X), unchecked((uint)point.Y));
        }
        return 0;
    }
}
