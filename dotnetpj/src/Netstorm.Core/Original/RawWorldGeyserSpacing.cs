// 세계 간헐천 간격 선택을 실제 간격 칸 수로 바꾸는 작은 값 변환을 담는다.
namespace Netstorm.Core.Original;

// 세계 간헐천 간격 값 변환을 담는 네임스페이스다. 2026-11-12 R4 지형 바인딩 입력 결선(0.453)에서 추가됐다.

// 원본 0041ca10의 스택 표(BYTE 0~3일 때만 정의된 네 항목)를 복원하는 값 변환이다. 2026-11-12 R4 지형 바인딩 입력 결선(0.453)에서 추가됐다.
public static class RawWorldGeyserSpacing
{
    /// <summary>원본의 선택별 간격 칸 수다. 순서대로 3·4·5·6칸이다.</summary>
    /// <remarks>2026-11-12 R4 지형 바인딩 입력 결선(0.453)에서 추가됐다. 근거: cpppj o/RawWorldGeyserBounds.cpp WorldGeyserSpacing.</remarks>
    private static readonly int[] Values = [3, 4, 5, 6];

    /// <summary>간격 선택 BYTE를 실제 간격 칸 수로 바꾼다. 원본 스택 표 범위 밖은 거부한다.</summary>
    /// <param name="selection">전투 옵션 6번의 간격 선택 BYTE다.</param>
    /// <returns>실제 간격 칸 수다.</returns>
    /// <remarks>2026-11-12 R4 지형 바인딩 입력 결선(0.453)에서 추가됐다. 근거: cpppj o/RawWorldGeyserBounds.cpp WorldGeyserSpacing.</remarks>
    public static int Spacing(byte selection)
    {
        if (selection >= Values.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(selection), "세계 간헐천 간격의 원문 표 범위 오류");
        }

        return Values[selection];
    }
}
