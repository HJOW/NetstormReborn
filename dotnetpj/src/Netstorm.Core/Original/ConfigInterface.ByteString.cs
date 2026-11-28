using Netstorm.Assets;

// 실제 고정 BYTE 목적지를 사용하는 설정 소비자를 기존 Config 저장소에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>전체 Config 파일 적재와 구별되는00441300의 BYTE 비교·복사 경계다.</summary>
public sealed partial class ConfigInterface
{
    /// <summary>00441300의 고정 BYTE 목적지를 첫 불일치까지 비교한 뒤 NUL까지 전방 복사한다. 누락 값과 실패 전 쓰기는 보존한다.</summary>
    public bool ReadStringBytes(string key, Span<byte> value)
    {
        string? text = _configuration.Get(key);
        if (text is null) return false;
        byte[] bytes = OriginalText.EncodeWindows1252(text);
        // 목적지 전체 길이를 미리 요구하지 않고 실제 strcmp가 읽는 첫 불일치/NUL까지만 검사한다.
        for (int index = 0; ; index++)
        {
            byte incoming = index < bytes.Length ? bytes[index] : (byte)0;
            if (index >= value.Length) throw new ArgumentOutOfRangeException(nameof(value), "Config 문자열 비교의 실제 목적 물리 부족");
            if (value[index] != incoming) break;
            if (incoming == 0) return false;
        }
        // 첫 불일치 뒤 같은 strcpy 순서로 쓰며 짧은 목적지에서 앞선 부분을 되돌리지 않는다.
        for (int index = 0; ; index++)
        {
            byte incoming = index < bytes.Length ? bytes[index] : (byte)0;
            if (index >= value.Length) throw new ArgumentOutOfRangeException(nameof(value), "Config 문자열 복사의 실제 목적 물리 부족");
            value[index] = incoming;
            if (incoming == 0) return true;
        }
    }
}
