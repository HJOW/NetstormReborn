using System.Text;

// 원문 바이트 경로 작성기를 파일 장치와 저장 소비자가 같은 수명으로 사용한다.
namespace Netstorm.Assets;

/// <summary>원문459c60의 한 부품이다. Kind는 -1루트/-2디렉터리/-3파일/-4확장자/0종료이며 Text는 현재 BYTE 뷰다.</summary>
/// <param name="Kind">부품 종류다.</param><param name="Text">처리 직전에 읽는 NUL 종료 바이트다. 같은 출력 고리의 별칭도 허용한다.</param>
public readonly record struct FileNamePart(int Kind, ReadOnlyMemory<byte> Text);

/// <summary>cpppj o/RawFileName의 10×256BYTE 출력 수명과 Format(459c60) 본체다. OS 경로 정규화를 하지 않는다.</summary>
public sealed class RawFileName
{
    /// <summary>열 번째 후속 호출에서 같은 물리가 덮이는 최초 BSS0 출력 고리다.</summary>
    private readonly byte[][] _buffers = Enumerable.Range(0, 10).Select(_ => new byte[256]).ToArray();
    /// <summary>현재 칸 번호다. 첫 작성 전에 0이며 호출마다 선증가한다.</summary>
    public int Index { get; private set; }
    /// <summary>NUL 뒤 보존 바이트까지 포함하는 현재 실제256BYTE다. 조회는 고리를 전진시키지 않는다.</summary>
    public ReadOnlyMemory<byte> CurrentBytes => _buffers[Index];

    /// <summary>각 부품을 원문 순서로 작성하고 같은 물리의 첫 NUL 앞 뷰를 반환한다. 실패 시 현재 칸/부분 출력은 보존한다.</summary>
    public ReadOnlyMemory<byte> Format(ReadOnlySpan<FileNamePart> parts, byte separator = (byte)'\\')
    {
        Index = (Index + 1) % 10; byte[] output = _buffers[Index]; Array.Clear(output); int size = 0, previous = 0; bool firstDirectory = true;
        // append는 NUL 포함 전체 폭을 먼저 검사하여 길이 실패 시 기존 prefix를 보존한다.
        void Append(string text)
        {
            if (size + text.Length >= output.Length) throw new ArgumentOutOfRangeException(nameof(parts), "원문 경로 작성기의256BYTE 물리 범위 초과");
            Encoding.Latin1.GetBytes(text, output.AsSpan(size)); size += text.Length; output[size] = 0;
        }
        // 빈 출력에서 직전 칸의 끝BYTE도0인 원문 경계를 보존한다.
        char Last() => size == 0 ? '\0' : (char)output[size - 1];
        // 후행 slash는 길이1을 보존하며 마지막 한 문자만 제거한다.
        string StripTrailing(string text) => text.Length > 1 && text[^1] == separator ? text[..^1] : text;
        // 종료0 뒤 부품은 읽지 않고 각 현재 뷰는 선택된 시점에만 복사한다.
        foreach (FileNamePart part in parts)
        {
            if (part.Kind == 0) break;
            ReadOnlySpan<byte> raw = part.Text.Span; int nul = raw.IndexOf((byte)0); if (nul >= 0) raw = raw[..nul];
            if (raw.Length >= 256) throw new ArgumentOutOfRangeException(nameof(parts), "원문 경로 작성기의 지역256BYTE 입력 범위 초과");
            string text = Encoding.Latin1.GetString(raw).Replace('/', (char)separator).Replace('\\', (char)separator);
            int translatedNul = text.IndexOf('\0'); if (translatedNul >= 0) text = text[..translatedNul];
            switch (part.Kind)
            {
                case -1:
                    if (previous != 0) throw new ArgumentException("원문 경로 루트는 첫 부품이어야 함", nameof(parts));
                    text = StripTrailing(text); Append(text);
                    if (text.Length != 0 && text[0] != separator && text[^1] != ':') Append(":");
                    break;
                case -2:
                    if (previous is -3 or -4) throw new ArgumentException("원문 파일/확장자 뒤 디렉터리 부품", nameof(parts));
                    if (!firstDirectory) text = text.TrimStart((char)separator); firstDirectory = false; text = StripTrailing(text);
                    if (previous == 0)
                    {
                        if (!text.Contains((char)separator) && !text.Contains(':')) Append(".\\");
                    }
                    else if (size != 0 && Last() != separator && (text.Length == 0 || text[0] != separator)) Append("\\");
                    Append(text); if (Last() != separator) Append("\\");
                    break;
                case -3:
                    if (previous is -3 or -4) throw new ArgumentException("원문 파일/확장자 뒤 파일 부품", nameof(parts));
                    text = StripTrailing(text.TrimStart((char)separator));
                    if (previous == 0) { if (!text.Contains((char)separator)) Append(".\\"); }
                    else if (Last() != separator) Append("\\");
                    Append(text); break;
                case -4:
                    if (previous is 0 or -1 or -2) throw new ArgumentException("원문 파일 없는 확장자 부품", nameof(parts));
                    text = text.TrimStart('.');
                    // 확장자 후행 dot 루프는 감소하므로 마지막 한 문자가 남을 때까지 모두 제거한다.
                    while (text.Length > 1 && text[^1] == '.') text = text[..^1];
                    if (Last() != '.') Append("."); Append(text); break;
                default: throw new ArgumentException("원문 경로 작성기의 알 수 없는 부품 종류", nameof(parts));
            }
            previous = part.Kind;
        }
        return output.AsMemory(0, size);
    }
}
