// 프로세스의 공통 실행 인터페이스와 슬롯 순서 실행기를 공유한다.
namespace Netstorm.Core.Original;

// 커널 모듈을 담는 네임스페이스다. 2026-11-07 F2d 커널 이식에서 추가됐다.

// 커널이 프레임 실행과 수명 처리를 호출하는 프로세스의 공통 인터페이스다. 2026-11-07 F2d 커널 이식에서 추가됐다.
public class BaseProcess : IDisposable
{
    /// <summary>원본 기반 클래스의 실행 메서드는 빈 함수다. 파생 프로세스가 재정의한다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    public virtual void RunFrame()
    {
    }

    /// <summary>커널이 소유권을 끝낼 때 부르는 가상 소멸 지점이다. 원본 가상 소멸자 슬롯 0에 대응하며 커널이 직접 호출하므로 GC 타이밍에 의존하지 않는다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    public virtual void Dispose()
    {
    }
}

// 패치 1·39999번, CD 1·3999번의 슬롯 번호다. 2026-11-07 F2d 커널 이식에서 추가됐다.
public static class KernelShape
{
    /// <summary>패치판의 확장된 슬롯 끝 번호다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    public const uint ProcessCapacity = 39999u;

    /// <summary>CD판의 실제 커널 끝 번호다. 판본 비교 장면에서 생성자 인자로 사용한다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    public const uint CdProcessCapacity = 3999u;
}

// 선택된 원문 진단만 실행하며 미연결 장치는 해당 실패 분기에서 명시적으로 거부한다. 2026-11-07 F2d 커널 이식에서 추가됐다.
public sealed class KernelDiagnostics
{
    /// <summary>경로의 대소문자와 원문 단언 행 번호를 선택한다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    public OriginalEdition Edition { get; set; } = OriginalEdition.Patch1078;

    /// <summary>소진 시 객체 덤프 없이 현재 프로세스 요약만 쓴다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    public Action? Summary { get; set; }

    /// <summary>원문 오류 문구를 같은 장기 상세 저장소에 복사한다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    public Action<string>? Detail { get; set; }

    /// <summary>정상 복귀 또는 예외의 부분 효과를 호출자가 그대로 관찰한다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>현재 진단 묶음의 얕은 사본을 만든다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <returns>복사된 진단 묶음이다.</returns>
    public KernelDiagnostics Clone() => new()
    {
        Edition = Edition,
        Summary = Summary,
        Detail = Detail,
        Assertion = Assertion,
    };
}

// 원본 슬롯 포인터와 단일 소유권을 함께 유지하는 프로세스 실행기다. 2026-11-07 F2d 커널 이식에서 추가됐다.
public sealed class Kernel
{
    /// <summary>원본처럼 소멸자 실행이 끝날 때까지 슬롯 포인터가 남는 실행 표다. 0번도 읽는다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    private readonly BaseProcess?[] _processes;

    /// <summary>같은 포인터를 재등록해도 소유권은 한 슬롯에만 둔다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    private readonly BaseProcess?[] _owned;

    /// <summary>Native 포인터 덮어쓰기에서 가상 소멸 효과를 추가하지 않고 최종 반납까지 소유권을 지킨다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    private readonly List<BaseProcess?> _retired = [];

    /// <summary>요약·상세·단언은 선택 시점의 현재 장치만 각각 복사해 호출한다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    private KernelDiagnostics _diagnostics = new();

    /// <summary>기본은 10.78의 39999, CD 비교는 3999다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    private readonly uint _capacity;

    /// <summary>현재 등록 수다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    private int _count;

    /// <summary>슬롯 배열을 힙에 둔다. 저장 파일 형식에는 영향이 없다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <param name="capacity">끝 슬롯 번호다. 1 이상 39999 이하여야 한다.</param>
    public Kernel(uint capacity = KernelShape.ProcessCapacity)
    {
        if (capacity == 0u || capacity > KernelShape.ProcessCapacity)
        {
            throw new ArgumentOutOfRangeException(nameof(capacity), capacity, "커널 슬롯 크기는 1·39999다.");
        }

        _processes = new BaseProcess?[capacity + 1u];
        _owned = new BaseProcess?[capacity + 1u];
        _capacity = capacity;
    }

    /// <summary>프로세스 소유권을 커널에 넘기고 앞에서부터 첫 빈 슬롯을 배정한다. 원본처럼 빈 앞 슬롯이 있어도 동일 포인터의 기존 뒤 슬롯까지 검색한다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <param name="process">넘길 프로세스다.</param>
    /// <returns>배정된 슬롯 번호다. 소진時は 0이다.</returns>
    public uint Add(BaseProcess process)
    {
        ArgumentNullException.ThrowIfNull(process);
        uint first = 0u;
        // 빈 앞 슬롯이 있어도 동일 포인터의 기존 뒤 슬롯까지 검색한다
        for (uint id = 1u; id <= _capacity; ++id)
        {
            if (_processes[id] is null && first == 0u)
            {
                first = id;
            }

            if (ReferenceEquals(_processes[id], process))
            {
                return id;
            }
        }

        // 설치 전에 던지면 넘겨받은 인자도 소멸한다. 원문 unique_ptr 인자의 풀기 중 소멸과 같다
        if (first == 0u)
        {
            Action? summary = _diagnostics.Summary;
            if (summary is null)
            {
                process.Dispose();
                throw new InvalidOperationException("커널 소진의 현재 프로세스 요약 미연결");
            }

            try
            {
                summary();
            }
            catch
            {
                process.Dispose();
                throw;
            }

            Action<string>? detail = _diagnostics.Detail;
            if (detail is null)
            {
                process.Dispose();
                throw new InvalidOperationException("커널 소진의 현재 상세 저장소 미연결");
            }

            try
            {
                detail("Out of processes. A process count trace has been dumped\nto procSumm.txt.  Please deliver this file with the bug report.\n");
            }
            catch
            {
                process.Dispose();
                throw;
            }

            Action<string, string, uint>? assertion = _diagnostics.Assertion;
            if (assertion is null)
            {
                process.Dispose();
                throw new InvalidOperationException("커널 소진의 현재 단언 미연결");
            }

            bool patch = _diagnostics.Edition == OriginalEdition.Patch1078;
            try
            {
                assertion("firstSlot != 0", patch ? "\\Ns\\O\\Kernel.cpp" : "\\ns\\o\\Kernel.cpp", patch ? 0x7aU : 0x77U);
            }
            catch
            {
                process.Dispose();
                throw;
            }
        }

        // 원문은 진단에서 빈 슬롯이 생겨도 first를 다시 찾지 않고 캡처한 0을 사용한다
        if (_owned[first] is not null)
        {
            _retired.Add(_owned[first]);
            _owned[first] = null;
        }

        // 원문의 기존 포인터 덮어쓰기 위치에는 가상 소멸자 호출이 없다
        if (_processes[first] is null)
        {
            ++_count;
        }

        _processes[first] = process;
        _owned[first] = process;
        return first;
    }

    /// <summary>이미 소유한 객체는 새 소유권을 만들지 않고 원본의 중복 등록 검색만 실행한다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <param name="process">찾을 프로세스 참조다.</param>
    /// <returns>기존 슬롯 번호다.</returns>
    public uint AddRef(BaseProcess process)
    {
        ArgumentNullException.ThrowIfNull(process);
        uint id = Find(process);
        if (id == 0u)
        {
            throw new ArgumentException("미등록 프로세스 참조의 소유권이 없습니다.", nameof(process));
        }

        return id;
    }

    /// <summary>원본 등록 함수가 이미 있는 프로세스의 번호를 반환하던 부분이다. 없으면 0이다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <param name="process">찾을 프로세스 참조다.</param>
    /// <returns>슬롯 번호다. 없으면 0이다.</returns>
    public uint Find(BaseProcess process)
    {
        ArgumentNullException.ThrowIfNull(process);
        // 포인터가 같은 슬롯을 앞에서부터 찾는다
        for (uint id = 1u; id <= _capacity; ++id)
        {
            if (ReferenceEquals(_processes[id], process))
            {
                return id;
            }
        }

        return 0u;
    }

    /// <summary>원본처럼 가상 소멸자를 호출하고 슬롯을 비운다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <param name="id">지울 슬롯 번호다.</param>
    public void Remove(uint id)
    {
        if (id > _capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(id), id, "커널 슬롯이 물리 범위를 벗어났습니다.");
        }

        if (_processes[id] is null)
        {
            Action<string, string, uint>? assertion = _diagnostics.Assertion;
            if (assertion is null)
            {
                throw new InvalidOperationException("커널 빈 슬롯의 현재 단언 미연결");
            }

            bool patch = _diagnostics.Edition == OriginalEdition.Patch1078;
            assertion("processes[pid] != NULL", patch ? "\\Ns\\O\\Kernel.cpp" : "\\ns\\o\\Kernel.cpp", patch ? 0x82U : 0x7fU);
        }

        // 가상 소멸자 안의 조회·찾기·재등록은 아직 원래 슬롯을 관찰한다. 소멸 뒤에만 빈 슬롯을 만든다
        if (_processes[id] is not null)
        {
            if (_owned[id] is null)
            {
                throw new InvalidOperationException("커널 소멸 중인 같은 포인터의 중복 삭제");
            }

            BaseProcess? victim = _owned[id];
            _owned[id] = null;
            victim!.Dispose();
        }

        if (_owned[id] is not null)
        {
            _retired.Add(_owned[id]);
            _owned[id] = null;
        }

        // 소멸자가 슬롯 0을 덮어쓴 경우도 원문 마지막 NULL 쓰기에는 새 소멸 효과가 없다
        if (_processes[id] is not null)
        {
            --_count;
        }

        _processes[id] = null;
    }

    /// <summary>원본 00471aa0의 슬롯 프로세스를 그대로 읽는다. 빈 슬롯과 범위 밖 번호는 null이다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <param name="id">읽을 슬롯 번호다.</param>
    /// <returns>슬롯 프로세스다. 없으면 null이다.</returns>
    public BaseProcess? Get(uint id) => id > _capacity ? null : _processes[id];

    /// <summary>슬롯 1부터 끝까지 한 번만 읽고 소멸자 완료 뒤 현재 슬롯을 비운다. 소멸자가 이미 지난 빈 슬롯에 새 객체를 넣으면 이번 Clear에서는 남는다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    public void Clear()
    {
        // 원본의 정확한 증가 순서를 유지하고 빈 슬롯은 건드리지 않는다
        for (uint id = 1u; id <= _capacity; ++id)
        {
            if (_processes[id] is not null)
            {
                Remove(id);
            }
        }
    }

    /// <summary>그 순간의 슬롯을 직접 읽으며 1·39999를 순회한다. 실행 중 뒤 슬롯에 등록된 프로세스는 같은 프레임에 실행되고, 이미 지난 슬롯이면 다음 프레임이다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    public void RunFrame()
    {
        // 원본의 즉시 슬롯 읽기를 유지한다. 목록을 복사하면 새 프로세스 실행 시점이 달라진다
        for (uint id = 1u; id <= _capacity; ++id)
        {
            _processes[id]?.RunFrame();
        }
    }

    /// <summary>원본 디버그용 프로세스 개수와 같은 현재 등록 수다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <returns>등록 수다.</returns>
    public int Size() => _count;

    /// <summary>판본별 마지막 등록 번호를 반환한다. 슬롯 0은 이 범위에 포함하지 않는다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <returns>끝 슬롯 번호다.</returns>
    public uint Capacity() => _capacity;

    /// <summary>실제 Client의 현재 진단 장치 또는 독립 원문 대조 장치를 연결한다. 값 조회·등록은 수행하지 않는다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <param name="diagnostics">연결할 진단 묶음이다.</param>
    public void BindDiagnostics(KernelDiagnostics diagnostics)
    {
        ArgumentNullException.ThrowIfNull(diagnostics);
        _diagnostics = diagnostics.Clone();
    }

    /// <summary>필터에 맞는 실제 소유권만 반복 반납하며 소멸자 재진입 뒤에는 항상 배열을 새로 조회한다. Native Clear와 구별되는 최종 반납이다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <param name="select">반납할 프로세스 선택이다. null이면 모두 반납한다.</param>
    public void DrainOwnedForShutdown(Func<BaseProcess, bool>? select = null)
    {
        Func<BaseProcess, bool>? selected = select;
        // 앞 슬롯 신규 등록과 retired 추가도 다음 순회에서 실제 현재값으로 처리한다
        for (; ; )
        {
            bool removed = false;
            // Native Clear 순서를 먼저 유지한다
            for (uint id = 1u; id <= _capacity; ++id)
            {
                if (_processes[id] is not null && (selected is null || selected(_processes[id]!)))
                {
                    Remove(id);
                    removed = true;
                }
            }

            if (_processes[0] is not null && (selected is null || selected(_processes[0]!)))
            {
                Remove(0u);
                removed = true;
            }

            // 소멸자의 추가·재진입에 반복자나 벡터 원소 참조를 남기지 않는다
            for (int index = 0; index < _retired.Count; ++index)
            {
                if (_retired[index] is not null && (selected is null || selected(_retired[index]!)))
                {
                    BaseProcess? victim = _retired[index];
                    _retired[index] = null;
                    victim!.Dispose();
                    removed = true;
                }
            }

            if (!removed)
            {
                break;
            }
        }
    }
}
