using Netstorm.Client.Player;
using Netstorm.Core.Original;

// Client 전용 기록 타입을 Core에 노출하지 않고 같은 장기 저장소의 실제 고유 Pre에 연결한다.
namespace Netstorm.Client;

/// <summary>RawSurfaceRegistryDeletion의 실제 약한 기록 owner 결합을 보유한다.</summary>
public sealed partial class ClientSurfaceGlobals
{
    /// <summary>최초 결합한 실제 기록 owner만 약하게 보유하며 현재 배열을 저장하지 않는다.</summary>
    private WeakReference<ClientPlayerRecords>? _deletionPlayerRecords;

    /// <summary>Native/대여 전 실제 기록 owner를 한 번 연결한다. 배열 조회나 초기화를 실행하지 않는다.</summary>
    /// <remarks>cpppj/src/client/RawSurfaceRegistryDeletion.cpp의 BindPlayerRecords/PlayerPreDestroy다.</remarks>
    public void BindPlayerRecords(ClientPlayerRecords records)
    {
        ArgumentNullException.ThrowIfNull(records);
        if (_deletionPlayerRecords is not null) throw new ArgumentException("장기 Player 삭제의 최초 실제 기록 owner 계약", nameof(records));
        var current = new WeakReference<ClientPlayerRecords>(records);
        RawSurfaceStorage storage = Storage;
        storage.BindPlayerPre((sid, flags) => RawPlayerWorld.PreDestroyNative(storage.Pool, sid, flags, new()
        {
            Current = () => ReadDeletionPlayerRecords(current),
            Base = (target, value) => storage.CommonDeletion().PreDestroy(target, value),
        }));
        _deletionPlayerRecords = current;
    }

    /// <summary>그 필드 접근 동안만 실제 기록 owner를 pin하고 그때의 base를 읽는다.</summary>
    private static PlayerRecordAccess ReadDeletionPlayerRecords(WeakReference<ClientPlayerRecords> source)
    {
        if (!source.TryGetTarget(out ClientPlayerRecords? current)) throw new InvalidOperationException("장기 Player 삭제의 실제 현재 기록 owner 미연결");
        return new(current, current.Records);
    }
}
