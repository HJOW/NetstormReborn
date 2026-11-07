// 공통 Form 부착의 현재 TYPE·가상 setter와 진단 뒤 부분 쓰기를 연결한다.
namespace Netstorm.Core.Original;

// Form 부착 모듈을 담는 네임스페이스다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.

// 선택된 자식만 요구하며 콜백 뒤 현재 바이트를 원문 위치에서 재조회한다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.
public sealed class FormAttachHooks
{
    /// <summary>현재 raw TYPE의 flags2를 읽는다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    public Func<byte, uint>? Genus { get; set; }

    /// <summary>현재 가상 표+8의 실제 부모 setter다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    public Action<int, uint, int>? SetParent { get; set; }

    /// <summary>현재 가상 표+c의 실제 이전 setter다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    public Action<int, uint, int>? SetPrevious { get; set; }

    /// <summary>직접 공통 Pop 자식이며 flags|0x100을 전달한다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    public Action<int, uint>? Pop { get; set; }

    /// <summary>Pop 뒤 현재 전체 contentFormType DWORD다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    public Func<uint>? ContentType { get; set; }

    /// <summary>원문 식·행의 정상 복귀도 허용한다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>현재 훅 묶음의 얕은 사본을 만든다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    /// <returns>복사된 훅 묶음이다.</returns>
    public FormAttachHooks Clone() => new()
    {
        Genus = Genus,
        SetParent = SetParent,
        SetPrevious = SetPrevious,
        Pop = Pop,
        ContentType = ContentType,
        Assertion = Assertion,
    };
}

// 원문 thiscall의 같은 물리 슬롯을 사용하며 클래스별 +74 접두와 구별한다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.
public static class RawFormAttach
{
    /// <summary>공통 원문 폭의 현재 메모리 접근과 선택 장치 요구를 공유한다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    /// <param name="device">쓸 장치다.</param>
    private static T Need<T>(T? device) where T : class
    {
        if (device is null)
        {
            throw new InvalidOperationException("Form 부착의 선택된 실제 장치 미연결");
        }

        return device;
    }

    /// <summary>원문 비정렬 WORD를 읽는다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static ushort Word(ReadOnlySpan<byte> raw, int offset) =>
        (ushort)(raw[offset] | (raw[offset + 1] << 8));

    /// <summary>각 가상 호출 직전 현재 DWORD다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    private static uint Table(ReadOnlySpan<byte> raw) => Word(raw, 0) | ((uint)Word(raw, 2) << 16);

    /// <summary>주변 payload를 보존하는 WORD 저장이다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 슬롯 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, ushort value)
    {
        raw[offset] = unchecked((byte)value);
        raw[offset + 1] = unchecked((byte)(value >> 8));
    }

    /// <summary>선행 상태 검증이나 실패 rollback을 추가하지 않는 양판본 공통 부착 부모다. 알 수 없는 실제 자식은 선택 위치에서 실패한다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="form">고를 Form SID다.</param>
    /// <param name="parent">부를 부모 SID다.</param>
    /// <param name="flags">원문 플래그다.</param>
    /// <param name="hooks">선택 장치 묶음이다.</param>
    public static void Attach(RawSidPool pool, int form, int parent, uint flags, FormAttachHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        bool patch = pool.Edition == OriginalEdition.Patch1078;
        void Diagnose(string expression, uint patchLine, uint cdLine) =>
            Need(hooks.Assertion)(expression, patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp", patch ? patchLine : cdLine);
        if (parent == 0)
        {
            if (patch)
            {
                return;
            }

            Diagnose("intoSid", 0u, 0x3c2u);
        }

        Span<byte> raw = pool.RawSlot(form);
        if ((Need(hooks.Genus)(raw[10]) & 0x44200u) != 0u)
        {
            Diagnose("!(getGenus() & (gEMPLACEMENT|gVORTEX |gFACTORY))", 0x3d0u, 0x3c3u);
        }

        Need(hooks.SetParent)(form, Table(raw), parent);
        Span<byte> parentRaw = pool.RawSlot(parent);
        // 원문은 부모 this와 머리 WORD를 함께 저장한다
        ushort head = Word(parentRaw, 6);
        if (head == form)
        {
            Need(hooks.Assertion)("n != getSid()", patch ? "c:\\ns\\o\\squid.h" : "\\ns\\o\\squid.h", patch ? 0x123u : 0x10cu);
        }

        Put(raw, 4, head);
        if (head != 0)
        {
            Need(hooks.SetPrevious)(head, Table(pool.Slot(head)), form);
        }

        Put(parentRaw, 6, unchecked((ushort)form));
        Need(hooks.SetPrevious)(form, Table(raw), 0);
        raw[11] |= 8;
        Need(hooks.Pop)(form, flags | 0x100u);
        // 원문은 현재 TYPE 바이트를 읽고 현재 전체 content DWORD와 비교한다
        byte typeAfterPop = raw[10];
        if (typeAfterPop == Need(hooks.ContentType)())
        {
            Diagnose("getType()!=contentFormType", 0x3e0u, 0x3d2u);
        }

        if (raw[10] > 0x45u)
        {
            Diagnose("isForm()", 0x3e3u, 0x3d5u);
        }
    }
}
