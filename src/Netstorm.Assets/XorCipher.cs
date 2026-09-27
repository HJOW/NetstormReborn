namespace Netstorm.Assets;

/// <summary>
/// 원본 게임이 TAFF 아카이브 엔트리와 설정 파일(.cfg)에 쓰는 고정 키 XOR 인코딩.
/// XOR 이므로 인코딩과 디코딩이 같은 연산이다. (docs/formats/taff.md)
/// </summary>
public static class XorCipher
{
    /// <summary>XOR 키. Netstorm.exe 안에서 "TAFF v%d.%d" 문자열 바로 뒤에 저장되어 있다.</summary>
    public static ReadOnlySpan<byte> Key => "mydoghasfleas"u8;

    /// <summary>
    /// 버퍼를 제자리에서 변환한다. 키 위치는 버퍼 시작 기준 0 부터 순환한다.
    /// </summary>
    /// <param name="data">변환할 데이터 (엔트리 또는 파일 전체)</param>
    public static void Apply(Span<byte> data)
    {
        ReadOnlySpan<byte> key = Key;
        // 각 바이트를 키의 (위치 % 키 길이) 번째 바이트와 XOR
        for (int i = 0; i < data.Length; i++)
        {
            data[i] ^= key[i % key.Length];
        }
    }
}
