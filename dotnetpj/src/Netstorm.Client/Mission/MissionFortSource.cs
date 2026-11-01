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

/// <summary>선택된 파일의 manager 이름과 지연 읽기를 함께 소유한다.</summary>
public sealed class MissionFortFileSelection
{
    /// <summary>선택 순간의 경로와 매퍼로 결정한 실제 파일 이름이다.</summary>
    public required string FileName { get; init; }

    /// <summary>선택한 경로와 source를 유지하며 호출할 때 파일을 읽는다.</summary>
    public required Func<byte[]> Read { get; init; }
}

/// <summary>논리 이름으로 경로와 manager 이름을 선택하며 파일을 열지 않는다.</summary>
/// <param name="selectedFort">선택할 논리 요새 이름.</param>
public delegate MissionFortFileSelection MissionFortFileSelector(string selectedFort);
