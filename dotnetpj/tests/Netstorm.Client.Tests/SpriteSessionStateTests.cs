using Netstorm.Client.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 공통 색표 소유자와 일반 객체 표시 상태의 결합을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>원본 표시 전역 초기값과 세션의 현재 값을 검증한다.</summary>
public sealed class SpriteSessionStateTests
{
    /// <summary>배너 설정과 진단이 바뀌면 현재 공통 소유자 표를 다시 읽는다.</summary>
    [Fact]
    public void KeepsLiveSharedOwnersAndCurrentFrameState()
    {
        var input = new ClientColorSortInput();
        var owners = input.OwnerTables;
        uint diagnostic = 6;
        Assert.Throws<InvalidOperationException>(() => SpriteSessionState.Create(owners, () => diagnostic));
        input.CaptureWithPrimary(new uint[256], new byte[0x3800]);
        owners.Initialize(input.Primary, 188, 82, new UIntBox());
        SpriteDrawState state = SpriteSessionState.Create(owners, () => diagnostic);
        Assert.Equal(158U, state.ShadowAliasType);
        Assert.Equal(new uint[] { 138, 144, 73 }, state.TimerTypes);
        Assert.Equal(1U, state.SharedUseBanners!());
        Assert.Equal(0U, state.SharedOwnerMapsCurrent!().Span[94]);
        owners.SetEnabled(0, new UIntBox());
        Assert.Equal(0U, state.SharedUseBanners());
        Assert.NotEqual(0U, state.SharedOwnerMapsCurrent().Span[94]);
        diagnostic = 0;
        Assert.Equal(0U, state.SharedPatchDiagnostic!());
        var colors = new PaletteColorTable();
        colors.Basic[0] = 8; colors.Basic[1] = 9; colors.Basic[5] = 10;
        // 원본 명명 색 열 번째 항목의 색 번호를 포함한다.
        for (int i = 0; i < 11; i++) colors.Named.Add(new(0, 0, 0, (uint)i));
        int[] palettes = [0, 3];
        SpriteSessionState.Refresh(state, 10, 20, 1, 2, true, palettes, colors, 17.5);
        Assert.Equal((10, 20, 1U, 2U, true), (state.CameraX, state.CameraY, state.LocalPlayer, state.SpecialMode, state.MarkerVisible));
        Assert.Same(palettes, state.OwnerPalettes);
        Assert.Equal(new uint[] { 10, 10, 9 }, state.HitPointColors);
        Assert.Equal(8U, state.BarBackground);
        Assert.Equal(17.5, state.Now);
    }
}
