using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>
/// 세션 사건과 화면 조작을 원본 효과음 이름으로 바꿔 모아 둔다. 실제 재생은 게임 본체의 <see cref="AudioPlayer"/> 가 한다.
/// 사건 ↔ 파일 대응은 exe 의 효과음 호출 위치로 정했다 (docs/exe/music.md 7절):
/// 다리 칸 제거 FUN_00422300 → bridgeFall.wav, 붕괴 구동 FUN_004227e0 → bridgeCrack.wav,
/// 건설 완료 FUN_00443e20 → buildDone.wav + 타입의 buildDoneSound(템플 templeComplete.wav · 워크샵 workshopComplete.wav),
/// 조각 회전 FUN_00446a70 → rotatePiece.wav, 조각 놓기(FUN_004473e0 등) → dropPiece.wav, 창 열기 → openGump.wav.
/// 원본은 위치에 따라 좌우·크기를 바꾸는 효과음이 있으나 클론은 아직 화면 위치를 반영하지 않는다.
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>건설 완료 공통 효과음</summary>
    private const string BuildDoneSound = "buildDone.wav";

    /// <summary>타입별 건설 완료 효과음 속성 이름 (.type buildDoneSound, exe Rifttype.cpp 0x870)</summary>
    private const string BuildDoneSoundProperty = "buildDoneSound";

    /// <summary>다리 칸이 무너질 때</summary>
    private const string BridgeFallSound = "bridgeFall.wav";

    /// <summary>다리 칸에 금이 갈 때</summary>
    private const string BridgeCrackSound = "bridgeCrack.wav";

    /// <summary>다리 조각을 놓을 때</summary>
    private const string DropPieceSound = "dropPiece.wav";

    /// <summary>다리 조각을 돌릴 때</summary>
    private const string RotatePieceSound = "rotatePiece.wav";

    /// <summary>창(gump)을 열 때</summary>
    private const string OpenGumpSound = "openGump.wav";

    /// <summary>아직 재생 장치로 넘기지 않은 효과음 이름</summary>
    private readonly List<string> _soundCues = [];

    /// <summary>쌓인 효과음 이름을 꺼내고 비운다. 한 프레임에 같은 소리가 여러 번(여러 칸이 함께 금 감 등)이면 한 번만 튼다.</summary>
    public IReadOnlyList<string> TakeSoundCues()
    {
        string[] cues = [.. _soundCues.Distinct(StringComparer.OrdinalIgnoreCase)];
        _soundCues.Clear();
        return cues;
    }

    /// <summary>효과음 하나를 요청한다</summary>
    /// <param name="name">원본 sound/ 파일 이름</param>
    private void QueueSound(string name) => _soundCues.Add(name);

    /// <summary>세션 사건에 대응하는 효과음을 요청한다</summary>
    /// <param name="sessionEvent">세션이 알린 사건</param>
    private void QueueEventSound(SessionEvent sessionEvent)
    {
        switch (sessionEvent.Kind)
        {
            case SessionEventKind.ShotFired:
                if (_session.Entity(sessionEvent.EntityId)?.Type.Definition.GetString("fireSound") is { Length: > 0 } fire)
                {
                    QueueSound(fire);
                }
                break;
            case SessionEventKind.EntityDestroyed:
                OnEntityDestroyed(sessionEvent.EntityId);
                break;
            case SessionEventKind.ShotBlocked:
                QueueSound("sunFenceImpact.wav");
                break;
            case SessionEventKind.BuildingCompleted:
                QueueSound(BuildDoneSound);
                if (_session.Entity(sessionEvent.EntityId)?.Type.Definition.GetString(BuildDoneSoundProperty) is { Length: > 0 } special)
                {
                    QueueSound(special);
                }
                break;
            case SessionEventKind.BridgePlaced:
                QueueSound(DropPieceSound);
                break;
            case SessionEventKind.BridgeCracked:
                QueueSound(BridgeCrackSound);
                break;
            case SessionEventKind.BridgeCollapsed:
                QueueSound(BridgeFallSound);
                break;
            case SessionEventKind.PriestCaptured:
                QueueSound("golemPickUp.wav");
                break;
            case SessionEventKind.SacrificeStarted:
            case SessionEventKind.SacrificeResumed:
                // 내 제단 의식이 시작되거나 사제가 돌아와 재개되면 희생 음악을 요청한다 (같은 곡이 재생 중이면 무시된다).
                // 2026-10-01 캠페인 1-2 녹화: 사제 복귀(18:28.2 priestMove3) 직후 18:28.8 에 thu22 를 끊고 sacrifice.mus 가 시작됐다.
                if (sessionEvent.Player == TestPlayer)
                {
                    _mySacrificeMusicRequested = true;
                }
                break;
            case SessionEventKind.SacrificeRune:
                QueueSacrificeRuneSound(sessionEvent.Text);
                break;
            case SessionEventKind.SacrificeRuneBurned:
                QueueSound("altarBurnCollapse.wav");
                break;
            case SessionEventKind.SacrificeCompleted:
                QueueSound("altarBurnCollapse.wav");
                QueueSound("itIsDone2.wav");
                break;
            case SessionEventKind.PriestSacrificed:
                QueueSound("priestSacrifice2.wav");
                break;
            case SessionEventKind.AltarConsumed:
                QueueSound("explodeSlot.wav");
                break;
            // 2026-10-02 녹음 판독(캠페인 1-1·1-2)으로 사건 시각과 맞춘 효과음
            case SessionEventKind.PriestSuspended:
                // 발판이 무너져 사제가 허공에서 기절할 때 (1-2 13:21.9·14:02.0)
                QueueSound("priestFall.wav");
                break;
            case SessionEventKind.PriestReleased:
                // 운반체 내려놓기·판매, 제단 판매로 포로가 풀릴 때 (1-2 11:06.7·11:34.2·14:41.2·16:49.3)
                QueueSound("priestFree.wav");
                break;
            case SessionEventKind.PriestBound:
                // 포로를 제단에 묶을 때 (1-2 15:16.3·17:36.5, 1-1 12:11.7)
                QueueSound("priestStruggleFade02-800.wav");
                break;
            case SessionEventKind.Salvaged:
                // 오브젝트를 판매(회수)할 때 해체 소리 (1-2 11:34.2 골렘·14:41.2·16:49.3 제단, exe FUN_0044b4b0)
                QueueSound("collapse.wav");
                break;
            case SessionEventKind.WorkshopUpgraded:
                // 워크샵 업그레이드 완료 (1-1 01:38.1, 1-2 01:44.4·08:39.2, exe FUN_004545e0)
                QueueSound("upgradeComplete.wav");
                break;
        }
    }

    /// <summary>의식에서 지키기 시작한 룬 이름을 원본 음성 파일에 연결한다.</summary>
    private void QueueSacrificeRuneSound(string rune) => QueueSound(rune.ToLowerInvariant() switch
    {
        "wind" => "forWind2.wav",
        "sun" => "forSun2.wav",
        "rain" => "forRain2.wav",
        "thunder" => "forThunder2.wav",
        "storm" => "forStorm2.wav",
        _ => "",
    });
}
