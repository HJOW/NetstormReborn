using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 같은 생성/트리 노드의 삭제 요청·등록 해제·소멸자 수명 영역이다.

/// <summary>원본 고정 길이 idle/overlay/삭제/dialog 표와 현재 삭제 전역이다.
/// 근거: cpppj/src/client/GumpLifetime.h. CD idle은 앞80칸만 사용하고 나머지는 보존한다.</summary>
public sealed class GumpLifetimeState
{
    /// <summary>매 프레임 Idle을 호출할 원본500칸 표.</summary>
    public uint[] Idle { get; } = new uint[500];
    /// <summary>화면 위 계층의 출력·입력 대상100칸.</summary>
    public uint[] Overlay { get; } = new uint[100];
    /// <summary>지연 삭제 요청1000칸. 요청 시 원본 한도는999다.</summary>
    public uint[] Kill { get; } = new uint[1000];
    /// <summary>현재 등록한 dialog16칸.</summary>
    public uint[] Dialogs { get; } = new uint[16];
    /// <summary>현재 삭제 대기 개수. 콜백 뒤 다시 읽는다.</summary>
    public uint KillCount { get; set; }
    /// <summary>dialog 표의 현재 사용 개수.</summary>
    public uint DialogCount { get; set; }
    /// <summary>현재 입력 캡처/초점의 UI 토큰.</summary>
    public uint Focus { get; set; }
    /// <summary>마지막 활성 dialog 토큰.</summary>
    public uint CurrentDialog { get; set; }
    /// <summary>패치판 assert 분기의 현재 진단 전역.</summary>
    public uint Diagnostics { get; set; }
}

/// <summary>가상 Push·Form 삭제·삭제 소멸자·해제의 현재 경계. 노드 저장소는 실행 중 안정된 객체를 반환해야 한다.</summary>
public sealed class GumpLifetimeHooks
{
    /// <summary>토큰의 기본/파생 생성 노드를 읽는다.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; set; }
    /// <summary>현재 노드의 가상 Push로 트리에서 분리한다.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>부착한 Form을 지정 플래그로 삭제한다.</summary>
    public Action<uint, uint>? DeleteForm { get; set; }
    /// <summary>현재 노드의 가상 deleting destructor를 지정 플래그로 부른다.</summary>
    public Action<uint, uint>? DeletingDestructor { get; set; }
    /// <summary>텍스트/객체 저장소를 해제한다. 실제 노드 해제는 이 경계의 책임이다.</summary>
    public Action<uint>? Release { get; set; }
}

/// <summary>자식 우선 Kill 요청과 Cleanup의 실제 등록 해제/소멸자를 같은 저장소에서 처리한다.
/// 근거: cpppj/src/client/GumpLifetime.cpp. 콜백 뒤 현재 플래그/표/개수를 읽고 예외 전 쓰기를 유지한다.</summary>
public sealed class GumpLifetime
{
    /// <summary>idle 등록 비트.</summary>
    private const uint Idle = 0x80;
    /// <summary>overlay 등록 비트.</summary>
    private const uint Overlay = 0x80000;
    /// <summary>트리에서 분리된 void 비트.</summary>
    private const uint Void = 8;
    /// <summary>삭제 대기 dead 비트.</summary>
    private const uint Dead = 0x10000;
    /// <summary>실제 소멸 중인 destroying 비트.</summary>
    private const uint Destroying = 4;
    /// <summary>등록 표/개수에 참여하지 않는 비트.</summary>
    private const uint Unregistered = 0x100000;
    /// <summary>같은 등록 표/루트/판본을 소유한 생성 전역.</summary>
    private readonly GumpCreateState _state;
    /// <summary>현재 삭제 표와 초점/dialog 전역.</summary>
    private readonly GumpLifetimeState _lifetime;
    /// <summary>안정된 노드와 외부 수명 처리 경계.</summary>
    private readonly GumpLifetimeHooks _hooks;

    /// <summary>같은 생성/트리 저장소와 수명 전역을 빌린다. 노드 조회만 필수이고 실제 도달한 외부 경계를 그때 요구한다.</summary>
    public GumpLifetime(GumpCreateState state, GumpLifetimeState lifetime, GumpLifetimeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Node is null) throw new ArgumentException("Gump 삭제 저장소 필요", nameof(hooks));
        _state = state; _lifetime = lifetime; _hooks = hooks;
    }

    /// <summary>없는 토큰은 원본 유효 포인터 계약 밖이므로 거부한다.</summary>
    private GumpCreateNode Node(uint token) => (token != 0 ? _hooks.Node!(token) : null) ?? throw new InvalidOperationException("Gump 삭제 토큰/저장소 없음");

    /// <summary>현재 판본의 실제 idle 표 길이를 반환한다.</summary>
    private int IdleSize() => _state.Collection.Edition == OriginalEdition.Patch1078 ? 500 : 80;

    /// <summary>첫 빈 idle 칸 또는 첫 일치 칸만 갱신한 뒤 플래그를 쓴다. 이미 같은 상태면 표를 읽지 않는다.</summary>
    public void SetIdle(uint token, bool enabled)
    {
        GumpCreateNode node = Node(token);
        if (enabled == ((node.Flags & Idle) != 0)) return;
        // 원본 표/플래그가 일치한다는 계약 아래 첫 후보만 선택한다.
        for (int i = 0; i < IdleSize(); i++)
        {
            if (_lifetime.Idle[i] != (enabled ? 0U : token)) continue;
            _lifetime.Idle[i] = enabled ? token : 0;
            node.Flags = enabled ? node.Flags | Idle : node.Flags & ~Idle; return;
        }
        throw new ArgumentException("Gump idle 원본 표/플래그 계약 위반");
    }

    /// <summary>마지막 빈 overlay 칸에 등록하거나 마지막 일치 하나만 지운다. 중복 enable은 원본 계약 오류다.</summary>
    public void SetOverlay(uint token, bool enabled)
    {
        GumpCreateNode node = Node(token); int found = _lifetime.Overlay.Length; int empty = found;
        // 원본 검색은 끝까지 진행하며 마지막 일치/빈 칸을 보존한다.
        for (int i = 0; i < _lifetime.Overlay.Length; i++)
        {
            if (_lifetime.Overlay[i] == token) found = i;
            if (_lifetime.Overlay[i] == 0) empty = i;
        }
        if (enabled)
        {
            if (found != _lifetime.Overlay.Length || empty == _lifetime.Overlay.Length) throw new ArgumentException("Gump overlay 원본 등록 계약 위반");
            _lifetime.Overlay[empty] = token; node.Flags |= Overlay;
        }
        else if (found != _lifetime.Overlay.Length) { _lifetime.Overlay[found] = 0; node.Flags &= ~Overlay; }
        else if (_state.Collection.Edition == OriginalEdition.Patch1078 && _lifetime.Diagnostics != 0) throw new InvalidOperationException("Gump overlay 원본 진단 계약 위반");
    }

    /// <summary>첫 dialog 일치를 제거/압축하고 마지막 항목을 현재 dialog로 지정한다. 없는 토큰은 노드 조회 없이 거짓이다.</summary>
    public bool RemoveDialog(uint token)
    {
        if (_lifetime.DialogCount > _lifetime.Dialogs.Length) throw new ArgumentException("Gump dialog 표 범위 초과");
        // 앞쪽 겹침 복사로 남은 항목 순서를 유지하며 비활성 뒤쪽은 보존한다.
        for (uint i = 0; i < _lifetime.DialogCount; i++)
        {
            if (_lifetime.Dialogs[i] != token) continue;
            Array.Copy(_lifetime.Dialogs, i + 1, _lifetime.Dialogs, i, _lifetime.DialogCount - i - 1);
            _lifetime.DialogCount--; _lifetime.Dialogs[_lifetime.DialogCount] = 0;
            _lifetime.CurrentDialog = _lifetime.DialogCount != 0 ? _lifetime.Dialogs[_lifetime.DialogCount - 1] : 0; return true;
        }
        return false;
    }

    /// <summary>독립 재귀 경로에서 자식부터 삭제 대기로 넣는다. 중복 요청은 제거하지 않는다.</summary>
    public void Kill(uint token) => Recurse(token, false, []);

    /// <summary>별도 재귀 경로에서 자식부터 등록 해제와 가상 소멸자를 실제로 호출한다.</summary>
    public void Destroy(uint token) => Recurse(token, true, []);

    /// <summary>자식 next를 호출 전에 보관하고 콜백 뒤 부모의 현재 플래그/표/개수를 다시 읽는 수명 본체다.</summary>
    private void Recurse(uint token, bool destroy, HashSet<uint> active)
    {
        GumpCreateNode node = Node(token);
        if (!active.Add(token)) throw new InvalidOperationException("Gump 삭제 자식 순환");
        var siblings = new HashSet<uint>();
        // 자식 소멸자가 링크/큐/저장소를 바꿀 수 있어 호출 뒤 자식을 다시 조회하지 않는다.
        for (uint child = node.Child; child != 0;)
        {
            if (!siblings.Add(child)) throw new InvalidOperationException("Gump 삭제 형제 순환");
            uint next = Node(child).Next; Recurse(child, destroy, active); child = next;
        }
        if (destroy)
        {
            if ((node.Flags & Unregistered) == 0)
            {
                if (node.Id >= _state.Entries.Length) throw new ArgumentOutOfRangeException(nameof(token), "Gump 등록 해제 id 범위 초과");
                if (_state.Collection.Edition == OriginalEdition.Patch1078) { _state.Entries[node.Id] = 0; _state.Count = unchecked(_state.Count - 1); }
                else { _state.Count = unchecked(_state.Count - 1); _state.Entries[node.Id] = 0; }
                _state.Collection.Root = _state.Entries[0];
                if (unchecked((int)_state.Count) < 0) throw new InvalidOperationException("Gump 등록 개수 음수");
            }
            node.Flags = (node.Flags & ~Dead) | Destroying;
            (_hooks.DeletingDestructor ?? throw new ArgumentException("Gump 가상 소멸자 경계 필요"))(token, 1);
        }
        else
        {
            if ((node.Flags & Idle) != 0) SetIdle(token, false);
            if ((node.Flags & Overlay) != 0) SetOverlay(token, false);
            if ((node.Flags & 0x40) != 0) (_hooks.DeleteForm ?? throw new ArgumentException("Gump Form 삭제 경계 필요"))(node.Form, 0);
            if ((node.Flags & Void) == 0) (_hooks.Push ?? throw new ArgumentException("Gump 가상 Push 경계 필요"))(token);
            if (_lifetime.KillCount >= 999) throw new ArgumentException("Gump 삭제 큐 원본 한도 초과");
            _lifetime.Kill[_lifetime.KillCount++] = token;
            if (_lifetime.Focus == token) _lifetime.Focus = 0;
            if (_state.Collection.OverrideHead == token) _state.Collection.OverrideHead = 0;
            RemoveDialog(token); node.Flags |= Dead;
        }
        active.Remove(token);
    }

    /// <summary>현재 큐의 비어 있지 않은 dead 항목을 파괴하고 소멸자 직후의 현재 개수로 반복 범위를 갱신한다.</summary>
    public void Cleanup()
    {
        uint count = _lifetime.KillCount;
        if (count > _lifetime.Kill.Length) throw new ArgumentException("Gump 정리 큐 범위 초과");
        // null 항목에서는 로컬 개수를 갱신하지 않는 원본 순서를 유지한다.
        for (uint i = 0; i < count; i++)
        {
            uint token = _lifetime.Kill[i]; if (token == 0) continue;
            if ((Node(token).Flags & Dead) == 0) throw new InvalidOperationException("Gump 정리 대상 dead 필요");
            Destroy(token); count = _lifetime.KillCount;
            if (count > _lifetime.Kill.Length) throw new ArgumentException("Gump 정리 콜백 큐 범위 초과");
        }
        Array.Clear(_lifetime.Kill, 0, (int)count); _lifetime.KillCount = 0;
    }

    /// <summary>루트를 제외한 등록 표999칸을 끝까지 읽어 현재 제외 signed BYTE와 타입이 다른 비 dead 노드를 Kill한다.
    /// ref는 C++ const int8 참조처럼 콜백 뒤 갱신된 같은 입력을 다음 칸에서 다시 읽기 위해 사용한다(004666d0).</summary>
    public void KillExceptType(ref sbyte excludedType)
    {
        // count/visible/void/미등록 플래그로 추가 필터를 하지 않고 현재 각 칸과 제외값만 읽는다.
        for (int index = 1; index < _state.Entries.Length; index++)
        {
            uint token = _state.Entries[index]; if (token == 0) continue;
            uint excluded = unchecked((uint)(int)excludedType);
            if (Node(token).Command != excluded && (Node(token).Flags & Dead) == 0) Kill(token);
        }
    }

    /// <summary>이전 루트 Kill→현재 큐 Cleanup→새 루트/id0 기록 순서다. count/serial/새 객체의 이전 등록 칸은 초기화하지 않는다.</summary>
    public void ReplaceRoot(uint token)
    {
        if (_state.Entries[0] != 0) Kill(_state.Entries[0]);
        Cleanup(); _state.Entries[0] = token; _state.Collection.Root = token; Node(token).Id = 0;
    }

    /// <summary>기본 가상 표 복원→현재 텍스트 해제→overlay/idle/삭제 큐의 모든 일치 제거를 수행한다. 등록/dialog/개수는 직접 바꾸지 않는다.</summary>
    public void DestructBase(uint token)
    {
        GumpCreateNode node = Node(token);
        node.Vtable = _state.Collection.Edition == OriginalEdition.Patch1078 ? 0x50ac70U : 0x5048a8U;
        if (_state.Collection.Edition == OriginalEdition.Patch1078 && _lifetime.Diagnostics != 0 && (node.Flags & Destroying) == 0)
            throw new InvalidOperationException("Gump 기본 소멸자 destroying 필요");
        if (node.TextPointer != 0) { (_hooks.Release ?? throw new ArgumentException("Gump 텍스트 해제 경계 필요"))(node.TextPointer); node.TextPointer = 0; }
        if (node.AlternateTextPointer != 0) { (_hooks.Release ?? throw new ArgumentException("Gump 대체 텍스트 해제 경계 필요"))(node.AlternateTextPointer); node.AlternateTextPointer = 0; }
        // 모든 overlay 중복을 지우되 플래그/개수는 그대로 둔다.
        for (int i = 0; i < _lifetime.Overlay.Length; i++) if (_lifetime.Overlay[i] == token) _lifetime.Overlay[i] = 0;
        // 현재 판본의 실제 idle 활성 구간만 지운다.
        for (int i = 0; i < IdleSize(); i++) if (_lifetime.Idle[i] == token) _lifetime.Idle[i] = 0;
        // 전체 삭제 큐의 뒤쪽 중복도 null로 만든다.
        for (int i = 0; i < _lifetime.Kill.Length; i++) if (_lifetime.Kill[i] == token) _lifetime.Kill[i] = 0;
    }

    /// <summary>기본 소멸자 뒤 하위 BYTE의 bit0이 켜졌을 때 객체를 해제하고 원래 토큰을 반환한다. 해제 후 노드는 읽지 않는다.</summary>
    public uint DeleteScalar(uint token, uint flags)
    {
        DestructBase(token);
        if ((flags & 1) != 0) (_hooks.Release ?? throw new ArgumentException("Gump 객체 해제 경계 필요"))(token);
        return token;
    }
}
