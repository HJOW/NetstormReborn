using System.Buffers.Binary;
using Netstorm.Core.Original;

// 현재 CS와 구역의 실제 객체만 원문 순서로 반납한다.
namespace Netstorm.Client;

/// <summary>cpppj/src/client/NativeMetaDisplay.cpp의00479560/004795a0 정리 본체다. 표시 갱신이나 CS 전체 초기화는 수행하지 않는다.</summary>
public static class NativeMetaDisplayCleanup
{
    /// <summary>실제51개 구역에서 양수 상태·비영 SID의 마커만 삭제하고 상태/좌표는 보존한다.</summary>
    public static void ClearZones(Memory<byte> records, Action<uint, uint>? destroy)
    {
        // 원문 포인터 순서대로 각 기록의 현재 상태와 SID를 다시 읽는다.
        for (uint index = 0; index < RawMetaZoneCleanup.Count; index++)
            RawMetaZoneCleanup.ClearMarker(records, index, destroy);
    }

    /// <summary>구역→참가자0..39의 SID/배열→전투0..9의 SID 순서다. 삭제 실패의 앞선 효과는 유지하며 콜백 뒤 현재 CS를 재조회한다.</summary>
    public static void Leave(ClientChallengeGlobals challenge, Memory<byte> zones, Action<uint, uint>? destroy)
    {
        ArgumentNullException.ThrowIfNull(challenge);
        ClearZones(zones, destroy);
        // SID0인 참가자는 배열도 보존한다. 비영이면 삭제 복귀 뒤 현재 기록에만 쓴다.
        for (uint index = 0; index < RawChallengeState.ParticipantCount; index++)
        {
            uint sid = BinaryPrimitives.ReadUInt32LittleEndian(challenge.Participant(index).Span.Slice(0xb8, 4));
            if (sid == 0) continue;
            (destroy ?? throw new InvalidOperationException("메타 이탈의 실제 가상 삭제 미연결"))(sid, 0);
            BinaryPrimitives.WriteUInt32LittleEndian(challenge.Participant(index).Span.Slice(0xb8, 4), 0);
            challenge.ClearArray(challenge.Participant(index).Slice(0xbc, 16));
        }
        // 전투0도 건너뛰지 않으며 삭제 뒤 그때의 같은 슬롯 SID만 지운다.
        for (uint index = 0; index < RawChallengeState.BattleCount; index++)
        {
            uint sid = BinaryPrimitives.ReadUInt32LittleEndian(challenge.Battle(index).Span.Slice(0x20, 4));
            if (sid == 0) continue;
            (destroy ?? throw new InvalidOperationException("메타 이탈의 실제 가상 삭제 미연결"))(sid, 0);
            BinaryPrimitives.WriteUInt32LittleEndian(challenge.Battle(index).Span.Slice(0x20, 4), 0);
        }
    }

    /// <summary>같은 Client/구역/Pool을 검증하고 각 선택 삭제 동안만 현재 전체 Runtime을 pin한다. 비영 SID가 없으면 Session을 요구하지 않는다.</summary>
    public static void LeaveCurrent(ClientChallengeGlobals challenge, ClientSurfaceGlobals surface, ClientLocalStateGlobals local)
    {
        ArgumentNullException.ThrowIfNull(challenge); ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(local);
        if (!ReferenceEquals(surface.Gameplay, local.Gameplay) || !ReferenceEquals(surface.Clock.LocalState, local))
            throw new ArgumentException("메타 이탈의 같은 실제 Client/로컬 상태 필요", nameof(local));
        Leave(challenge, local.MetaZoneRecords, (sid, flags) =>
        {
            var game = surface.Gameplay;
            var session = game.RequireSession();
            if (!ReferenceEquals(session.Pool, surface.Storage.Pool) || !ReferenceEquals(session.LocalState, local))
                throw new InvalidOperationException("메타 이탈의 현재 Session/Pool/Client 불일치");
            if (sid > ushort.MaxValue || sid >= (uint)session.Pool.Capacity)
                throw new ArgumentOutOfRangeException(nameof(sid), "메타 이탈의 실제 DWORD SID 범위");
            using var pin = game.PinCurrentRuntime(session);
            session.DestroyWorldObject((int)sid, flags);
        });
    }
}
