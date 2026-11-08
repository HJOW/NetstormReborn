// 현재 TYPE 두 조회와 frameCheck 진단 뒤 표시 인수 재조회를 같은 순서로 실행한다.
namespace Netstorm.Core.Original;

// 표시 부모 모듈을 담는 네임스페이스다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.

// 선택된 TYPE의 주소 정체성을 유지하는 비소유 필드 묶음이다. 호출 동안 같은 표에서 다시 조회해 현재값을 읽는다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.
public readonly record struct RawSquidDisplayTypeView(
    uint Token,
    uint Flags1,
    int FrameCount,
    bool Loaded,
    int Header);

// TYPE 조회와 이미 존재하는 로그·진단·표시 자식만 외부에 둔다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.
public sealed class RawSquidDisplayParentHooks
{
    /// <summary>raw의 현재 BYTE 번호로 같은 표의 뷰를 반환한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    public Func<uint, RawSquidDisplayTypeView>? Type { get; set; }

    /// <summary>선택된 TYPE 포인터의 번호만 반환하며 호출자의 frame·abstract는 읽지 않는다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    public Func<RawSquidDisplayTypeView, uint>? TypeNumber { get; set; }

    /// <summary>TYPE 번호·고정 frame·번호 조회 전 abstract를 받는 오류 상세 writer다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    public Action<uint, int, uint>? FrameError { get; set; }

    /// <summary>원문 진단이며 복귀할 수 있다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>현재 TYPE·고정 frame·현재 x·y 비트·marker16이다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    public Action<uint, int, uint, uint, uint>? Mark { get; set; }
}

// 004ad500의 현재 TYPE 두 조회·frameCheck 진단·표시 요청을 순수 부모로 보존한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.
public static class RawSquidDisplayParent
{
    /// <summary>원본 BYTE·DWORD 폭과 부호 감김만 공통으로 다룬다.</summary>
    /// <param name="callback">검사할 장치다.</param>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? callback) where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("Squid 표시 부모의 선택된 실제 장치 미연결");
        }

        return callback;
    }

    /// <summary>정확한 4바이트를 리틀 엔디언 DWORD로 읽는다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset)
    {
        // 비정렬 호스트 포인터를 역참조하지 않는다.
        uint result = 0u;
        for (int i = 0; i < 4; i++)
        {
            result |= (uint)raw[offset + i] << (8 * i);
        }

        return result;
    }

    /// <summary>원문의 두 배 count는 하위 DWORD 감김 뒤 부호 비교다.</summary>
    /// <param name="type">선택된 TYPE 뷰다.</param>
    /// <param name="frame">검사할 프레임 번호다.</param>
    /// <returns>범위 안이면 참이다.</returns>
    private static bool FrameRange(RawSquidDisplayTypeView type, int frame)
    {
        // TYPE 필드는 호출 순간의 현재 값이다.
        int count = unchecked((int)(unchecked((uint)type.FrameCount) * ((type.Flags1 & 0x440000u) != 0 ? 2u : 1u)));
        return frame >= 0 && frame < count;
    }

    /// <summary>null SHP와 두 해제 표식은 유효 프레임 범위와 별개로 검사한다.</summary>
    /// <param name="type">선택된 TYPE 뷰다.</param>
    /// <returns>표시할 그림이 있으면 참이다.</returns>
    private static bool ShapePresent(RawSquidDisplayTypeView type)
    {
        return type.Loaded && type.Header != -0x22222223 && type.Header != -0x32323233;
    }

    /// <summary>TYPE 번호 조회 전에 abstract를 고정한 뒤 실제 세 인자 상세 writer를 호출한다.</summary>
    /// <param name="raw">현재 슬롯 바이트다.</param>
    /// <param name="type">선택된 TYPE 뷰다.</param>
    /// <param name="frame">고정한 프레임 번호다.</param>
    /// <param name="hooks">외부 장치 묶음이다.</param>
    private static void Report(ReadOnlySpan<byte> raw, RawSquidDisplayTypeView type, int frame, RawSquidDisplayParentHooks hooks)
    {
        uint abstractValue = raw[40] & 1u;
        uint number = Need(hooks.TypeNumber)(type);
        Need(hooks.FrameError)(number, frame, abstractValue);
        // 두 장치 사이에 checking을 임의로 다시 검사하지 않는다.
    }

    /// <summary>raw 주소·수명과 반환 TYPE 필드의 주소는 모든 callback 동안 유지되어야 한다. frame만 진입 시 고정한다.</summary>
    /// <param name="raw">50바이트 슬롯이다.</param>
    /// <param name="checking">현재 진단 DWORD의 비소유 뷰다.</param>
    /// <param name="hooks">외부 장치 묶음이다.</param>
    public static void Run(ReadOnlySpan<byte> raw, RawCheckingFlag checking, RawSquidDisplayParentHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(checking);
        ArgumentNullException.ThrowIfNull(hooks);
        if (raw.Length != 50)
        {
            throw new ArgumentException("Squid 표시 부모의 물리 50바이트 슬롯 필요");
        }

        // 진단이 raw frame을 바꿔도 원문 지역값은 유지한다.
        int frame = unchecked((int)Read(raw, 36));
        if (checking)
        {
            RawSquidDisplayTypeView type = Need(hooks.Type)(raw[RawSidPool.TypeOffset]);
            bool range = FrameRange(type, frame);
            // 첫 조회가 checking·raw를 바꿔도 선택된 TYPE 뷰를 유지한다.
            if (!range && checking)
            {
                Report(raw, type, frame, hooks);
            }

            if (!range || !ShapePresent(type))
            {
                Need(hooks.Assertion)("getTypeStruct()->frameCheck( isAbstract(), theFrame, true )", "\\Ns\\O\\Squid.cpp", 0x846u);
            }
        }

        // 진단 복귀 뒤 현재 raw TYPE 번호로 새 뷰를 조회한다.
        RawSquidDisplayTypeView current = Need(hooks.Type)(raw[RawSidPool.TypeOffset]);
        if (FrameRange(current, frame))
        {
            if (ShapePresent(current))
            {
                Need(hooks.Mark)(raw[RawSidPool.TypeOffset], frame, Read(raw, 14), Read(raw, 18), (uint)((raw[40] >> 2) & 1) << 4);
            }
        }
        else if (checking)
        {
            Report(raw, current, frame, hooks);
        }
    }
}
