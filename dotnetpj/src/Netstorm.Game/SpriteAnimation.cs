using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;

namespace Netstorm.Game;

/// <summary>
/// 셰이프 블록의 연속된 프레임을 텍스처로 만들어 순환 재생하는 개발용 애니메이션.
/// 각 프레임은 기준점(원본 핫스팟)을 기준으로 그린다.
/// </summary>
internal sealed class SpriteAnimation : IDisposable
{
    private readonly List<(Texture2D Texture, Point Offset)> _frames = [];
    private readonly double _secondsPerFrame;
    private double _elapsed;
    private int _current;

    /// <summary>표시용 이름</summary>
    public string Name { get; }

    /// <summary>셰이프 블록의 프레임 범위로 애니메이션을 만든다</summary>
    /// <param name="device">텍스처를 만들 그래픽 장치</param>
    /// <param name="shapes">셰이프 데이터베이스</param>
    /// <param name="palette">적용할 팔레트</param>
    /// <param name="typeName">타입 이름 (예: "sunCannon")</param>
    /// <param name="firstFrame">첫 프레임 번호</param>
    /// <param name="frameCount">프레임 수</param>
    /// <param name="framesPerSecond">초당 프레임 수</param>
    public SpriteAnimation(GraphicsDevice device, ShapeDatabase shapes, Palette palette,
        string typeName, int firstFrame, int frameCount, double framesPerSecond)
    {
        Name = typeName;
        _secondsPerFrame = 1.0 / framesPerSecond;
        ShapeBlock block = shapes.FindBlock(typeName)
            ?? throw new ArgumentException($"셰이프 블록이 없는 타입: {typeName}", nameof(typeName));
        int end = Math.Min(firstFrame + frameCount, block.Frames.Count);
        // 지정한 범위의 프레임을 텍스처로 변환한다 (특수 레코드는 건너뜀)
        for (int i = firstFrame; i < end; i++)
        {
            ShapeFrame frame = block.Frames[i];
            if (frame.IsSpecial)
            {
                continue;
            }
            _frames.Add((ToTexture(device, shapes.Decode(frame), palette), new Point(frame.XMin, frame.YMin)));
        }
    }

    /// <summary>경과 시간에 따라 현재 프레임을 넘긴다</summary>
    /// <param name="deltaSeconds">지난 프레임 이후 경과 시간(초)</param>
    public void Update(double deltaSeconds)
    {
        if (_frames.Count == 0)
        {
            return;
        }
        _elapsed += deltaSeconds;
        // 누적 시간이 프레임 길이를 넘을 때마다 다음 프레임으로
        while (_elapsed >= _secondsPerFrame)
        {
            _elapsed -= _secondsPerFrame;
            _current = (_current + 1) % _frames.Count;
        }
    }

    /// <summary>현재 프레임을 기준점 위치에 그린다</summary>
    /// <param name="batch">스프라이트 배치 (Begin 호출된 상태)</param>
    /// <param name="anchor">기준점을 둘 화면 위치</param>
    /// <param name="scale">확대 배율</param>
    public void Draw(SpriteBatch batch, Vector2 anchor, float scale)
    {
        if (_frames.Count == 0)
        {
            return;
        }
        (Texture2D texture, Point offset) = _frames[_current];
        batch.Draw(texture, anchor + offset.ToVector2() * scale, null, Color.White, 0f, Vector2.Zero, scale,
            SpriteEffects.None, 0f);
    }

    /// <summary>8비트 인덱스 이미지를 팔레트로 칠해 RGBA 텍스처로 만든다 (투명 픽셀은 알파 0)</summary>
    /// <param name="device">그래픽 장치</param>
    /// <param name="image">디코딩된 이미지</param>
    /// <param name="palette">팔레트</param>
    /// <param name="remap">선택적인 256색 인덱스 변환표. 비어 있으면 원본 팔레트를 사용한다.</param>
    internal static Texture2D ToTexture(GraphicsDevice device, IndexedImage image, Palette palette, ReadOnlyMemory<byte> remap = default)
    {
        var pixels = new Color[image.Width * image.Height];
        ReadOnlySpan<byte> table = remap.Span;
        // 픽셀마다 팔레트 색을 적용한다
        for (int i = 0; i < pixels.Length; i++)
        {
            if (image.Opaque[i])
            {
                byte index = image.Indices[i];
                Rgb c = palette[table.IsEmpty ? index : table[index]];
                pixels[i] = new Color(c.R, c.G, c.B);
            }
        }
        var texture = new Texture2D(device, image.Width, image.Height);
        texture.SetData(pixels);
        return texture;
    }

    /// <summary>텍스처를 해제한다</summary>
    public void Dispose()
    {
        // 모든 프레임 텍스처 해제
        foreach ((Texture2D texture, _) in _frames)
        {
            texture.Dispose();
        }
        _frames.Clear();
    }
}
