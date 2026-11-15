// World의 현재 실행기 대여를 먼저 반납하고 마지막에 장기 물리 대여를 해제한다.
namespace Netstorm.Core.Original;

/// <summary>같은 Storage를 다음 World가 빌릴 수 있게 명시 일반 종료를 연결한다.</summary>
public sealed partial class RawSurfaceWorld
{
    /// <summary>정상 종료를 중복 실행하지 않는 관리 수명 관문이다. 게임 전역으로 쓰지 않는다.</summary>
    private bool _disposed;

    /// <summary>현재 동기 실행이 끝난 일반 종료에서 자기 프로세스와 짧은 장치를 먼저 반납하고 물리 lease를 끝낸다.</summary>
    /// <remarks>State 반납으로 남긴 이전 Regular는 ProcessHost 저장소의 기존 계약대로 유지한다.</remarks>
    public void Dispose()
    {
        if (_disposed) return;
        BindRuntimePin(null);
        DrainCurrentProcessBindingsForShutdown();
        _processUse?.Dispose(); _processUse = null;
        if (_factoryRuntime is { } runtime) { _factory!.ReleaseRuntime(runtime); _factoryRuntime = null; }
        _postPop?.ReleaseRuntimeBindings();
        _storage.ReleaseContainedUnpop(CurrentContainedUnpop());
        ReleaseStorageDisplay();
        _storageUse?.Dispose(); _disposed = true; GC.SuppressFinalize(this);
    }
}
