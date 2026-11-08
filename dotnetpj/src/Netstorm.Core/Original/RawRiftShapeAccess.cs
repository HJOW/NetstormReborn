// 실제 00419850의 낮은 DWORD 감김과 진단 뒤 현재 SHP 접근을 구현한다.
namespace Netstorm.Core.Original;

// SHP 접근 모듈을 담는 네임스페이스다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.

// 도달한 상세·진단·현재 SHP 장치만 요구한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.
public sealed class RiftShapeAccessHooks
{
    /// <summary>0049adc0의 선택된 TYPE 번호다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    public Func<RawSquidDisplayTypeView, uint>? TypeNumber { get; set; }

    /// <summary>00436d40의 현재 상세 owner에 세 저장 인자를 전달한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    public Action<uint, int, uint>? FrameError { get; set; }

    /// <summary>원문 rifttype.h:e1이며 checking과 무관하다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>진단 복귀 뒤 선택된 TYPE의 현재 SHP에서 저장 frame을 읽는다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    public Func<uint, int, float[]>? Shape { get; set; }
}

// SHP 프레임의 범위·해제 표식·상세 오류·진단 복귀 뒤 물리 접근 순서를 보존한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.
public static class RawRiftShapeAccess
{
    /// <summary>원문 TYPE+e8의 그림자·abstract 두 배 프레임 마스크다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    private const uint DoubleFrames = 0x440000u;

    /// <summary>원문 TYPE 첫 DWORD의 두 CRT 표식이다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    private const int Freed = -0x22222223;

    /// <summary>원문 TYPE 첫 DWORD의 두 CRT 표식이다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    private const int Uninitialized = -0x32323233;

    /// <summary>선택한 callback이 실행 중 교체돼도 해당 호출의 수명을 보존한다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">검사할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T ShapeNeed<T>(T? function) where T : class
    {
        // 도달하지 않은 장치는 요구하지 않는다.
        if (function is null)
        {
            throw new InvalidOperationException("SHP 접근의 선택된 실제 장치 미연결");
        }

        return function;
    }

    /// <summary>원문 00419850은 범위를 진단한 뒤 같은 frame으로 계속하며 SHP·TYPE을 다른 정상 입력으로 보정하지 않는다.</summary>
    /// <param name="type">선택된 TYPE 뷰다.</param>
    /// <param name="frame">읽을 프레임 번호다.</param>
    /// <param name="abstractValue">저장된 abstract 인자다.</param>
    /// <param name="hooks">도달한 장치 묶음이다.</param>
    /// <returns>현재 SHP의 셀 가로·세로다.</returns>
    public static float[] ReadFrame(RawSquidDisplayTypeView type, int frame, uint abstractValue, RiftShapeAccessHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        // 부모가 저장한 TYPE·frame을 진단 이후에도 유지한다.
        int count = unchecked((int)(unchecked((uint)type.FrameCount) * ((type.Flags1 & DoubleFrames) != 0 ? 2u : 1u)));
        bool range = frame >= 0 && frame < count;
        if (!range)
        {
            uint number = ShapeNeed(hooks.TypeNumber)(type);
            ShapeNeed(hooks.FrameError)(number, frame, abstractValue);
        }

        if (!range || !type.Loaded || type.Header == Freed || type.Header == Uninitialized)
        {
            ShapeNeed(hooks.Assertion)("frameCheck( abstract, frame, false )", "..\\o\\rifttype.h", 0xe1u);
        }

        // 실제 물리 범위 밖은 이 소비 지점의 호스트 오류로 남긴다.
        return ShapeNeed(hooks.Shape)(type.Token, frame);
    }
}
