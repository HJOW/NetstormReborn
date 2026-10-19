// 이 파일은 dotnetpj/tools/gen_original_pop_vtables.py 가 만든다. 직접 고치지 말고 도구를 다시 실행한다.
// 입력(cpppj, 읽기 전용): PopVtables.inc sha256=7e1989b0f2d13457b72388c7df9d231411b556aa7c47e1130e1616a3d5a7de16
// 10.78 부분만 옮겼다. 결과는 원본 기계어 기대값 pop-vtables.tsv 로 검사한다 (W02 Pop 조각, 2026-10-20).
namespace Netstorm.Core.Original;

public static partial class OriginalPopDisplay
{
    /// <summary>Pop 의 파생 firstPop/postPop 효과를 지원하는 가상 표 26개(10.78). 호스트 포인터가 아니라 클래스 식별 값이다.</summary>
    private static readonly uint[] Table =
    [
        0x00501DD0,
        0x00502370,
        0x00502448,
        0x00502510,
        0x005025D8,
        0x005026A0,
        0x00502778,
        0x00502840,
        0x00502908,
        0x005029D0,
        0x00502AA8,
        0x00502B70,
        0x00502E68,
        0x00502F30,
        0x00502FF8,
        0x005030C0,
        0x00503188,
        0x00503360,
        0x00506E50,
        0x005092D0,
        0x00509388,
        0x005096A0,
        0x00509758,
        0x00509B98,
        0x0050DB78,
        0x0050EB98,
    ];
}
