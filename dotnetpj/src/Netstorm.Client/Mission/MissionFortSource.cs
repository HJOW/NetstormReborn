namespace Netstorm.Client.Mission;

/// <summary>
/// 미션이 선택한 같은 파일 읽기의 원본 이미지와 실제 Template 파일 이름을 함께 전달한다.
/// 근거: cpppj/src/client/MissionFortSource.h. 논리 미션 이름과 저장에 사용할 파일 경로의 수명을 분리한다.
/// (R3k-12, 2026-10-31)
/// </summary>
public sealed class MissionFortSourceImage
{
    /// <summary>실제 선택 source가 읽은 같은 Fort 파일 원문이다.</summary>
    public byte[] Image { get; set; } = [];

    /// <summary>실제 manager+c에 설치할 파일 경로다. 편집 작업본은 같은 쓰기 가능 영역의 경로다.</summary>
    public string FileName { get; set; } = "";
}

/// <summary>
/// 입력은 논리 요새 이름이며 출력은 같은 읽기의 파일 이미지/경로다.
/// 근거: cpppj/src/client/MissionFortSource.h 의 MissionFortFileReader. (R3k-12, 2026-10-31)
/// </summary>
/// <param name="selectedFort">고른 논리 요새 이름.</param>
public delegate MissionFortSourceImage MissionFortFileReader(string selectedFort);
