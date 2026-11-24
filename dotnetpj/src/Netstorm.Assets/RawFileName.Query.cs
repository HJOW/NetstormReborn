namespace Netstorm.Assets; // 같은 기존 경로 고리에 나머지 조회 본체를 연결한다.

/// <summary>cpppj/src/o/RawFileName.cpp/.h의459b20 이름·459990 디렉터리·459890 전체 조회 본체다.</summary>
public sealed partial class RawFileName
{
    /// <summary>원문처럼 전체 입력을 고리에 복사한 뒤 마지막 구분자/colon 다음 내부 뷰를 빌린다. 구분자가 없으면 빈 뷰를 둔다. 0.983에서 추가됐다.</summary>
    /// <param name="fileSpec">조회할 원문 바이트 경로다. 첫 NUL 앞에서 자른다.</param>
    /// <param name="extension">거짓이면 끝 확장자를 459550 규칙으로 제거한다.</param>
    /// <param name="separator">구분자로 취급하는 현재 BYTE다.</param>
    /// <returns>같은 고리 칸의 내부 뷰이며 별도 복사를 만들지 않는다.</returns>
    public ReadOnlyMemory<byte> Name(ReadOnlyMemory<byte> fileSpec, bool extension = true, byte separator = (byte)'\\')
    {
        Index = (Index + 1) % 10; byte[] output = _buffers[Index]; Array.Clear(output);
        ReadOnlySpan<byte> input = fileSpec.Span; int nul = input.IndexOf((byte)0); if (nul >= 0) input = input[..nul];
        if (input.Length >= output.Length) throw new ArgumentOutOfRangeException(nameof(fileSpec), "원문 FileSpec 이름 조회의256BYTE 물리 범위 초과");
        input.CopyTo(output); int end = input.Length, offset = end; bool found = false;
        // 원문 마지막 구분자/colon을 뒤에서 찾아 그 다음 주소를 빌린다.
        while (offset > 0) { offset--; if (output[offset] == separator || output[offset] == (byte)':') { offset++; found = true; break; } }
        if (!found) { output[0] = 0; return output.AsMemory(0, 0); }
        int size = end - offset;
        if (!extension && size > 1)
        {
            // 459550은 선두 dot을 확장자로 지우지 않으며 구분자/colon을 만나면 중단한다.
            for (int at = end - 1; at > offset; at--)
            {
                if (output[at] == (byte)'.') { output[at] = 0; size = at - offset; break; }
                if (output[at] == separator || output[at] == (byte)':') break;
            }
        }

        return output.AsMemory(offset, size);
    }

    /// <summary>459990의 전체(-1)/단계별 디렉터리와 부분 버퍼를 같은 고리에 반환한다. 파일 시스템의 parent_path와 다른 dot/UNC/단계 경계를 유지한다. 0.983에서 추가됐다.</summary>
    /// <param name="fileSpec">조회할 원문 바이트 경로다. 첫 NUL 앞에서 자른다.</param>
    /// <param name="level">돌아볼 단계 수다. -1이면 첫 suffix에서 바로 반환한다.</param>
    /// <param name="separator">구분자로 취급하는 현재 BYTE다.</param>
    /// <returns>같은 고리 칸의 현재 첫NUL까지 뷰이며 뒤에 남은 이전 복사 바이트는 지우지 않는다.</returns>
    public ReadOnlyMemory<byte> Directory(ReadOnlyMemory<byte> fileSpec, int level = -1, byte separator = (byte)'\\')
    {
        Index = (Index + 1) % 10; byte[] output = _buffers[Index]; Array.Clear(output);
        ReadOnlySpan<byte> input = fileSpec.Span; int nul = input.IndexOf((byte)0); if (nul >= 0) input = input[..nul];
        if (input.Length >= output.Length) throw new ArgumentOutOfRangeException(nameof(fileSpec), "원문 FileSpec 디렉터리 조회의256BYTE 물리 범위 초과");
        byte[] query = input.ToArray();
        // 현재 첫NUL까지 길이를 잰다.
        int OutputLength() { int end = Array.IndexOf(output, (byte)0); return end < 0 ? output.Length : end; }
        // 원문 strcpy처럼 이번 끝NUL까지만 덮어쓴다.
        void CopySuffix(int from) { ReadOnlySpan<byte> suffix = query.AsSpan(from); suffix.CopyTo(output); output[suffix.Length] = 0; }
        int cursor = query.Length >= 2 && query[0] == separator && query[1] == separator ? 2 : 0;
        // -1은 첫 suffix에서 반환하고 나머지는 명시 단계만 순회한다.
        for (long step = 0; step <= level || level == -1; step++)
        {
            int relative = query.AsSpan(cursor).IndexOf(separator);
            if (relative < 0) throw new ArgumentException("원문 디렉터리 조회의 NULL 구분자 읽기", nameof(fileSpec));
            int cut = cursor + relative;
            if (cursor < cut && query[cut - 1] == (byte)'.') cut--;
            if (level == -1)
            {
                CopySuffix(cut); int size = OutputLength();
                // 원문 역방향 탐색은 offset0의 구분자를 검사하지 않는다.
                for (int at = size != 0 ? size - 1 : 0; at > 0; at--) { if (output[at] == separator) { output[at + 1] = 0; break; } }
                return output.AsMemory(0, OutputLength());
            }

            cursor = cut + 1;
            if (query.AsSpan(cursor).IndexOf(separator) < 0) { if (step == 0 && level < 1) { output[0] = separator; output[1] = 0; } else output[0] = 0; return output.AsMemory(0, OutputLength()); }
            CopySuffix(cut);
            int forward = output.AsSpan(1, OutputLength() - 1).IndexOf(separator);
            if (forward < 0) throw new ArgumentOutOfRangeException(nameof(fileSpec), "원문 디렉터리 조회의 실제 구분자 부족");
            output[1 + forward] = 0;
        }

        int total = OutputLength();
        if (total == 0 || output[total - 1] != separator)
        {
            if (total + 1 >= output.Length) throw new ArgumentOutOfRangeException(nameof(fileSpec), "원문 디렉터리 연결의256BYTE 물리 범위 초과");
            output[total] = (byte)'\\'; output[total + 1] = 0;
        }

        return output.AsMemory(0, OutputLength());
    }

    /// <summary>459890의 전체 또는 무조건+2주소부터 첫NUL까지 복사한다. .\\인지 검사는 하지 않는다. 0.983에서 추가됐다.</summary>
    /// <param name="fileSpec">조회할 원문 바이트 경로다. prefix가 거짓이면 앞 2BYTE를 건너뛴다.</param>
    /// <param name="prefix">참이면 전체를, 거짓이면 +2주소부터 복사한다.</param>
    /// <returns>같은 고리 칸에 복사한 현재 뷰이다.</returns>
    public ReadOnlyMemory<byte> Full(ReadOnlyMemory<byte> fileSpec, bool prefix = true)
    {
        Index = (Index + 1) % 10; byte[] output = _buffers[Index]; Array.Clear(output);
        ReadOnlySpan<byte> input = fileSpec.Span;
        if (!prefix) { if (input.Length < 2) throw new ArgumentException("원문Full의+2물리입력범위없음", nameof(fileSpec)); input = input[2..]; }
        int nul = input.IndexOf((byte)0); if (nul >= 0) input = input[..nul];
        if (input.Length >= output.Length) throw new ArgumentOutOfRangeException(nameof(fileSpec), "원문Full의256BYTE물리범위초과");
        input.CopyTo(output); return output.AsMemory(0, input.Length);
    }
}
