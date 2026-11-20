using System.Runtime.Versioning;

// 기존 실제 연소 장치의 명시 뷰 입력도 같은 Native 카메라 저장 위치를 갱신한다.
namespace Netstorm.Client.Tests;

/// <summary>창 없는 실제 자산/음향/팔레트 장면에서 카메라 setter 연결을 검사한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>원문 camera-view의17/29와-17/-29 관찰에 있는 현재 DWORD들을 대조한다.</summary>
    [Theory, SupportedOSPlatform("windows")]
    [InlineData(17, 29, 0x3f880000u, 0x4028ba2fu, 0x41880000u, 0x41e80000u)]
    [InlineData(-17, -29, 0xbf880000u, 0xc028ba2fu, 0xc1880000u, 0xc1e80000u)]
    public void DisplayViewWritesAllCurrentCameraFields(int x, int y, uint worldX, uint worldY, uint pixelX, uint pixelY)
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); var game = scene.Session.Gameplay; scene.Visuals.SetView(x, y, new(0, 0, 1024, 768));
        Assert.Equal(x, game.ContentScreenOrigin[0]); Assert.Equal(y, game.CursorGlobals.Placement.CurrentOriginY());
        Assert.Equal(new[] { worldX, worldY }, game.Camera.Snapshot.WorldBits); Assert.Equal(new[] { pixelX, pixelY }, game.Camera.Snapshot.PixelFloatBits);
    }
}
