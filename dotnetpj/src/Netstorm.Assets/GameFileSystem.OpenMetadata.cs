using System.Text;

namespace Netstorm.Assets; // 등록 snapshot의 출처만 실제 FILE 열기 장치에 제공한다.

/// <summary>Source41a340의 파일명/부분 위치·길이/암호 flags 값이다.</summary>
/// <param name="FileName">등록한 실제 파일명이다.</param><param name="Offset">시작 DWORD의 signed 비트다.</param><param name="Length">길이 DWORD의 signed 비트다.</param><param name="Encrypted">등록 flags의 bit1 값이다.</param>
public sealed record GameArchiveOpenMetadata(string FileName, int Offset, int Length, uint Encrypted);

/// <summary>현재 등록 순서/목록을 같은 소유자에서 빌리는 실제 열기 메타데이터다.</summary>
public sealed partial class GameFileSystem
{
    /// <summary>Source AsciiLower/구분자/처음./한번·선행slash 규칙이다. UTF-8 BYTE를 Unicode 대소문자로 바꾸지 않는다.</summary>
    private static string RawArchiveKey(ReadOnlySpan<byte> value)
    {
        int end = value.IndexOf((byte)0); if (end >= 0) value = value[..end]; byte[] bytes = value.ToArray();
        // 원문 ASCII 대문자와 backslash만 각 BYTE에서 변환한다.
        for (int index = 0; index < bytes.Length; index++) { if (bytes[index] is >= (byte)'A' and <= (byte)'Z') bytes[index] += 32; else if (bytes[index] == (byte)'\\') bytes[index] = (byte)'/'; }
        string text = Encoding.Latin1.GetString(bytes); if (text.StartsWith("./", StringComparison.Ordinal)) text = text[2..]; return text.TrimStart('/');
    }
    /// <summary>선택 목록의 첫 엔트리만 찾고 내용/현재 파일 존재/경로 고리는 조회하지 않는다.</summary>
    public GameArchiveOpenMetadata? FindArchiveOpen(ReadOnlyMemory<byte> name, bool secondary = false)
    {
        string key = RawArchiveKey(name.Span); int start = secondary ? _primaryArchiveCount : 0, end = secondary ? Archives.Count : _primaryArchiveCount;
        // 같은 등록 순서와 중복 엔트리의 첫 일치를 보존한다.
        for (int index = start; index < end; index++)
        {
            TaffArchive archive = Archives[index];
            foreach (TaffEntry entry in archive.Entries) // 기존 snapshot dictionary의 마지막 중복 선택과 구별한다.
            {
                if (RawArchiveKey(Encoding.Latin1.GetBytes(entry.Name)) != key) continue;
                string file = archive.SourcePath ?? throw new InvalidOperationException("실제 파일명이 없는 메모리 TAFF의 FILE 열기 출처 미연결");
                return new(file, unchecked((int)(archive.DataOffset + (uint)entry.Offset)), entry.Size, archive.Flags & 2u);
            }
        }
        return null;
    }
}
