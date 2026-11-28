using System.Text;

// 실제 곡 이름·상위 옵션·파일 존재 조회를 같은 음악 제어 소유자에 연결한다.
namespace Netstorm.Client.Audio;

/// <summary>원문 주/보조 음악 디렉터리다. null은 이전 조회 문자열을 보존하고 빈 문자열은 역슬래시부터 다시 시작한다.</summary>
/// <param name="Primary">주 디렉터리 포인터의 현재 문자열 또는 null이다.</param>
/// <param name="Secondary">보조 디렉터리 포인터의 현재 문자열 또는 null이다.</param>
public readonly record struct MusicDirectories(string? Primary = null, string? Secondary = null);

/// <summary>선택됐을 때 요구하는 실제 음악 채널의 함수 경계다. 채널 상태나 성공을 합성하지 않는다.</summary>
public sealed class MusicPlaybackDevices
{
    /// <summary>실제 현재 채널의 활성 비트를 조회한다.</summary>
    public Func<bool>? Active { get; init; }
    /// <summary>같은 디렉터리와 파일 조회 장치로 실제 채널을 연다. RIFF/파일/버퍼 수명은 호출 대상이 소유해야 한다.</summary>
    public Func<string, MusicDirectories, Func<string, string?>, bool>? Open { get; init; }
    /// <summary>같은 실제 채널의 버퍼를 확보한다.</summary>
    public Func<bool>? EnsureBuffer { get; init; }
    /// <summary>같은 실제 채널을 원문 파일 반복값0/1로 시작한다.</summary>
    public Func<uint, bool>? Start { get; init; }
    /// <summary>같은 실제 채널의 버퍼와 열린 파일을 원문 정지 순서로 반납한다.</summary>
    public Action? Stop { get; init; }
}

/// <summary>원문 ASCII 비교와 좁은 BYTE 이름의 C문자열 경계를 공유한다.</summary>
internal static class MusicByteText
{
    /// <summary>입력에 첫NUL이 있으면 그 앞만 이름으로 사용한다. Unicode를 임의 물음표로 치환하지 않는다.</summary>
    internal static string CString(string text)
    {
        int end = text.IndexOf('\0');
        string name = end >= 0 ? text[..end] : text;
        Validate(name);
        return name;
    }
    /// <summary>모든 문자가 실제 한BYTE인지 확인한다.</summary>
    internal static void Validate(string text)
    {
        // 원문 좁은 문자열이 아닌 호스트 입력은 상태 쓰기 전에 거부한다.
        foreach (char value in text)
            if (value > byte.MaxValue) throw new ArgumentException("음악 이름의 실제 BYTE 범위 밖 문자", nameof(text));
    }
    /// <summary>원문 C locale처럼 ASCII A~Z만 접어 같은 길이의 BYTE 이름을 비교한다.</summary>
    internal static bool Same(string first, string second)
    {
        if (first.Length != second.Length) return false;
        // 각 현재 BYTE의 ASCII 대소문자만 무시한다.
        for (int index = 0; index < first.Length; index++)
        {
            char left = first[index], right = second[index];
            if (left is >= 'A' and <= 'Z') left = (char)(left + 32);
            if (right is >= 'A' and <= 'Z') right = (char)(right + 32);
            if (left != right) return false;
        }
        return true;
    }
}

/// <summary>cpppj/src/client/SoundMusicSelection.cpp의 같은256BYTE 현재 곡 이름과 signed 음악 옵션이다.</summary>
public sealed class MusicSelectionState
{
    /// <summary>새 Client에서만 빈 이름으로 시작하는 실제256BYTE 저장소다.</summary>
    private readonly byte[] _name = new byte[256];
    /// <summary>원문 음악 옵션의 초기1이다. 0만 비활성이며 fallback이 매번 현재값을 읽는다.</summary>
    public int Enabled { get; set; } = 1;
    /// <summary>동일 실제 저장소를 읽고 쓰며 별도 이름 사본을 만들지 않는다.</summary>
    public Memory<byte> Raw() => _name;
    /// <summary>첫NUL 앞의 현재 좁은 이름을 읽고 종료 없는256BYTE는 명시 오류로 거부한다.</summary>
    public string Current()
    {
        int end = Array.IndexOf(_name, (byte)0);
        if (end < 0) throw new ArgumentException("현재 음악 이름의 실제256BYTE NUL 없음");
        return Encoding.Latin1.GetString(_name, 0, end);
    }
    /// <summary>원문 Assign처럼 첫255BYTE만 복사·0패딩하고 마지막BYTE는 보존한다.</summary>
    public void Assign(string name)
    {
        ArgumentNullException.ThrowIfNull(name);
        MusicByteText.Validate(name);
        int length = Math.Min(name.Length, _name.Length - 1);
        // 원문 string_view의 BYTE를 같은 주소 접두부에 복사한다.
        for (int index = 0; index < length; index++) _name[index] = (byte)name[index];
        _name.AsSpan(length, _name.Length - 1 - length).Clear();
    }
}

/// <summary>원문 FindMusicFile의 두 조회 문자열 작성기다. 실제 존재 여부는 공급된 파일 장치가 판정한다.</summary>
public static class MusicFileLookup
{
    /// <summary>각 경로가 있으면 재설정하고 없으면 이전 조회에 이름을 다시 붙여 첫 실제 파일을 찾는다.</summary>
    public static string? FindMusicFile(string name, MusicDirectories directories, Func<string, string?> find)
    {
        ArgumentNullException.ThrowIfNull(name);
        ArgumentNullException.ThrowIfNull(find);
        string query = "";
        // 같은 원문256BYTE 조회 버퍼의 현재 문자열을 작성하고 범위를 검사한다.
        void Append(string? directory)
        {
            if (directory is not null) query = directory + '\\';
            query += name;
            MusicByteText.Validate(query);
            if (query.Length > 255 || query.Contains('\0')) throw new ArgumentException("음악 조회의 원문256BYTE 경계 밖 문자열");
        }
        Append(directories.Primary);
        string? found = find(query);
        if (found is not null) return found;
        Append(directories.Secondary);
        return find(query);
    }
}

/// <summary>원문 두 Select 부모다. 실제 채널/파일 열기/worker는 미이식이며 선택된 장치 누락은 제어 본체가 해당 위치에서 거부한다.</summary>
public sealed class MusicSelection
{
    /// <summary>같은 현재256BYTE 이름과 음악 옵션 소유자다.</summary>
    private readonly MusicSelectionState _state;
    /// <summary>동일 SoundState·Stopping·명령 잠금의 실제 음악 제어다.</summary>
    private readonly SoundMusicControl _music;
    /// <summary>상위 존재 조회와 실제 공개 Play에 같은 값으로 전달할 주/보조 경로다.</summary>
    private readonly MusicDirectories _directories;
    /// <summary>실제 파일 조회 장치이며 찾지 못한 경우에만 null을 반환한다.</summary>
    private readonly Func<string, string?> _find;
    /// <summary>아직 없는 채널은 null이며 자연 미준비 관문 뒤 실제로 선택되면 오류가 난다.</summary>
    private readonly MusicPlaybackDevices? _devices;

    /// <summary>실제 상위 선택 소유자와 파일 조회를 결합하고 채널/파일/버퍼를 만들거나 성공으로 대체하지 않는다.</summary>
    public MusicSelection(MusicSelectionState state, SoundMusicControl music, MusicDirectories directories,
        Func<string, string?> find, MusicPlaybackDevices? devices = null)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(music);
        ArgumentNullException.ThrowIfNull(find);
        _state = state; _music = music; _directories = directories; _find = find; _devices = devices;
    }
    /// <summary>상위 옵션과 fallback이 같은 현재 signed 음악 옵션을 다시 읽는다.</summary>
    public void Select(string? name) => Select(name, _state.Enabled);
    /// <summary>새 파일 존재 확인 뒤 이전 곡 정지·현재 이름 저장·특수곡 반복 선택을 원문 순서로 실행한다.</summary>
    public void Select(string? name, int enabled)
    {
        if (name is null) throw new ArgumentException("선택할 음악 이름이 null이다", nameof(name));
        string selected = MusicByteText.CString(name), current = _state.Current();
        if (!MusicByteText.Same(selected, current))
        {
            if (selected.Length != 0 && current.Length != 0 && MusicFileLookup.FindMusicFile(selected, _directories, _find) is not null)
                _music.Stop(_devices);
            _state.Assign(selected);
        }
        if (enabled != 0)
            _ = _music.Play(selected, MusicByteText.Same(selected, "fanfare.mus") || MusicByteText.Same(selected, "defeat.mus") ? 0u : 1u,
                _directories, _find, _devices, Select);
        else _music.Stop(_devices);
    }
}
