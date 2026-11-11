namespace Netstorm.Core.Original;

/// <summary>
/// 공간 해제(Unpop)가 "표시 갱신"을 공통 경로(update88/update8c)로 처리할 수 있는 가상 표(클래스 식별 값)의 지원 표(10.78·CD 10.72).
/// 이 표에 없는 클래스는 파생 표시 갱신 효과가 아직 복원되지 않았으므로 Unpop 이 상태를 바꾸기 전에 거부한다.
/// 표 데이터(.Data.cs)는 dotnetpj/tools/gen_original_unpop_vtables.py 가 cpppj 목록에서 만들고,
/// 원본 기계어 기대값 unpop-vtables.tsv 의 10.78 83행으로 검증한다.
/// 지원 비트: 1 은 일반 경로, 2 는 소용돌이 플래그(0x2000)를 켠 호출의 경로다.
/// 근거: docs/exe/cpp-unpop-reconstruction.md, cpppj/src/o/SquidUnpop.cpp (W02 Unpop 조각, 2026-10-16. CD 표는 F2c-11, 2026-11-04).
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

    /// <summary>
    /// 판본별 공통 표시 경로가 이 호출 플래그에서 표시 갱신을 지원하는지 돌려준다(cpppj <c>SquidUnpop::SupportsDisplay</c>).
    /// 확정된 지원 함수가 아니면 미복원 override 로 지원하지 않는다.
    /// </summary>
    /// <param name="edition">원본 판본.</param>
    /// <param name="vtable">슬롯 +0 의 가상 표 값.</param>
    /// <param name="flags">Unpop 호출 플래그. <see cref="VortexFlag"/> 가 켜지면 소용돌이 경로 지원을 요구한다.</param>
    /// <remarks>2026-11-04 F2c-11에서 추가했다.</remarks>
    public static bool Supports(OriginalEdition edition, uint vtable, uint flags)
    {
        // 메타·일반 Sprite 두 가상 표는 실제 메타 위치에 분배하므로 항상 지원한다. 2026-11-11 0.355에서 cpppj HEAD를 반영했다.
        if (edition == OriginalEdition.Patch1078 && (vtable == 0x0050C9F0 || vtable == 0x0050CC08))
        {
            return true;
        }
        // 패치판의 무조건 지원 분기다
        if (edition == OriginalEdition.Patch1078 && (vtable == 0x0050C3B0 || vtable == 0x00509810))
        {
            return true;
        }
        DisplayVtable[] values = edition == OriginalEdition.Patch1078 ? Table : CdTable;
        uint need = (flags & VortexFlag) != 0 ? 2U : 1U;
        // 확인한 가상 함수가 아니면 미복원 override 를 적용하지 않는다
        foreach (DisplayVtable entry in values)
        {
            if (entry.Vtable == vtable)
            {
                return (entry.Mask & need) != 0;
            }
        }
        return false;
    }
}
