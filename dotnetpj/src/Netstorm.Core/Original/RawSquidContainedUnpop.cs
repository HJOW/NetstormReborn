// Form 종속 Unpop의 같은 풀·상태·효과 경계를 공유한다.
namespace Netstorm.Core.Original;

// 종속 해제 모듈을 담는 네임스페이스다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.

// 타입·검사 주소 DWORD와 세션 모드 읽기를 담는 상태다. 원본 `SquidContainedUnpopState`를 옮긴다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.
public sealed class SquidContainedUnpopState
{
    /// <summary>부모 탐색의 기본 타입 DWORD 상자다. 비우면 안 된다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    public UIntBox? ContentFormType { get; set; }

    /// <summary>UI 명령 탐색의 타입 DWORD 상자다. 비우면 안 된다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    public UIntBox? GumpFormType { get; set; }

    /// <summary>종속 UI 명령 번호 DWORD 상자다. 비우면 안 된다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    public UIntBox? UiEvent { get; set; }

    /// <summary>죽음 검사 진단 뷰다. null이면 죽음 검사를 건너뛴다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    public RawCheckingFlag? CheckingDead { get; set; }

    /// <summary>메타맵 모드 읽기다. 비우면 죽음 검사 때 거부된다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    public Func<bool>? InMetamapMode { get; set; }

    /// <summary>서버 권한 읽기다. 비우면 죽음 검사 때 거부된다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    public Func<bool>? Boss { get; set; }
}

// 종속 분리와 UI 통지가 부르는 외부 효과 묶음이다. 원본 `SquidContainedUnpopHooks`를 옮긴다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.
public sealed class SquidContainedUnpopHooks
{
    /// <summary>현재 가상 표의 부모 getter다. 비우면 분리 때 거부된다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    public Func<int, int>? Parent { get; set; }

    /// <summary>현재 가상 표의 이전 getter다. 비우면 분리 때 거부된다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    public Func<int, int>? Previous { get; set; }

    /// <summary>현재 가상 표의 이전 setter다. 비우면 분리 때 거부된다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    public Action<int, int>? SetPrevious { get; set; }

    /// <summary>부모의 실제 종속 끊김 알림이다. 비우면 분리 때 거부된다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    public Action<int, int, uint>? ParentNotice { get; set; }

    /// <summary>원문 입력 큐로 UI 명령을 넣는다. 비우면 UI 통지 때 거부된다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    public Action<uint, uint, uint>? Command { get; set; }

    /// <summary>원문 식·파일·줄을 전달하는 진단이다. 비우면 진단 때 거부된다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

// Form 종속 Unpop의 분리·UI 통지를 같은 풀에서 실행한다. 원본 `RawSquidContainedUnpop`을 옮긴다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.
public sealed class RawSquidContainedUnpop
{
    /// <summary>빈 슬롯 비트다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    private const byte FreeBit = 1;

    /// <summary>죽음 비트다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    private const byte DeadBit = 2;

    /// <summary>들어 있는 내용물 비트다. 분리하면 지운다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    private const byte ContainedBit = 8;

    /// <summary>Form 탐색기의 마지막 Form 타입 번호다. 이보다 크면 UI 탐색을 건너뛴다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    private const byte LastForm = 0x45;

    /// <summary>분리 진단의 원본 파일이다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    private const string SquidSource = "\\Ns\\O\\Squid.cpp";

    /// <summary>UI 탐색 진단의 원본 파일이다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    private const string FinderSource = "\\Ns\\O\\Squidfinder.cpp";

    /// <summary>Form·자산·UI 번호를 함께 쓰는 같은 실제 풀이다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입·검사 주소와 세션 모드 읽기다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    private readonly SquidContainedUnpopState _state;

    /// <summary>분리·통지의 외부 효과 묶음이다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    private readonly SquidContainedUnpopHooks _hooks;

    /// <summary>같은 풀·상태·효과로 종속 해제를 만든다. 상자·풀을 복제하지 않는다.</summary>
    /// <param name="pool">Form·자산·UI 번호를 함께 쓰는 풀이다.</param>
    /// <param name="state">타입·검사 주소와 세션 모드 읽기다. 세 상자는 비우면 안 된다.</param>
    /// <param name="hooks">분리·통지의 외부 효과 묶음이다.</param>
    public RawSquidContainedUnpop(RawSidPool pool, SquidContainedUnpopState state, SquidContainedUnpopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (state.ContentFormType is null || state.GumpFormType is null || state.UiEvent is null)
        {
            throw new ArgumentException("종속 해제의 실제 타입 상자가 없습니다.", nameof(state));
        }

        _pool = pool;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>이 종속 해제가 쓰는 슬롯 풀이다. World·Session 연결이 같은 풀인지 확인할 때 쓴다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    /// <returns>같은 실제 풀이다.</returns>
    public RawSidPool Pool() => _pool;

    /// <summary>현재 세션이 빌리는 실제 부모 탐색 번호다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    /// <returns>현재 부모 탐색 DWORD다.</returns>
    public uint ContentFormType() => _state.ContentFormType!.Value;

    /// <summary>슬롯 바이트에서 원문 WORD 하나를 읽는다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 WORD다.</returns>
    private static ushort LinkWord(ReadOnlySpan<byte> raw, int offset) =>
        (ushort)(raw[offset] | (raw[offset + 1] << 8));

    /// <summary>인접 payload를 바꾸지 않는 원문 WORD 쓰기다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void PutLink(int sid, int offset, ushort value)
    {
        // 원본 LinkRaw처럼 빈 슬롯 검사 없이 같은 어셈블리의 쓰기 보기로 쓴다.
        Span<byte> raw = _pool.RawSlot(sid);
        raw[offset] = unchecked((byte)value);
        raw[offset + 1] = unchecked((byte)(value >> 8));
    }

    /// <summary>비운 효과 장치를 거부하고 받은 장치를 돌려준다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    /// <typeparam name="T">효과 대리자 형식이다.</typeparam>
    /// <param name="hook">확인할 효과 장치다.</param>
    /// <returns>받은 효과 장치다.</returns>
    private static T Need<T>(T? hook) where T : class
    {
        if (hook is null)
        {
            throw new InvalidOperationException("종속 해제의 실제 효과 장치가 없습니다.");
        }

        return hook;
    }

    /// <summary>판본별 원본 식·파일·줄로 진단한다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    /// <param name="expression">진단 식이다.</param>
    /// <param name="patchLine">패치판 줄 번호다.</param>
    /// <param name="cdLine">CD판 줄 번호다.</param>
    private void Assert(string expression, uint patchLine, uint cdLine)
    {
        // CD 파일 문자열의 앞부분 잘림을 원문 그대로 둔다.
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        Need(_hooks.Assertion)(expression, patch ? SquidSource : "\\ns\\o\\Squid.cpp", patch ? patchLine : cdLine);
    }

    /// <summary>void를 다시 건드리지 않고 종속 체인·부모 머리를 정리한다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    /// <param name="sid">떼어낼 번호다.</param>
    public void Detach(int sid)
    {
        if ((_pool.Slot(sid)[RawSidPool.StateOffset] & ContainedBit) == 0)
        {
            Assert("containedFlag", 0x3a3u, 0x39au);
        }

        int parent = Need(_hooks.Parent)(sid);
        if (parent == 0)
        {
            Assert("parentSid", 0x3a5u, 0x39cu);
        }

        if ((_pool.Slot(parent)[RawSidPool.StateOffset] & FreeBit) != 0)
        {
            Assert("!p.isFree()", 0x3a8u, 0x39fu);
        }

        if ((_pool.Slot(parent)[RawSidPool.StateOffset] & DeadBit) != 0)
        {
            return;
        }

        if ((_pool.Slot(sid)[RawSidPool.StateOffset] & ContainedBit) == 0)
        {
            Assert("containedFlag", 0x3b1u, 0x3a8u);
        }

        // 이전 getter의 다음 번호로 이전·다음을 잇는다.
        int next = LinkWord(_pool.Slot(sid), 4);
        int previous = Need(_hooks.Previous)(sid);
        if (previous != 0)
        {
            bool patch = _pool.Edition == OriginalEdition.Patch1078;
            if (previous == next)
            {
                Need(_hooks.Assertion)("n != getSid()", patch ? "c:\\ns\\o\\squid.h" : "\\ns\\o\\squid.h", patch ? 0x123u : 0x10cu);
            }

            PutLink(previous, 4, unchecked((ushort)next));
        }

        if (next != 0)
        {
            Need(_hooks.SetPrevious)(next, previous);
        }

        if (LinkWord(_pool.Slot(parent), 6) == sid)
        {
            if (previous != 0)
            {
                Assert("prev == 0", 0x3c0u, 0x3b7u);
            }

            PutLink(parent, 6, unchecked((ushort)next));
        }

        _pool.AllocatedBytes(sid)[RawSidPool.StateOffset] &= unchecked((byte)~ContainedBit);
        if (_pool.Slot(sid)[RawSidPool.TypeOffset] > LastForm || _pool.Slot(sid)[RawSidPool.TypeOffset] == _state.ContentFormType!.Value)
        {
            Need(_hooks.ParentNotice)(parent, sid, 2u);
        }
    }

    /// <summary>한 객체 자식의 Form 체인을 훑어 UI 명령을 큐에 넣는다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    /// <param name="sid">훑을 부모 번호다.</param>
    public void NotifyUi(int sid)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if (patch && _state.CheckingDead is not null && _state.CheckingDead
            && !Need(_state.InMetamapMode)() && (_pool.Slot(sid)[RawSidPool.StateOffset] & DeadBit) != 0
            && !Need(_state.Boss)())
        {
            Need(_hooks.Assertion)("inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy", FinderSource, 0x158u);
        }

        // Form 탐색기의 반환 next를 따라간다.
        int next = LinkWord(_pool.Slot(sid), 6);
        var seen = new HashSet<ushort>();
        while (next != 0)
        {
            int current = next;
            if (!seen.Add(unchecked((ushort)current)))
            {
                throw new InvalidOperationException("종속 UI 탐색의 Form 체인 순환입니다.");
            }

            ReadOnlySpan<byte> raw = _pool.Slot(current);
            next = LinkWord(raw, 4);
            if (raw[RawSidPool.TypeOffset] > LastForm)
            {
                continue;
            }

            if (_pool.Slot(current)[RawSidPool.TypeOffset] == _state.GumpFormType!.Value)
            {
                Need(_hooks.Command)(_state.UiEvent!.Value, unchecked((uint)sid), 0u);
                return;
            }
        }
    }

    /// <summary>분리한 뒤 UI를 다시 조회한다. 두 호출 순서를 바꾸지 않는다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    /// <param name="sid">해제할 번호다.</param>
    public void Unpop(int sid)
    {
        Detach(sid);
        NotifyUi(sid);
    }
}
