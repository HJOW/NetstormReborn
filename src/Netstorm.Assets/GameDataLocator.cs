namespace Netstorm.Assets;

/// <summary>
/// 클론 게임 데이터 폴더(netstorm.tarc 와 d/ 폴더가 있는 곳)를 찾고,
/// 그 안의 파일을 대소문자 무시로 찾는다 (Linux 는 파일 시스템이 대소문자를 구분하므로 필요).
/// </summary>
public static class GameDataLocator
{
    /// <summary>데이터 폴더를 직접 지정하는 환경 변수 이름</summary>
    public const string EnvironmentVariable = "NETSTORM_DATA";

    /// <summary>데이터 폴더 판별에 쓰는 표지 파일</summary>
    private const string MarkerFile = "netstorm.tarc";

    /// <summary>빌드·배포 출력에 포함되는 클론 데이터 폴더 이름.</summary>
    private const string PackagedDataFolder = "game-data";

    /// <summary>저장소에서 관리하는 클론 데이터 소스의 상대 경로.</summary>
    private const string RepositoryDataFolder = "assets/game-data";

    /// <summary>상위 폴더로 거슬러 올라가며 찾을 최대 단계 수</summary>
    private const int MaxParentLevels = 8;

    /// <summary>
    /// 데이터 폴더를 찾는다. 순서: 명시적 환경 변수 → 출력의 game-data/ → 저장소의 assets/game-data/.
    /// </summary>
    /// <returns>찾은 폴더 경로, 없으면 null</returns>
    public static string? FindDataDirectory() => FindDataDirectory(
        AppContext.BaseDirectory, Directory.GetCurrentDirectory(),
        Environment.GetEnvironmentVariable(EnvironmentVariable));

    /// <summary>탐색 시작 경로를 지정해 클론 데이터를 찾는다. 분석용 원본 폴더는 자동 탐색하지 않는다.</summary>
    /// <param name="executableDirectory">실행 파일 폴더.</param>
    /// <param name="workingDirectory">현재 작업 폴더.</param>
    /// <param name="dataOverride">명시적으로 지정한 별도 데이터 폴더. 유효하지 않으면 기본 탐색을 계속한다.</param>
    public static string? FindDataDirectory(string executableDirectory, string workingDirectory, string? dataOverride = null)
    {
        if (!string.IsNullOrEmpty(dataOverride) && IsDataDirectory(dataOverride))
        {
            return Path.GetFullPath(dataOverride);
        }
        // 실행 파일 폴더와 현재 작업 폴더에서 각각 위로 올라가며 검사
        foreach (string start in new[] { executableDirectory, workingDirectory })
        {
            DirectoryInfo? dir = new(start);
            // 최대 단계 수만큼 상위 폴더로 이동하며 검사
            for (int level = 0; dir != null && level < MaxParentLevels; level++, dir = dir.Parent)
            {
                string candidate = Path.Combine(dir.FullName, PackagedDataFolder);
                if (IsDataDirectory(candidate))
                {
                    return Path.GetFullPath(candidate);
                }
                candidate = Path.Combine(dir.FullName, RepositoryDataFolder);
                if (IsDataDirectory(candidate))
                {
                    return Path.GetFullPath(candidate);
                }
            }
        }
        return null;
    }

    /// <summary>폴더가 게임 데이터 폴더인지 표지 파일로 검사한다.</summary>
    /// <param name="directory">검사할 폴더</param>
    public static bool IsDataDirectory(string directory) =>
        Directory.Exists(directory) && FindFile(directory, MarkerFile) != null;

    /// <summary>
    /// 기준 폴더 아래의 상대 경로를 대소문자 무시로 찾는다. 경로 구분자는 \ 와 / 모두 허용한다.
    /// </summary>
    /// <param name="baseDirectory">기준 폴더</param>
    /// <param name="relativePath">상대 경로 (예: "d\GIFCLOUD.COL")</param>
    /// <returns>실제 파일 경로, 없으면 null</returns>
    public static string? FindFile(string baseDirectory, string relativePath) =>
        FindEntry(baseDirectory, relativePath, lastIsDirectory: false);

    /// <summary>
    /// 기준 폴더 아래의 하위 폴더를 대소문자 무시로 찾는다. 경로 구분자는 \ 와 / 모두 허용한다.
    /// </summary>
    /// <param name="baseDirectory">기준 폴더</param>
    /// <param name="relativePath">상대 경로 (예: "d")</param>
    /// <returns>실제 폴더 경로, 없으면 null</returns>
    public static string? FindDirectory(string baseDirectory, string relativePath) =>
        FindEntry(baseDirectory, relativePath, lastIsDirectory: true);

    /// <summary>경로 조각을 하나씩 대소문자 무시로 따라가며 파일 또는 폴더를 찾는다</summary>
    /// <param name="baseDirectory">기준 폴더</param>
    /// <param name="relativePath">상대 경로</param>
    /// <param name="lastIsDirectory">마지막 조각이 폴더이면 true, 파일이면 false</param>
    private static string? FindEntry(string baseDirectory, string relativePath, bool lastIsDirectory)
    {
        string current = baseDirectory;
        string[] parts = relativePath.Split(['\\', '/'], StringSplitOptions.RemoveEmptyEntries);
        if (!Directory.Exists(current) || (parts.Length == 0 && !lastIsDirectory))
        {
            return null;
        }
        // 경로 조각마다 현재 폴더에서 대소문자 무시로 일치하는 항목을 찾는다
        for (int i = 0; i < parts.Length; i++)
        {
            bool last = i == parts.Length - 1 && !lastIsDirectory;
            string exact = Path.Combine(current, parts[i]);
            if (last ? File.Exists(exact) : Directory.Exists(exact))
            {
                current = exact;
                continue;
            }
            IEnumerable<string> candidates = last
                ? Directory.EnumerateFiles(current)
                : Directory.EnumerateDirectories(current);
            string? match = candidates.FirstOrDefault(
                c => string.Equals(Path.GetFileName(c), parts[i], StringComparison.OrdinalIgnoreCase));
            if (match == null)
            {
                return null;
            }
            current = match;
        }
        return current;
    }
}
