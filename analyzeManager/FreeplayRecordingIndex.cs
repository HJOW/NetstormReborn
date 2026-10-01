using System.Text.Json;

namespace Netstorm.AnalyzeManager;

/// <summary>자유 플레이 녹화의 분할 파일과 시각 보조 파일을 AI가 찾을 수 있게 기록한다.</summary>
public static class FreeplayRecordingIndex
{
    /// <summary>녹화 중단 때 닫힌 파일만 수집해 세션 색인을 원자적으로 갱신한다.</summary>
    public static string Write(SessionStore store, string sessionId)
    {
        string directory = store.FreeplayDirectory(sessionId);
        if (!Directory.Exists(directory)) throw new DirectoryNotFoundException(directory);
        string path = Path.Combine(directory, "recording-index.json");
        string temporary = path + ".tmp";
        SessionStore.RejectReparse(path);
        SessionStore.RejectReparse(temporary);
        var index = new
        {
            sessionId,
            updatedUtc = DateTimeOffset.UtcNow,
            videoFormat = "MJPEG AVI, 10 FPS, 소리·커서 없음",
            audioFormat = "별도 WAV, Windows 기본 출력 장치",
            inputFormat = "별도 UTF-8 JSONL, 키·마우스 조작, 게임/화면 좌표, UTC·sessionElapsedMs·recordingElapsedMs",
            videos = Parts(directory, "video-*.avi", ".frames.csv"),
            audios = Parts(directory, "audio-*.wav", ".start.txt"),
            inputs = Parts(directory, "input-*.jsonl", null),
        };
        SessionStore.WriteSmallFile(temporary, JsonSerializer.SerializeToUtf8Bytes(index, SessionStore.Json));
        File.Move(temporary, path, true);
        return path;
    }

    /// <summary>번호순 파일 목록과 각 파일의 크기·대응 시각 파일을 구성한다.</summary>
    private static RecordingPart[] Parts(string directory, string pattern, string? timingExtension)
    {
        return Directory.EnumerateFiles(directory, pattern)
            .OrderBy(file => file, StringComparer.Ordinal)
            .Select(file => Part(directory, file, timingExtension))
            .ToArray();
    }

    /// <summary>녹화 조각과 보조 파일의 크기를 확인하고 상대 파일명만 색인에 넣는다.</summary>
    private static RecordingPart Part(string directory, string file, string? timingExtension)
    {
        SessionStore.RejectReparse(file);
        long bytes = new FileInfo(file).Length;
        if (bytes >= SessionStore.FileLimitBytes) throw new InvalidDataException($"50 MB 이상 녹화 파일: {file}");
        string? timingFile = timingExtension == null ? null : Path.GetFileNameWithoutExtension(file) + timingExtension;
        if (timingFile != null)
        {
            string timingPath = Path.Combine(directory, timingFile);
            SessionStore.RejectReparse(timingPath);
            if (!File.Exists(timingPath)) throw new InvalidDataException($"녹화 시각 파일이 없습니다: {timingPath}");
            if (new FileInfo(timingPath).Length >= SessionStore.FileLimitBytes)
                throw new InvalidDataException($"50 MB 이상 녹화 시각 파일: {timingPath}");
        }
        return new RecordingPart(Path.GetFileName(file), bytes, timingFile);
    }

    /// <summary>각 녹화 파일의 상대 이름·크기와 대응하는 시각 파일 이름.</summary>
    private sealed record RecordingPart(string File, long Bytes, string? TimingFile);
}
