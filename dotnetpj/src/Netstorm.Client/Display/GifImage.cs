namespace Netstorm.Client.Display; // 파일RGB 표 대신 실제 팔레트 색 번호를 보존하는 GIF 장치 경계다.

/// <summary>cpppj/src/client/GifImage.cpp의 일반/배경 판독기.일반 LZW/확장/인터레이스는 재대조한 기존 Assets 판독기를 공유한다.</summary>
public static class GifImage
{
    /// <summary>일반GIF의 논리화면·확장/투명·LZW/인터레이스를 읽고 같은 원본0/255 opacity로 연결한다.</summary>
    public static IndexedImage DecodeGif(ReadOnlySpan<byte> bytes)
    {
        var input = Netstorm.Assets.GifImage.Decode(bytes); byte[] opacity = new byte[input.Opaque.Length];
        // CLR bool 저장소를 원본의 실제0/255 BYTE 값으로만 변환한다.
        for (int i = 0; i < opacity.Length; i++) opacity[i] = input.Opaque[i] ? (byte)255 : (byte)0; return new() { Width = input.Width, Height = input.Height, Indices = input.Indices, Opacity = opacity };
    }
    /// <summary>원문407171은 전역표 바로 뒤의 첫descriptor를 요구하고 논리크기/오프셋을 무시한다.보호 파일은 수정하지 않는다.</summary>
    public static IndexedImage DecodeBackgroundGif(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length < 13) throw new InvalidDataException("BackgroundGif 머리 잘림"); int descriptor = 13 + ((bytes[10] & 0x80) != 0 ? 3 << ((bytes[10] & 7) + 1) : 0); if (descriptor > bytes.Length || bytes.Length - descriptor < 10 || bytes[descriptor] != 0x2c) throw new InvalidDataException("BackgroundGif 첫descriptor 필요"); byte[] normalized = bytes.ToArray(); bytes.Slice(descriptor + 5, 4).CopyTo(normalized.AsSpan(6)); normalized.AsSpan(descriptor + 1, 4).Clear(); return DecodeGif(normalized);
    }
}
