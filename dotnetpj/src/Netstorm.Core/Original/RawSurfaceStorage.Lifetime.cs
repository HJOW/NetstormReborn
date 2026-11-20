// 같은 장기 물리의 일반 초기화와 준비 대여·반납을 분리한다.
namespace Netstorm.Core.Original;

/// <summary>World보다 오래 사는 Storage의 실제 배열 주소와 단일 대여를 관리한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>동시 설치를 거부한 뒤에만 이전 물리를 초기화한다. TYPE·삭제 기록과 배열 주소는 유지한다. 일반 설치만 기존 새 목록 의미를 유지하며 prepared·반납은 현재 SID·count·용량을 보존한다. 2026-11-07 F2d 저장소 풀 생성에서 추가됐다.</summary>
    /// <returns>반납 경계다.</returns>
    public Use Acquire()
    {
        if (_borrowed)
        {
            throw new InvalidOperationException("실제 월드 저장소 중복 대여");
        }

        var use = new Use(this);
        GameAux().ResetForGeneralWorld();
        BoardTargetState.Invalid = new() { X = -10000.0f, Y = -10000.0f };
        _authority.Server = true;
        _authority.Boss = true;
        if (_generation > 0ul)
        {
            Pool.Reset(0);
            Hash.Reset();
            Array.Clear(Spots);
        }

        Array.Clear(PriestOwnerState);
        // 일반 fresh 호환 초기화도 장기 실행기가 빌린 두 실제 배열의 주소를 유지한다.
        Array.Clear(Graph.Records);
        Array.Clear(Graph.Stack);
        RawGraphStorage.ResetTable(Graph);
        SidLists.PrepareGeneralWorld();
        ResetSharedStatesForGeneralWorld();
        Array.Clear(TotalMade.LocalCounts);
        Array.Clear(TotalMade.LocalSecondaryCounts);
        Array.Clear(TotalMade.GlobalCounts);
        Array.Clear(Islands.Records);
        RawIslandRuntime.ResetRawIslandRecords(Islands.Records);
        Chunks.Mode = 1;
        Chunks.Reset();
        _borrowed = true;
        ++_generation;
        return use;
    }

    /// <summary>앞선 명시 정리가 끝난 실제 배열을 초기화 없이 대여하며 동시 사용은 같은 관문에서 거부한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <returns>반납 경계다.</returns>
    public Use AcquirePrepared()
    {
        if (_borrowed)
        {
            throw new InvalidOperationException("준비된 실제 월드 저장소 중복 대여");
        }

        var use = new Use(this);
        _borrowed = true;
        ++_generation;
        return use;
    }

    /// <summary>대여 중에는 두 번째 월드가 같은 풀을 초기화하지 못하게 한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public sealed class Use : IDisposable
    {
        /// <summary>빌린 저장소다. 반납 뒤에는 null이다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
        private RawSurfaceStorage? _storage;

        /// <summary>이미 검증한 단일 소유자의 반납 경계다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
        /// <param name="storage">빌린 저장소다.</param>
        internal Use(RawSurfaceStorage storage)
        {
            _storage = storage;
        }

        /// <summary>현재 살아 있는 같은 Surface 대여만 외곽 Runtime에 인계하도록 정체성을 검사한다.</summary>
        public bool Uses(RawSurfaceStorage storage) => ReferenceEquals(_storage, storage);

        /// <summary>월드의 모든 모듈을 반납한 뒤 대여만 해제한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
        public void Dispose()
        {
            if (_storage is not null)
            {
                _storage._borrowed = false;
                _storage = null;
            }
        }
    }
}
