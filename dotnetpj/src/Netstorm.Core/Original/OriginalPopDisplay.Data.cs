// 이 파일은 dotnetpj/tools/gen_original_pop_vtables.py 가 만든다. 직접 고치지 말고 도구를 다시 실행한다.
// 입력(cpppj, 읽기 전용): PopVtables.inc sha256=7e1989b0f2d13457b72388c7df9d231411b556aa7c47e1130e1616a3d5a7de16
// 10.78 몫과 CD 10.72 몫(kCdPopVtables)을 함께 옮겼다. 결과는 원본 기계어 기대값 pop-vtables.tsv 로 검사한다 (W02 Pop 조각, 2026-10-20. CD 표는 F2c-11, 2026-11-04).
namespace Netstorm.Core.Original;

/// <summary>Pop 의 표시·파생 효과를 지원하는 가상 표 주소(10.78·CD 10.72)를 담는 데이터 전용 클래스다. RawPop·RawPostPop 이 Supports 조회에 쓴다. 값은 이 도구가 만들며 직접 고치지 않는다. (W02 Pop 조각 2026-10-20 생성·summary 추가)</summary>
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

    /// <summary>Pop 의 파생 firstPop/postPop 효과를 지원하는 가상 표 16개(CD 10.72). (F2c-11, 2026-11-04)</summary>
    private static readonly uint[] CdTable =
    [
        0x00500030,
        0x00500818,
        0x005019F8,
        0x005026B8,
        0x00502780,
        0x00502848,
        0x00502910,
        0x005029D8,
        0x00502AA0,
        0x00503008,
        0x005031B0,
        0x00503A78,
        0x00503B30,
        0x005050F0,
        0x005058D8,
        0x00506A18,
    ];
}
