namespace Netstorm.Core.Original;

/// <summary>
/// 공간 해제(Unpop)가 "표시 갱신"을 공통 경로(update88/update8c)로 처리할 수 있는 가상 표(클래스 식별 값)의 지원 표(10.78).
/// 이 표에 없는 클래스는 파생 표시 갱신 효과가 아직 복원되지 않았으므로 Unpop 이 상태를 바꾸기 전에 거부한다.
/// 표 데이터(.Data.cs)는 dotnetpj/tools/gen_original_unpop_vtables.py 가 cpppj 목록에서 만들고,
/// 원본 기계어 기대값 unpop-vtables.tsv 의 10.78 83행으로 검증한다.
/// 지원 비트: 1 은 일반 경로, 2 는 소용돌이 플래그(0x2000)를 켠 호출의 경로다.
/// 근거: docs/exe/cpp-unpop-reconstruction.md, cpppj/src/o/SquidUnpop.cpp (W02 Unpop 조각, 2026-10-16).
/// </summary>
public static partial class OriginalUnpopDisplay
{
    /// <summary>호출 플래그의 소용돌이 비트. 켜지면 지원 비트 2 를, 꺼지면 1 을 요구한다.</summary>
    public const uint VortexFlag = 0x2000;

    /// <summary>표의 한 항목: 가상 표 값과 지원 비트.</summary>
    /// <param name="Vtable">슬롯 +0 의 가상 표 값(클래스 식별 값).</param>
    /// <param name="Mask">지원 비트(1 일반, 2 소용돌이 경로).</param>
    private readonly record struct DisplayVtable(uint Vtable, byte Mask);

    /// <summary>표의 항목 수(10.78 은 83개).</summary>
    public static int Count => Table.Length;

    /// <summary>
    /// 가상 표가 이 호출 플래그에서 공통 표시 갱신을 지원하는지 돌려준다. 표에 없는 값은 지원하지 않는다(미복원 override).
    /// </summary>
    /// <param name="vtable">슬롯 +0 의 가상 표 값.</param>
    /// <param name="flags">Unpop 호출 플래그. <see cref="VortexFlag"/> 가 켜지면 소용돌이 경로 지원을 요구한다.</param>
    public static bool Supports(uint vtable, uint flags)
    {
        uint need = (flags & VortexFlag) != 0 ? 2U : 1U;
        // 확인한 가상 함수가 아니면 미복원 override 를 적용하지 않는다
        foreach (DisplayVtable entry in Table)
        {
            if (entry.Vtable == vtable)
            {
                return (entry.Mask & need) != 0;
            }
        }
        return false;
    }
}
