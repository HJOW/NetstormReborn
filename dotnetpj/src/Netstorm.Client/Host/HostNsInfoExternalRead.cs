using Netstorm.Client.Display;

namespace Netstorm.Client.Host; // NSInfo가 공유하는 현재 자산과 화면 색상 전역을 제공한다.

/// <summary>원본 NSInfo 바깥 읽기의 자산 종류와 팔레트 주소를 현재 화면 값으로 해석한다.</summary>
public static class HostNsInfoExternalRead
{
    /// <summary>알려진 주소만 읽고 그 외 주소는 호출자가 다른 장치에서 찾도록 반환한다.</summary>
    /// <param name="address">원본 전역 주소다.</param>
    /// <param name="fortType">현재 fortGump의 실제 런타임 TYPE 번호다.</param>
    /// <param name="colors">현재 화면의 파일 팔레트 색상 표다.</param>
    /// <param name="value">주소에 해당하는 현재 값이다.</param>
    /// <returns>이 장치가 해당 주소를 처리하면 참이다.</returns>
    public static bool TryRead(uint address, uint fortType, PaletteColorTable colors, out uint value)
    {
        ArgumentNullException.ThrowIfNull(colors);
        switch (address)
        {
            case 0x5411c8u: value = fortType; return true;
            case 0x5bec3cu: value = colors.Basic[8]; return true;
            case 0x5bec2cu: value = colors.Basic[7]; return true;
            case 0x5b5e58u: value = colors.Basic[1]; return true;
            case 0x59afd8u: value = colors.Basic[5]; return true;
            case 0x5c78d0u: value = colors.Named[36].Index; return true;
        }
        if (address >= 0x5accfcu && address <= 0x5acd18u && (address & 3u) == 0)
        {
            value = colors.Named[42 + (int)((address - 0x5accfcu) / 4u)].Index;
            return true;
        }
        if (address >= 0x5c78b0u && address <= 0x5c78ccu && (address & 3u) == 0)
        {
            value = colors.Named[50 + (int)((address - 0x5c78b0u) / 4u)].Index;
            return true;
        }
        value = 0;
        return false;
    }
}
