// PATCH 정렬 Content 부착의 링크·현재 가상 호출·늦은 Pop·부모 통지를 공유한다.
namespace Netstorm.Core.Original;

// Content 부착 모듈을 담는 네임스페이스다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.

// 선택한 장치를 원문 소비 위치에서만 요구한다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.
public sealed class ContentAttachHooks
{
    /// <summary>반복마다 현재 전체 DWORD를 읽는다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    public Func<uint>? ContentType { get; set; }

    /// <summary>선택해 저장한 가상 표+4 getter다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    public Func<int, uint, int>? Previous { get; set; }

    /// <summary>현재 또는 저장한 가상 표+c WORD setter다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    public Action<int, uint, int>? SetPrevious { get; set; }

    /// <summary>순서 바이트 뒤 현재 가상 표+8이다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    public Action<int, uint, int>? SetParent { get; set; }

    /// <summary>링크·부모·contained 뒤 실제 공통 Pop이다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    public Action<int, uint>? Pop { get; set; }

    /// <summary>저장한 부모의 이유1 통지다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    public Action<int, int, uint>? Notice { get; set; }

    /// <summary>진단 복귀 뒤 저장값·현재값을 구별한다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>현재 훅 묶음의 얕은 사본을 만든다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    /// <returns>복사된 훅 묶음이다.</returns>
    public ContentAttachHooks Clone() => new()
    {
        ContentType = ContentType,
        Previous = Previous,
        SetPrevious = SetPrevious,
        SetParent = SetParent,
        Pop = Pop,
        Notice = Notice,
        Assertion = Assertion,
    };
}

// 사제·전투 꼬리와 구별되는 정렬·부착 공통 접두다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.
public static class RawContentAttach
{
    /// <summary>장치 교체 중에도 선택한 callable을 복귀까지 보관한다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    /// <param name="device">쓸 장치다.</param>
    private static T Need<T>(T? device) where T : class
    {
        if (device is null)
        {
            throw new InvalidOperationException("정렬 Content 부착의 선택된 장치 미연결");
        }

        return device;
    }

    /// <summary>비정렬 WORD 폭이다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static ushort Word(ReadOnlySpan<byte> raw, int at) =>
        (ushort)(raw[at] | (raw[at + 1] << 8));

    /// <summary>다른 payload는 보존한다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 슬롯 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int at, ushort value)
    {
        raw[at] = unchecked((byte)value);
        raw[at + 1] = unchecked((byte)(value >> 8));
    }

    /// <summary>진단 복귀 뒤 저장한 this에 쓴다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="next">쓸 다음 번호다.</param>
    /// <param name="hooks">선택 장치 묶음이다.</param>
    private static void SetNext(RawSidPool pool, int sid, int next, ContentAttachHooks hooks)
    {
        if (next == sid)
        {
            Need(hooks.Assertion)("n != getSid()", "c:\\ns\\o\\squid.h", 0x123u);
        }

        Put(pool.RawSlot(sid), 4, unchecked((ushort)next));
    }

    /// <summary>통지까지 실행한다. dead·free 반환은 거짓이며 부분 실패는 rollback하지 않는다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="form">고를 Form SID다.</param>
    /// <param name="parent">부를 부모 SID다.</param>
    /// <param name="flags">원문 플래그다.</param>
    /// <param name="hooks">선택 장치 묶음이다.</param>
    /// <returns>부착했으면 참이다.</returns>
    public static bool Attach(RawSidPool pool, int form, int parent, uint flags, ContentAttachHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("현재 정렬 Content 부모의 10.78 계약 위반", nameof(pool));
        }

        Span<byte> parentRaw = pool.RawSlot(parent);
        Span<byte> raw = pool.RawSlot(form);
        if ((parentRaw[11] & 3u) != 0u)
        {
            Need(hooks.Assertion)("!into.isDead() && !into.isFree()", "\\Ns\\O\\Squid.cpp", 0xbe6u);
        }

        if ((raw[11] & 3u) != 0u)
        {
            Need(hooks.Assertion)("!isDead() && !isFree()", "\\Ns\\O\\Squid.cpp", 0xbe7u);
        }

        if (((parentRaw[11] & 3u) != 0u) || ((raw[11] & 3u) != 0u))
        {
            return false;
        }

        ushort group = Word(raw, 20);
        uint order = 0u;
        int current = Word(parentRaw, 6);
        int previous = 0;
        bool inserted = false;
        var visited = new HashSet<int>();
        // 다음 WORD·현재 content DWORD를 원문 위치에서 읽는 반복문이다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.
        while (current != 0)
        {
            if (!visited.Add(current))
            {
                throw new InvalidOperationException("정렬 Content 부착의 비종료 체인 순환");
            }

            ReadOnlySpan<byte> candidate = pool.Slot(current);
            uint content = Need(hooks.ContentType)();
            if (candidate[10] == content && (((Word(candidate, 20) ^ group) & 0xfcffu) == 0u))
            {
                if (order < candidate[23])
                {
                    SetNext(pool, form, current, hooks);
                    uint ownTable = Table(raw);
                    int before = Need(hooks.Previous)(current, Table(candidate));
                    Need(hooks.SetPrevious)(form, ownTable, before);
                    Need(hooks.SetPrevious)(current, Table(candidate), form);
                    if (Need(hooks.Previous)(form, Table(raw)) == 0)
                    {
                        Put(parentRaw, 6, unchecked((ushort)form));
                    }
                    else
                    {
                        int selected = Need(hooks.Previous)(form, Table(raw));
                        SetNext(pool, selected, form, hooks);
                    }

                    inserted = true;
                    break;
                }

                ++order;
            }

            previous = current;
            current = Word(candidate, 4);
        }

        if (!inserted)
        {
            if (previous != 0)
            {
                SetNext(pool, previous, form, hooks);
                Need(hooks.SetPrevious)(form, Table(raw), previous);
                SetNext(pool, form, 0, hooks);
            }
            else
            {
                Put(parentRaw, 6, unchecked((ushort)form));
                SetNext(pool, form, 0, hooks);
                Need(hooks.SetPrevious)(form, Table(raw), 0);
            }
        }

        raw[23] = unchecked((byte)order);
        Need(hooks.SetParent)(form, Table(raw), parent);
        raw[11] |= 8;
        Need(hooks.Pop)(form, flags | 0x100u);
        Need(hooks.Notice)(parent, form, 1u);
        return true;
    }

    /// <summary>소비 위치의 가상 표 토큰이다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    private static uint Table(ReadOnlySpan<byte> raw) => Word(raw, 0) | ((uint)Word(raw, 2) << 16);
}
