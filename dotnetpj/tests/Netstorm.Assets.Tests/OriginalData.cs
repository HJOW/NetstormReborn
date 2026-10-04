namespace Netstorm.Assets.Tests;

/// <summary>
/// 원본 포맷을 보존한 클론 데이터 접근. 저장소의 assets/game-data/를 사용하며 누락은 테스트 실패다.
/// </summary>
internal static class OriginalData
{
    /// <summary>찾은 클론 데이터 폴더 (없으면 null). 한 번만 탐색한다.</summary>
    private static readonly Lazy<string?> DirectoryLazy = new(GameDataLocator.FindDataDirectory);

    /// <summary>필수 클론 데이터 폴더를 반환한다. 누락 시 테스트를 실패시킨다.</summary>
    public static string RequireDirectory()
    {
        string? dir = DirectoryLazy.Value;
        Assert.True(dir != null, $"클론 데이터 폴더가 없습니다: assets/game-data/ 또는 {GameDataLocator.EnvironmentVariable}");
        return dir!;
    }

    /// <summary>클론 데이터 안의 필수 파일 경로를 대소문자 무시로 찾는다. 누락은 테스트 실패다.</summary>
    /// <param name="relativePath">상대 경로 (예: "d/_shapes.shp")</param>
    public static string RequireFile(string relativePath)
    {
        string? path = GameDataLocator.FindFile(RequireDirectory(), relativePath);
        Assert.True(path != null, $"클론 데이터 파일이 없습니다: {relativePath}");
        return path!;
    }

    /// <summary>저장소 루트의 fonts/ 폴더 안 파일 경로 (저장소에 포함되어 있으므로 항상 존재해야 한다)</summary>
    /// <param name="fileName">글꼴 파일 이름</param>
    public static string RepositoryFont(string fileName)
    {
        DirectoryInfo? dir = new(AppContext.BaseDirectory);
        // 상위 폴더로 올라가며 fonts/<파일> 을 찾는다
        while (dir != null)
        {
            string candidate = Path.Combine(dir.FullName, "fonts", fileName);
            if (File.Exists(candidate))
            {
                return candidate;
            }
            dir = dir.Parent;
        }
        throw new FileNotFoundException($"저장소 fonts/ 에서 찾지 못함: {fileName}");
    }
}
