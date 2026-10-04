using Netstorm.Assets;

namespace Netstorm.Core.Tests;

/// <summary>
/// Core 테스트용 클론 데이터 접근. assets/game-data/가 누락되면 테스트를 실패시킨다.
/// 타입 목록과 파일 시스템은 읽기 전용이라 한 번만 만들어 여러 테스트가 함께 쓴다.
/// 반면 GameResources 의 설정 조회(ConfigStore)는 조회 중에 임시 층을 넣었다 빼는 가변 상태라서, xUnit 이 테스트 클래스를
/// 병렬로 실행하면 서로의 층을 밟아 엉뚱한 맵·미션을 읽을 수 있다. 그래서 RequireResources 는 호출마다 새 인스턴스를 돌려준다.
/// </summary>
internal static class OriginalData
{
    /// <summary>찾은 원본 데이터 폴더 (없으면 null)</summary>
    private static readonly Lazy<string?> DirectoryLazy = new(GameDataLocator.FindDataDirectory);

    /// <summary>원본 파일 시스템 (폴더가 없으면 null). 만든 뒤에는 바뀌지 않는다.</summary>
    private static readonly Lazy<GameFileSystem?> FilesLazy = new(() =>
        DirectoryLazy.Value == null ? null : GameFileSystem.Open(DirectoryLazy.Value));

    /// <summary>원본 타입 목록 (폴더가 없으면 null)</summary>
    private static readonly Lazy<TypeCatalog?> TypesLazy = new(() =>
        FilesLazy.Value == null ? null : new GameResources(FilesLazy.Value, GameLanguage.English).LoadTypes());

    /// <summary>클론 자원. 호출마다 설정 층이 따로인 새 인스턴스를 만든다. 누락 시 테스트 실패다.</summary>
    public static GameResources RequireResources()
    {
        Assert.True(FilesLazy.Value != null, $"클론 데이터 폴더가 없습니다: assets/game-data/ 또는 {GameDataLocator.EnvironmentVariable}");
        return new GameResources(FilesLazy.Value!, GameLanguage.English);
    }

    /// <summary>원본 타입 목록. 없으면 현재 테스트를 건너뛴다</summary>
    public static TypeCatalog RequireTypes()
    {
        RequireResources();
        return TypesLazy.Value!;
    }
}
