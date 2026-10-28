using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 색 표 fixture의 공통 판독기. 리틀 엔디언 DWORD와 바이트 hex를 읽는다.
/// </summary>
internal static class ColorMapFixture
{
    /// <summary>hex를 바이트 배열로 바꾼다.</summary>
    public static byte[] Bytes(string hex) => Convert.FromHexString(hex);

    /// <summary>리틀 엔디언 바이트에서 DWORD 256개를 읽는다.</summary>
    public static uint[] Dwords(byte[] raw)
    {
        if (raw.Length < 1024)
        {
            throw new ArgumentException("논리 팔레트는 1024바이트여야 합니다", nameof(raw));
        }
        var result = new uint[256];
        for (int i = 0; i < 256; i++)
        {
            result[i] = raw[i * 4] | ((uint)raw[i * 4 + 1] << 8) | ((uint)raw[i * 4 + 2] << 16) | ((uint)raw[i * 4 + 3] << 24);
        }
        return result;
    }
}
