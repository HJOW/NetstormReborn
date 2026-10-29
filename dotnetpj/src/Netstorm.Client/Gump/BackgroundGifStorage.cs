using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // 실제GIF 적재/독립버퍼/원시복사에 쓰는 장치 소유 영역이다.

/// <summary>RawBackgroundGifControls.cpp의 실제 비트맵 소유/파일·픽셀 경계 부분.전체 노드/창 소유자는 상위 조립 대상이다.</summary>
public sealed class BackgroundGifStorage
{
    /// <summary>현재 실제원시그림의독립소유.</summary>
    private readonly Dictionary<uint, IndexedImage> _bitmaps = [];
    /// <summary>문자열/다른그림 토큰과구별하는 단조증가 별칭.</summary>
    private uint _nextBitmap = 0x72000000;
    /// <summary>실제파일읽기 경계.</summary>
    private readonly Func<string, byte[]> _read;
    /// <summary>보이는할당밖BYTE에서만요구하는 실제물리공급.</summary>
    private readonly Func<uint, long, byte>? _outside;
    /// <summary>같은 실제 파일 장치와 선택적물리경계를 빌린다.</summary>
    public BackgroundGifStorage(Func<string, byte[]> read, Func<uint, long, byte>? outside = null) { ArgumentNullException.ThrowIfNull(read); _read = read; _outside = outside; }
    /// <summary>현재실제GIF를읽고 descriptor폭/높이와독립소유 토큰을돌려준다.</summary>
    public uint Load(string path, ref int width, ref int height) { var image = GifImage.DecodeBackgroundGif(_read(path)); width = image.Width; height = image.Height; if (_nextBitmap == 0x72ffffff) throw new OverflowException("BackgroundGif 이미지 주소 별칭 소진"); uint token = _nextBitmap++; _bitmaps.Add(token, image); return token; }
    /// <summary>같은소유버퍼만정확히한번반납한다.0도잘못된반납이다.</summary>
    public void Release(uint token) { if (token == 0 || !_bitmaps.Remove(token)) throw new InvalidOperationException("BackgroundGif 같은 소유 메모리 반납 불일치"); }
    /// <summary>현재실제소유색번호/치수를빌린다.</summary>
    public IndexedImage Bitmap(uint token) => _bitmaps.TryGetValue(token, out var image) ? image : throw new ArgumentOutOfRangeException(nameof(token), "BackgroundGif 실제 원시 메모리 없음");
    /// <summary>게임값을바꾸지않는현재소유버퍼수.</summary>
    public int ImageCount => _bitmaps.Count;
    /// <summary>현재원시메모리를같은표면에복사한다.투명마스크/배율/색표를추가하지않는다.</summary>
    public void Copy(BackgroundGifCopy request, GumpPixelTarget pixels) { var image = Bitmap(request.Bitmap); Func<long, byte>? outside = _outside is null ? null : offset => _outside(request.Bitmap, offset); pixels.RawBitmap(image.Indices, request.Stride, (long)request.Source.Top * request.Stride + request.Source.Left, new(unchecked(request.Source.Right - request.Source.Left), unchecked(request.Source.Bottom - request.Source.Top)), request.Position, new(int.MinValue, int.MinValue, int.MaxValue, int.MaxValue), outside); }
    /// <summary>현재파일/소유/원시복사만연결하고다른기본/입력장치를보존한다.</summary>
    public void Bind(BackgroundGifGumpHooks hooks, GumpPixelTarget pixels) { ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(pixels); hooks.Load = Load; hooks.ReleaseBitmap = Release; hooks.Copy = request => Copy(request, pixels); }
}
