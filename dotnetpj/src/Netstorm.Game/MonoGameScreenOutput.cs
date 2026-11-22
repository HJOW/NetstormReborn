using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Client.Display;

namespace Netstorm.Client.Host;

/// <summary>
/// 논리 화면의 갱신 사각형을 RGBA 텍스처에 올리는 <see cref="IScreenOutput"/> 구현.
/// 원본 GDI BitBlt 자리에 인덱스를 현재 팔레트로 풀어 <see cref="ScreenTextureConverter"/>로 바꾼 뒤 부분 업로드한다.
/// 근거: cpppj/src/client/Screen.cpp 의 004a1800 창 복사. (R1-2a, 2026-10-28)
/// </summary>
public sealed class MonoGameScreenOutput : IScreenOutput
{
    /// <summary>논리 화면 전체를 담는 RGBA 텍스처.</summary>
    private readonly Texture2D _texture;

    /// <summary>
    /// 논리 화면 크기의 텍스처로 만든다. 화면 크기가 바뀌면 호스트가 출력도 다시 만든다.
    /// </summary>
    /// <param name="device">그래픽 장치.</param>
    /// <param name="screen">논리 화면.</param>
    public MonoGameScreenOutput(GraphicsDevice device, Screen screen)
    {
        ArgumentNullException.ThrowIfNull(device);
        ArgumentNullException.ThrowIfNull(screen);
        _texture = new Texture2D(device, screen.Width, screen.Height, false, SurfaceFormat.Color);
    }

    /// <summary>그리기에 쓰는 텍스처.</summary>
    public Texture2D Texture => _texture;

    /// <summary>
    /// 갱신 사각형만 RGBA로 바꿔 텍스처에 올린다. 게임 루프와 같은 스레드에서만 부른다.
    /// </summary>
    /// <param name="screen">갱신된 화면.</param>
    /// <param name="rect">화면 안으로 잘린 갱신 사각형(비어 있지 않다).</param>
    public void Present(Screen screen, ScreenRect rect)
    {
        ArgumentNullException.ThrowIfNull(screen);
        if (screen.Width != _texture.Width || screen.Height != _texture.Height)
        {
            throw new ArgumentException("호스트 출력 텍스처와 논리 화면 크기가 다릅니다", nameof(screen));
        }
        byte[] rgba = ScreenTextureConverter.ConvertToRgba(screen, rect);
        var area = new Rectangle(rect.Left, rect.Top, rect.Right - rect.Left, rect.Bottom - rect.Top);
        _texture.SetData(0, area, rgba, 0, rgba.Length);
    }

    /// <summary>팔레트 적용 뒤 수직 동기를 기다린다. MonoGame이 Present 때 맞추므로 비워 둔다.</summary>
    /// <param name="screen">대상 화면.</param>
    public void WaitVerticalBlank(Screen screen)
    {
    }
}
