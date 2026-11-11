// 원본 지형 미리보기의 8비트 bitmap과 최초 문자 분류 캐시를 복원한다(cpppj o/RawTerrainBitmap.h/.cpp).
// 2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 이식했다.

namespace Netstorm.Core.Original;

/// <summary>
/// Native 반환 triple의 포인터를 소유 배열로 나타낸 지형 비트맵 이미지다
/// (cpppj <c>o/RawTerrainBitmap.h</c>의 <c>TerrainBitmapImage</c>).
/// </summary>
/// <param name="Width">원문 signed 출력 폭이다.</param>
/// <param name="Height">원문 signed 출력 높이다.</param>
/// <param name="Pixels">행 우선 8비트 원본 색 번호 배열이다.</param>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public readonly record struct TerrainBitmapImage(int Width, int Height, byte[] Pixels);

/// <summary>
/// 첫 요청의 녹색 분류 값은 팔레트/Session 교체 뒤에도 같은 Client 수명으로 유지한다
/// (cpppj <c>o/RawTerrainBitmap.h</c>의 <c>TerrainBitmapState</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class TerrainBitmapState
{
    /// <summary>005c8bb8의 최초 BSS 0 / 정상 초기화 1이다.</summary>
    public uint Initialized { get; set; }

    /// <summary>005c8ab0의 원문 255 BYTE clear 범위다. code 'x'만 최초 녹색 하위 BYTE로 채운다.</summary>
    public byte[] Classified { get; } = new byte[255];

    /// <summary>005411f0의 현재 TYPE이며 지면 5411dc와 별개다.</summary>
    public uint FringeType { get; set; } = 102u;
}

/// <summary>
/// 실제 SHP 0을 200×200 / 255 초기 버퍼에 그린 현재 텍스처다
/// (cpppj <c>o/RawTerrainBitmap.h</c>의 <c>TerrainBitmapTexture</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class TerrainBitmapTexture
{
    /// <summary>원본 Squid 헤더의 폭이며 반복 주기는 폭-1이다.</summary>
    public int Width { get; set; }

    /// <summary>원본 Squid 헤더의 높이이며 반복 주기는 높이-1이다.</summary>
    public int Height { get; set; }

    /// <summary>004a2b10의 200 stride 임시 표면과 같은 40,000바이트 픽셀 배열이다.</summary>
    public byte[] Pixels { get; } = new byte[40000];
}

/// <summary>
/// 타입/SHP/현재 팔레트 조회만 외부에서 공급하는 콜백 묶음이다
/// (cpppj <c>o/RawTerrainBitmap.h</c>의 <c>TerrainBitmapHooks</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class TerrainBitmapHooks
{
    /// <summary>최초 분류 때만 005acd20의 현재 색 번호를 읽는다.</summary>
    public Func<uint>? Green { get; set; }

    /// <summary>실제 현재 TYPE 0 / RLE 결과를 빌려온다.</summary>
    public Func<uint, TerrainBitmapTexture>? Texture { get; set; }
}

/// <summary>
/// 원문 004c1860의 수직 fringe → 지면 texture 순서와 로컬 난수 소비를 유지하는 비트맵 렌더러다
/// (cpppj <c>o/RawTerrainBitmap</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class RawTerrainBitmap
{
    /// <summary>Client 최초 분류 상태다.</summary>
    private readonly TerrainBitmapState _state;

    /// <summary>현재 지면 TYPE를 읽는 대리자다.</summary>
    private readonly Func<uint> _islandType;

    /// <summary>선택되는 실제 조회 장치만 소유한다.</summary>
    private readonly TerrainBitmapHooks _hooks;

    /// <summary>
    /// 상태와 지면 타입 읽기 대리자·훅으로 렌더러를 구성한다.
    /// </summary>
    /// <param name="state">분류 캐시 상태다.</param>
    /// <param name="islandType">현재 지면 타입 번호 조회 대리자다.</param>
    /// <param name="hooks">녹색·텍스처 조회 훅이다.</param>
    public RawTerrainBitmap(TerrainBitmapState state, Func<uint> islandType, TerrainBitmapHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(islandType);
        ArgumentNullException.ThrowIfNull(hooks);
        _state = state;
        _islandType = islandType;
        _hooks = hooks;
    }

    /// <summary>
    /// 고정 지면 타입 번호를 받는 편의 생성자다.
    /// </summary>
    /// <param name="state">분류 캐시 상태다.</param>
    /// <param name="islandType">지면 타입 번호다.</param>
    /// <param name="hooks">녹색·텍스처 조회 훅이다.</param>
    public RawTerrainBitmap(TerrainBitmapState state, uint islandType, TerrainBitmapHooks hooks)
        : this(state, () => islandType, hooks)
    {
    }

    /// <summary>
    /// 원본 X열 우선 문자를 행 우선 전체 픽셀로 바꾼다.
    /// </summary>
    /// <param name="codes">65,536바이트 문자 배열이다.</param>
    /// <param name="width">출력 폭이다.</param>
    /// <param name="height">출력 높이다.</param>
    /// <param name="random">로컬 난수다.</param>
    /// <returns>그려진 8비트 지형 비트맵이다.</returns>
    public TerrainBitmapImage Render(ReadOnlySpan<byte> codes, int width, int height, GameRandom random)
    {
        ArgumentNullException.ThrowIfNull(random);
        if (codes.Length != 65536 || width < 0 || height < 0 || width > 256 || height > 256)
        {
            throw new ArgumentOutOfRangeException(nameof(width), "지형 bitmap의 실제 문자 저장소 범위");
        }
        if (_state.Initialized == 0)
        {
            Array.Clear(_state.Classified, 0, _state.Classified.Length);
            if (_hooks.Green is null)
            {
                throw new InvalidOperationException("지형 bitmap 현재 녹색 조회 미연결");
            }
            _state.Classified['x'] = (byte)_hooks.Green();
            _state.Initialized = 1u;
        }
        if (_hooks.Texture is null)
        {
            throw new InvalidOperationException("지형 bitmap 실제 texture 조회 미연결");
        }
        TerrainBitmapTexture fringe = _hooks.Texture(_state.FringeType);
        ValidateTexture(fringe);
        byte[] pixels = new byte[width * height];

        // 원문은 각 X열의 Y를 아래에서 위로 진행한다.
        for (int x = 0; x < width; x++)
        {
            // 위 칸을 함께 읽으므로 실제 Y 0은 첫 fringe 순회에서 제외한다.
            for (int y = height - 1; y > 0; y--)
            {
                byte code = codes[x * 256 + y];
                if (code != (byte)'=')
                {
                    continue;
                }
                byte before = codes[x * 256 + y - 1];
                if (before == (byte)'x')
                {
                    int length = unchecked((int)random.Next(3u)) + 3;
                    // 첫 샘플은 texture의 1행이며 아래 끝은 출력 높이로 제한한다.
                    for (int step = 0; step < length && y + step < height; step++)
                    {
                        int sample = ((step + 1) % (fringe.Height - 1)) * 200 + x % (fringe.Width - 1);
                        pixels[(y + step) * width + x] = fringe.Pixels[sample];
                    }
                }
                else if (before != (byte)'=')
                {
                    pixels[y * width + x] = 120;
                }
            }
        }

        TerrainBitmapTexture ground = _hooks.Texture(_islandType());
        ValidateTexture(ground);

        // 원본 두 번째 순회는 출력 행을 먼저 진행한다.
        for (int y = 0; y < height; y++)
        {
            // 각 행의 X를 처음부터 다시 읽어 지면 텍스처를 채운다.
            for (int x = 0; x < width; x++)
            {
                byte code = codes[x * 256 + y];
                if (code >= 128)
                {
                    throw new ArgumentOutOfRangeException(nameof(codes), "지형 bitmap의 원문 signed 문자 분류 범위");
                }
                if (_state.Classified[code] != 0)
                {
                    pixels[y * width + x] = ground.Pixels[(y % (ground.Height - 1)) * 200 + x % (ground.Width - 1)];
                }
            }
        }

        return new TerrainBitmapImage(width, height, pixels);
    }

    /// <summary>비정상 텍스처의 원문 나눗셈/잘못된 주소를 정상 이미지로 가장하지 않는다.</summary>
    private static void ValidateTexture(TerrainBitmapTexture texture)
    {
        ArgumentNullException.ThrowIfNull(texture);
        if (texture.Width <= 1 || texture.Height <= 1 || texture.Width > 200 || texture.Height > 200 || texture.Pixels.Length < 40000)
        {
            throw new ArgumentOutOfRangeException(nameof(texture), "지형 bitmap의 실제 texture 헤더 범위");
        }
    }
}
