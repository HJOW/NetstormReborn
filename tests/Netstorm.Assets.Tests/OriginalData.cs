namespace Netstorm.Assets.Tests;

/// <summary>
/// 테스트용 원본 데이터 접근. 원본(originals/)은 저장소에 없으므로 CI 등에서는 찾지 못할 수 있고,
/// 그 경우 원본이 필요한 테스트는 건너뛴다.
/// </summary>
internal static class OriginalData
{
    /// <summary>찾은 원본 데이터 폴더 (없으면 null). 한 번만 탐색한다</summary>
    private static readonly Lazy<string?> DirectoryLazy = new(GameDataLocator.FindDataDirectory);

    /// <summary>원본 데이터 폴더 경로. 없으면 현재 테스트를 건너뛴다</summary>
    public static string RequireDirectory()
    {
        string? dir = DirectoryLazy.Value;
        Assert.SkipWhen(dir == null, $"원본 데이터 폴더가 없어 건너뜀 ({GameDataLocator.EnvironmentVariable} 로 지정 가능)");
        return dir!;
    }

    /// <summary>원본 데이터 폴더 안의 파일 경로 (대소문자 무시). 없으면 테스트를 건너뛴다</summary>
    /// <param name="relativePath">상대 경로 (예: "d/_shapes.shp")</param>
    public static string RequireFile(string relativePath)
    {
        string? path = GameDataLocator.FindFile(RequireDirectory(), relativePath);
        Assert.SkipWhen(path == null, $"원본 파일이 없어 건너뜀: {relativePath}");
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
