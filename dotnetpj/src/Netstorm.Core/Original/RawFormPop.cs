// 공통 Form Pop의 진단 후 상태·현재 권한과 공유 postPop 깊이를 연결한다.
namespace Netstorm.Core.Original;

// Form Pop 모듈을 담는 네임스페이스다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.

// 선택된 자식만 요구하며 원문 저장값과 현재값을 구별한다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.
public sealed class FormPopHooks
{
    /// <summary>진단 복귀 뒤 현재 서버 권한을 읽는다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    public Func<bool>? Server { get; set; }

    /// <summary>고정 -1/-1/0과 정규화 flags를 현재 전송 자식에 전달한다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    public Action<int, uint, uint, uint, uint>? Transmit { get; set; }

    /// <summary>현재 공통 basePostPop DWORD를 읽는다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    public Func<uint>? Depth { get; set; }

    /// <summary>원문 DWORD 증가/감소를 같은 장기 저장소에 쓴다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    public Action<uint>? SetDepth { get; set; }

    /// <summary>증가 후 현재 VT+20과 flags를 선택한다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    public Action<int, uint, uint>? PostPop { get; set; }

    /// <summary>원문 식/파일/행의 정상 복귀를 허용한다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>현재 훅 묶음의 얕은 사본을 만든다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    /// <returns>복사된 훅 묶음이다.</returns>
    public FormPopHooks Clone() => new()
    {
        Server = Server,
        Transmit = Transmit,
        Depth = Depth,
        SetDepth = SetDepth,
        PostPop = PostPop,
        Assertion = Assertion,
    };
}

// 실제 Form +70과 공통 부착의 직접 Pop 호출이 공유하는 부모다. PATCH004ac560/CD004abfb0 부모만 실행하며 원문에 없는 선행 guard를 추가하지 않는다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.
public static class RawFormPop
{
    /// <summary>앞선 상태·깊이 쓰기를 유지하며 선택 장치를 요구한다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    /// <param name="device">쓸 장치다.</param>
    private static T Need<T>(T? device) where T : class
    {
        if (device is null)
        {
            throw new InvalidOperationException("공통 Form Pop의 선택된 실제 장치 미연결");
        }

        return device;
    }

    /// <summary>공통 Form Pop 몸체다. void·전송 비트의 완료 쓰기를 실패 시 되돌리지 않는다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="form">Pop 할 Form 번호다.</param>
    /// <param name="flags">호출자가 준 Pop 플래그다.</param>
    /// <param name="hooks">선택 장치 묶음이다.</param>
    public static void PopCommonForm(RawSidPool pool, int form, uint flags, FormPopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        bool patch = pool.Edition == OriginalEdition.Patch1078;
        void Diagnose(string expression, uint patchLine, uint cdLine) =>
            Need(hooks.Assertion)(expression, patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp", patch ? patchLine : cdLine);
        // 정상 복귀 뒤 원문 현재값을 다시 읽는다
        if (form == 0 || form >= pool.Capacity)
        {
            Diagnose("isValid()", 0x35bu, 0x352u);
        }

        Span<byte> raw = pool.RawSlot(form);
        if ((raw[11] & 4u) == 0)
        {
            Diagnose("isVoid()", 0x35cu, 0x353u);
        }

        byte state = raw[11];
        raw[11] = (byte)(state & ~4u);
        if ((state & 0x10u) != 0)
        {
            Diagnose("!xmitJustCreatedFlag", 0x35fu, 0x356u);
        }

        if (!Need(hooks.Server)())
        {
            if ((flags & 8u) == 0)
            {
                flags |= 0x10u;
            }
        }
        else
        {
            uint end = patch ? 15000u : 6000u;
            // 예약 0~4는 원문 client 범위에 포함하지 않는다
            if (form < 5 || (uint)form >= end)
            {
                if ((flags & 0x10u) == 0)
                {
                    flags |= 8u;
                }
            }
            else
            {
                if ((flags & 8u) != 0)
                {
                    Diagnose("!(popFlags & pfSERVER_POP)", 0x364u, 0x35bu);
                }

                flags |= 0x10u;
            }

            if ((flags & 0x50u) == 0)
            {
                if ((flags & 7u) != 0)
                {
                    raw[11] |= unchecked((byte)0x10u);
                }

                Need(hooks.Transmit)(form, 0xffffffffu, 0xffffffffu, 0u, flags);
                raw[11] &= unchecked((byte)~0x10u);
            }
        }

        uint depth = Need(hooks.Depth)();
        Need(hooks.SetDepth)(unchecked(depth + 1u));
        // 깊이 증가 뒤 현재 VT DWORD다
        uint table = (uint)raw[0] | ((uint)raw[1] << 8) | ((uint)raw[2] << 16) | ((uint)raw[3] << 24);
        Need(hooks.PostPop)(form, table, flags);
        if (depth != Need(hooks.Depth)())
        {
            Diagnose("curPostPop == basePostPop", 0x392u, 0x389u);
        }
    }
}
