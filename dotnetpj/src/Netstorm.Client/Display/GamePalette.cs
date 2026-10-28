namespace Netstorm.Client.Display;

/// <summary>
/// 256색 게임 팔레트 파일의 판독기. 0x308바이트 COL(RGB 순서) 또는 0x400바이트 BGRX(Windows RGBQUAD 순서) 파일을 읽는다.
/// 원본 팔레트 로더는 COL 헤더 내용보다 파일 크기로 형식을 정한다.
/// 근거: 원본 004a4850 ↔ CD 00424760, cpppj/src/client/Screen.cpp 의 GamePalette. (2026-10-28 R1에서 이식)
/// </summary>
public sealed class GamePalette
{
    /// <summary>COL 파일의 크기(8바이트 머리 + 256색 × 3바이트 + 꼬리).</summary>
    private const int ColFileSize = 0x308;

    /// <summary>BGRX 파일의 크기(256색 × 4바이트).</summary>
    private const int BgrxFileSize = 0x400;

    /// <summary>원본 팔레트 번호 순서의 RGB 색 256개.</summary>
    private readonly PaletteColor[] _colors = new PaletteColor[256];

    /// <summary>BGRX 파일의 원본 예약 바이트. 색 거리나 표시 RGB에는 쓰지 않는다.</summary>
    private readonly byte[] _reserved = new byte[256];

    /// <summary>파일이 BGRX 형식이면 참. RGB COL은 예약 바이트를 쓰지 않는다.</summary>
    private readonly bool _bgrx;

    /// <summary>
    /// 팔레트 파일의 바이트를 읽는다. 크기가 0x308 또는 0x400이 아니면 <see cref="InvalidDataException"/>을 던진다.
    /// 사용: <c>new GamePalette(File.ReadAllBytes(path))</c>.
    /// </summary>
    /// <param name="bytes">팔레트 파일 전체.</param>
    public GamePalette(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length != ColFileSize && bytes.Length != BgrxFileSize) throw new InvalidDataException("Unsupported game palette size");
        _bgrx = bytes.Length == BgrxFileSize;
        // 256개의 원본 색 번호를 순서를 바꾸지 않고 옮긴다.
        for (int i = 0; i < _colors.Length; i++)
        {
            if (!_bgrx) _colors[i] = new PaletteColor(bytes[8 + i * 3], bytes[9 + i * 3], bytes[10 + i * 3]);
            else
            {
                _colors[i] = new PaletteColor(bytes[i * 4 + 2], bytes[i * 4 + 1], bytes[i * 4]);
                _reserved[i] = bytes[i * 4 + 3];
            }
        }
    }

    /// <summary>원본 팔레트 번호에 대응하는 RGB 색을 돌려준다. 팔레트 안에 투명 색을 강제로 지정하지 않는다.</summary>
    /// <param name="index">0~255의 팔레트 번호.</param>
    public PaletteColor Color(byte index) => _colors[index];

    /// <summary>
    /// BGRX 파일은 네 번째 바이트를 돌려준다. RGB COL은 예약 바이트를 읽지 않으므로 null을 돌려주어 기존 저장값을 보존하게 한다.
    /// </summary>
    /// <param name="index">0~255의 팔레트 번호.</param>
    public byte? Reserved(byte index) => _bgrx ? _reserved[index] : null;
}

/// <summary>
/// 논리 팔레트에서 가장 가까운 색을 찾는 원본 색 검색. 원본 004a2820 / CD 004246e0 에 해당한다.
/// 근거: cpppj/src/client/Screen.cpp 의 FindPaletteColor, fixture palettecolor-x86.tsv(786행). (2026-10-28 R1에서 이식)
/// </summary>
public static class PaletteSearch
{
    /// <summary>색 검색의 초기 거리 상한(00509ce0 / CD 004246e3 의 float32 0x4b189680). 모두 이 이상이면 0번을 돌려준다.</summary>
    private const float InitialColorDistance = 10000000.0f;

    /// <summary>
    /// 논리 팔레트(R·G·B·플래그 DWORD 256개)에서 RGB 제곱 거리가 가장 작은 색 번호를 찾는다. 동률은 먼저 나온 번호를 유지한다.
    /// 좌표 차·곱·합은 32비트로 감고, 비교는 부호 있는 거리와 float32 최솟값으로 한다. 실제 x87은 새 정수 거리를 반올림하기 전에
    /// 이전 float32 최솟값과 비교하므로 거리를 먼저 float로 바꾸지 않는다(큰 거리의 동률 처리가 달라진다).
    /// </summary>
    /// <param name="logical">리틀 엔디언 DWORD(빨강·초록·파랑·플래그) 256개.</param>
    /// <param name="red">찾을 빨강.</param>
    /// <param name="green">찾을 초록.</param>
    /// <param name="blue">찾을 파랑.</param>
    public static uint FindPaletteColor(ReadOnlySpan<uint> logical, int red, int green, int blue)
    {
        if (logical.Length != 256) throw new ArgumentException("논리 팔레트는 256색이어야 한다.", nameof(logical));
        float best = InitialColorDistance;
        uint chosen = 0;
        // 256색을 번호 순으로 검색한다. 플래그 바이트는 색 거리에 쓰지 않는다.
        for (int i = 0; i < logical.Length; i++)
        {
            uint color = logical[i];
            uint dr = unchecked((color & 255U) - (uint)red);
            uint dg = unchecked(((color >> 8) & 255U) - (uint)green);
            uint db = unchecked(((color >> 16) & 255U) - (uint)blue);
            int distance = unchecked((int)(dr * dr + dg * dg + db * db));
            if ((double)distance < (double)best)
            {
                best = (float)distance;
                chosen = (uint)i;
            }
        }
        return chosen;
    }
}
