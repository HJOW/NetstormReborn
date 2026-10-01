namespace Netstorm.AnalyzeManager;

/// <summary>Windows 도구와 리눅스용 YouTube 전용 빌드가 함께 쓰는 명령줄 처리.</summary>
public static class CommandLine
{
    /// <summary>옵션 중복과 누락을 거부하며 소비한 인자만 제거한다.</summary>
    public static string? TakeOption(List<string> arguments, string name)
    {
        int index = arguments.IndexOf(name);
        if (index < 0) return null;
        if (index + 1 >= arguments.Count || arguments.LastIndexOf(name) != index)
            throw new ArgumentException($"옵션 값 누락 또는 중복: {name}");
        string value = arguments[index + 1];
        arguments.RemoveRange(index, 2);
        return value;
    }

    /// <summary>명시 경로나 현재 작업/실행 경로의 부모에서 원본이 있는 저장소를 찾는다.</summary>
    public static string FindRepository(string? explicitPath)
    {
        if (explicitPath != null)
        {
            string path = Path.GetFullPath(explicitPath);
            if (!File.Exists(Path.Combine(path, "originals", "Netstorm.exe"))) throw new ArgumentException("저장소에 originals/Netstorm.exe가 없습니다.");
            return path;
        }
        // MCP 클라이언트의 작업 경로가 달라도 빌드 출력의 부모로 저장소를 찾는다.
        foreach (string start in new[] { Environment.CurrentDirectory, AppContext.BaseDirectory })
        {
            DirectoryInfo? directory = new(start);
            // 루트까지 올라가되 파일을 바꾸지 않고 존재 여부만 검사한다.
            while (directory != null)
            {
                if (File.Exists(Path.Combine(directory.FullName, "AGENTS.md")) && File.Exists(Path.Combine(directory.FullName, "originals", "Netstorm.exe"))) return directory.FullName;
                directory = directory.Parent;
            }
        }
        throw new ArgumentException("--repo로 NetstormReborn 저장소를 지정하세요.");
    }
}
