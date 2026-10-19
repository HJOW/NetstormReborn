namespace Netstorm.Core.Original;

/// <summary>
/// 공간 등록(Pop)이 파생 firstPop/postPop 효과를 호출해도 되는지 가리는 가상 표(클래스 식별 값)의 지원 표(10.78).
/// 표에 없는 클래스는 파생 공간 등록 효과가 아직 복원되지 않았으므로 Pop 이 상태를 바꾸기 전에 거부한다.
/// 표 자료(.Data.cs)는 dotnetpj/tools/gen_original_pop_vtables.py 가 cpppj 목록에서 만들고,
/// 원본 기계어 기대값 pop-vtables.tsv 의 10.78 83행으로 검증한다.
/// 판정은 원본 SquidPop::Supports(004b02d0, 10.78)와 같다: 마나 대체 주소 3개는 앞 분기로 가리고,
/// 0x50C3B0·0x509810 은 원본 표시 지원의 무조건 지원 분기로 true, 0x5070C0 과 표 26개는
/// 공통 표시 지원(<see cref="OriginalUnpopDisplay.Supports"/>)으로 가린다.
/// 근거: cpppj/src/o/SquidPop.cpp (W02 Pop 조각, 2026-10-20).
/// </summary>
public static partial class OriginalPopDisplay
{
    /// <summary>마나 대체 주소 3개(원본 004b02d0의 앞 분기). 이 값들은 공통 표시 지원 표로 직접 가린다.</summary>
    private const uint ManaVtable0 = 0x0050C3B0;

    /// <summary>마나 대체 주소 3개 중 둘째.</summary>
    private const uint ManaVtable1 = 0x00509810;

    /// <summary>마나 대체 주소 3개 중 셋째.</summary>
    private const uint ManaVtable2 = 0x005070C0;

    /// <summary>표의 항목 수(10.78 은 26개).</summary>
    public static int Count => Table.Length;

    /// <summary>
    /// 가상 표가 이 호출 플래그에서 Pop 의 파생 효과를 지원하는지 돌려준다.
    /// 표에 없는 값과 공통 표시 갱신을 지원하지 않는 값은 지원하지 않는다(미복원 override).
    /// </summary>
    /// <param name="vtable">슬롯 +0 의 가상 표 값.</param>
    /// <param name="flags">Pop 호출 플래그. 소용돌이 비트(0x2000)가 켜지면 소용돌이 경로 지원을 요구한다.</param>
    public static bool Supports(uint vtable, uint flags)
    {
        // 마나 대체 주소 3개는 원본 SquidPop::Supports 의 앞 분기로 공통 표시 지원 결과를 그대로 쓴다.
        // 0x50C3B0·0x509810 은 원본 SquidUnpop::SupportsDisplay 가 무조건 지원하므로 true 다(표의 마스크 0과 무관).
        // 이 무조건 지원 분기는 Unpop C# port 에 아직 없으므로 Pop 에서 직접 반영한다.
        if (vtable == ManaVtable0 || vtable == ManaVtable1)
        {
            return true;
        }
        if (vtable == ManaVtable2)
        {
            return OriginalUnpopDisplay.Supports(vtable, flags);
        }
        // 표에 있으면 공통 표시 지원으로 가린다
        foreach (uint candidate in Table)
        {
            if (candidate == vtable)
            {
                return OriginalUnpopDisplay.Supports(vtable, flags);
            }
        }
        return false;
    }
}
