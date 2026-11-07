// 공통 포함 링크의 가상 표 선택과 부모·이전 WORD 조회를 공유한다.
namespace Netstorm.Core.Original;

// 포함 링크 모듈을 담는 네임스페이스다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.

// 물리 주소 대신 같은 SID 슬롯과 현재 가상 표를 쓰는 링크 조회다. 분리·설정 본체는 후속 단위에서 잇는다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.
public static class RawContainedLink
{
    /// <summary>원문 WORD 전체만 읽는다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static ushort LinkWord(ReadOnlySpan<byte> raw, int offset) =>
        (ushort)(raw[offset] | (raw[offset + 1] << 8));

    /// <summary>인접 payload를 바꾸지 않는 원문 WORD 쓰기다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void PutLink(RawSidPool pool, int sid, int offset, ushort value)
    {
        Span<byte> raw = pool.RawSlot(sid);
        raw[offset] = unchecked((byte)value);
        raw[offset + 1] = unchecked((byte)(value >> 8));
    }

    /// <summary>선택한 실제 가상 표의 getter·setter 배치다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="edition">원본 판본이다.</param>
    /// <param name="table">가상 표 주소다.</param>
    /// <returns>Form 배치면 참이다.</returns>
    public static bool FormLinkLayoutAtTable(OriginalEdition edition, uint table)
    {
        uint[] forms = edition == OriginalEdition.Patch1078 ? ContainedLinkTables.PatchForms : ContainedLinkTables.CdForms;
        // 네 Form 생성자다
        foreach (uint form in forms)
        {
            if (form == table)
            {
                return true;
            }
        }

        ContentNoticeEntry[] entries = edition == OriginalEdition.Patch1078 ? ContainedLinkTables.PatchNotices : ContainedLinkTables.CdNotices;
        // 허용 목록에 있으면 내용물 배치다
        foreach (ContentNoticeEntry entry in entries)
        {
            if (entry.Table == table)
            {
                return false;
            }
        }

        throw new InvalidOperationException("공통 contained 링크의 미복원 현재 가상 표");
    }

    /// <summary>일반 경로는 호출 직전 현재 가상 표를 선택한다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">조회할 번호다.</param>
    /// <returns>Form 배치면 참이다.</returns>
    public static bool FormLinkLayout(RawSidPool pool, int sid)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        uint table = LinkWord(raw, 0) | ((uint)LinkWord(raw, 2) << 16);
        return FormLinkLayoutAtTable(pool.Edition, table);
    }

    /// <summary>같은 현재 가상 표의 부모 WORD getter다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">조회할 번호다.</param>
    /// <returns>부모 SID다.</returns>
    public static int CommonContainedParent(RawSidPool pool, int sid)
    {
        ArgumentNullException.ThrowIfNull(pool);
        bool form = FormLinkLayout(pool, sid);
        return LinkWord(pool.Slot(sid), form ? 16 : 14);
    }

    /// <summary>같은 현재 가상 표의 이전 WORD getter다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">조회할 번호다.</param>
    /// <returns>이전 SID다.</returns>
    public static int CommonContainedPrevious(RawSidPool pool, int sid)
    {
        ArgumentNullException.ThrowIfNull(pool);
        bool form = FormLinkLayout(pool, sid);
        return LinkWord(pool.Slot(sid), form ? 14 : 16);
    }

    /// <summary>저장한 가상 표의 getter WORD다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">조회할 번호다.</param>
    /// <param name="table">선택한 가상 표 주소다.</param>
    /// <returns>이전 SID다.</returns>
    public static int CommonContainedPreviousAtTable(RawSidPool pool, int sid, uint table)
    {
        ArgumentNullException.ThrowIfNull(pool);
        bool form = FormLinkLayoutAtTable(pool.Edition, table);
        return LinkWord(pool.Slot(sid), form ? 14 : 16);
    }

    /// <summary>확인한 실제 가상 표+8의 하위 WORD만 바꾼다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="parent">쓸 부모 SID다.</param>
    public static void CommonContainedSetParent(RawSidPool pool, int sid, int parent)
    {
        ArgumentNullException.ThrowIfNull(pool);
        bool form = FormLinkLayout(pool, sid);
        PutLink(pool, sid, form ? 16 : 14, unchecked((ushort)parent));
    }

    /// <summary>공통 자산의 상위 WORD 쓰기와 동일한 비트 결과다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="previous">쓸 이전 SID다.</param>
    public static void CommonContainedSetPrevious(RawSidPool pool, int sid, int previous)
    {
        ArgumentNullException.ThrowIfNull(pool);
        bool form = FormLinkLayout(pool, sid);
        PutLink(pool, sid, form ? 14 : 16, unchecked((ushort)previous));
    }

    /// <summary>선택한 가상 표+c를 실행한다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="table">선택한 가상 표 주소다.</param>
    /// <param name="previous">쓸 이전 SID다.</param>
    public static void CommonContainedSetPreviousAtTable(RawSidPool pool, int sid, uint table, int previous)
    {
        ArgumentNullException.ThrowIfNull(pool);
        bool form = FormLinkLayoutAtTable(pool.Edition, table);
        PutLink(pool, sid, form ? 14 : 16, unchecked((ushort)previous));
    }
}
