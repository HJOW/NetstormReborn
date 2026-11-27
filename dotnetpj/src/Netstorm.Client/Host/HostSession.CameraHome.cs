// 미션 입장 카메라를 내 사제·월드 가운데로 옮긴다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// 실제 지도 영역의 저장·Commit·Present·Pan을 같은 세션 카메라에서 실행한다.
public sealed partial class HostSession
{
    /// <summary>미니맵의 월드 좌표를 현재 카메라 목표로 요청하고 실제 적용은 프레임에 맡긴다.</summary>
    /// <param name="x">미니맵에서 선택한 월드 X 좌표다.</param>
    /// <param name="y">미니맵에서 선택한 월드 Y 좌표다.</param>
    /// <param name="screenWidth">현재 논리 화면의 가로 크기다.</param>
    /// <param name="screenHeight">현재 논리 화면의 세로 크기다.</param>
    public void RequestMinimapCenter(float x, float y, int screenWidth, int screenHeight)
    {
        Gameplay.Camera.RequestCenter(x, y, screenWidth, screenHeight, Gameplay.MapRevision);
    }

    /// <summary>미션 입장 카메라를 내 사제·월드 가운데로 옮긴다. 목표만 쓰고 즉시 뒤집기는 매 프레임에 맡긴다.</summary>
    /// <param name="state">현재 모드·소유자·화면·갱신 전역이다.</param>
    /// <param name="types">미션 타입 표다.</param>
    /// <param name="screenWidth">중심 이동 때 읽는 실제 화면 가로 크기다.</param>
    /// <param name="screenHeight">중심 이동 때 읽는 실제 화면 세로 크기다.</param>
    /// <remarks>2026-11-26 1.076에서 R5-4c용으로 추가했다. 근거: cpppj ClientRendererFrame.cpp의 ResetCamera가 세션 카메라를 고향으로 옮긴 뒤 메뉴에서 CommitCameraNow를 부르는 결선이다. DN 제품은 매 프레임 Commit하므로 목표 쓰기까지만 수행한다. 입장 Reset 경로에서는 Carrier preview에 닿지 않아 억제 깊이를 독립 카운터로 둔다.</remarks>
    public void ResetMissionCamera(CameraHomeState state, IReadOnlyList<OriginalTypeRecord> types, int screenWidth, int screenHeight)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(types);
        uint suppression = 0;
        var home = new RawCameraHome(Pool, types, state, (x, y) =>
        {
            CameraWorldRedraw.Value = unchecked(CameraWorldRedraw.Value + 1);
            Gameplay.Camera.RequestCenter(x, y, screenWidth, screenHeight, Gameplay.MapRevision);
        }, new CameraHomeHooks
        {
            SuppressionDepth = () => suppression,
            SetSuppressionDepth = value => suppression = value,
        });
        home.Reset();
    }
}
