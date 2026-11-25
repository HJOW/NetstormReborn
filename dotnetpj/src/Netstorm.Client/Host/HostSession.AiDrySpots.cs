using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// AI 부족 좌표 스캔·통지를 같은 실제 본체/기록 저장소에 연결한다.
public sealed partial class HostSession
{
    /// <summary>같은 실제 본체/기록 저장소를 빌리는 부족 좌표 본체다.</summary>
    private RawAiDrySpots? _aiDrySpots;

    /// <summary>조립된 부족 좌표 본체를 빌려준다. 조립 전에는 명시 오류다.</summary>
    public RawAiDrySpots AiDrySpots => _aiDrySpots ?? throw new InvalidOperationException("AI 부족 좌표 미연결");

    /// <summary>에너지 승인과 로그 장치를 부족 좌표 하위에 연결한다.</summary>
    /// <remarks>2026-11-25 R4-5b 부족 좌표 호출자용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 2899~2900행의 dry 갈고리 조립과 2937~2938행의 frameHooks다. 에너지 승인(004734d0)은 별도 공급 장치(R6)가 필요해 명시 거부한다.</remarks>
    private void BuildAiDrySpotsDevice()
    {
        if (_aiDrySpots is not null)
        {
            throw new InvalidOperationException("AI 부족 좌표 장치 중복 연결");
        }

        _aiDrySpots = new RawAiDrySpots(AiBrainMemory, Pool, new AiDrySpotHooks
        {
            Energy = (_, _, _, _, _) => throw new InvalidOperationException("AI 부족 좌표 에너지 승인 미연결(R6): 004734d0"),
            Log = (brain, fake) => LogAiDrySpotNote(brain, Gameplay.AiDebug.Word(0x52f3dc), fake),
        });
    }

    /// <summary>현재 brain의 0x260 소유자를 읽어 Killed [Fake] Dry Spot 로그를 남긴다. 장치·런타임 통지가 같은 문구를 공유한다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="kind">기록할 로그 종류다.</param>
    /// <param name="fake">가짜 좌표면 참이다.</param>
    internal void LogAiDrySpotNote(uint token, uint kind, bool fake)
    {
        uint owner = BinaryPrimitives.ReadUInt32LittleEndian(AiBrainMemory.Bytes(token).AsSpan(0x260, 4));
        _aiRuntimeLogs.Add(kind + ":AI" + unchecked((int)owner) + ": " + (fake ? "Killed Fake Dry Spot" : "Killed Dry Spot"));
    }

    /// <summary>현재 owner1..8의 실제 brain에서 부족 좌표 스캔을 수행한다. brain이 없으면 0이다.</summary>
    /// <remarks>근거: cpppj RawCampaignSession.cpp 4908행의 RecheckAiDrySpots다. 매 프레임 호출은 R4-5e 틱 결선에서 잇는다.</remarks>
    public uint RecheckAiDrySpots(uint owner)
    {
        if (owner < 1 || owner > 8)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "AI 부족 좌표 소유자는 1..8이어야 합니다.");
        }

        if (!AiBrains.TryGetValue(owner, out uint brain))
        {
            return 0;
        }

        return AiDrySpots.Scan(brain);
    }
}
