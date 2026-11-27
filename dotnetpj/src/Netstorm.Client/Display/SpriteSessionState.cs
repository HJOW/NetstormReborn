// 실제 세션이 일반 객체 출력에 공급할 원본 전역을 구성한다.
namespace Netstorm.Client.Display;

/// <summary>원본 표시 전역의 초기값과 현재 게임 상태를 같은 객체 표시 상태에 연결한다.</summary>
public static class SpriteSessionState
{
    /// <summary>실제 공통 소유자 표와 패치 진단을 빌려 10.78 표시 상태를 만든다.</summary>
    /// <param name="owners">Client의 실제 소유자 색표다.</param>
    /// <param name="diagnostic">현재 패치 진단 DWORD 조회다.</param>
    /// <returns>같은 공통 전역을 읽는 객체 표시 상태다.</returns>
    /// <remarks>cpppj RawCampaignFactoryDisplay 생성자의 실제 TYPE·선택색 초기값을 따른다.</remarks>
    public static SpriteDrawState Create(OwnerColorTableState owners, Func<uint> diagnostic)
    {
        ArgumentNullException.ThrowIfNull(owners);
        ArgumentNullException.ThrowIfNull(diagnostic);
        if (!owners.Ready) throw new InvalidOperationException("객체 표시 공통 소유자 색표 미준비");
        return new SpriteDrawState
        {
            FilteredFlags = 2421574144u,
            WideOwnerType = 164,
            UnfilteredType = 84,
            ShadowAliasType = 158,
            SharedPatchDiagnostic = diagnostic,
            MarkerExcludeType = 157,
            MarkerType = 96,
            StatusType = 118,
            AbstractStatusType = 117,
            TimerProcessType = 31,
            StatusProcessType = 57,
            CostType = 122,
            TimerTypes = [138, 144, 73],
            SelectionColors = [84, 84, 249, 184, 140, 150, 112, 156, 222],
            SharedOwnerMapsCurrent = () => owners.Tokens,
            SharedUseBanners = () => owners.Enabled,
        };
    }

    /// <summary>매 프레임 현재 카메라·플레이어·팔레트·시계를 같은 표시 상태에 반영한다.</summary>
    /// <param name="state">Create가 만든 현재 표시 상태다.</param>
    /// <param name="cameraX">현재 카메라 가로 픽셀이다.</param>
    /// <param name="cameraY">현재 카메라 세로 픽셀이다.</param>
    /// <param name="localPlayer">현재 로컬 플레이어 번호다.</param>
    /// <param name="specialMode">현재 특수 추상 표시 값이다.</param>
    /// <param name="battle">현재 전투 표시 여부다.</param>
    /// <param name="ownerPalettes">현재 플레이어별 색 번호 배열이다.</param>
    /// <param name="colors">현재 화면의 원본 명명 색상 표다.</param>
    /// <param name="now">현재 게임 시계다.</param>
    public static void Refresh(SpriteDrawState state, int cameraX, int cameraY, uint localPlayer, uint specialMode,
        bool battle, int[] ownerPalettes, PaletteColorTable colors, double now)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(ownerPalettes);
        ArgumentNullException.ThrowIfNull(colors);
        if (colors.Named.Count <= 10) throw new ArgumentException("객체 표시 명명 색상 미준비", nameof(colors));
        state.CameraX = cameraX;
        state.CameraY = cameraY;
        state.LocalPlayer = localPlayer;
        state.SpecialMode = specialMode;
        state.MarkerVisible = battle;
        state.OwnerPalettes = ownerPalettes;
        state.BarBackground = colors.Basic[0];
        state.HitPointColors = [colors.Named[10].Index, colors.Basic[5], colors.Basic[1]];
        state.Now = now;
    }
}
