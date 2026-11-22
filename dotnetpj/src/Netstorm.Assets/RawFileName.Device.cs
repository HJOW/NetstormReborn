namespace Netstorm.Assets; // 같은 기존 경로 고리에 Source 장치 getter를 연결한다.

/// <summary>cpppj/src/o/RawFileName.cpp/.h의459900 장치 조회 본체다.</summary>
public sealed partial class RawFileName
{
    /// <summary>같은 고리를 선행 clear한 뒤 전체 입력과 NUL 뒤 잔여를 보존한다.</summary>
    public ReadOnlyMemory<byte> Device(ReadOnlyMemory<byte> fileSpec, byte separator = (byte)'\\')
    {
        Index = (Index + 1) % 10; byte[] output = _buffers[Index]; Array.Clear(output);
        int end = fileSpec.Span.IndexOf((byte)0); if (end < 0) end = fileSpec.Length;
        if (end >= output.Length) throw new ArgumentOutOfRangeException(nameof(fileSpec), "원문 FileSpec 장치 조회의256BYTE 물리 범위 초과");
        var input = fileSpec.Span[..end]; input.CopyTo(output); int size = end;
        if (size >= 2 && input[0] == separator && input[1] == separator)
        {
            int serverEnd = input[2..].IndexOf(separator); if (serverEnd >= 0) { size = serverEnd + 2; output[size] = 0; }
        }
        else { int colon = input.IndexOf((byte)':'); size = colon < 0 ? 0 : colon + 1; output[size] = 0; }
        return output.AsMemory(0, size);
    }
}
